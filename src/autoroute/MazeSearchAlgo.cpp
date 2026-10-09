#include "autoroute/MazeSearchAlgo.h"
#include "autoroute/MazeExpansionEngine.h"
#include "autoroute/DrillPage.h"
#include "autoroute/DrillPageArray.h"
#include "board/Via.h"

#include "autoroute/AutorouteEngine.h"
#include "autoroute/DrillPage.h"
#include "autoroute/ExpansionDrill.h"
#include "autoroute/MazeFanoutDiagnostics.h"
#include "autoroute/ItemAutorouteInfo.h"
#include "autoroute/MazeRipupResolver.h"
#include "autoroute/MazeTraceShover.h"
#include "board/BasicBoard.h"
#include "board/ConductionArea.h"
#include "board/Item.h"
#include "board/ItemSelectionFilter.h"
#include "board/Pin.h"
#include "board/PolylineTrace.h"
#include "board/RoutingBoard.h"
#include "board/Communication.h"
#include "board/ShapeSearchTree.h"
#include "board/Trace.h"
#include "board/Via.h"
#include "datastructures/Stoppable.h"
#include "geometry/planar/FloatLine.h"
#include "geometry/planar/IntOctagon.h"
#include "geometry/planar/Simplex.h"
#include "geometry/planar/TileShape.h"
#include "logger/FRLogger.h"
#include "rules/BoardRules.h"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace freerouting::autoroute {

using geometry::planar::FloatLine;
using geometry::planar::FloatPoint;
using geometry::planar::IntBox;
using geometry::planar::TileShape;
using logger::FRLogger;

MazeSearchAlgo::MazeSearchAlgo(board::RoutingBoard* p_board, datastructures::Stoppable* p_stoppable)
    : expansion_engine_(std::make_unique<MazeExpansionEngine>(this))
    , board_(p_board)
    , stoppable_(p_stoppable)
    , fanout_diagnostics_(std::make_unique<MazeFanoutDiagnostics>(control_))
    , ripup_resolver_(std::make_unique<MazeRipupResolver>(this))
{
    random_generator_.set_seed(control_.ripup_costs);
}

MazeSearchAlgo::~MazeSearchAlgo() = default;

void MazeSearchAlgo::set_control(const AutorouteControl& p_ctrl)
{
    control_ = p_ctrl;
    random_generator_.set_seed(control_.ripup_costs);
    // The diagnostics hold a reference to the control of this search, so they
    // need to be recreated when the control is replaced.
    fanout_diagnostics_ = std::make_unique<MazeFanoutDiagnostics>(control_);
}

bool MazeSearchAlgo::passes_fanout_filter(const MazeListElement& p_element) const
{
    // Port of the anonymous TreeSet of Java MazeSearchEngine, which limits the
    // expansion of a fanout pass to the neighborhood of the fanout pin.
    if (!control_.is_fanout || !control_.fanout_start_pin_center.has_value())
    {
        return true;
    }
    const FloatPoint pin_center_float = *control_.fanout_start_pin_center;

    // The board resolution in micrometers. Divergence: Java scales the
    // resolution with Unit.scale(resolution, Unit.UM, unit), which converts
    // from the board unit to micrometers with inverted arguments. That
    // Java behaviour is reproduced here, so that the fanout limits stay the
    // same as in Java.
    double resolution = 1.0;
    if (board_ != nullptr && board_->communication() != nullptr)
    {
        resolution = board_->communication()->get_resolution(board::Unit::UM);
    }

    const bool on_start_layer = p_element.next_room != nullptr
        && p_element.next_room->get_layer() == control_.fanout_start_pin_layer;
    if (on_start_layer)
    {
        // Java RouterSettings.fanout.maxEscapeLengthMm, with a default of 3000 um.
        const double max_len = control_.max_escape_length_um.value_or(3000.0);
        const FloatPoint entry_point = p_element.shape_entry.a().middle_point(p_element.shape_entry.b());
        const double dist = entry_point.distance(pin_center_float);
        if (dist > max_len * resolution)
        {
            fanout_diagnostics_->trace("reject_far_on_start_layer",
                "dist=" + std::to_string(dist) + ", limit=" + std::to_string(max_len * resolution));
            return false;
        }
    }
    const auto* drill = dynamic_cast<const ExpansionDrill*>(p_element.door);
    if (drill != nullptr)
    {
        // Java RouterSettings.fanout.minEscapeLengthMm, with a default of 500 um.
        const double min_len = control_.min_escape_length_um.value_or(500.0);
        const double drill_dist = drill->location.to_float().distance(pin_center_float);
        if (drill_dist < min_len * resolution)
        {
            fanout_diagnostics_->trace("reject_near_drill",
                "drill_dist=" + std::to_string(drill_dist) + ", limit="
                    + std::to_string(min_len * resolution));
            return false;
        }
    }
    return true;
}

namespace {

/**
 * Looks for pins with more than 1 nets and reduces the shapes of traces of
 * foreign nets, which are already connected to such a pin, so that the pin
 * center is not blocked for connection. Port of the Java private static
 * MazeSearchEngine.reduceTraceShapesAtTiePins(Collection<Item>, int,
 * ShapeSearchTree).
 */
void reduce_trace_shapes_at_tie_pins(const std::set<board::Item*>& p_item_list, int p_own_net_no,
    board::ShapeSearchTree* p_search_tree)
{
    if (p_search_tree == nullptr)
    {
        return;
    }
    for (board::Item* current_item : p_item_list)
    {
        if (current_item == nullptr)
        {
            continue;
        }
        auto* current_tie_pin = dynamic_cast<board::Pin*>(current_item);
        if (current_tie_pin == nullptr || current_item->net_count() <= 1)
        {
            continue;
        }
        for (board::Item* current_contact : current_item->get_normal_contacts())
        {
            auto* current_trace = dynamic_cast<board::PolylineTrace*>(current_contact);
            if (current_trace == nullptr || current_trace->contains_net(p_own_net_no))
            {
                continue;
            }
            p_search_tree->reduce_trace_shape_at_tie_pin(current_tie_pin, current_trace);
        }
    }
}

} // namespace

std::unique_ptr<MazeSearchAlgo> MazeSearchAlgo::get_instance(board::RoutingBoard* p_board,
    datastructures::Stoppable* p_stoppable)
{
    return std::make_unique<MazeSearchAlgo>(p_board, p_stoppable);
}

