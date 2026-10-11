// Port of Java app/freerouting/board/facade/RoutingBoard.fanout().
//
// The Java method lives in the board package, but it needs the autoroute
// package (AutorouteControl, AutorouteEngine), which is not allowed to be
// referenced from the board module. The C++ port therefore declares the method
// in src/board/RoutingBoard.h and implements it here, in the autoroute module.

#include <algorithm>
#include <cmath>

#include "autoroute/AutorouteAttemptResult.h"
#include "autoroute/AutorouteAttemptState.h"
#include "autoroute/AutorouteControl.h"
#include "autoroute/AutorouteEngine.h"
#include "board/Component.h"
#include "board/Pin.h"
#include "board/RoutingBoard.h"
#include "datastructures/TimeLimit.h"
#include "settings/RouterSettings.h"

namespace freerouting::board {

namespace {

/**
 * Returns the squared distance of the center of the bounding box of
 * p_item to p_pin_center. Used to sort the unconnected items of the fanout
 * pass by their distance to the fanout pin.
 */
double distance_sq_to_pin_center(const Item* p_item,
    const geometry::planar::FloatPoint& p_pin_center)
{
    const geometry::planar::IntBox box = p_item->get_bounding_box();
    const double cx = (static_cast<double>(box.ll.x) + static_cast<double>(box.ur.x)) / 2.0;
    const double cy = (static_cast<double>(box.ll.y) + static_cast<double>(box.ur.y)) / 2.0;
    const double dx = cx - p_pin_center.x;
    const double dy = cy - p_pin_center.y;
    return dx * dx + dy * dy;
}

} // namespace

autoroute::AutorouteAttemptResult RoutingBoard::fanout(Pin* p_pin,
    settings::RouterSettings* p_router_settings,
    int p_ripup_costs,
    datastructures::Stoppable* p_stoppable,
    datastructures::TimeLimit* p_time_limit)
{
    if (p_pin == nullptr)
    {
        return autoroute::AutorouteAttemptResult(autoroute::AutorouteAttemptState::FAILED,
            "The fanout pin is null.");
    }
    if (p_pin->first_layer() != p_pin->last_layer() || p_pin->net_count() != 1)
    {
        return autoroute::AutorouteAttemptResult(autoroute::AutorouteAttemptState::ALREADY_CONNECTED,
            "The pin '" + p_pin->to_string() + "' is already connected.");
    }
    const int pin_net_no = p_pin->get_net_no(0);
    const int pin_layer = p_pin->first_layer();
    const ItemSet pin_connected_set = p_pin->get_connected_set(pin_net_no);
    for (const Item* current_item : pin_connected_set)
    {
        if (current_item->first_layer() != pin_layer || current_item->last_layer() != pin_layer)
        {
            return autoroute::AutorouteAttemptResult(
                autoroute::AutorouteAttemptState::ALREADY_CONNECTED,
                "The pin '" + p_pin->to_string() + "' is already connected.");
        }
    }
    const ItemSet unconnected_set = p_pin->get_unconnected_set(pin_net_no);
    if (unconnected_set.empty())
    {
        return autoroute::AutorouteAttemptResult(autoroute::AutorouteAttemptState::NO_UNCONNECTED_NETS,
            "The pin '" + p_pin->to_string() + "' is already connected.");
    }
    const geometry::planar::FloatPoint pin_center =
        p_pin->get_center().to_float();
    std::vector<Item*> sorted_unconnected_list(unconnected_set.begin(), unconnected_set.end());
    std::stable_sort(sorted_unconnected_list.begin(), sorted_unconnected_list.end(),
        [&pin_center](const Item* p_item1, const Item* p_item2)
        {
            return distance_sq_to_pin_center(p_item1, pin_center)
                < distance_sq_to_pin_center(p_item2, pin_center);
        });

    autoroute::AutorouteControl ctrl_settings(this, pin_net_no, p_router_settings);
    ctrl_settings.is_fanout = true;
    // Divergence: Java combines the via rule of the control with the first
    // global via rule of the board, if RouterSettings.fanout.fallbackToBoardVias
    // is set. The C++ RouterSettings has no fanout settings and the C++
    // BoardRules has no global via rule list yet, so this branch is missing.

    Component* pin_component =
        components() != nullptr ? components()->get(p_pin->get_component_no()) : nullptr;
    const std::optional<std::string> pin_name = p_pin->name();
    if (pin_component != nullptr && pin_name.has_value())
    {
        ctrl_settings.fanout_start_pin_name = pin_component->get_name() + "-" + *pin_name;
    }
    else
    {
        ctrl_settings.fanout_start_pin_name = p_pin->to_string();
    }
    ctrl_settings.fanout_start_pin_center = pin_center;
    ctrl_settings.fanout_start_pin_layer = p_pin->first_layer();
    ctrl_settings.remove_unconnected_vias = false;
    if (p_ripup_costs >= 0)
    {
        ctrl_settings.ripup_allowed = true;
        ctrl_settings.ripup_costs = p_ripup_costs;
    }

    // Java calls init_autoroute(..., false), which always creates a new
    // AutorouteEngine, so a local engine is equivalent here.
    autoroute::AutorouteEngine autoroute_engine(
        this, ctrl_settings.trace_clearance_class_no, false);
    autoroute_engine.set_stoppable(p_stoppable);
    autoroute_engine.init_connection(pin_net_no, p_stoppable,
        p_time_limit != nullptr ? p_time_limit->get_limit() : 0);

    bool has_result = false;
    autoroute::AutorouteAttemptResult result;
    if (sorted_unconnected_list.size() <= 4)
    {
        if (!sorted_unconnected_list.empty())
        {
            // 1. Try to route to the closest target first
            const ItemSet closest_target_set = { sorted_unconnected_list.front() };
            result = autoroute_engine.autoroute_connection(pin_connected_set, closest_target_set,
                ctrl_settings);
            has_result = true;

            // 2. If that fails and we have other targets, fall back to searching the entire
            // unconnected set at once
            if (result.state != autoroute::AutorouteAttemptState::ROUTED
                && result.state != autoroute::AutorouteAttemptState::ALREADY_CONNECTED
                && sorted_unconnected_list.size() > 1)
            {
                result = autoroute_engine.autoroute_connection(pin_connected_set, unconnected_set,
                    ctrl_settings);
            }
        }
    }
    else
    {
        // For large nets (e.g. power/ground/buses), route to the entire unconnected set at once to
        // avoid CPU thrashing
        result = autoroute_engine.autoroute_connection(pin_connected_set, unconnected_set,
            ctrl_settings);
        has_result = true;
    }
    if (!has_result)
    {
        result = autoroute::AutorouteAttemptResult(autoroute::AutorouteAttemptState::FAILED,
            "No target items to route connection.");
    }

    if (result.state == autoroute::AutorouteAttemptState::ROUTED)
    {
        constexpr int time_limit_to_prevent_endless_loop = 1000;
        opt_changed_area({ pin_net_no }, nullptr,
            p_router_settings != nullptr ? p_router_settings->get_trace_pull_tight_accuracy() : 0,
            p_stoppable, nullptr, time_limit_to_prevent_endless_loop);
    }
    return result;
}

} // namespace freerouting::board