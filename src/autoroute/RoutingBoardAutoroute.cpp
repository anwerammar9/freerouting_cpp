// Port of Java app/freerouting/board/facade/RoutingBoard.autoroute().
//
// The Java method lives in the board package, but it needs the autoroute
// package (AutorouteControl, AutorouteEngine), which is not allowed to be
// referenced from the board module. The C++ port therefore declares the method
// in src/board/RoutingBoard.h and implements it here, in the autoroute module.

#include <set>
#include <string>
#include <vector>

#include "autoroute/AutorouteAttemptResult.h"
#include "autoroute/AutorouteAttemptState.h"
#include "autoroute/AutorouteControl.h"
#include "autoroute/AutorouteEngine.h"
#include "board/ConductionArea.h"
#include "board/Item.h"
#include "board/RoutingBoard.h"
#include "datastructures/TimeLimit.h"
#include "logger/FRLogger.h"
#include "rules/Net.h"
#include "settings/RouterSettings.h"

namespace freerouting::board {

namespace {

/**
 * Returns true if p_item is a Java `Connectable` item. The C++ Connectable
 * interface is a dead draft implemented by no real item, so a connectable item
 * is a routeable item (trace, drill) or a conduction area, mirroring the guard
 * used by the Java RoutingBoard.autoroute() facade.
 */
bool item_is_connectable(const Item* p_item)
{
    return p_item != nullptr
        && (p_item->is_trace() || p_item->is_via() || p_item->is_pin()
            || dynamic_cast<const ConductionArea*>(p_item) != nullptr);
}

} // namespace

autoroute::AutorouteAttemptResult RoutingBoard::autoroute(board::Item* p_item,
    settings::RouterSettings* p_router_settings, int p_via_costs,
    datastructures::Stoppable* p_stoppable, datastructures::TimeLimit* p_time_limit)
{
    if (!item_is_connectable(p_item) || p_item->net_count() == 0)
    {
        const std::string item_name = p_item != nullptr ? p_item->to_string() : "null";
        return autoroute::AutorouteAttemptResult(
            autoroute::AutorouteAttemptState::NO_CONNECTIONS,
            "The item '" + item_name + "' is not connectable.");
    }
    if (p_item->net_count() > 1)
    {
        logger::FRLogger::instance().warn(
            "RoutingBoard.autoroute: netCount > 1 not yet implemented");
    }
    const int route_net_no = p_item->get_net_no(0);
    autoroute::AutorouteControl ctrl_settings(this, route_net_no, p_router_settings, p_via_costs,
        p_router_settings != nullptr ? p_router_settings->get_trace_costs()
                                     : std::vector<autoroute::ExpansionCostFactor>());
    ctrl_settings.remove_unconnected_vias = false;
    const std::set<Item*> route_start_set = p_item->get_connected_set(route_net_no);
    rules::Net* route_net = get_rules().get_nets().get(route_net_no);
    if (route_net != nullptr && route_net->contains_plane())
    {
        for (Item* current_item : route_start_set)
        {
            if (dynamic_cast<ConductionArea*>(current_item) != nullptr)
            {
                return autoroute::AutorouteAttemptResult(
                    autoroute::AutorouteAttemptState::CONNECTED_TO_PLANE,
                    "The item '" + current_item->to_string() + "' is connected to a plane.");
            }
        }
    }
    const std::set<Item*> route_dest_set = p_item->get_unconnected_set(route_net_no);
    if (route_dest_set.empty())
    {
        return autoroute::AutorouteAttemptResult(
            autoroute::AutorouteAttemptState::ALREADY_CONNECTED,
            "The item '" + p_item->to_string() + "' is already connected.");
    }

    // Java calls init_autoroute(..., false), which always creates a new
    // AutorouteEngine and then initConnection(), so a local engine is
    // equivalent here.
    autoroute::AutorouteEngine current_autoroute_engine(
        this, ctrl_settings.trace_clearance_class_no, false);
    current_autoroute_engine.set_stoppable(p_stoppable);
    current_autoroute_engine.init_connection(route_net_no, p_stoppable,
        p_time_limit != nullptr ? p_time_limit->get_limit() : 0);

    autoroute::AutorouteAttemptResult result = current_autoroute_engine.autoroute_connection(
        route_start_set, route_dest_set, ctrl_settings);
    if (result.state == autoroute::AutorouteAttemptState::ROUTED)
    {
        constexpr int time_limit_to_prevent_endless_loop = 1000;
        opt_changed_area({ route_net_no }, nullptr,
            p_router_settings != nullptr ? p_router_settings->get_trace_pull_tight_accuracy() : 0,
            p_stoppable, nullptr, time_limit_to_prevent_endless_loop);
    }
    return result;
}

} // namespace freerouting::board