std::unique_ptr<MazeSearchAlgo> MazeSearchAlgo::get_instance(
    const std::set<board::Item*>& p_start_items,
    const std::set<board::Item*>& p_dest_items,
    AutorouteEngine* p_autoroute_engine,
    const AutorouteControl& p_ctrl,
    datastructures::Stoppable* p_stoppable)
{
    if (p_autoroute_engine == nullptr)
    {
        return nullptr;
    }
    std::unique_ptr<MazeSearchAlgo> result =
        std::make_unique<MazeSearchAlgo>(p_autoroute_engine->board(), p_stoppable);
    result->set_autoroute_engine(p_autoroute_engine);
    result->set_control(p_ctrl);
    result->search_tree_ = p_autoroute_engine->get_autoroute_search_tree();
    result->destination_distance_ = std::make_unique<DestinationDistance>(
        result->control_.trace_costs, result->control_.layer_active, result->control_.min_normal_via_cost,
        result->control_.min_cheap_via_cost);
    if (!result->init(p_start_items, p_dest_items))
    {
        return nullptr;
    }
    return result;
}

bool MazeSearchAlgo::init(const std::set<board::Item*>& p_start_items,
    const std::set<board::Item*>& p_dest_items)
{
    if (autoroute_engine_ == nullptr || search_tree_ == nullptr || destination_distance_ == nullptr)
    {
        return false;
    }

    // Looks for pins with more than 1 nets and reduces shapes of traces of
    // foreign nets, which are already connected to such a pin, so that the pin
    // center is not blocked for connection. Port of the Java private static
    // MazeSearchEngine.reduceTraceShapesAtTiePins calls at the start of init.
    reduce_trace_shapes_at_tie_pins(p_start_items, control_.net_no, search_tree_);
    reduce_trace_shapes_at_tie_pins(p_dest_items, control_.net_no, search_tree_);

    start_items_ = p_start_items;
    dest_items_ = p_dest_items;

    // process the destination items
    bool destination_ok = false;
    for (board::Item* current_item : p_dest_items)
    {
        if (is_stop_requested())
        {
            return false;
        }
        if (current_item == nullptr)
        {
            continue;
        }
        auto current_info = current_item->get_autoroute_info();
        current_info->set_start_info(false);
        for (int i = 0; i < current_item->tree_shape_count(*search_tree_); i++)
        {
            const TileShape* current_tree_shape = current_item->get_tree_shape(*search_tree_, i);
            if (current_tree_shape != nullptr)
            {
                destination_distance_->join(current_tree_shape->bounding_box(), current_item->shape_layer(i));
            }
        }
        destination_ok = true;
    }
    if (!destination_ok && control_.is_fanout)
    {
        // destination set is not needed for fanout
        IntBox board_bounding_box = board_ != nullptr ? board_->get_bounding_box() : IntBox::EMPTY;
        destination_distance_->join(board_bounding_box, 0);
        destination_distance_->join(board_bounding_box, control_.layer_count - 1);
        destination_ok = true;
    }

    if (!destination_ok)
    {
        FRLogger::instance().debug("MazeSearchAlgo.init: Failed - no valid destination items found (dest set size: "
                                   + std::to_string(p_dest_items.size()) + ", isFanout: "
                                   + (control_.is_fanout ? "1" : "0") + ")");
        return false;
    }

    // process the start items
    std::vector<IncompleteFreeSpaceExpansionRoom*> start_rooms;
    for (board::Item* current_item : p_start_items)
    {
        if (is_stop_requested())
        {
            return false;
        }
        if (current_item == nullptr)
        {
            continue;
        }
        auto current_info = current_item->get_autoroute_info();
        current_info->set_start_info(true);
        if (is_connectable(current_item))
        {
            for (int i = 0; i < current_item->tree_shape_count(*search_tree_); i++)
            {
                const TileShape* contained_shape = get_trace_connection_shape(current_item, i);
                if (contained_shape == nullptr)
                {
                    continue;
                }
                // Java MazeSearchEngine.init(): the start room shape is null,
                // i.e. the whole plane; the connection shape is the contained
                // shape of the room.
                IncompleteFreeSpaceExpansionRoom* new_start_room =
                    autoroute_engine_->add_incomplete_expansion_room(kWholePlaneShape,
                        current_item->shape_layer(i), contained_shape->to_Simplex());
                start_rooms.push_back(new_start_room);
            }
        }
    }

    // complete the start rooms
    std::vector<CompleteFreeSpaceExpansionRoom*> completed_start_rooms;

    if (autoroute_engine_->get_maintain_database())
    {
        // add the completed start rooms carried over from the last autoroute to the
        // start rooms.
        std::vector<CompleteFreeSpaceExpansionRoom*> carried_over =
            autoroute_engine_->get_rooms_with_target_items(p_start_items);
        completed_start_rooms.insert(completed_start_rooms.end(), carried_over.begin(), carried_over.end());
    }

    for (IncompleteFreeSpaceExpansionRoom* current_room : start_rooms)
    {
        if (is_stop_requested())
        {
            return false;
        }
        std::vector<CompleteFreeSpaceExpansionRoom*> current_completed_rooms =
            autoroute_engine_->complete_expansion_room(current_room);
        completed_start_rooms.insert(
            completed_start_rooms.end(), current_completed_rooms.begin(), current_completed_rooms.end());
    }

    // Put the ItemExpansionDoors of the completed start rooms into
    // the mazeExpansionList.
    bool start_ok = false;
    int expansion_doors_found = 0;
    int expansion_doors_destination = 0;
    for (CompleteFreeSpaceExpansionRoom* current_room : completed_start_rooms)
    {
        if (current_room == nullptr)
        {
            continue;
        }
        for (ExpansionDoor* door_base : current_room->target_doors())
        {
            auto* current_door = dynamic_cast<TargetItemExpansionDoor*>(door_base);
            if (current_door == nullptr)
            {
                continue;
            }
            expansion_doors_found++;
            if (is_stop_requested())
            {
                return false;
            }
            if (current_door->is_destination_door())
            {
                expansion_doors_destination++;
                continue;
            }
            const TileShape* connection_shape =
                get_trace_connection_shape(current_door->item, current_door->tree_entry_no);
            if (connection_shape == nullptr)
            {
                continue;
            }
            // Divergence: the C++ rooms are IntBox based, so the connection
            // shape is intersected with the room bounding box instead of the
            // exact Simplex room shape.
            IntBox connection_box = connection_shape->bounding_box().intersection(current_room->get_shape());
            if (connection_box.is_empty())
            {
                continue;
            }
            FloatPoint current_center = connection_box.centre_of_gravity();
            FloatLine shape_entry(current_center, current_center);
            double sorting_value = destination_distance_->calculate(current_center, current_room->get_layer());
            MazeListElement new_list_element(current_door, 0, nullptr, 0, 0, sorting_value, current_room,
                shape_entry, false, MazeSearchElement::Adjustment::NONE, false);
            maze_expansion_list_.insert(new_list_element);
            start_ok = true;
        }
    }
    if (!start_ok)
    {
        FRLogger::instance().debug(
            "MazeSearchAlgo.init: Failed - no accessible expansion doors found (start items: "
            + std::to_string(p_start_items.size()) + ", start rooms: " + std::to_string(start_rooms.size())
            + ", completed start rooms: " + std::to_string(completed_start_rooms.size())
            + ", expansion doors found: " + std::to_string(expansion_doors_found) + ", destination doors: "
            + std::to_string(expansion_doors_destination) + ", ripupAllowed: "
            + (control_.ripup_allowed ? "1" : "0") + ", ripupCosts: " + std::to_string(control_.ripup_costs) + ")");
    }
    initialized_ = start_ok;
    return start_ok;
}

