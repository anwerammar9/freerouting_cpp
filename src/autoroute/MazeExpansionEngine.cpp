#include "autoroute/MazeExpansionEngine.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

#include "autoroute/AutorouteControl.h"
#include "autoroute/AutorouteEngine.h"
#include "autoroute/DrillPage.h"
#include "autoroute/DrillPageArray.h"
#include "autoroute/ExpansionDrill.h"
#include "autoroute/ExpansionRoom.h"
#include "autoroute/ItemAutorouteInfo.h"
#include "board/Via.h"
#include "autoroute/MazeFanoutDiagnostics.h"
#include "autoroute/MazeSearchAlgo.h"
#include "autoroute/MazeSearchElement.h"
#include "board/ForcedPadAlgo.h"
#include "board/ForcedViaAlgo.h"
#include "board/Padstack.h"
#include "board/Pin.h"
#include "board/RoutingBoard.h"
#include "board/Via.h"
#include "autoroute/ExpansionCostFactor.h"
#include "logger/FRLogger.h"
#include "geometry/planar/Shape.h"
#include "geometry/planar/TileShape.h"
#include "rules/ViaInfo.h"
#include "rules/ViaRule.h"

namespace freerouting::autoroute {

namespace {

/**
 * Returns the compensated trace half width of p_layer, or 0 for an
 * uninitialised control. Mirrors Java AutorouteControl.compensatedTraceHalfWidth.
 */
double compensated_trace_half_width_of(const AutorouteControl& p_ctrl, int p_layer)
{
    if (p_layer < 0 || p_layer >= static_cast<int>(p_ctrl.compensated_trace_half_width.size()))
    {
        return 0;
    }
    return p_ctrl.compensated_trace_half_width[static_cast<size_t>(p_layer)];
}

/** Returns the trace half width of p_layer, or 0 for an uninitialised control. */
int trace_half_width_of(const AutorouteControl& p_ctrl, int p_layer)
{
    if (p_layer < 0 || p_layer >= static_cast<int>(p_ctrl.trace_half_width.size()))
    {
        return 0;
    }
    return p_ctrl.trace_half_width[static_cast<size_t>(p_layer)];
}

/** Returns the via radius of p_layer, or 0 for an uninitialised control. */
double via_radius_of(const AutorouteControl& p_ctrl, int p_layer)
{
    if (p_layer < 0 || p_layer >= static_cast<int>(p_ctrl.via_radius_arr.size()))
    {
        return 0;
    }
    return p_ctrl.via_radius_arr[static_cast<size_t>(p_layer)];
}

/** Returns the trace cost of p_layer, or the neutral cost for an empty control. */
const ExpansionCostFactor& trace_costs_of(const AutorouteControl& p_ctrl, int p_layer)
{
    static const ExpansionCostFactor kNeutralCost(1.0, 1.0);
    if (p_layer < 0 || p_layer >= static_cast<int>(p_ctrl.trace_costs.size()))
    {
        return kNeutralCost;
    }
    return p_ctrl.trace_costs[static_cast<size_t>(p_layer)];
}

} // namespace

void MazeExpansionEngine::expand_to_drill(ExpansionDrill* p_drill,
    const MazeListElement& p_from_element, int p_add_costs)
{
    // Port of Java MazeExpansionEngine.expandToDrill().
    AutorouteControl& ctrl = search_->control();
    CompleteExpansionRoom* next_room = p_from_element.next_room;
    if (p_drill == nullptr || next_room == nullptr)
    {
        return;
    }
    const int layer = next_room->get_layer();
    const double trace_half_width = compensated_trace_half_width_of(ctrl, layer);
    // Java calls TileShape.minWidth() on the room shape. The C++ room shape is
    // an IntBox, whose min_width() is the smaller of the two side lengths.
    const bool room_shape_is_thin = next_room->get_shape().min_width() < 2 * trace_half_width;

    if (room_shape_is_thin)
    {
        // Divergence: Java additionally requires that the backtrack door does
        // not intersect the drill shape in this case, which is only reachable
        // for a drill expanded from a drill page. The C++ port keeps the thin
        // room rejection, as the intersection test needs the backtrack door
        // shape of a page expansion.
        search_->fanout_diagnostics().trace("drill_rejected_thin_room_no_backtrack_intersection",
            "drill=" + MazeSearchAlgo::describe_expandable(p_drill) + ", from_door="
                + MazeSearchAlgo::describe_expandable(p_from_element.door) + ", backtrack="
                + MazeSearchAlgo::describe_expandable(p_from_element.backtrack_door) + ", room="
                + MazeSearchAlgo::describe_room(next_room));
        return;
    }

    const double via_radius = via_radius_of(ctrl, layer);
    const geometry::planar::IntBox shrinked_drill_shape = p_drill->get_shape().shrink(
        static_cast<int>(std::ceil(via_radius)));
    geometry::planar::FloatPoint compare_corner =
        p_from_element.shape_entry.a().middle_point(p_from_element.shape_entry.b());

    if (dynamic_cast<DrillPage*>(p_from_element.door) != nullptr)
    {
        if (auto* backtrack_door = dynamic_cast<TargetItemExpansionDoor*>(
                p_from_element.backtrack_door))
        {
            if (auto* pin = dynamic_cast<board::Pin*>(backtrack_door->item))
            {
                std::optional<geometry::planar::FloatPoint> nearest_exit_corner =
                    pin->nearest_trace_exit_corner(p_drill->location.to_float(),
                        static_cast<int>(trace_half_width), layer);
                if (nearest_exit_corner.has_value())
                {
                    compare_corner = *nearest_exit_corner;
                }
            }
        }
    }

    const geometry::planar::FloatPoint nearest_point =
        shrinked_drill_shape.nearest_point(compare_corner);
    const geometry::planar::FloatLine shape_entry(nearest_point, nearest_point);
    const int section_index = layer - p_drill->first_layer;
    if (section_index < 0
        || section_index >= static_cast<int>(p_drill->room_arr.size()))
    {
        return;
    }

    const ExpansionCostFactor& trace_cost = trace_costs_of(ctrl, layer);
    double expansion_value = p_from_element.expansion_value + p_add_costs
        + nearest_point.weighted_distance(compare_corner, trace_cost.horizontal, trace_cost.vertical);

    ExpandableObject* new_backtrack_door = nullptr;
    int new_section_no_of_backtrack_door = 0;
    if (dynamic_cast<DrillPage*>(p_from_element.door) != nullptr)
    {
        new_backtrack_door = p_from_element.backtrack_door;
        new_section_no_of_backtrack_door = p_from_element.section_no_of_backtrack_door;
    }
    else
    {
        new_backtrack_door = p_from_element.door;
        new_section_no_of_backtrack_door = p_from_element.section_no_of_door;
        expansion_value += ctrl.min_normal_via_cost;
    }

    double sorting_value = expansion_value;
    if (search_->destination_distance() != nullptr)
    {
        sorting_value += search_->destination_distance()->calculate(nearest_point, layer);
    }

    MazeListElement new_element(p_drill, section_index, new_backtrack_door,
        new_section_no_of_backtrack_door, expansion_value, sorting_value, nullptr, shape_entry,
        p_from_element.room_ripped, MazeSearchElement::Adjustment::NONE, false);
    search_->add_to_maze_expansion_list(new_element);
    search_->fanout_diagnostics().trace("drill_accepted",
        "drill=" + MazeSearchAlgo::describe_expandable(p_drill) + ", room="
            + MazeSearchAlgo::describe_room(p_from_element.next_room) + ", nearestPoint="
            + nearest_point.to_string() + ", expansionValue=" + std::to_string(expansion_value));
}

void MazeExpansionEngine::expand_to_drill_page(DrillPage* p_drill_page,
    const MazeListElement& p_from_element)
{
    // Port of Java MazeExpansionEngine.expandToDrillPage().
    if (p_drill_page == nullptr || p_from_element.next_room == nullptr)
    {
        return;
    }
    AutorouteControl& ctrl = search_->control();
    const int layer = p_from_element.next_room->get_layer();
    const geometry::planar::FloatPoint from_element_shape_entry_middle =
        p_from_element.shape_entry.a().middle_point(p_from_element.shape_entry.b());
    const geometry::planar::FloatPoint nearest_point =
        p_drill_page->shape.nearest_point(from_element_shape_entry_middle);
    const double expansion_value = p_from_element.expansion_value + ctrl.min_normal_via_cost;

    const ExpansionCostFactor& trace_cost = trace_costs_of(ctrl, layer);
    double sorting_value = expansion_value
        + nearest_point.weighted_distance(from_element_shape_entry_middle, trace_cost.horizontal,
            trace_cost.vertical);
    if (search_->destination_distance() != nullptr)
    {
        sorting_value += search_->destination_distance()->calculate(nearest_point, layer);
    }

    MazeListElement new_element(p_drill_page, layer, p_from_element.door,
        p_from_element.section_no_of_door, expansion_value, sorting_value, p_from_element.next_room,
        p_from_element.shape_entry, p_from_element.room_ripped,
        MazeSearchElement::Adjustment::NONE, false);
    search_->add_to_maze_expansion_list(new_element);
}

void MazeExpansionEngine::expand_to_drills_of_page(const MazeListElement& p_from_element)
{
    // Port of Java MazeExpansionEngine.expandToDrillsOfPage().
    AutorouteControl& ctrl = search_->control();
    AutorouteEngine* autoroute_engine = search_->autoroute_engine();
    auto* drill_page = dynamic_cast<DrillPage*>(p_from_element.door);
    if (drill_page == nullptr || autoroute_engine == nullptr || p_from_element.next_room == nullptr)
    {
        return;
    }
    const int from_room_layer = p_from_element.section_no_of_door;
    const std::vector<ExpansionDrill*> drill_list =
        drill_page->get_drills(autoroute_engine, ctrl.attach_smd_allowed);
    MazeFanoutDiagnostics& fanout_diagnostics = search_->fanout_diagnostics();
    if (fanout_diagnostics.enabled())
    {
        fanout_diagnostics.trace("drill_page_scan",
            "candidate_count=" + std::to_string(drill_list.size()) + ", attachSmdAllowed="
                + std::string(ctrl.attach_smd_allowed ? "true" : "false") + ", room="
                + MazeSearchAlgo::describe_room(p_from_element.next_room) + ", from_door="
                + MazeSearchAlgo::describe_expandable(p_from_element.door));
        if (drill_list.empty())
        {
            fanout_diagnostics.trace("drill_page_empty", "no_candidates=true");
        }
    }
    bool first_mismatch_logged = false;
    for (ExpansionDrill* current_drill : drill_list)
    {
        if (current_drill == nullptr)
        {
            continue;
        }
        const int section_index = from_room_layer - current_drill->first_layer;
        if (section_index < 0
            || section_index >= static_cast<int>(current_drill->room_arr.size()))
        {
            fanout_diagnostics.trace("drill_rejected_section_out_of_range",
                "drill=" + MazeSearchAlgo::describe_expandable(current_drill) + ", section="
                    + std::to_string(section_index) + ", room_arr_len="
                    + std::to_string(current_drill->room_arr.size()));
            continue;
        }
        if (current_drill->room_arr[static_cast<size_t>(section_index)] != p_from_element.next_room)
        {
            fanout_diagnostics.trace("drill_rejected_room_mismatch",
                "drill=" + MazeSearchAlgo::describe_expandable(current_drill) + ", expected_room="
                    + MazeSearchAlgo::describe_room(p_from_element.next_room) + ", drill_room="
                    + MazeSearchAlgo::describe_room(
                        current_drill->room_arr[static_cast<size_t>(section_index)]));
            if (!first_mismatch_logged && fanout_diagnostics.enabled())
            {
                first_mismatch_logged = true;
                // Java logs System.identityHashCode of both rooms; the C++ port
                // logs the addresses of the rooms instead.
                logger::FRLogger::instance().trace(std::string("FANOUT_DIAG event=first_room_mismatch_detail")
                    + ", pin=" + fanout_diagnostics.label_for_log() + ", net="
                    + std::to_string(ctrl.net_no) + ", drillLocation=" + current_drill->location.to_string()
                    + ", expansion_room_id="
                    + std::to_string(reinterpret_cast<uintptr_t>(p_from_element.next_room))
                    + ", drill_room_id="
                    + std::to_string(reinterpret_cast<uintptr_t>(
                        current_drill->room_arr[static_cast<size_t>(section_index)])));
            }
            continue;
        }
        MazeSearchElement* drill_element = current_drill->get_maze_search_element(section_index);
        if (drill_element != nullptr && drill_element->is_occupied)
        {
            fanout_diagnostics.trace("drill_rejected_section_occupied",
                "drill=" + MazeSearchAlgo::describe_expandable(current_drill) + ", section="
                    + std::to_string(section_index));
            continue;
        }
        expand_to_drill(current_drill, p_from_element, 0);
    }
}

void MazeExpansionEngine::expand_to_other_layers(const MazeListElement& p_list_element)
{
    // Port of Java MazeExpansionEngine.expandToOtherLayers().
    AutorouteControl& ctrl = search_->control();
    auto* current_drill = dynamic_cast<ExpansionDrill*>(p_list_element.door);
    if (current_drill == nullptr)
    {
        return;
    }
    const int section_no_of_door = p_list_element.section_no_of_door;
    if (section_no_of_door < 0
        || section_no_of_door >= static_cast<int>(current_drill->room_arr.size()))
    {
        return;
    }
    const int from_layer = current_drill->first_layer + section_no_of_door;
    const std::vector<int> net_numbers{ctrl.net_no};

    int via_lower_bound = 0;
    int via_upper_bound = -1;
    bool smd_attached_on_component_side = false;
    bool smd_attached_on_solder_side = false;
    bool room_ripped = false;

    CompleteExpansionRoom* drill_room =
        current_drill->room_arr[static_cast<size_t>(section_no_of_door)];
    if (auto* obstacle_room = dynamic_cast<ObstacleExpansionRoom*>(drill_room))
    {
        // The drill hits an obstacle room, which may only be expanded when that
        // obstacle is a via of the via rule of the current net.
        if (!ctrl.ripup_allowed)
        {
            return;
        }
        board::Item* current_obstacle_item = obstacle_room->get_item();
        auto* current_obstacle_via = dynamic_cast<board::Via*>(current_obstacle_item);
        if (current_obstacle_via == nullptr)
        {
            return;
        }
        board::Padstack* current_obstacle_padstack = current_obstacle_via->get_padstack();
        if (current_obstacle_padstack == nullptr || ctrl.via_rule == nullptr
            || !ctrl.via_rule->contains_padstack(current_obstacle_padstack)
            || current_obstacle_item->get_clearance_class() != ctrl.via_clearance_class)
        {
            return;
        }
        via_lower_bound = current_obstacle_padstack->get_from_layer();
        via_upper_bound = current_obstacle_padstack->get_to_layer();
        room_ripped = true;
    }
    else
    {
        room_ripped = false;
        const int via_lower_limit = std::max(current_drill->first_layer, ctrl.via_lower_bound);
        const int via_upper_limit = std::min(current_drill->last_layer, ctrl.via_upper_bound);

        int current_layer = from_layer;
        for (;;)
        {
            const CompleteExpansionRoom* current_room =
                current_drill->room_arr[static_cast<size_t>(current_layer - current_drill->first_layer)];
            if (current_room == nullptr)
            {
                break;
            }
            const board::ForcedPadAlgo::CheckDrillResult drill_result = check_layer_with_any_matching_via(
                *current_drill, current_layer, current_room->get_shape(), net_numbers);
            if (drill_result == board::ForcedPadAlgo::CheckDrillResult::NOT_DRILLABLE)
            {
                via_lower_bound = current_layer + 1;
                break;
            }
            if (drill_result == board::ForcedPadAlgo::CheckDrillResult::DRILLABLE_WITH_ATTACH_SMD)
            {
                if (current_layer == 0)
                {
                    smd_attached_on_component_side = true;
                }
                else if (current_layer == ctrl.layer_count - 1)
                {
                    smd_attached_on_solder_side = true;
                }
            }
            if (current_layer <= via_lower_limit)
            {
                via_lower_bound = via_lower_limit;
                break;
            }
            --current_layer;
        }
        if (via_lower_bound > current_drill->first_layer)
        {
            return;
        }

        current_layer = from_layer + 1;
        for (;;)
        {
            if (current_layer > via_upper_limit)
            {
                via_upper_bound = via_upper_limit;
                break;
            }
            const CompleteExpansionRoom* current_room =
                current_drill->room_arr[static_cast<size_t>(current_layer - current_drill->first_layer)];
            if (current_room == nullptr)
            {
                break;
            }
            const board::ForcedPadAlgo::CheckDrillResult drill_result = check_layer_with_any_matching_via(
                *current_drill, current_layer, current_room->get_shape(), net_numbers);
            if (drill_result == board::ForcedPadAlgo::CheckDrillResult::NOT_DRILLABLE)
            {
                via_upper_bound = current_layer - 1;
                break;
            }
            if (drill_result == board::ForcedPadAlgo::CheckDrillResult::DRILLABLE_WITH_ATTACH_SMD)
            {
                if (current_layer == ctrl.layer_count - 1)
                {
                    smd_attached_on_solder_side = true;
                }
            }
            ++current_layer;
        }
        if (via_upper_bound < current_drill->last_layer)
        {
            return;
        }
    }

    for (int to_layer = via_lower_bound; to_layer <= via_upper_bound; ++to_layer)
    {
        if (to_layer == from_layer)
        {
            continue;
        }
        int current_first_layer = 0;
        int current_last_layer = 0;
        if (to_layer < from_layer)
        {
            current_first_layer = to_layer;
            current_last_layer = from_layer;
        }
        else
        {
            current_first_layer = from_layer;
            current_last_layer = to_layer;
        }

        // Check that a via of the via rule covers the layer range of the drill
        // and that the attach rule of the SMD pads is respected.
        bool mask_found = false;
        for (const ViaMask& current_via_info : ctrl.via_info_arr)
        {
            if (current_first_layer >= current_via_info.from_layer
                && current_last_layer <= current_via_info.to_layer
                && current_via_info.from_layer >= via_lower_bound
                && current_via_info.to_layer <= via_upper_bound)
            {
                const bool mask_ok =
                    !((current_via_info.from_layer == 0 && smd_attached_on_component_side)
                        || (current_via_info.to_layer == ctrl.layer_count - 1
                            && smd_attached_on_solder_side))
                    || current_via_info.attach_smd_allowed;
                if (mask_ok)
                {
                    mask_found = true;
                    break;
                }
            }
        }
        if (!mask_found)
        {
            continue;
        }

        const int current_room_index = to_layer - current_drill->first_layer;
        if (current_room_index < 0
            || current_room_index >= static_cast<int>(current_drill->room_arr.size()))
        {
            continue;
        }
        MazeSearchElement* current_drill_layer_info =
            current_drill->get_maze_search_element(current_room_index);
        if (current_drill_layer_info != nullptr && current_drill_layer_info->is_occupied)
        {
            continue;
        }

        double expansion_value = p_list_element.expansion_value;
        if (from_layer >= 0 && from_layer < static_cast<int>(ctrl.add_via_costs.size()))
        {
            const ViaCost& via_cost = ctrl.add_via_costs[static_cast<size_t>(from_layer)];
            if (to_layer >= 0 && to_layer < static_cast<int>(via_cost.to_layer.size()))
            {
                expansion_value += via_cost.to_layer[static_cast<size_t>(to_layer)];
            }
        }
        const geometry::planar::FloatPoint shape_entry_middle =
            p_list_element.shape_entry.a().middle_point(p_list_element.shape_entry.b());
        double sorting_value = expansion_value;
        if (search_->destination_distance() != nullptr)
        {
            sorting_value += search_->destination_distance()->calculate(shape_entry_middle, to_layer);
        }

        MazeListElement new_element(current_drill, current_room_index, current_drill,
            p_list_element.section_no_of_door, expansion_value, sorting_value,
            current_drill->room_arr[static_cast<size_t>(current_room_index)],
            p_list_element.shape_entry, room_ripped, MazeSearchElement::Adjustment::NONE, false);
        search_->add_to_maze_expansion_list(new_element);
    }
}

board::ForcedPadAlgo::CheckDrillResult MazeExpansionEngine::check_layer_with_any_matching_via(
    const ExpansionDrill& p_drill, int p_layer, const geometry::planar::IntBox& p_room_shape,
    const std::vector<int>& p_net_nos)
{
    // Port of Java MazeExpansionEngine.checkLayerWithAnyMatchingVia().
    AutorouteControl& ctrl = search_->control();
    if (ctrl.via_rule == nullptr || search_->autoroute_engine() == nullptr
        || search_->autoroute_engine()->board() == nullptr)
    {
        return board::ForcedPadAlgo::CheckDrillResult::NOT_DRILLABLE;
    }
    bool drillable_with_attach_smd = false;
    const int via_count = ctrl.via_rule->via_count();
    for (int i = 0; i < via_count; ++i)
    {
        std::shared_ptr<rules::ViaInfo> via_info = ctrl.via_rule->get_via(i);
        if (via_info == nullptr)
        {
            continue;
        }
        std::shared_ptr<board::Padstack> via_padstack = via_info->get_padstack();
        if (via_padstack == nullptr || p_layer < via_padstack->get_from_layer()
            || p_layer > via_padstack->get_to_layer())
        {
            continue;
        }
        const geometry::planar::Shape* via_shape = via_padstack->get_shape(p_layer);
        const double via_radius =
            via_shape == nullptr ? 0.0 : 0.5 * via_shape->bounding_box().max_width();
        const int trace_half_width = trace_half_width_of(ctrl, p_layer);
        const double required_radius = std::max(via_radius, static_cast<double>(trace_half_width));
        board::ForcedPadAlgo::CheckDrillResult result = board::ForcedViaAlgo::check_layer(
            required_radius, via_info->get_clearance_class(), via_info->attach_smd_allowed(),
            p_room_shape, p_drill.location, p_layer, p_net_nos,
            ctrl.max_shove_trace_recursion_depth, 0, search_->autoroute_engine()->board(),
            trace_half_width, ctrl.trace_clearance_class_no);
        if (result == board::ForcedPadAlgo::CheckDrillResult::DRILLABLE)
        {
            return result;
        }
        if (result == board::ForcedPadAlgo::CheckDrillResult::DRILLABLE_WITH_ATTACH_SMD)
        {
            drillable_with_attach_smd = true;
        }
    }
    return drillable_with_attach_smd ? board::ForcedPadAlgo::CheckDrillResult::DRILLABLE_WITH_ATTACH_SMD
        : board::ForcedPadAlgo::CheckDrillResult::NOT_DRILLABLE;
}

ExpansionDrill* MazeExpansionEngine::get_autoroute_drill_info(board::Via* p_via,
    board::ShapeSearchTree* p_autoroute_tree)
{
    if (p_via == nullptr || p_autoroute_tree == nullptr)
    {
        return nullptr;
    }

    auto it = via_drill_cache_.find(p_via);
    if (it != via_drill_cache_.end() && it->second != nullptr)
    {
        return it->second.get();
    }

    geometry::planar::IntBox drill_shape = geometry::planar::TileShape::get_instance(p_via->get_center());
    auto drill = std::make_unique<ExpansionDrill>(drill_shape, p_via->get_center(),
        p_via->get_from_layer(), p_via->get_to_layer());

    int via_layer_count = p_via->get_to_layer() - p_via->get_from_layer() + 1;
    if (via_layer_count > static_cast<int>(drill->room_arr.size()))
    {
        via_layer_count = static_cast<int>(drill->room_arr.size());
    }
    auto autoroute_info = p_via->get_autoroute_info();
    if (autoroute_info != nullptr)
    {
        for (int i = 0; i < via_layer_count; ++i)
        {
            drill->room_arr[static_cast<size_t>(i)] = autoroute_info->get_expansion_room(i, p_autoroute_tree);
        }
    }

    ExpansionDrill* result = drill.get();
    via_drill_cache_[p_via] = std::move(drill);
    return result;
}

} // namespace freerouting::autoroute
