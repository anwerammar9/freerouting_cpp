#include "drc/DesignRulesChecker.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <map>
#include <set>
#include <sstream>
#include <unordered_set>

#include "board/BoardOutline.h"
#include "board/Communication.h"
#include "board/Component.h"
#include "board/ConductionArea.h"
#include "board/Trace.h"
#include "core/Package.h"
#include "geometry/planar/Point.h"
#include "geometry/planar/Shape.h"
#include "geometry/planar/Simplex.h"
#include "logger/FRLogger.h"
#include "rules/ClearanceMatrix.h"
#include "rules/Net.h"

namespace freerouting {
namespace drc {

namespace planar = freerouting::geometry::planar;
namespace rules = freerouting::rules;
namespace board = freerouting::board;

namespace {

/**
 * Live-mirror of Java Connectable: traces, vias, pins and conduction areas can
 * participate in rat-nest/incomplete calculations. The Java Connectable
 * interface is not ported yet, so the check is done by type.
 */
bool is_connectable(const board::Item* p_item)
{
    if (p_item == nullptr)
    {
        return false;
    }
    if (p_item->is_trace() || p_item->is_via() || p_item->is_pin())
    {
        return true;
    }
    return dynamic_cast<const board::ConductionArea*>(p_item) != nullptr;
}

/** Simple (last) class name of an item for descriptions. */
std::string class_simple_name(const board::Item* p_item)
{
    if (dynamic_cast<const board::BoardOutline*>(p_item) != nullptr)
    {
        return "BoardOutline";
    }
    if (dynamic_cast<const board::ObstacleArea*>(p_item) != nullptr)
    {
        return "ObstacleArea";
    }
    return "Item";
}

/**
 * Mirrors Java Item.calculateClearanceBetweenTwoShapes: binary search for the
 * minimal enlargement (split between the two compensation values) at which the
 * enlarged shapes still intersect.
 */
double calculate_clearance_between_two_shapes(const planar::IntOctagon& p_shape1,
                                              const planar::IntOctagon& p_shape2,
                                              double p_minimum_clearance,
                                              int p_cl_comp1,
                                              int p_cl_comp2)
{
    if (p_shape1.to_Simplex().intersection(p_shape2.to_Simplex()).dimension() == 2)
    {
        return 0.0;
    }
    double low = 0.0;
    double high = p_minimum_clearance;
    double sum_comp = p_cl_comp1 + p_cl_comp2;
    double factor1 = (sum_comp > 0) ? (static_cast<double>(p_cl_comp1) / sum_comp) : 0.5;
    double factor2 = (sum_comp > 0) ? (static_cast<double>(p_cl_comp2) / sum_comp) : 0.5;
    for (int iter = 0; iter < 16; ++iter)
    {
        double mid = (low + high) * 0.5;
        planar::Simplex s1 = p_shape1.offset(mid * factor1).to_Simplex();
        planar::Simplex s2 = p_shape2.offset(mid * factor2).to_Simplex();
        if (s1.intersection(s2).dimension() == 2)
        {
            high = mid;
        }
        else
        {
            low = mid;
        }
    }
    return low;
}

} // namespace

DesignRulesChecker::DesignRulesChecker(board::RoutingBoard* p_board,
                                       const settings::DesignRulesCheckerSettings& p_drc_settings)
    : routing_board_(p_board)
    , drc_settings_(p_drc_settings)
{
}

std::vector<std::shared_ptr<ClearanceViolation>> DesignRulesChecker::get_all_clearance_violations()
{
    std::vector<std::shared_ptr<ClearanceViolation>> all_violations;
    if (routing_board_ == nullptr)
    {
        return all_violations;
    }

    std::unordered_set<std::string> seen_violations;
    for (int i = 0; i < routing_board_->get_item_count(); ++i)
    {
        board::Item* item = routing_board_->get_item(i);
        if (item == nullptr)
        {
            continue;
        }
        auto item_violations = calculate_clearance_violations_for_item(item, *routing_board_);
        for (const auto& violation : item_violations)
        {
            int id1 = violation->get_first_item()->get_id();
            int id2 = violation->get_second_item()->get_id();
            std::string key = (id1 < id2)
                ? (std::to_string(id1) + "-" + std::to_string(id2) + "-"
                   + std::to_string(violation->get_layer()))
                : (std::to_string(id2) + "-" + std::to_string(id1) + "-"
                   + std::to_string(violation->get_layer()));
            if (seen_violations.insert(key).second)
            {
                all_violations.push_back(violation);
            }
        }
    }
    return all_violations;
}

std::vector<std::shared_ptr<ClearanceViolation>>
    DesignRulesChecker::calculate_clearance_violations_for_item(board::Item* p_item,
                                                               board::RoutingBoard& p_board)
{
    std::vector<std::shared_ptr<ClearanceViolation>> result;
    board::ShapeSearchTree* default_tree = p_board.search_tree_manager().get_default_tree();
    if (default_tree == nullptr)
    {
        return result;
    }
    const rules::ClearanceMatrix& matrix = p_board.get_rules().get_clearance_matrix();
    board::SearchTreeManager& tree_manager = p_board.search_tree_manager();

    for (int i = 0; i < p_item->tile_shape_count(); ++i)
    {
        planar::IntOctagon current_tile_shape = p_item->get_tile_shape(i);
        int curr_layer = p_item->shape_layer(i);

        std::vector<board::ShapeSearchTree::TreeEntry> overlapping_items =
            default_tree->overlapping_tree_entries_with_clearance(
                current_tile_shape, curr_layer, {}, p_item->get_clearance_class());

        for (const auto& current_entry : overlapping_items)
        {
            board::Item* current_item = dynamic_cast<board::Item*>(current_entry.object);
            if (current_item == nullptr || current_entry.object == p_item)
            {
                continue;
            }

            bool is_obstacle = current_item->is_obstacle(p_item);

            // Look, if both traces are connected to the same tie pin.
            // In this case they are allowed to overlap without sharing a net.
            if (is_obstacle && p_item->is_trace() && current_item->is_trace())
            {
                board::Trace* this_trace = static_cast<board::Trace*>(p_item);
                const planar::Point& contact_point = this_trace->first_corner();
                std::set<board::Item*> current_contacts =
                    this_trace->get_normal_contacts(contact_point, true);
                bool contact_found = current_contacts.find(current_item) != current_contacts.end();
                if (!contact_found)
                {
                    const planar::Point& last_contact_point = this_trace->last_corner();
                    current_contacts = this_trace->get_normal_contacts(last_contact_point, true);
                    contact_found =
                        current_contacts.find(current_item) != current_contacts.end();
                }
                if (contact_found)
                {
                    for (board::Item* current_contact : current_contacts)
                    {
                        if (current_contact->is_pin() && current_contact->shares_net(p_item)
                            && current_contact->shares_net(current_item))
                        {
                            is_obstacle = false;
                            break;
                        }
                    }
                }
            }

            // Pins whose pad protrudes outside the board outline must stay obstacles.
            bool this_is_outline =
                dynamic_cast<board::BoardOutline*>(p_item) != nullptr && current_item->is_pin();
            bool other_is_outline =
                dynamic_cast<board::BoardOutline*>(current_item) != nullptr && p_item->is_pin();
            if (is_obstacle && (this_is_outline || other_is_outline))
            {
                board::BoardOutline* outline = this_is_outline
                    ? static_cast<board::BoardOutline*>(p_item)
                    : static_cast<board::BoardOutline*>(current_item);
                planar::IntOctagon pin_tile_shape = this_is_outline
                    ? current_item->get_tile_shape(current_entry.shape_index_in_object)
                    : current_tile_shape;
                std::shared_ptr<planar::TileShape> outline_shape = outline->get_tile_shape();
                bool inside_outline = outline_shape != nullptr;
                if (inside_outline)
                {
                    // IntOctagon corners are always bounded (8 corners).
                    int corner_count = pin_tile_shape.border_line_count();
                    for (int ci = 0; inside_outline && ci < corner_count; ++ci)
                    {
                        if (!outline_shape->contains(pin_tile_shape.corner(ci)))
                        {
                            inside_outline = false;
                        }
                    }
                }
                if (inside_outline)
                {
                    is_obstacle = false;
                }
            }

            if (!is_obstacle)
            {
                continue;
            }

            // Get the two shapes the clearance is calculated between.
            planar::IntOctagon shape1 = current_tile_shape;
            planar::IntOctagon shape2 = current_item->get_tile_shape(current_entry.shape_index_in_object);
            if (shape1.is_empty() || shape2.is_empty())
            {
                continue;
            }

            double minimum_clearance = static_cast<double>(
                matrix.get_value(current_item->get_clearance_class(),
                                 p_item->get_clearance_class(),
                                 curr_layer,
                                 false));

            int cl_comp1 = 0;
            int cl_comp2 = 0;
            if (tree_manager.is_clearance_compensation_used())
            {
                cl_comp1 = default_tree->clearance_compensation_value(p_item->get_clearance_class(),
                                                                      curr_layer);
                cl_comp2 = default_tree->clearance_compensation_value(
                    current_item->get_clearance_class(), curr_layer);
            }
            else
            {
                cl_comp1 = static_cast<int>(std::round(0.5 * minimum_clearance));
                cl_comp2 = static_cast<int>(std::round(minimum_clearance - cl_comp1));
            }

            planar::IntOctagon enlarged_shape1 = (cl_comp1 > 0) ? shape1.offset(cl_comp1) : shape1;
            planar::IntOctagon enlarged_shape2 = (cl_comp2 > 0) ? shape2.offset(cl_comp2) : shape2;
            planar::Simplex s_intersection =
                enlarged_shape1.to_Simplex().intersection(enlarged_shape2.to_Simplex());
            if (s_intersection.dimension() == 2)
            {
                double actual_clearance = calculate_clearance_between_two_shapes(
                    shape1, shape2, minimum_clearance, cl_comp1, cl_comp2);

                result.push_back(std::make_shared<ClearanceViolation>(
                    p_item,
                    current_item,
                    std::make_shared<planar::Simplex>(s_intersection),
                    curr_layer,
                    minimum_clearance,
                    actual_clearance));
            }
        }
    }
    return result;
}

std::vector<UnconnectedItems> DesignRulesChecker::get_all_unconnected_items()
{
    std::vector<UnconnectedItems> result;
    if (routing_board_ == nullptr)
    {
        return result;
    }

    // Group items by net
    std::map<int, std::vector<board::Item*>> items_by_net;
    for (int i = 0; i < routing_board_->get_item_count(); ++i)
    {
        board::Item* item = routing_board_->get_item(i);
        if (item != nullptr && is_connectable(item) && !item->get_net_nos().empty())
        {
            items_by_net[item->get_net_nos()[0]].push_back(item);
        }
    }

    // For each net, find truly unconnected items
    for (auto& entry : items_by_net)
    {
        int net_no = entry.first;
        std::vector<board::Item*>& net_items = entry.second;
        if (static_cast<int>(net_items.size()) <= 1)
        {
            continue;
        }

        std::vector<std::vector<board::Item*>> connected_sets;
        std::unordered_set<board::Item*> processed_items;
        for (board::Item* item : net_items)
        {
            if (processed_items.find(item) != processed_items.end())
            {
                continue;
            }
            std::set<board::Item*> connected_set = item->get_connected_set(net_no, false);
            std::vector<board::Item*> set_items;
            for (board::Item* connected_item : connected_set)
            {
                if (std::find(net_items.begin(), net_items.end(), connected_item) != net_items.end())
                {
                    set_items.push_back(connected_item);
                }
            }
            if (!set_items.empty())
            {
                connected_sets.push_back(set_items);
                processed_items.insert(set_items.begin(), set_items.end());
            }
        }

        // If there are multiple connected sets, we have unconnected items.
        // Report only ONE entry per net, but include items from the disconnected groups.
        if (connected_sets.size() >= 2)
        {
            board::Item* item1 = find_representative_item(connected_sets[0]);
            board::Item* item2 = find_representative_item(connected_sets[1]);
            if (item1 != nullptr && item2 != nullptr)
            {
                std::vector<board::Item*> unconnected_group_items;
                unconnected_group_items.insert(unconnected_group_items.end(),
                                               connected_sets[0].begin(),
                                               connected_sets[0].end());
                unconnected_group_items.insert(unconnected_group_items.end(),
                                               connected_sets[1].begin(),
                                               connected_sets[1].end());
                result.emplace_back(item1, item2, unconnected_group_items);
            }
        }
    }

    // Check for dangling traces - traces with unconnected ends
    for (int i = 0; i < routing_board_->get_item_count(); ++i)
    {
        board::Item* item = routing_board_->get_item(i);
        if (item == nullptr)
        {
            continue;
        }
        auto* trace = dynamic_cast<board::Trace*>(item);
        if (trace == nullptr)
        {
            continue;
        }
        std::set<board::Item*> start_contacts = trace->get_start_contacts();
        std::set<board::Item*> end_contacts = trace->get_end_contacts();
        if (start_contacts.empty() || end_contacts.empty())
        {
            bool already_added = false;
            for (const UnconnectedItems& current : result)
            {
                if (current.get_first() == trace)
                {
                    already_added = true;
                    break;
                }
            }
            if (!already_added)
            {
                result.emplace_back(trace, nullptr, std::string("track_dangling"));
            }
        }
    }

    // Check for dangling vias - vias not connected or connected on only one layer
    for (int i = 0; i < routing_board_->get_item_count(); ++i)
    {
        board::Item* item = routing_board_->get_item(i);
        if (item == nullptr)
        {
            continue;
        }
        auto* via = dynamic_cast<board::Via*>(item);
        if (via == nullptr)
        {
            continue;
        }
        if (via->is_tail())
        {
            result.emplace_back(via, nullptr, std::string("via_dangling"));
        }
    }

    return result;
}

std::vector<ZoneIslandViolation> DesignRulesChecker::get_zone_island_violations()
{
    std::vector<ZoneIslandViolation> result;
    if (routing_board_ == nullptr)
    {
        return result;
    }

    // NOTE (documented deviation): the C++ Area model stores a single outside
    // border contour per conduction area (PolylineArea::get_border + holes),
    // while Java's decomposeAreaIntoIslands can only produce more than one
    // island when a fill is composed of several additive contours. A single
    // contour therefore always decomposes to exactly one island and the Java
    // "if (islands.size() <= 1) continue;" short-circuit skips every area.
    // Zone island violations are consequently never emitted; the logic is kept
    // so it lights up as soon as a multi-contour Area model is ported.
    double min_dead_copper_area = get_resolution_mm() * get_resolution_mm();
    (void)min_dead_copper_area;

    for (int i = 0; i < routing_board_->get_item_count(); ++i)
    {
        auto* ca = dynamic_cast<board::ConductionArea*>(routing_board_->get_item(i));
        if (ca == nullptr)
        {
            continue;
        }
        std::shared_ptr<planar::Area> fill_area = ca->get_area();
        if (fill_area == nullptr || fill_area->is_empty())
        {
            continue;
        }
        // decomposeAreaIntoIslands: with a single-border area exactly one
        // island exists, so the fragmentation check below never triggers.
        int islands = 1;
        if (islands <= 1)
        {
            continue;
        }
    }

    return result;
}

board::Item* DesignRulesChecker::find_representative_item(const std::vector<board::Item*>& p_connected_set)
{
    // Prefer Pins
    for (board::Item* item : p_connected_set)
    {
        if (dynamic_cast<board::Pin*>(item) != nullptr)
        {
            return item;
        }
    }
    // Then Traces
    for (board::Item* item : p_connected_set)
    {
        if (item->is_trace())
        {
            return item;
        }
    }
    // Finally any item
    return p_connected_set.empty() ? nullptr : p_connected_set[0];
}

std::unique_ptr<DrcReport> DesignRulesChecker::generate_report(const std::string& p_source_file,
                                                               const std::string& p_coordinate_unit)
{
    std::string version = "Freerouting " + std::string(constants::FREEROUTING_VERSION);
    auto report = std::make_unique<DrcReport>(p_coordinate_unit, p_source_file, version);

    // Get all clearance violations
    auto violations = get_all_clearance_violations();
    logger::FRLogger::instance().trace("DRC check started: total_clearance_violations="
        + std::to_string(violations.size()) + ", coordinate_unit=" + p_coordinate_unit
        + ", source_file=" + p_source_file);

    for (const auto& violation : violations)
    {
        auto drc_violation = convert_to_drc_violation(violation.get(), p_coordinate_unit);
        report->add_violation(drc_violation);

        logger::FRLogger::instance().trace("DRC violation: type=clearance, layer="
            + std::to_string(violation->get_layer()));
    }

    // Get all unconnected items
    auto unconnected_items = get_all_unconnected_items();
    logger::FRLogger::instance().trace("Unconnected items found: count="
        + std::to_string(unconnected_items.size()));

    for (const UnconnectedItems& unconnected_item : unconnected_items)
    {
        auto drc_violation = convert_to_drc_violation(&unconnected_item, p_coordinate_unit);
        if (unconnected_item.get_type() == "track_dangling"
            || unconnected_item.get_type() == "via_dangling")
        {
            report->add_violation(drc_violation);
        }
        else
        {
            report->add_unconnected_item(drc_violation);
        }
    }

    // Get all zone island violations
    auto zone_violations = get_zone_island_violations();
    for (const ZoneIslandViolation& zv : zone_violations)
    {
        auto drc_violation = convert_to_drc_violation(&zv, p_coordinate_unit);
        if (zv.get_type() == "isolated_island_unconnected")
        {
            report->add_unconnected_item(drc_violation);
        }
        else
        {
            report->add_violation(drc_violation);
        }
    }

    logger::FRLogger::instance().trace("DRC check completed: total_violations="
        + std::to_string(report->violations.size()) + ", total_unconnected="
        + std::to_string(report->unconnected_items.size()) + ", total_zone_islands="
        + std::to_string(zone_violations.size()));

    return report;
}

std::string DesignRulesChecker::generate_report_json(const std::string& p_source_file,
                                                     const std::string& p_coordinate_unit)
{
    return generate_report(p_source_file, p_coordinate_unit)->to_json();
}

std::shared_ptr<DrcViolation> DesignRulesChecker::convert_to_drc_violation(
    const ClearanceViolation* p_violation, const std::string& p_coordinate_unit)
{
    std::vector<std::shared_ptr<DrcViolationItem>> items;

    std::string first_item_desc = get_item_description(p_violation->get_first_item());
    std::string second_item_desc = get_item_description(p_violation->get_second_item());

    planar::FloatPoint first_center =
        p_violation->get_first_item()->get_bounding_box().centre_of_gravity();
    planar::FloatPoint second_center =
        p_violation->get_second_item()->get_bounding_box().centre_of_gravity();

    items.push_back(std::make_shared<DrcViolationItem>(
        first_item_desc,
        DrcPosition(convert_coordinate(first_center.x, p_coordinate_unit),
                    convert_coordinate(first_center.y, p_coordinate_unit)),
        std::to_string(p_violation->get_first_item()->get_id())));
    items.push_back(std::make_shared<DrcViolationItem>(
        second_item_desc,
        DrcPosition(convert_coordinate(second_center.x, p_coordinate_unit),
                    convert_coordinate(second_center.y, p_coordinate_unit)),
        std::to_string(p_violation->get_second_item()->get_id())));

    std::string type = "clearance";
    if (is_hole(p_violation->get_first_item()) || is_hole(p_violation->get_second_item()))
    {
        type = "holeClearance";
    }

    std::ostringstream description_stream;
    description_stream << std::fixed << std::setprecision(4);
    description_stream << (type == "holeClearance" ? "Hole clearance violation between "
                                                   : "Clearance violation between ")
                       << first_item_desc << " and " << second_item_desc << " (expected: "
                       << convert_coordinate(p_violation->get_expected_clearance(), p_coordinate_unit)
                       << " " << p_coordinate_unit << ", actual: "
                       << convert_coordinate(p_violation->get_actual_clearance(), p_coordinate_unit)
                       << " " << p_coordinate_unit << ")";

    return std::make_shared<DrcViolation>(type, description_stream.str(), "error", items);
}

std::shared_ptr<DrcViolation> DesignRulesChecker::convert_to_drc_violation(
    const UnconnectedItems* p_unconnected_item, const std::string& p_coordinate_unit)
{
    std::vector<std::shared_ptr<DrcViolationItem>> items;

    const std::string type = p_unconnected_item->get_type();
    if (type == "track_dangling" || type == "via_dangling")
    {
        board::Item* item = p_unconnected_item->get_first();
        std::string item_desc = (type == "via_dangling") ? get_item_description(item)
                                                         : get_detailed_trace_description(item, p_coordinate_unit);

        planar::FloatPoint center = item->get_bounding_box().centre_of_gravity();
        items.push_back(std::make_shared<DrcViolationItem>(
            item_desc,
            DrcPosition(convert_coordinate(center.x, p_coordinate_unit),
                        convert_coordinate(center.y, p_coordinate_unit)),
            std::to_string(item->get_id())));

        std::string description;
        if (type == "via_dangling")
        {
            description = "Via is not connected or connected on only one layer";
        }
        else if (type == "track_dangling")
        {
            description = "Track has unconnected end";
        }
        else
        {
            description = "Unconnected item: " + item_desc;
        }
        return std::make_shared<DrcViolation>(type, description, "warning", items);
    }

    // Create items for all items from the unconnected net
    for (board::Item* item : p_unconnected_item->get_all_items())
    {
        std::string item_desc = get_item_description(item);
        planar::FloatPoint center = item->get_bounding_box().centre_of_gravity();
        items.push_back(std::make_shared<DrcViolationItem>(
            item_desc,
            DrcPosition(convert_coordinate(center.x, p_coordinate_unit),
                        convert_coordinate(center.y, p_coordinate_unit)),
            std::to_string(item->get_id())));
    }

    std::string from_item_desc = get_item_description(p_unconnected_item->get_first());
    std::string description;
    if (p_unconnected_item->get_second() != nullptr)
    {
        description = "Unconnected items: " + from_item_desc + " and "
            + get_item_description(p_unconnected_item->get_second()) + " ("
            + std::to_string(p_unconnected_item->get_all_items().size()) + " total items in net)";
    }
    else
    {
        description = "Unconnected item: " + from_item_desc;
    }

    return std::make_shared<DrcViolation>(type, description, "warning", items);
}

std::shared_ptr<DrcViolation> DesignRulesChecker::convert_to_drc_violation(
    const ZoneIslandViolation* p_zone_violation, const std::string& p_coordinate_unit)
{
    std::vector<std::shared_ptr<DrcViolationItem>> items;

    items.push_back(std::make_shared<DrcViolationItem>(
        get_item_description(p_zone_violation->get_conduction_area()),
        DrcPosition(convert_coordinate(p_zone_violation->get_center_x(), p_coordinate_unit),
                    convert_coordinate(p_zone_violation->get_center_y(), p_coordinate_unit)),
        std::to_string(p_zone_violation->get_conduction_area()->get_id())));

    for (board::Item* item : p_zone_violation->get_items_in_island())
    {
        planar::FloatPoint center = item->get_bounding_box().centre_of_gravity();
        items.push_back(std::make_shared<DrcViolationItem>(
            get_item_description(item),
            DrcPosition(convert_coordinate(center.x, p_coordinate_unit),
                        convert_coordinate(center.y, p_coordinate_unit)),
            std::to_string(item->get_id())));
    }

    std::string net_name = "unknown";
    if (p_zone_violation->get_net_number() > 0 && routing_board_ != nullptr)
    {
        rules::Net* net = routing_board_->get_rules().get_nets().get(p_zone_violation->get_net_number());
        if (net != nullptr)
        {
            net_name = net->get_name();
        }
    }

    std::string description;
    if (p_zone_violation->get_type() == "isolated_island_dead_copper")
    {
        description = "Isolated copper island without electrical connection on net [" + net_name
            + "] (Conduction Area #" + std::to_string(p_zone_violation->get_conduction_area()->get_id()) + ")";
    }
    else
    {
        description = "Isolated copper island on net [" + net_name + "] containing "
            + std::to_string(p_zone_violation->get_items_in_island().size())
            + " disconnected item(s) from Conduction Area #"
            + std::to_string(p_zone_violation->get_conduction_area()->get_id());
    }

    std::string severity =
        (p_zone_violation->get_type() == "isolated_island_unconnected") ? "error" : "warning";
    return std::make_shared<DrcViolation>(p_zone_violation->get_type(), description, severity, items);
}

bool DesignRulesChecker::is_hole(const board::Item* p_item)
{
    if (dynamic_cast<const board::Via*>(p_item) != nullptr)
    {
        return true;
    }
    return dynamic_cast<const board::Pin*>(p_item) != nullptr;
}

std::string DesignRulesChecker::get_item_description(const board::Item* p_item) const
{
    std::string desc;
    if (p_item->is_trace())
    {
        desc = "Trace";
    }
    else if (p_item->is_via())
    {
        desc = "Via";
    }
    else if (p_item->is_pin())
    {
        desc = "Pin";
    }
    else if (dynamic_cast<const board::ConductionArea*>(p_item) != nullptr)
    {
        desc = "Conduction Area";
    }
    else
    {
        desc = class_simple_name(p_item);
    }

    if (!p_item->get_net_nos().empty())
    {
        desc += " [" + get_net_name(p_item->get_net_nos()[0]) + "]";
    }
    return desc;
}

std::string DesignRulesChecker::get_detailed_trace_description(const board::Item* p_item,
                                                               const std::string& p_coordinate_unit) const
{
    std::string desc = "Track";

    if (!p_item->get_net_nos().empty())
    {
        desc += " [" + get_net_name(p_item->get_net_nos()[0]) + "]";
    }

    const board::Trace* trace = dynamic_cast<const board::Trace*>(p_item);
    if (trace != nullptr)
    {
        int layer = trace->get_layer();
        std::string layer_name;
        if (routing_board_ != nullptr && layer >= 0 && layer < routing_board_->get_layer_count())
        {
            layer_name = routing_board_->get_layer_structure().get_layer(layer)->name;
        }
        else
        {
            layer_name = "Layer " + std::to_string(layer);
        }
        desc += " on " + layer_name;

        std::ostringstream length_stream;
        length_stream << std::fixed << std::setprecision(4)
                      << convert_coordinate(trace->get_length(), p_coordinate_unit);
        desc += ", length " + length_stream.str() + " " + p_coordinate_unit;
    }

    return desc;
}

std::string DesignRulesChecker::get_net_name(int p_net_no) const
{
    if (routing_board_ == nullptr || p_net_no <= 0)
    {
        return "unknown";
    }
    rules::Net* net = routing_board_->get_rules().get_nets().get(p_net_no);
    return (net != nullptr) ? net->get_name() : ("NET" + std::to_string(p_net_no));
}

double DesignRulesChecker::convert_coordinate(double p_board_coordinate,
                                              const std::string& p_coordinate_unit) const
{
    board::Communication* comm = (routing_board_ != nullptr) ? routing_board_->communication() : nullptr;
    board::Unit board_unit = (comm != nullptr) ? comm->get_unit() : board::Unit::MIL;
    int resolution = (comm != nullptr) ? comm->get_resolution() : 1;

    // Board coordinates are stored in the resolved native DSN units; multiply
    // with the same factor used by BoardStatistics to obtain mm.
    double mm_per_board_unit =
        board::UnitHelper::scale(1.0, board_unit, board::Unit::MM) / (resolution > 0 ? resolution : 1);
    double mm = p_board_coordinate * mm_per_board_unit;

    board::Unit target_unit;
    if (p_coordinate_unit == "mm")
    {
        target_unit = board::Unit::MM;
    }
    else if (p_coordinate_unit == "mil")
    {
        target_unit = board::Unit::MIL;
    }
    else if (p_coordinate_unit == "inch")
    {
        target_unit = board::Unit::INCH;
    }
    else if (p_coordinate_unit == "um")
    {
        target_unit = board::Unit::UM;
    }
    else
    {
        target_unit = board_unit;
    }
    return board::UnitHelper::scale(mm, board::Unit::MM, target_unit);
}

double DesignRulesChecker::get_resolution_mm() const
{
    board::Communication* comm = (routing_board_ != nullptr) ? routing_board_->communication() : nullptr;
    if (comm != nullptr)
    {
        return board::UnitHelper::scale(
            static_cast<double>(comm->get_resolution()), comm->get_unit(), board::Unit::MM);
    }
    return board::UnitHelper::scale(1.0, board::Unit::MIL, board::Unit::MM);
}

void DesignRulesChecker::calculate_all_incompletes()
{
    if (routing_board_ == nullptr)
    {
        return;
    }
    const rules::Nets& nets = routing_board_->get_rules().get_nets();
    int max_net_no = nets.max_net_no();

    std::vector<std::vector<board::Item*>> net_item_lists(max_net_no);
    for (int i = 0; i < routing_board_->get_item_count(); ++i)
    {
        board::Item* item = routing_board_->get_item(i);
        if (item == nullptr || !is_connectable(item))
        {
            continue;
        }
        for (int net_no : item->get_net_nos())
        {
            if (net_no >= 1 && net_no <= max_net_no)
            {
                net_item_lists[net_no - 1].push_back(item);
            }
        }
    }

    // Mirrors the Java correction: for each net with >= 1 item, max(0,
    // endpoint_count - 1) connections are needed (minimum spanning tree),
    // where endpoints are Pins and ConductionAreas.
    int total_items = 0;
    max_connections = 0;
    for (const auto& list : net_item_lists)
    {
        total_items += static_cast<int>(list.size());
        if (list.empty())
        {
            continue;
        }
        int endpoint_count = 0;
        for (board::Item* item : list)
        {
            if (dynamic_cast<board::Pin*>(item) != nullptr
                || dynamic_cast<board::ConductionArea*>(item) != nullptr)
            {
                ++endpoint_count;
            }
        }
        max_connections += std::max(0, endpoint_count - 1);
    }
    logger::FRLogger::instance().trace("Calculated maxConnections=" + std::to_string(max_connections)
        + ", total_items=" + std::to_string(total_items) + ", netCount="
        + std::to_string(max_net_no));

    net_incompletes_.assign(max_net_no, nullptr);
    for (int i = 0; i < max_net_no; ++i)
    {
        int net_no = i + 1;
        net_incompletes_[i] =
            std::make_shared<interactive::NetIncompletes>(net_no, net_item_lists[i], routing_board_);
    }
}

void DesignRulesChecker::recalculate_net_incompletes(int p_net_no)
{
    if (net_incompletes_.empty())
    {
        calculate_all_incompletes();
        return;
    }
    if (p_net_no >= 1 && p_net_no <= static_cast<int>(net_incompletes_.size()))
    {
        net_incompletes_[p_net_no - 1] = std::make_shared<interactive::NetIncompletes>(
            p_net_no,
            routing_board_->get_connectable_items(p_net_no),
            routing_board_);
    }
}

int DesignRulesChecker::get_incomplete_count()
{
    if (net_incompletes_.empty())
    {
        calculate_all_incompletes();
    }
    int result = 0;
    for (const auto& net_incompletes : net_incompletes_)
    {
        result += net_incompletes->count();
    }
    logger::FRLogger::instance().trace("Total incomplete count: " + std::to_string(result));
    return result;
}

int DesignRulesChecker::get_incomplete_count(int p_net_no)
{
    if (net_incompletes_.empty())
    {
        calculate_all_incompletes();
    }
    if (p_net_no <= 0 || p_net_no > static_cast<int>(net_incompletes_.size()))
    {
        return 0;
    }
    return net_incompletes_[p_net_no - 1]->count();
}

int DesignRulesChecker::get_length_violation_count()
{
    if (net_incompletes_.empty())
    {
        calculate_all_incompletes();
    }
    int result = 0;
    for (const auto& net_incompletes : net_incompletes_)
    {
        if (net_incompletes->get_length_violation() != 0.0)
        {
            ++result;
        }
    }
    return result;
}

double DesignRulesChecker::get_length_violation(int p_net_no)
{
    if (net_incompletes_.empty())
    {
        calculate_all_incompletes();
    }
    if (p_net_no <= 0 || p_net_no > static_cast<int>(net_incompletes_.size()))
    {
        return 0.0;
    }
    return net_incompletes_[p_net_no - 1]->get_length_violation();
}

bool DesignRulesChecker::recalculate_length_violations()
{
    if (net_incompletes_.empty())
    {
        calculate_all_incompletes();
        return true; // Technically changed from nothing to something
    }
    bool result = false;
    for (const auto& net_incompletes : net_incompletes_)
    {
        if (net_incompletes->calc_length_violation())
        {
            result = true;
        }
    }
    return result;
}

std::vector<std::shared_ptr<interactive::RatsNest::AirLine>>
    DesignRulesChecker::get_all_airlines()
{
    if (net_incompletes_.empty())
    {
        calculate_all_incompletes();
    }
    std::vector<std::shared_ptr<interactive::RatsNest::AirLine>> result;
    for (const auto& net_incompletes : net_incompletes_)
    {
        for (const auto& line : net_incompletes->incompletes())
        {
            result.push_back(line);
        }
    }
    return result;
}

interactive::NetIncompletes* DesignRulesChecker::get_net_incompletes(int p_net_no)
{
    if (net_incompletes_.empty())
    {
        calculate_all_incompletes();
    }
    if (p_net_no <= 0 || p_net_no > static_cast<int>(net_incompletes_.size()))
    {
        return nullptr;
    }
    return net_incompletes_[p_net_no - 1].get();
}

DrcSummaryResponse DesignRulesChecker::generate_summary()
{
    DrcSummaryResponse summary;

    auto violations = get_all_clearance_violations();
    summary.clearance_violations_count = static_cast<int>(violations.size());

    std::vector<UnconnectedItems> unconnecteds = get_all_unconnected_items();
    std::set<int> unconnected_net_numbers;
    int dangling_stubs_count = 0;
    for (const UnconnectedItems& u : unconnecteds)
    {
        if (u.get_type() == "track_dangling" || u.get_type() == "via_dangling")
        {
            ++dangling_stubs_count;
        }
        else if (u.get_first() != nullptr && !u.get_first()->get_net_nos().empty())
        {
            unconnected_net_numbers.insert(u.get_first()->get_net_nos()[0]);
        }
        else
        {
            ++dangling_stubs_count;
        }
    }

    double mm_per_board_unit = 0.0;
    board::Communication* comm = (routing_board_ != nullptr) ? routing_board_->communication() : nullptr;
    board::Unit board_unit = (comm != nullptr) ? comm->get_unit() : board::Unit::MIL;
    int resolution = (comm != nullptr) ? comm->get_resolution() : 1;
    mm_per_board_unit =
        board::UnitHelper::scale(1.0, board_unit, board::Unit::MM) / (resolution > 0 ? resolution : 1);
    auto to_mm = [mm_per_board_unit](double p_value) { return p_value * mm_per_board_unit; };

    // 1. Process clearance violations
    for (const auto& v : violations)
    {
        std::string layer_name;
        if (routing_board_ != nullptr && v->get_layer() >= 0
            && v->get_layer() < routing_board_->get_layer_count())
        {
            layer_name = routing_board_->get_layer_structure().get_layer(v->get_layer())->name;
        }
        else
        {
            layer_name = "Layer " + std::to_string(v->get_layer());
        }

        std::string item1_desc = format_item_with_context(v->get_first_item());
        std::string item2_desc = format_item_with_context(v->get_second_item());

        double expected_mm = to_mm(v->get_expected_clearance());
        double actual_mm = to_mm(v->get_actual_clearance());
        double shortfall_mm = std::max(0.0, expected_mm - actual_mm);

        std::ostringstream explanation_stream;
        explanation_stream << std::fixed << std::setprecision(3);
        explanation_stream << "Clearance violation between " << item1_desc << " and " << item2_desc
                           << " on " << layer_name << ": required " << expected_mm
                           << " mm, found " << actual_mm << " mm (shortfall: " << shortfall_mm
                           << " mm).";

        std::vector<std::string> items = {item1_desc, item2_desc};
        summary.violations.emplace_back("clearance",
                                        "error",
                                        layer_name,
                                        explanation_stream.str(),
                                        expected_mm,
                                        actual_mm,
                                        shortfall_mm,
                                        items);
    }

    // 2. Process unconnected items
    for (const UnconnectedItems& u : unconnecteds)
    {
        std::string item1_desc = format_item_with_context(u.get_first());
        std::string item2_desc = (u.get_second() != nullptr) ? format_item_with_context(u.get_second())
                                                              : std::string();

        std::string net_name = "unknown";
        if (u.get_first() != nullptr && !u.get_first()->get_net_nos().empty())
        {
            net_name = get_net_name(u.get_first()->get_net_nos()[0]);
        }

        std::string explanation;
        if (u.get_type() == "track_dangling")
        {
            explanation = "Dangling trace segment on net [" + net_name
                + "] with unconnected endpoint.";
        }
        else if (u.get_type() == "via_dangling")
        {
            explanation = "Dangling via on net [" + net_name + "] connected on at most one layer.";
        }
        else if (!item2_desc.empty())
        {
            std::ostringstream explanation_stream;
            explanation_stream << "Unconnected net [" << net_name << "]: break between "
                               << item1_desc << " and " << item2_desc << " ("
                               << u.get_all_items().size() << " elements).";
            explanation = explanation_stream.str();
        }
        else
        {
            explanation = "Unconnected item on net [" + net_name + "]: " + item1_desc + ".";
        }

        std::vector<std::string> items;
        if (u.get_first() != nullptr)
        {
            items.push_back(item1_desc);
        }
        if (!item2_desc.empty())
        {
            items.push_back(item2_desc);
        }

        summary.violations.emplace_back(u.get_type(),
                                        "warning",
                                        "all",
                                        explanation,
                                        std::nullopt,
                                        std::nullopt,
                                        std::nullopt,
                                        items);
    }

    // 3. Process zone island violations
    auto zone_violations = get_zone_island_violations();
    for (const ZoneIslandViolation& zv : zone_violations)
    {
        std::string layer_name;
        if (routing_board_ != nullptr && zv.get_layer() >= 0
            && zv.get_layer() < routing_board_->get_layer_count())
        {
            layer_name = routing_board_->get_layer_structure().get_layer(zv.get_layer())->name;
        }
        else
        {
            layer_name = "Layer " + std::to_string(zv.get_layer());
        }

        std::string net_name = "unknown";
        if (routing_board_ != nullptr && zv.get_net_number() > 0)
        {
            rules::Net* net = routing_board_->get_rules().get_nets().get(zv.get_net_number());
            if (net != nullptr)
            {
                net_name = net->get_name();
            }
        }

        std::vector<std::string> items;
        items.push_back("Zone on " + layer_name + " [" + net_name + "]");
        for (board::Item* it : zv.get_items_in_island())
        {
            items.push_back(format_item_with_context(it));
        }

        std::string explanation;
        std::string severity;
        if (zv.get_type() == "isolated_island_unconnected")
        {
            severity = "error";
            if (zv.get_net_number() > 0)
            {
                unconnected_net_numbers.insert(zv.get_net_number());
            }
            else
            {
                ++dangling_stubs_count;
            }
            std::ostringstream explanation_stream;
            explanation_stream << "Copper pour on " << layer_name << " [net " << net_name
                               << "] is fragmented: island contains "
                               << zv.get_items_in_island().size()
                               << " pin(s)/via(s) disconnected from main pour.";
            explanation = explanation_stream.str();
        }
        else
        {
            severity = "warning";
            double area_mm2 = zv.get_area_in_board_units()
                / (get_resolution_mm() * get_resolution_mm());
            std::ostringstream explanation_stream;
            explanation_stream << std::fixed << std::setprecision(2);
            explanation_stream << "Dead copper island detected on " << layer_name << " [net "
                               << net_name << "]: area " << area_mm2
                               << " mm\xC2\xB2 with no electrical connections.";
            explanation = explanation_stream.str();
        }

        summary.violations.emplace_back(zv.get_type(),
                                        severity,
                                        layer_name,
                                        explanation,
                                        std::nullopt,
                                        std::nullopt,
                                        std::nullopt,
                                        items);
    }

    summary.unconnected_nets_count = static_cast<int>(unconnected_net_numbers.size()) + dangling_stubs_count;

    // 4. Detect spatial congestion zones
    summary.congestion_zones = detect_congestion_zones(violations);

    // 5. Generate actionable hints
    summary.hints = generate_actionable_hints(violations, unconnecteds, summary.congestion_zones);

    return summary;
}

std::string DesignRulesChecker::format_item_with_context(const board::Item* item) const
{
    if (item == nullptr)
    {
        return "none";
    }
    std::string result;
    const board::Pin* pin = dynamic_cast<const board::Pin*>(item);
    if (pin != nullptr)
    {
        result += "Pin ";
        board::Component* comp = nullptr;
        if (routing_board_ != nullptr)
        {
            const board::Components* comps = routing_board_->components();
            if (comps != nullptr)
            {
                comp = comps->get(item->get_component_no());
            }
        }
        if (comp != nullptr)
        {
            result += comp->get_name() + ".";
            // core::Package::Pin has no name field (documented deviation: use the pin number).
            result += std::to_string(pin->get_pin_no());
        }
        else
        {
            result += "#" + std::to_string(pin->get_pin_no());
        }
    }
    else if (item->is_trace())
    {
        result = "Trace";
    }
    else if (item->is_via())
    {
        result = "Via";
    }
    else if (dynamic_cast<const board::ConductionArea*>(item) != nullptr)
    {
        result = "ConductionArea";
    }
    else
    {
        result = class_simple_name(item);
    }

    if (!item->get_net_nos().empty())
    {
        result += " (net " + get_net_name(item->get_net_nos()[0]) + ")";
    }
    return result;
}

std::vector<CongestionZone> DesignRulesChecker::detect_congestion_zones(
    const std::vector<std::shared_ptr<ClearanceViolation>>& p_violations)
{
    std::vector<CongestionZone> zones;
    if (p_violations.empty())
    {
        return zones;
    }

    double mm_per_board_unit = 0.0;
    board::Communication* comm = (routing_board_ != nullptr) ? routing_board_->communication() : nullptr;
    board::Unit board_unit = (comm != nullptr) ? comm->get_unit() : board::Unit::MIL;
    int resolution = (comm != nullptr) ? comm->get_resolution() : 1;
    mm_per_board_unit =
        board::UnitHelper::scale(1.0, board_unit, board::Unit::MM) / (resolution > 0 ? resolution : 1);
    auto to_mm = [mm_per_board_unit](double p_value) { return p_value * mm_per_board_unit; };

    // Simple centroid clustering with a 5.0 mm neighborhood threshold
    const double cluster_radius_mm = 5.0;
    std::vector<planar::IntPoint> points;
    for (const auto& v : p_violations)
    {
        if (v->get_shape() != nullptr)
        {
            points.push_back(v->get_shape()->centre_of_gravity().round());
        }
    }

    std::vector<bool> visited(points.size(), false);
    for (size_t i = 0; i < points.size(); ++i)
    {
        if (visited[i])
        {
            continue;
        }
        visited[i] = true;
        const planar::IntPoint& p1 = points[i];
        double p1_x_mm = to_mm(p1.to_float().x);
        double p1_y_mm = to_mm(p1.to_float().y);

        int cluster_count = 1;
        double sum_x = p1_x_mm;
        double sum_y = p1_y_mm;

        for (size_t j = i + 1; j < points.size(); ++j)
        {
            if (visited[j])
            {
                continue;
            }
            const planar::IntPoint& p2 = points[j];
            double p2_x_mm = to_mm(p2.to_float().x);
            double p2_y_mm = to_mm(p2.to_float().y);
            double dx = p1_x_mm - p2_x_mm;
            double dy = p1_y_mm - p2_y_mm;
            if (std::hypot(dx, dy) <= cluster_radius_mm)
            {
                visited[j] = true;
                ++cluster_count;
                sum_x += p2_x_mm;
                sum_y += p2_y_mm;
            }
        }

        if (cluster_count >= 2)
        {
            double avg_x = sum_x / cluster_count;
            double avg_y = sum_y / cluster_count;
            std::ostringstream description_stream;
            description_stream << std::fixed << std::setprecision(1);
            description_stream << "High bottleneck concentration: " << cluster_count
                               << " violations within " << cluster_radius_mm << " mm of (";
            description_stream << std::setprecision(2);
            description_stream << avg_x << ", " << avg_y << ") mm.";
            zones.emplace_back(avg_x, avg_y, cluster_radius_mm, cluster_count, description_stream.str());
        }
    }
    return zones;
}

std::vector<std::string> DesignRulesChecker::generate_actionable_hints(
    const std::vector<std::shared_ptr<ClearanceViolation>>& p_violations,
    const std::vector<UnconnectedItems>& p_unconnecteds,
    const std::vector<CongestionZone>& p_zones)
{
    std::vector<std::string> hints;

    double mm_per_board_unit = 0.0;
    board::Communication* comm = (routing_board_ != nullptr) ? routing_board_->communication() : nullptr;
    board::Unit board_unit = (comm != nullptr) ? comm->get_unit() : board::Unit::MIL;
    int resolution = (comm != nullptr) ? comm->get_resolution() : 1;
    mm_per_board_unit =
        board::UnitHelper::scale(1.0, board_unit, board::Unit::MM) / (resolution > 0 ? resolution : 1);
    auto to_mm = [mm_per_board_unit](double p_value) { return p_value * mm_per_board_unit; };

    if (!p_violations.empty())
    {
        double max_shortfall = 0.0;
        for (const auto& v : p_violations)
        {
            max_shortfall = std::max(max_shortfall,
                                     to_mm(v->get_expected_clearance()) - to_mm(v->get_actual_clearance()));
        }
        if (max_shortfall > 0.0)
        {
            std::ostringstream hint_stream;
            hint_stream << std::fixed << std::setprecision(3);
            hint_stream << "Maximum clearance shortfall is " << max_shortfall
                        << " mm. Consider reducing trace width or clearance class threshold if manufacturing constraints permit.";
            hints.push_back(hint_stream.str());
        }
    }

    if (!p_zones.empty())
    {
        std::ostringstream hint_stream;
        hint_stream << "Detected " << p_zones.size()
                    << " congested hotspot zone(s). Increasing pin escape distance or spreading component pads in these areas may resolve routing conflicts.";
        hints.push_back(hint_stream.str());
    }

    if (!p_unconnecteds.empty())
    {
        long dangling_count = 0;
        for (const UnconnectedItems& u : p_unconnecteds)
        {
            if (u.get_type() == "track_dangling" || u.get_type() == "via_dangling")
            {
                ++dangling_count;
            }
        }
        if (dangling_count > 0)
        {
            hints.push_back(std::to_string(dangling_count)
                + " dangling stub(s) detected. Running an optimizer cleanup pass will prune unneeded stubs.");
        }
        long incomplete_net_count = static_cast<long>(p_unconnecteds.size()) - dangling_count;
        if (incomplete_net_count > 0)
        {
            hints.push_back(std::to_string(incomplete_net_count)
                + " unrouted net connection(s) remain. Verify layer count or allow additional routing passes (-mp / maxPasses).");
        }
    }

    if (hints.empty())
    {
        hints.push_back("All design rules satisfied. Board is DRC clean.");
    }

    return hints;
}

} // namespace drc
} // namespace freerouting