MazeSearchAlgo::Result MazeSearchAlgo::find_connection()
{
    while (occupy_next_element())
    {
        continue;
    }
    Result result;
    if (destination_door_ == nullptr)
    {
        return result;
    }
    result.destination_door = destination_door_;
    result.section_no_of_door = section_no_of_destination_door_;
    return result;
}

bool MazeSearchAlgo::occupy_next_element()
{
    if (destination_door_ != nullptr)
    {
        return false; // destination already reached
    }

    std::optional<MazeListElement> list_element;
    MazeSearchElement* current_door_section = nullptr;
    // Search the next element, which is not yet expanded.
    bool next_element_found = false;
    while (!maze_expansion_list_.empty())
    {
        if (is_stop_requested())
        {
            return false;
        }

        auto it = maze_expansion_list_.begin();
        list_element = *it;
        maze_expansion_list_.erase(it);

        int current_section_no = list_element->section_no_of_door;
        if (list_element->door == nullptr)
        {
            list_element.reset();
            continue;
        }
        current_door_section = list_element->door->get_maze_search_element(current_section_no);
        if (current_door_section == nullptr)
        {
            list_element.reset();
            continue;
        }

        if (!current_door_section->is_occupied)
        {
            next_element_found = true;
            break;
        }
    }
    if (!next_element_found || !list_element)
    {
        return false;
    }

    current_door_section->backtrack_door = list_element->backtrack_door;
    current_door_section->section_no_of_backtrack_door = list_element->section_no_of_backtrack_door;
    current_door_section->room_ripped = list_element->room_ripped;
    current_door_section->ripup_cost = list_element->ripup_cost;
    current_door_section->adjustment = list_element->adjustment;

    if (dynamic_cast<DrillPage*>(list_element->door) != nullptr)
    {
        // A drill page is not expanded itself, but into its drills.
        expansion_engine_->expand_to_drills_of_page(*list_element);
        return true;
    }

    if (auto* current_door = dynamic_cast<TargetItemExpansionDoor*>(list_element->door))
    {
        if (current_door->is_destination_door())
        {
            // The destination is reached.
            destination_door_ = current_door;
            section_no_of_destination_door_ = list_element->section_no_of_door;
            return false;
        }
    }

    if (control_.is_fanout && dynamic_cast<ExpansionDrill*>(list_element->door) != nullptr
        && dynamic_cast<ExpansionDrill*>(list_element->backtrack_door) != nullptr)
    {
        // The fanout search is completed after the first drill.
        destination_door_ = list_element->door;
        section_no_of_destination_door_ = list_element->section_no_of_door;
        return false;
    }

    if (control_.vias_allowed && dynamic_cast<ExpansionDrill*>(list_element->door) != nullptr
        && dynamic_cast<ExpansionDrill*>(list_element->backtrack_door) == nullptr)
    {
        expansion_engine_->expand_to_other_layers(*list_element);
    }

    if (list_element->next_room != nullptr)
    {
        if (!expand_to_room_doors(*list_element))
        {
            return true; // occupation by ripup is delayed or nothing was expanded
            // In case nothing was expanded allow the section to be occupied from
            // somewhere else, if the next room is thin.
        }
    }
    current_door_section->is_occupied = true;
    return true;
}

bool MazeSearchAlgo::is_stop_requested() const
{
    if (stoppable_ != nullptr && stoppable_->is_stop_requested())
    {
        return true;
    }
    if (autoroute_engine_ != nullptr)
    {
        return autoroute_engine_->is_stop_requested();
    }
    return false;
}

bool MazeSearchAlgo::is_layer_active(int p_layer) const
{
    if (control_.layer_active.empty())
    {
        return true;
    }
    if (p_layer < 0 || static_cast<size_t>(p_layer) >= control_.layer_active.size())
    {
        FRLogger::instance().warn("MazeSearchAlgo.is_layer_active: layer out of range");
        return true;
    }
    return control_.layer_active[static_cast<size_t>(p_layer)];
}

double MazeSearchAlgo::compensated_trace_half_width_for_layer(int p_layer) const
{
    if (p_layer < 0 || static_cast<size_t>(p_layer) >= control_.compensated_trace_half_width.size())
    {
        FRLogger::instance().warn(
            "MazeSearchAlgo.compensated_trace_half_width_for_layer: layer out of range");
        return 0.0;
    }
    return control_.compensated_trace_half_width[static_cast<size_t>(p_layer)];
}

double MazeSearchAlgo::bend_cost_for_layer(int p_layer) const
{
    if (p_layer < 0 || static_cast<size_t>(p_layer) >= control_.bend_costs.size())
    {
        return 0.0;
    }
    return control_.bend_costs[static_cast<size_t>(p_layer)];
}

const ExpansionCostFactor& MazeSearchAlgo::trace_costs_for_layer(int p_layer) const
{
    static const ExpansionCostFactor kNeutral(1.0, 1.0);
    if (p_layer < 0 || static_cast<size_t>(p_layer) >= control_.trace_costs.size())
    {
        return kNeutral;
    }
    return control_.trace_costs[static_cast<size_t>(p_layer)];
}

