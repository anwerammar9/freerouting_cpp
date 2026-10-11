#include "designforms/specctra/parser/Wiring.h"

#include <cmath>
#include <memory>
#include <regex>
#include <string>

#include "board/BoardLibrary.h"
#include "board/ConductionArea.h"
#include "board/Item.h"
#include "board/ItemSelectionFilter.h"
#include "board/Padstack.h"
#include "board/Padstacks.h"
#include "board/PolylineTrace.h"
#include "board/RoutingBoard.h"
#include "board/Trace.h"
#include "board/Via.h"
#include "designforms/specctra/parser/CoordinateTransform.h"
#include "designforms/specctra/parser/DsnFile.h"
#include "designforms/specctra/parser/IJFlexScanner.h"
#include "designforms/specctra/parser/Keyword.h"
#include "designforms/specctra/parser/Layer.h"
#include "designforms/specctra/parser/Path.h"
#include "designforms/specctra/parser/ReadScopeParameter.h"
#include "designforms/specctra/parser/Shape.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/Polygon.h"
#include "geometry/planar/Polyline.h"
#include "logger/FRLogger.h"
#include "rules/BoardRules.h"
#include "rules/DefaultItemClearanceClasses.h"
#include "rules/Net.h"
#include "rules/NetClass.h"

namespace freerouting::designforms::specctra::parser {

namespace fp = geometry::planar;

namespace {

/** Java (int) Math.round(double). */
int java_round(double p_value) { return static_cast<int>(std::floor(p_value + 0.5)); }

/** Java currentPadstackName.replaceAll("\\.\\d+", ""). */
std::string clean_padstack_name(const std::string& p_name)
{
    static const std::regex kSuffix("\\.\\d+");
    return std::regex_replace(p_name, kSuffix, "");
}

/** Java (int) cast of a double (truncation toward zero), used in a warning text. */
std::string java_int_text(double p_value) { return std::to_string(static_cast<long long>(p_value)); }

} // namespace

board::FixedState Wiring::calc_fixed(IJFlexScanner& p_scanner)
{
    board::FixedState result = board::FixedState::NOT_FIXED; // Java UNFIXED
    Token next_token = p_scanner.next_token();
    if (next_token.is(Keyword::SHOVE_FIXED))
    {
        result = board::FixedState::SHOVE_FIXED;
    }
    else if (next_token.is(Keyword::FIX))
    {
        result = board::FixedState::SYSTEM_FIXED;
    }
    else if (!next_token.is(Keyword::NORMAL))
    {
        result = board::FixedState::USER_FIXED;
    }
    next_token = p_scanner.next_token();
    if (!next_token.is(Keyword::CLOSED_BRACKET))
    {
        logger::FRLogger::instance().warn("Wiring.is_fixed: ) expected at '" + p_scanner.get_scope_identifier() + "'");
        return board::FixedState::NOT_FIXED;
    }
    return result;
}

Net::Id Wiring::read_net_id(IJFlexScanner& p_scanner)
{
    int subnet_number = 0;
    const std::string net_name = p_scanner.next_string();
    p_scanner.set_scope_identifier(net_name);
    Token next_token = p_scanner.next_token();
    if (next_token.is_int())
    {
        subnet_number = next_token.int_value;
        next_token = p_scanner.next_token();
    }
    if (!next_token.is(Keyword::CLOSED_BRACKET))
    {
        logger::FRLogger::instance().warn(
            "Wiring.read_net_id: closing bracket expected at '" + p_scanner.get_scope_identifier() + "'");
    }
    return Net::Id{net_name, subnet_number};
}

std::vector<rules::Net*> Wiring::get_subnets(const std::optional<Net::Id>& p_net_id, rules::BoardRules& p_rules)
{
    std::vector<rules::Net*> found_nets;
    if (p_net_id)
    {
        if (p_net_id->subnet_number > 0)
        {
            if (rules::Net* found_net = p_rules.get_nets().get(p_net_id->name, p_net_id->subnet_number))
            {
                found_nets.push_back(found_net);
            }
        }
        else
        {
            found_nets = p_rules.get_nets().get_all_by_name(p_net_id->name);
        }
    }
    return found_nets;
}

bool Wiring::via_exists(const fp::IntPoint& p_location, const board::Padstack& p_padstack,
    const std::vector<int>& p_net_numbers, const board::RoutingBoard& p_board)
{
    const board::ItemSelectionFilter filter(board::ItemSelectionFilter::SelectableChoices::VIAS);
    const int from_layer = p_padstack.get_from_layer();
    const int to_layer = p_padstack.get_to_layer();
    for (board::Item* current_item : p_board.pick_items(p_location, from_layer, &filter))
    {
        const auto* current_via = dynamic_cast<const board::Via*>(current_item);
        if (current_via != nullptr && current_via->nets_equal(p_net_numbers) && current_via->get_center() == p_location
            && current_via->first_layer() == from_layer && current_via->last_layer() == to_layer)
        {
            return true;
        }
    }
    return false;
}

bool Wiring::read_scope(ReadScopeParameter& p_par) const
{
    logger::FRLogger& log = logger::FRLogger::instance();
    Token next_token;
    for (;;)
    {
        const Token prev_token = next_token;
        next_token = p_par.scanner.next_token();
        if (next_token.is_eof())
        {
            log.warn("Wiring.read_scope: unexpected end of file at '" + p_par.scanner.get_scope_identifier() + "'");
            return false;
        }
        if (next_token.is(Keyword::CLOSED_BRACKET))
        {
            // end of scope
            break;
        }
        bool read_ok = true;
        if (prev_token.is(Keyword::OPEN_BRACKET))
        {
            if (next_token.is(Keyword::WIRE))
            {
                read_wire_scope(p_par);
            }
            else if (next_token.is(Keyword::VIA))
            {
                read_ok = read_via_scope(p_par);
            }
            else
            {
                skip_scope(p_par.scanner);
            }
        }
        if (!read_ok)
        {
            return false;
        }
    }
    board::RoutingBoard* board = p_par.get_board();
    try
    {
        board->normalize_all_traces();
    }
    catch (const std::exception&)
    {
        const std::string msg = "Wiring: normalization of traces failed";
        log.debug(msg);
        p_par.warnings.push_back(msg);
    }
    return true;
}

board::Item* Wiring::read_wire_scope(ReadScopeParameter& p_par)
{
    logger::FRLogger& log = logger::FRLogger::instance();
    std::optional<Net::Id> net_id;
    std::optional<std::string> clearance_class_name;
    board::FixedState fixed = board::FixedState::NOT_FIXED;
    std::unique_ptr<Shape> path; // Used, if a trace is read.
    std::unique_ptr<Shape> border_shape; // Used, if a conduction area is read.
    std::vector<std::unique_ptr<Shape>> hole_list;
    const LayerStructure* layer_structure = p_par.layer_structure.get();
    Token next_token;
    for (;;)
    {
        const Token prev_token = next_token;
        next_token = p_par.scanner.next_token();
        if (next_token.is_eof())
        {
            log.warn("Wiring.read_wire_scope: unexpected end of file at '" + p_par.scanner.get_scope_identifier()
                + "'");
            return nullptr;
        }
        if (next_token.is(Keyword::CLOSED_BRACKET))
        {
            // end of scope
            break;
        }
        if (prev_token.is(Keyword::OPEN_BRACKET))
        {
            if (next_token.is(Keyword::POLYGON_PATH))
            {
                path = Shape::read_polygon_path_scope(p_par.scanner, layer_structure);
            }
            else if (next_token.is(Keyword::POLYLINE_PATH))
            {
                path = Shape::read_polyline_path_scope(p_par.scanner, layer_structure);
            }
            else if (next_token.is(Keyword::RECTANGLE))
            {
                border_shape = Shape::read_rectangle_scope(p_par.scanner, layer_structure);
            }
            else if (next_token.is(Keyword::POLYGON))
            {
                border_shape = Shape::read_polygon_scope(p_par.scanner, layer_structure);
            }
            else if (next_token.is(Keyword::CIRCLE))
            {
                border_shape = Shape::read_circle_scope(p_par.scanner, layer_structure);
            }
            else if (next_token.is(Keyword::WINDOW))
            {
                hole_list.push_back(Shape::read_scope(p_par.scanner, layer_structure));
                // overread the closing bracket
                next_token = p_par.scanner.next_token();
                if (!next_token.is(Keyword::CLOSED_BRACKET))
                {
                    log.warn("Wiring.read_wire_scope: closing bracket expected at '"
                        + p_par.scanner.get_scope_identifier() + "'");
                    return nullptr;
                }
            }
            else if (next_token.is(Keyword::NET))
            {
                net_id = read_net_id(p_par.scanner);
            }
            else if (next_token.is(Keyword::CLEARANCE_CLASS))
            {
                clearance_class_name = DsnFile::read_string_scope(p_par.scanner);
            }
            else if (next_token.is(Keyword::TYPE))
            {
                fixed = calc_fixed(p_par.scanner);
            }
            else
            {
                skip_scope(p_par.scanner);
            }
        }
    }
    if (path == nullptr && border_shape == nullptr)
    {
        const std::string msg = "Wiring: wire has no shape at '" + p_par.scanner.get_scope_identifier() + "'";
        log.warn(msg);
        p_par.warnings.push_back(msg);
        return nullptr;
    }
    board::RoutingBoard* board = p_par.get_board();
    rules::BoardRules& board_rules = board->mutable_rules();
    const CoordinateTransform& transform = *p_par.coordinate_transform;

    rules::NetClass* net_class = board_rules.get_default_net_class();
    std::vector<int> net_numbers;
    for (rules::Net* current_net : get_subnets(net_id, board_rules))
    {
        net_numbers.push_back(current_net->get_net_no());
        net_class = current_net->get_net_class();
    }
    int clearance_class_index = -1;
    if (clearance_class_name)
    {
        clearance_class_index = board_rules.get_clearance_matrix().get_no(*clearance_class_name);
    }
    const auto* wire_path = dynamic_cast<const Path*>(path.get());
    int layer_index;
    int half_width;
    if (path != nullptr)
    {
        layer_index = path->layer->no;
        half_width = java_round(transform.dsn_to_board(wire_path->width / 2));
    }
    else
    {
        layer_index = border_shape->layer->no;
        half_width = 0;
    }
    if (layer_index < 0 || layer_index >= board->get_layer_count())
    {
        const std::string& layer_name = path != nullptr ? path->layer->name : border_shape->layer->name;
        const std::string msg = "Wiring: wire ignored — unknown layer '" + layer_name + "' at '"
            + p_par.scanner.get_scope_identifier() + "'";
        log.warn(msg);
        p_par.warnings.push_back(msg);
        return nullptr;
    }

    const fp::IntBox bounding_box = board->get_bounding_box();

    board::Item* result = nullptr;
    if (border_shape != nullptr)
    {
        if (clearance_class_index < 0)
        {
            clearance_class_index = net_class->get_default_item_clearance_classes().get(rules::ItemClass::AREA);
        }
        std::vector<std::unique_ptr<Shape>> area;
        area.push_back(std::move(border_shape));
        for (auto& hole : hole_list)
        {
            area.push_back(std::move(hole));
        }
        std::shared_ptr<fp::Area> conduction_area = to_board_area(Shape::transform_area_to_board(area, transform));
        result = board->insert_conduction_area(
            conduction_area, layer_index, net_numbers, clearance_class_index, false, fixed);
    }
    else if (dynamic_cast<const PolygonPath*>(path.get()) != nullptr)
    {
        if (clearance_class_index < 0)
        {
            clearance_class_index = net_class->get_default_item_clearance_classes().get(rules::ItemClass::TRACE);
        }
        const std::vector<double>& coordinates = wire_path->coordinate_arr;
        std::vector<fp::IntPoint> corners(coordinates.size() / 2);
        for (std::size_t i = 0; i < corners.size(); ++i)
        {
            const double x = coordinates[2 * i];
            const double y = coordinates[2 * i + 1];
            const fp::FloatPoint current_corner = transform.dsn_to_board(x, y);
            // Java IntBox.contains(FloatPoint) is the TileShape border-line test.
            if (!static_cast<const fp::TileShape&>(bounding_box).contains(current_corner))
            {
                const std::string msg = "Wiring: wire corner (" + java_int_text(x) + "," + java_int_text(y)
                    + ") is outside board bounds at '" + p_par.scanner.get_scope_identifier() + "'";
                log.warn(msg);
                p_par.warnings.push_back(msg);
                return nullptr;
            }
            corners[i] = current_corner.round();
        }

        const fp::Polygon polygon(corners);
        // if it doesn't have two different points, it's not a valid polygon, so we must skip it
        const std::vector<fp::IntPoint> polygon_corners = polygon.corner_array();
        // A wire is degenerate if it has fewer than 2 corners, or if all corners map to the same point
        // (zero-length trace); such traces cause infinite normalization cycles and must be skipped.
        bool has_distinct_corner = false;
        for (std::size_t i = 1; i < polygon_corners.size(); ++i)
        {
            if (!(polygon_corners[i] == polygon_corners[0]))
            {
                has_distinct_corner = true;
                break;
            }
        }
        const bool is_degenerate = polygon_corners.size() < 2 || !has_distinct_corner;
        if (!is_degenerate)
        {
            auto trace_polyline = std::make_shared<fp::Polyline>(fp::Polyline::from_corners(polygon_corners));
            // Traces are not yet normalized here because cycles may be removed premature.
            result = board->insert_trace_without_cleaning(
                trace_polyline, layer_index, half_width, net_numbers, clearance_class_index, fixed);
        }
        else
        {
            const std::string msg = "Wiring: degenerate wire trace skipped (all "
                + std::to_string(polygon_corners.size())
                + " corners are identical — zero-length trace) on layer '" + path->layer->name
                + "'. This is likely a DSN export issue in your EDA tool.";
            log.debug(msg);
            p_par.warnings.push_back(msg);
        }
    }
    else if (dynamic_cast<const PolylinePath*>(path.get()) != nullptr)
    {
        if (clearance_class_index < 0)
        {
            clearance_class_index = net_class->get_default_item_clearance_classes().get(rules::ItemClass::TRACE);
        }
        const std::vector<double>& coordinates = wire_path->coordinate_arr;
        std::vector<fp::Line> lines;
        lines.reserve(coordinates.size() / 4);
        for (std::size_t i = 0; i < coordinates.size() / 4; ++i)
        {
            const fp::FloatPoint current_a = transform.dsn_to_board(coordinates[4 * i], coordinates[4 * i + 1]);
            const fp::FloatPoint current_b = transform.dsn_to_board(coordinates[4 * i + 2], coordinates[4 * i + 3]);
            lines.emplace_back(current_a.round(), current_b.round());
        }
        auto trace_polyline = std::make_shared<fp::Polyline>(lines);
        result = board->insert_trace_without_cleaning(
            trace_polyline, layer_index, half_width, net_numbers, clearance_class_index, fixed);
    }
    else
    {
        log.warn("Wiring.read_wire_scope: unexpected Path subclass at '" + p_par.scanner.get_scope_identifier() + "'");
        return nullptr;
    }
    if (result != nullptr && result->net_count() == 0)
    {
        try_correct_net(result);
    }
    return result;
}

void Wiring::try_correct_net(board::Item* p_item)
{
    auto* current_trace = dynamic_cast<board::Trace*>(p_item);
    if (current_trace == nullptr)
    {
        return;
    }
    board::ItemSet contacts = current_trace->get_normal_contacts(current_trace->first_corner(), true);
    board::ItemSet last_contacts = current_trace->get_normal_contacts(current_trace->last_corner(), true);
    contacts.insert(last_contacts.begin(), last_contacts.end());
    int corrected_net_no = 0;
    for (board::Item* current_contact : contacts)
    {
        if (current_contact->net_count() == 1)
        {
            corrected_net_no = current_contact->get_net_no(0);
            break;
        }
    }
    if (corrected_net_no != 0)
    {
        p_item->assign_net_no(corrected_net_no);
    }
}

bool Wiring::read_via_scope(ReadScopeParameter& p_par)
{
    logger::FRLogger& log = logger::FRLogger::instance();
    board::FixedState fixed = board::FixedState::NOT_FIXED;
    // read the padstack name
    Token next_token = p_par.scanner.next_token();
    if (!next_token.is_string())
    {
        log.warn("Wiring.read_via_scope: padstack name expected at '" + p_par.scanner.get_scope_identifier() + "'");
        return false;
    }
    const std::string padstack_name = next_token.string_value;
    p_par.scanner.set_scope_identifier(padstack_name);
    // read the location
    double location[2] = {0, 0};
    for (double& coordinate : location)
    {
        next_token = p_par.scanner.next_token();
        if (next_token.is_number())
        {
            coordinate = next_token.number();
        }
        else
        {
            log.warn("Wiring.read_via_scope: number expected at '" + p_par.scanner.get_scope_identifier() + "'");
            return false;
        }
    }
    std::optional<Net::Id> net_id;
    std::optional<std::string> clearance_class_name;
    for (;;)
    {
        const Token prev_token = next_token;
        next_token = p_par.scanner.next_token();
        if (next_token.is_eof())
        {
            log.warn(
                "Wiring.read_via_scope: unexpected end of file at '" + p_par.scanner.get_scope_identifier() + "'");
            return false;
        }
        if (next_token.is(Keyword::CLOSED_BRACKET))
        {
            // end of scope
            break;
        }
        if (prev_token.is(Keyword::OPEN_BRACKET))
        {
            if (next_token.is(Keyword::NET))
            {
                net_id = read_net_id(p_par.scanner);
            }
            else if (next_token.is(Keyword::CLEARANCE_CLASS))
            {
                clearance_class_name = DsnFile::read_string_scope(p_par.scanner);
            }
            else if (next_token.is(Keyword::TYPE))
            {
                fixed = calc_fixed(p_par.scanner);
            }
            else
            {
                skip_scope(p_par.scanner);
            }
        }
    }
    board::RoutingBoard* board = p_par.get_board();
    rules::BoardRules& board_rules = board->mutable_rules();
    board::Padstack* current_padstack = board->library().padstacks().get(clean_padstack_name(padstack_name));
    if (current_padstack == nullptr)
    {
        const std::string msg = "Wiring: via padstack '" + padstack_name + "' not found at '"
            + p_par.scanner.get_scope_identifier() + "'";
        log.warn(msg);
        p_par.warnings.push_back(msg);
        return false;
    }
    rules::NetClass* net_class = board_rules.get_default_net_class();
    const std::vector<rules::Net*> found_nets = get_subnets(net_id, board_rules);
    if (net_id && found_nets.empty())
    {
        const std::string msg = "Wiring: via net '" + net_id->name + "' not found at '"
            + p_par.scanner.get_scope_identifier() + "'";
        log.warn(msg);
        p_par.warnings.push_back(msg);
    }
    std::vector<int> net_numbers(found_nets.size(), 0);
    for (rules::Net* current_net : found_nets)
    {
        // Java parity: currentIndex is never incremented, so every net is written to index 0 and
        // the other entries stay 0.
        net_numbers[0] = current_net->get_net_no();
        net_class = current_net->get_net_class();
    }
    int clearance_class_index = -1;
    if (clearance_class_name)
    {
        clearance_class_index = board_rules.get_clearance_matrix().get_no(*clearance_class_name);
    }
    if (clearance_class_index < 0)
    {
        clearance_class_index = net_class->get_default_item_clearance_classes().get(rules::ItemClass::VIA);
    }
    const fp::IntPoint board_location = p_par.coordinate_transform->dsn_to_board(location[0], location[1]).round();
    if (via_exists(board_location, *current_padstack, net_numbers, *board))
    {
        const std::string msg = "Wiring: duplicate via skipped at (" + std::to_string(board_location.x) + ", "
            + std::to_string(board_location.y) + ")";
        log.warn(msg);
        p_par.warnings.push_back(msg);
    }
    else
    {
        const bool attach_allowed = p_par.via_at_smd_allowed && current_padstack->is_attach_allowed();
        board->insert_via(
            current_padstack, board_location, net_numbers, clearance_class_index, fixed, attach_allowed);
    }
    return true;
}

} // namespace freerouting::designforms::specctra::parser