bool MazeSearchAlgo::expand_to_room_doors(const MazeListElement& p_list_element)
{
    // Complete the neighbour rooms to make sure, that the
    // doors of this room will not change later on.
    CompleteExpansionRoom* next_room = p_list_element.next_room;
    if (next_room == nullptr)
    {
        return false;
    }
    int layer_index = next_room->get_layer();

    bool layer_active = is_layer_active(layer_index);
    if (!layer_active)
    {
        if (board_ != nullptr && layer_index >= 0
            && layer_index < board_->get_layer_structure().layer_count()
            && board_->get_layer_structure().get_layer(layer_index)->is_signal)
        {
            return true;
        }
    }

    double half_width = compensated_trace_half_width_for_layer(layer_index);
    bool current_door_is_small = false;
    if (auto* current_door = dynamic_cast<ExpansionDoor*>(p_list_element.door))
    {
        double half_width_add = half_width + AutorouteEngine::TRACE_WIDTH_TOLERANCE;
        if (control_.with_neckdown)
        {
            // try evtl. neckdown at a destination pin
            double neck_down_half_width = check_neck_down_at_dest_pin(next_room);
            if (neck_down_half_width > 0)
            {
                half_width_add = std::min(half_width_add, neck_down_half_width);
                half_width = half_width_add;
            }
        }
        current_door_is_small = door_is_small(current_door, 2 * half_width_add);
    }

    int door_count_before_completion = static_cast<int>(next_room->get_doors().size());
    if (autoroute_engine_ != nullptr)
    {
        autoroute_engine_->complete_neighbour_rooms(next_room);
    }
    int door_count_after_completion = static_cast<int>(next_room->get_doors().size());
    FRLogger::instance().trace("ROOM_COMPLETE_SYNC"
                               + std::string(", net=") + std::to_string(control_.net_no) + ", layer="
                               + std::to_string(layer_index) + ", from_section="
                               + std::to_string(p_list_element.section_no_of_door) + ", backtrack_section="
                               + std::to_string(p_list_element.section_no_of_backtrack_door) + ", from_door="
                               + describe_expandable(p_list_element.door) + ", nextRoom="
                               + describe_room(next_room) + ", door_count_before="
                               + std::to_string(door_count_before_completion) + ", door_count_after="
                               + std::to_string(door_count_after_completion));

    FloatPoint shape_entry_middle = p_list_element.shape_entry.a().middle_point(p_list_element.shape_entry.b());

    if (control_.with_neckdown)
    {
        if (auto* door = dynamic_cast<TargetItemExpansionDoor*>(p_list_element.door))
        {
            // try evtl. neckdown at a start pin
            if (auto* pin = dynamic_cast<board::Pin*>(door->item))
            {
                double neckdown_half_width = pin->get_trace_neckdown_halfwidth(layer_index);
                if (neckdown_half_width > 0)
                {
                    half_width = std::min(half_width, neckdown_half_width);
                }
            }
        }
    }

    bool next_room_is_thick = true;
    if (auto* room = dynamic_cast<ObstacleExpansionRoom*>(next_room))
    {
        next_room_is_thick = room_shape_is_thick(room);
    }
    else
    {
        geometry::planar::Simplex next_room_shape = next_room->get_shape_tile();
        if (next_room_shape.min_width() < 2 * half_width)
        {
            next_room_is_thick = false; // to prevent problems with the opposite side
        }
        else if (!p_list_element.already_checked && p_list_element.door != nullptr
                 && p_list_element.door->get_dimension() == 1 && !current_door_is_small)
        {
            // The algorithm below works only, if location is on the border of roomShape.
            // That is only the case for 1 dimensional doors.
            // For small doors the check is done in check_leaving_via below.
            std::vector<FloatPoint> nearest_points =
                next_room_shape.nearest_border_points_approx(shape_entry_middle, 2);
            if (nearest_points.size() < 2)
            {
                FRLogger::instance().warn(
                    "MazeSearchAlgo.expand_to_room_doors: nearestPoints.length == 2 expected");
                next_room_is_thick = false;
            }
            else
            {
                double current_distance = nearest_points[1].distance(shape_entry_middle);
                next_room_is_thick = current_distance > half_width + 1;
            }
        }
    }

    if (!layer_active)
    {
        if (auto* drill = dynamic_cast<ExpansionDrill*>(p_list_element.door))
        {
            // check for drill to a foreign conduction area on split plane.
            if (board_ != nullptr)
            {
                board::ItemSelectionFilter filter(board::ItemSelectionFilter::SelectableChoices::CONDUCTION);
                std::set<board::Item*> picked_items =
                    board_->pick_items(drill->location, layer_index, &filter);
                for (board::Item* current_item : picked_items)
                {
                    if (current_item == nullptr || !current_item->contains_net(control_.net_no))
                    {
                        return true;
                    }
                }
            }
        }
    }

    bool something_expanded =
        expand_to_target_doors(p_list_element, next_room_is_thick, current_door_is_small, shape_entry_middle);

    if (!layer_active)
    {
        return true;
    }

    int ripup_costs = 0;

    if (dynamic_cast<CompleteFreeSpaceExpansionRoom*>(next_room) != nullptr)
    {
        if (!p_list_element.already_checked)
        {
            if (current_door_is_small)
            {
                bool enter_through_small_door = false;
                if (next_room_is_thick)
                {
                    // Check to enter the thick room from a ripped item through a small door
                    // (after ripup).
                    enter_through_small_door = ripup_resolver_->check_leaving_ripped_item(p_list_element);
                }
                if (!enter_through_small_door)
                {
                    return something_expanded;
                }
            }
        }
    }
    else if (auto* obstacle_room = dynamic_cast<ObstacleExpansionRoom*>(next_room))
    {
        if (!p_list_element.already_checked)
        {
            bool room_rippable = false;
            if (control_.ripup_allowed)
            {
                ripup_costs = ripup_resolver_->check_ripup(
                    p_list_element, obstacle_room->get_item(), current_door_is_small);
                room_rippable = ripup_costs >= 0;
            }

            if (ripup_costs != ALREADY_RIPPED_COSTS && next_room_is_thick)
            {
                board::Item* obstacle_item = obstacle_room->get_item();
                if (!current_door_is_small && control_.max_shove_trace_recursion_depth > 0
                    && dynamic_cast<board::PolylineTrace*>(obstacle_item) != nullptr)
                {
                    bool shoved = shove_trace_room(p_list_element, obstacle_room);
                    if (!shoved)
                    {
                        if (ripup_costs > 0)
                        {
                            // Delay the occupation by ripup to allow shoving the room by another
                            // door section.
                            MazeListElement new_element(p_list_element.door, p_list_element.section_no_of_door,
                                p_list_element.backtrack_door, p_list_element.section_no_of_backtrack_door,
                                p_list_element.expansion_value + ripup_costs,
                                p_list_element.sorting_value + ripup_costs, p_list_element.next_room,
                                p_list_element.shape_entry, true, p_list_element.adjustment, true);
                            new_element.ripup_cost = ripup_costs;
                            maze_expansion_list_.insert(new_element);
                        }
                        return something_expanded;
                    }
                }
            }
            if (!room_rippable)
            {
                return true;
            }
        }
    }

    // Java iterates over a snapshot of the doors, so that the door list may
    // change while expanding.
    std::vector<ExpansionDoor*> room_doors_snapshot = next_room->get_doors();
    FRLogger::instance().trace("ROOM_DOOR context from_section="
                               + std::to_string(p_list_element.section_no_of_door) + ", backtrack_section="
                               + std::to_string(p_list_element.section_no_of_backtrack_door) + ", from_door="
                               + describe_expandable(p_list_element.door) + ", nextRoom="
                               + describe_room(next_room) + ", net=" + std::to_string(control_.net_no));
    for (size_t door_index = 0; door_index < room_doors_snapshot.size(); door_index++)
    {
        FRLogger::instance().trace("ROOM_DOOR candidate index=" + std::to_string(door_index) + ", from_section="
                                   + std::to_string(p_list_element.section_no_of_door)
                                   + ", backtrack_section="
                                   + std::to_string(p_list_element.section_no_of_backtrack_door) + ", from_door="
                                   + describe_expandable(p_list_element.door) + ", nextRoom="
                                   + describe_room(next_room) + ", candidate="
                                   + describe_expandable(room_doors_snapshot[door_index]) + ", net="
                                   + std::to_string(control_.net_no));
    }

    for (ExpansionDoor* to_door : room_doors_snapshot)
    {
        if (to_door == p_list_element.door)
        {
            continue;
        }
        if (expand_to_door(to_door, p_list_element, ripup_costs, next_room_is_thick,
                MazeSearchElement::Adjustment::NONE))
        {
            something_expanded = true;
        }
    }

    // Expand also the drill pages intersecting the room.
    if (control_.vias_allowed && dynamic_cast<ExpansionDrill*>(p_list_element.door) == nullptr)
    {
        if ((something_expanded || next_room_is_thick)
            && dynamic_cast<CompleteFreeSpaceExpansionRoom*>(next_room) != nullptr)
        {
            // Avoid setting something_expanded to true when next_room is thin, to
            // allow occupying it by different sections of the door.
            DrillPageArray* drill_page_array =
                autoroute_engine_ != nullptr ? autoroute_engine_->drill_page_array() : nullptr;
            if (drill_page_array != nullptr)
            {
                for (DrillPage* to_drill_page :
                    drill_page_array->overlapping_pages(next_room->get_shape()))
                {
                    expansion_engine_->expand_to_drill_page(to_drill_page, p_list_element);
                    something_expanded = true;
                }
            }
        }
        else if (auto* obstacle_room = dynamic_cast<ObstacleExpansionRoom*>(next_room))
        {
            board::Item* current_obstacle_item = obstacle_room->get_item();
            if (auto* current_via = dynamic_cast<board::Via*>(current_obstacle_item))
            {
                if (search_tree_ != nullptr)
                {
                    ExpansionDrill* via_drill_info =
                        expansion_engine_->get_autoroute_drill_info(current_via, search_tree_);
                    expansion_engine_->expand_to_drill(via_drill_info, p_list_element, ripup_costs);
                }
            }
        }
    }

    return something_expanded;
}

bool MazeSearchAlgo::expand_to_target_doors(const MazeListElement& p_list_element,
    bool p_next_room_is_thick,
    bool p_current_door_is_small,
    const FloatPoint& p_shape_entry_middle)
{
    CompleteExpansionRoom* next_room = p_list_element.next_room;
    if (next_room == nullptr)
    {
        return false;
    }
    if (p_current_door_is_small)
    {
        bool enter_through_small_door = false;
        if (auto* door = dynamic_cast<ExpansionDoor*>(p_list_element.door))
        {
            CompleteExpansionRoom* from_room = door->other_room(next_room);
            if (dynamic_cast<ObstacleExpansionRoom*>(from_room) != nullptr)
            {
                // otherwise entering through the small door may fail, because it was not
                // checked.
                enter_through_small_door = true;
            }
        }
        if (!enter_through_small_door)
        {
            return false;
        }
    }
    bool result = false;
    const std::vector<ExpansionDoor*>& target_doors = next_room->target_doors();
    for (ExpansionDoor* door_base : target_doors)
    {
        auto* to_door = dynamic_cast<TargetItemExpansionDoor*>(door_base);
        if (to_door == nullptr || to_door == p_list_element.door)
        {
            continue;
        }
        // Validate index before calling - prevents warning when indices become stale
        // during routing
        if (search_tree_ == nullptr || to_door->item == nullptr)
        {
            continue;
        }
        int tree_shape_count = to_door->item->tree_shape_count(*search_tree_);
        if (to_door->tree_entry_no < 0 || to_door->tree_entry_no >= tree_shape_count)
        {
            // Index out of range (trace was modified during routing)
            continue;
        }
        const TileShape* target_shape = get_trace_connection_shape(to_door->item, to_door->tree_entry_no);
        if (target_shape == nullptr)
        {
            // Item's tree shape index out of range (can happen when traces are modified
            // during routing)
            continue;
        }
        std::optional<FloatPoint> connection_point = target_shape->nearest_point_approx(p_shape_entry_middle);
        if (!connection_point)
        {
            continue;
        }
        if (!p_next_room_is_thick)
        {
            // check the line from shapeEntryMiddle to the nearest point.
            // Divergence: Java RoutingBoard.check_forced_trace_polyline() is
            // not ported yet, so the line is not verified here.
            geometry::planar::IntPoint check_point_0 = p_shape_entry_middle.round();
            geometry::planar::IntPoint check_point_1 = connection_point->round();
            if (!(check_point_0 == check_point_1))
            {
                FRLogger::instance().trace("MazeSearchAlgo.expand_to_target_doors: forced trace polyline check"
                                           " is not ported yet, the check is skipped, net="
                                               + std::to_string(control_.net_no));
            }
        }

        FloatLine new_shape_entry(*connection_point, *connection_point);

        if (expand_to_door_section(to_door, 0, new_shape_entry, p_list_element, 0,
                MazeSearchElement::Adjustment::NONE))
        {
            result = true;
        }
    }
    return result;
}

bool MazeSearchAlgo::expand_to_door(ExpansionDoor* p_to_door,
    const MazeListElement& p_list_element,
    int p_add_costs,
    bool p_next_room_is_thick,
    MazeSearchElement::Adjustment p_adjustment)
{
    CompleteExpansionRoom* next_room = p_list_element.next_room;
    if (next_room == nullptr || p_to_door == nullptr)
    {
        return false;
    }
    double half_width = compensated_trace_half_width_for_layer(next_room->get_layer());
    bool something_expanded = false;
    std::vector<FloatLine> line_sections = p_to_door->get_section_segments(half_width);
    int section_count = p_to_door->maze_search_element_count();

    for (int i = 0; i < static_cast<int>(line_sections.size()); i++)
    {
        MazeSearchElement* section = p_to_door->get_maze_search_element(i);
        if (section != nullptr && section->is_occupied)
        {
            continue;
        }
        if (i >= section_count)
        {
            continue;
        }
        std::optional<FloatLine> new_shape_entry;
        if (p_next_room_is_thick)
        {
            new_shape_entry = line_sections[i];
            if (p_to_door->get_dimension() == 1 && line_sections.size() == 1
                && dynamic_cast<CompleteFreeSpaceExpansionRoom*>(p_to_door->first_room_) != nullptr
                && dynamic_cast<CompleteFreeSpaceExpansionRoom*>(p_to_door->second_room_) != nullptr)
            {
                // check entering the toDoor at an acute corner of the shape of
                // listElement.nextRoom
                FloatPoint shape_entry_middle =
                    new_shape_entry->a().middle_point(new_shape_entry->b());
                geometry::planar::Simplex room_shape = next_room->get_shape_tile();
                if (room_shape.min_width() < 2 * half_width)
                {
                    return false;
                }
                std::vector<FloatPoint> nearest_points =
                    room_shape.nearest_border_points_approx(shape_entry_middle, 2);
                if (nearest_points.size() < 2
                    || nearest_points[1].distance(shape_entry_middle) <= half_width + 1)
                {
                    return false;
                }
            }
        }
        else
        {
            // expand only doors on the opposite side of the room from the shapeEntry.
            if (p_to_door->get_dimension() == 1 && i == 0
                && line_sections[0].b().distance_square(line_sections[0].a()) < 1)
            {
                // toDoor is small belonging to a via or thin room
                continue;
            }
            new_shape_entry = segment_projection(p_list_element.shape_entry, line_sections[i]);
            if (!new_shape_entry)
            {
                continue;
            }
        }

        if (expand_to_door_section(
                p_to_door, i, *new_shape_entry, p_list_element, p_add_costs, p_adjustment))
        {
            something_expanded = true;
        }
    }
    return something_expanded;
}

bool MazeSearchAlgo::expand_to_door_section(ExpandableObject* p_door,
    int p_section_index,
    const FloatLine& p_shape_entry,
    const MazeListElement& p_from_element,
    int p_add_costs,
    MazeSearchElement::Adjustment p_adjustment)
{
    if (p_door == nullptr)
    {
        return false;
    }
    MazeSearchElement* door_section = p_door->get_maze_search_element(p_section_index);
    if (door_section == nullptr)
    {
        return false;
    }
    bool door_section_occupied = door_section->is_occupied;
    if (door_section_occupied)
    {
        FRLogger::instance().trace("RAW_SECTION skip selected_section="
                                   + std::to_string(p_section_index) + ", from_section="
                                   + std::to_string(p_from_element.section_no_of_door) + ", backtrack_section="
                                   + std::to_string(p_from_element.section_no_of_backtrack_door) + ", occupied="
                                   + (door_section_occupied ? "1" : "0") + ", adjustment="
                                   + std::to_string(static_cast<int>(p_adjustment)) + ", door="
                                   + describe_expandable(p_door) + ", from_door="
                                   + describe_expandable(p_from_element.door) + ", net="
                                   + std::to_string(control_.net_no));
        return false;
    }
    CompleteExpansionRoom* from_room = p_from_element.next_room;
    if (from_room == nullptr)
    {
        return false;
    }
    CompleteExpansionRoom* next_room = p_door->other_room(from_room);
    int layer = from_room->get_layer();
    FloatPoint shape_entry_middle = p_shape_entry.a().middle_point(p_shape_entry.b());

    double bend_cost_penalty = 0.0;
    double bend_cost = bend_cost_for_layer(layer);
    if (bend_cost > 0.0 && p_from_element.backtrack_door != nullptr)
    {
        FloatPoint from_mid = p_from_element.shape_entry.a().middle_point(p_from_element.shape_entry.b());
        // Build vectors prev->current and current->next to detect a direction change.
        FloatPoint backtrack_cog = p_from_element.backtrack_door->get_shape().centre_of_gravity();
        double prev_dx = from_mid.x - backtrack_cog.x;
        double prev_dy = from_mid.y - backtrack_cog.y;
        double next_dx = shape_entry_middle.x - from_mid.x;
        double next_dy = shape_entry_middle.y - from_mid.y;
        double cross_product = prev_dx * next_dy - prev_dy * next_dx;
        double sq_len_prev = prev_dx * prev_dx + prev_dy * prev_dy;
        double sq_len_next = next_dx * next_dx + next_dy * next_dy;
        // Use a normalized threshold (sin^2 of angle > 0.01, approx. 5.7 deg) to be
        // scale-independent.
        if (sq_len_prev > 0.0 && sq_len_next > 0.0
            && (cross_product * cross_product) > 0.01 * sq_len_prev * sq_len_next)
        {
            bend_cost_penalty = bend_cost;
        }
    }

    const ExpansionCostFactor& trace_cost = trace_costs_for_layer(layer);
    double expansion_value = p_from_element.expansion_value + p_add_costs + bend_cost_penalty
        + shape_entry_middle.weighted_distance(
            p_from_element.shape_entry.a().middle_point(p_from_element.shape_entry.b()), trace_cost.horizontal,
            trace_cost.vertical);
    double sorting_value = expansion_value;
    if (destination_distance_ != nullptr)
    {
        sorting_value += destination_distance_->calculate(shape_entry_middle, layer);
    }
    bool room_ripped = (p_add_costs > 0 && p_adjustment == MazeSearchElement::Adjustment::NONE)
        || (p_from_element.already_checked && p_from_element.room_ripped);

    MazeListElement new_element(p_door, p_section_index, p_from_element.door,
        p_from_element.section_no_of_door, expansion_value, sorting_value, next_room, p_shape_entry, room_ripped,
        p_adjustment, false);
    // Store the direct ripup cost on this element (non-zero only when this specific door
    // caused a ripup; propagated roomRipped from a parent keeps ripupCost=0).
    if (p_add_costs > 0 && p_adjustment == MazeSearchElement::Adjustment::NONE)
    {
        new_element.ripup_cost = p_add_costs;
    }
    FRLogger::instance().trace("RAW_SECTION assign selected_section=" + std::to_string(p_section_index)
                               + ", from_section=" + std::to_string(p_from_element.section_no_of_door)
                               + ", backtrack_section=" + std::to_string(p_from_element.section_no_of_backtrack_door)
                               + ", add_costs=" + std::to_string(p_add_costs) + ", adjustment="
                               + std::to_string(static_cast<int>(p_adjustment)) + ", roomRipped="
                               + (room_ripped ? "1" : "0") + ", expansionValue=" + std::to_string(expansion_value)
                               + ", sortingValue=" + std::to_string(sorting_value) + ", door="
                               + describe_expandable(p_door) + ", from_door="
                               + describe_expandable(p_from_element.door) + ", net="
                               + std::to_string(control_.net_no));
    maze_expansion_list_.insert(new_element);
    return true;
}

bool MazeSearchAlgo::door_is_small(ExpansionDoor* p_door, double p_trace_width) const
{
    if (p_door == nullptr)
    {
        return true;
    }
    if (p_door->get_dimension() == 1
        || (dynamic_cast<CompleteFreeSpaceExpansionRoom*>(p_door->first_room_) != nullptr
            && dynamic_cast<CompleteFreeSpaceExpansionRoom*>(p_door->second_room_) != nullptr))
    {
        IntBox door_shape = p_door->get_shape();
        if (door_shape.is_empty())
        {
            FRLogger::instance().trace("MazeSearchAlgo:door_is_small doorShape is empty");
            return true;
        }

        double door_length;
        rules::AngleRestriction angle_restriction =
            board_ != nullptr ? board_->get_rules().get_trace_angle_restriction()
                              : rules::AngleRestriction::FORTY_FIVE;
        if (angle_restriction == rules::AngleRestriction::RIGHT)
        {
            IntBox door_box = door_shape.bounding_box();
            door_length = door_box.max_width();
        }
        else if (angle_restriction == rules::AngleRestriction::FORTY_FIVE)
        {
            // Divergence: the C++ TileShape has no bounding_octagon(), so the
            // octagon is derived from the bounding box, which is identical for
            // the IntBox based C++ doors.
            geometry::planar::IntOctagon door_oct = door_shape.bounding_box().bounding_octagon();
            door_length = door_oct.max_width();
        }
        else
        {
            std::optional<FloatLine> door_line_segment = door_shape.diagonal_corner_segment();
            if (!door_line_segment)
            {
                return true;
            }
            door_length = door_line_segment->b().distance(door_line_segment->a());
        }
        return door_length < p_trace_width;
    }
    return false;
}

bool MazeSearchAlgo::room_shape_is_thick(ObstacleExpansionRoom* p_obstacle_room) const
{
    if (p_obstacle_room == nullptr)
    {
        return false;
    }
    board::Item* obstacle_item = p_obstacle_room->get_item();
    int layer = p_obstacle_room->get_layer();
    double obstacle_half_width;
    if (obstacle_item == nullptr)
    {
        obstacle_half_width = 0;
    }
    else if (auto* trace = dynamic_cast<board::Trace*>(obstacle_item))
    {
        obstacle_half_width = trace->get_half_width();
        if (search_tree_ != nullptr)
        {
            obstacle_half_width += search_tree_->clearance_compensation_value(obstacle_item->get_clearance_class(),
                layer);
        }
    }
    else if (auto* via = dynamic_cast<board::Via*>(obstacle_item))
    {
        const TileShape* via_tree_shape =
            search_tree_ != nullptr ? via->get_tree_shape_on_layer(search_tree_, layer) : nullptr;
        if (via_tree_shape != nullptr)
        {
            obstacle_half_width = 0.5 * via_tree_shape->max_width();
        }
        else
        {
            obstacle_half_width = 0.5 * via->get_tile_shape(0).max_width();
        }
    }
    else
    {
        FRLogger::instance().warn("MazeSearchAlgo.room_shape_is_thick: unexpected obstacle item");
        obstacle_half_width = 0;
    }
    return obstacle_half_width >= compensated_trace_half_width_for_layer(layer);
}

bool MazeSearchAlgo::shove_trace_room(const MazeListElement& p_list_element,
    ObstacleExpansionRoom* p_obstacle_room)
{
    if (p_list_element.door == nullptr || p_obstacle_room == nullptr)
    {
        return true;
    }
    if (p_list_element.section_no_of_door != 0
        && p_list_element.section_no_of_door != p_list_element.door->maze_search_element_count() - 1)
    {
        // No delay of occupation necessary because inner sections of a door
        // are currently not shoved.
        return true;
    }
    bool result = false;
    if (p_list_element.adjustment != MazeSearchElement::Adjustment::RIGHT)
    {
        std::vector<MazeTraceShover::DoorSection> left_to_door_section_list;
        if (MazeTraceShover::check_shove_trace_line(p_list_element, p_obstacle_room, board_, control_, false,
                left_to_door_section_list))
        {
            result = true;
        }
        for (const MazeTraceShover::DoorSection& current_left_door_section : left_to_door_section_list)
        {
            if (current_left_door_section.door == nullptr)
            {
                continue;
            }
            // The door is the link door to the next room.
            MazeSearchElement::Adjustment current_adjustment = current_left_door_section.door->get_dimension() == 2
                ? MazeSearchElement::Adjustment::LEFT
                : MazeSearchElement::Adjustment::NONE;
            expand_to_door_section(current_left_door_section.door, current_left_door_section.section_index,
                current_left_door_section.section_line, p_list_element, 0, current_adjustment);
        }
    }

    if (p_list_element.adjustment != MazeSearchElement::Adjustment::LEFT)
    {
        std::vector<MazeTraceShover::DoorSection> right_to_door_section_list;
        if (MazeTraceShover::check_shove_trace_line(p_list_element, p_obstacle_room, board_, control_, true,
                right_to_door_section_list))
        {
            result = true;
        }
        for (const MazeTraceShover::DoorSection& current_right_door_section : right_to_door_section_list)
        {
            if (current_right_door_section.door == nullptr)
            {
                continue;
            }
            // The door is the link door to the next room.
            MazeSearchElement::Adjustment current_adjustment = current_right_door_section.door->get_dimension() == 2
                ? MazeSearchElement::Adjustment::RIGHT
                : MazeSearchElement::Adjustment::NONE;
            expand_to_door_section(current_right_door_section.door, current_right_door_section.section_index,
                current_right_door_section.section_line, p_list_element, 0, current_adjustment);
        }
    }
    return result;
}

int MazeSearchAlgo::check_neck_down_at_dest_pin(CompleteExpansionRoom* p_room) const
{
    if (p_room == nullptr)
    {
        return 0;
    }
    for (ExpansionDoor* door_base : p_room->target_doors())
    {
        auto* target_door = dynamic_cast<TargetItemExpansionDoor*>(door_base);
        if (target_door == nullptr)
        {
            continue;
        }
        if (auto* pin = dynamic_cast<board::Pin*>(target_door->item))
        {
            return pin->get_trace_neckdown_halfwidth(p_room->get_layer());
        }
    }
    return 0;
}

std::optional<FloatLine> MazeSearchAlgo::segment_projection(const FloatLine& p_from_segment,
    const FloatLine& p_to_segment)
{
    FloatLine check_segment = p_from_segment.adjust_direction(p_to_segment);
    std::optional<FloatLine> first_projection = p_to_segment.segment_projection(check_segment);
    std::optional<FloatLine> second_projection = p_to_segment.segment_projection_2(check_segment);
    if (first_projection && second_projection)
    {
        FloatPoint result_a;
        // Divergence: Java compares the point objects by reference (==), which
        // is only true for the endpoint reuse of segment_projection(). Value
        // equality expresses the same intent.
        if (first_projection->a() == p_to_segment.a() || second_projection->a() == p_to_segment.a())
        {
            result_a = p_to_segment.a();
        }
        else if (first_projection->a().distance_square(p_to_segment.a())
                 <= second_projection->a().distance_square(p_to_segment.a()))
        {
            result_a = first_projection->a();
        }
        else
        {
            result_a = second_projection->a();
        }
        FloatPoint result_b;
        if (first_projection->b() == p_to_segment.b() || second_projection->b() == p_to_segment.b())
        {
            result_b = p_to_segment.b();
        }
        else if (first_projection->b().distance_square(p_to_segment.b())
                 <= second_projection->b().distance_square(p_to_segment.b()))
        {
            result_b = first_projection->b();
        }
        else
        {
            result_b = second_projection->b();
        }
        return FloatLine(result_a, result_b);
    }
    if (first_projection)
    {
        return first_projection;
    }
    return second_projection;
}

const TileShape* MazeSearchAlgo::get_trace_connection_shape(board::Item* p_item, int p_index) const
{
    if (p_item == nullptr || search_tree_ == nullptr)
    {
        return nullptr;
    }
    if (!is_connectable(p_item))
    {
        return nullptr;
    }
    return p_item->get_tree_shape(*search_tree_, p_index);
}

bool MazeSearchAlgo::is_connectable(const board::Item* p_item)
{
    // C++ analogue of the Java `instanceof Connectable` guard: the C++
    // Connectable interface is a dead draft implemented by no real item, so a
    // connectable object is a routeable item (trace, drill) or a conduction
    // area. This mirrors CompleteFreeSpaceExpansionRoom::calculate_target_doors.
    if (p_item == nullptr)
    {
        return false;
    }
    return p_item->is_trace() || p_item->is_via() || p_item->is_pin()
        || dynamic_cast<const board::ConductionArea*>(p_item) != nullptr;
}

std::vector<ExpandableObject*> MazeSearchAlgo::reconstruct_path(const Result& p_result) const
{
    std::vector<ExpandableObject*> result;
    if (p_result.destination_door == nullptr)
    {
        return result;
    }
    ExpandableObject* current_door = p_result.destination_door;
    int current_section_no = p_result.section_no_of_door;
    // The maze search elements of a door are consumed while searching, so the
    // backtrack chain is only available as long as the elements were not reset.
    int guard = 0;
    while (current_door != nullptr && guard < 100000)
    {
        result.push_back(current_door);
        MazeSearchElement* element = current_door->get_maze_search_element(current_section_no);
        if (element == nullptr || element->backtrack_door == nullptr)
        {
            break;
        }
        ExpandableObject* next_door = element->backtrack_door;
        int next_section_no = element->section_no_of_backtrack_door;
        if (next_door == current_door)
        {
            break;
        }
        current_door = next_door;
        current_section_no = next_section_no;
        guard++;
    }
    std::reverse(result.begin(), result.end());
    return result;
}

std::string MazeSearchAlgo::describe_expandable(const ExpandableObject* p_door)
{
    if (p_door == nullptr)
    {
        return "null";
    }
    std::string section_count = "uninitialized";
    try
    {
        section_count = std::to_string(p_door->maze_search_element_count());
    }
    catch (...)
    {
        section_count = "uninitialized";
    }
    if (auto* target_door = dynamic_cast<const TargetItemExpansionDoor*>(p_door))
    {
        return "TargetItemExpansionDoor/item=" + std::to_string(target_door->item != nullptr
                                                                     ? target_door->item->get_id()
                                                                     : 0)
            + "/tree_entry=" + std::to_string(target_door->tree_entry_no) + "/dim="
            + std::to_string(p_door->get_dimension()) + "/sections=" + section_count;
    }
    if (auto* drill = dynamic_cast<const ExpansionDrill*>(p_door))
    {
        return "ExpansionDrill/location=" + drill->location.to_string() + "/layers="
            + std::to_string(drill->first_layer) + "-" + std::to_string(drill->last_layer) + "/dim="
            + std::to_string(p_door->get_dimension()) + "/sections=" + section_count;
    }
    if (dynamic_cast<const DrillPage*>(p_door) != nullptr)
    {
        return "DrillPage/dim=" + std::to_string(p_door->get_dimension()) + "/sections=" + section_count;
    }
    IntBox bounds = p_door->get_shape().bounding_box();
    return std::string(dynamic_cast<const ExpansionDoor*>(p_door) != nullptr ? "ExpansionDoor" : "ExpandableObject")
        + "/bounds=[(" + std::to_string(bounds.ll.x) + "," + std::to_string(bounds.ll.y) + ")..("
        + std::to_string(bounds.ur.x) + "," + std::to_string(bounds.ur.y) + ")]/dim="
        + std::to_string(p_door->get_dimension()) + "/sections=" + section_count;
}

std::string MazeSearchAlgo::describe_room(const CompleteExpansionRoom* p_room)
{
    if (p_room == nullptr)
    {
        return "null";
    }
    IntBox bounds = p_room->get_shape().bounding_box();
    std::string name = "CompleteExpansionRoom";
    if (dynamic_cast<const CompleteFreeSpaceExpansionRoom*>(p_room) != nullptr)
    {
        name = "CompleteFreeSpaceExpansionRoom";
    }
    else if (dynamic_cast<const ObstacleExpansionRoom*>(p_room) != nullptr)
    {
        name = "ObstacleExpansionRoom";
    }
    return name + "/layer=" + std::to_string(p_room->get_layer()) + "/bounds=[(" + std::to_string(bounds.ll.x) + ","
        + std::to_string(bounds.ll.y) + ")..(" + std::to_string(bounds.ur.x) + ","
        + std::to_string(bounds.ur.y) + ")]";
}

} // namespace freerouting::autoroute
