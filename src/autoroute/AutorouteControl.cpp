#include "autoroute/AutorouteControl.h"
#include "board/RoutingBoard.h"
#include "settings/RouterSettings.h"
#include "rules/BoardRules.h"
#include "rules/NetClass.h"
#include "rules/ViaRule.h"
#include <algorithm>
#include <cmath>

namespace freerouting::autoroute {

AutorouteControl::AutorouteControl() = default;

AutorouteControl::AutorouteControl(board::RoutingBoard* p_board, int p_net_no, settings::RouterSettings* p_settings)
    : settings(p_settings)
{
    if (!p_board) return;

    layer_count = p_board->get_layer_count();
    trace_half_width.resize(layer_count, 0);
    compensated_trace_half_width.resize(layer_count, 0);
    layer_active.resize(layer_count, true);
    via_radius_arr.resize(layer_count, 0.0);
    add_via_costs.resize(layer_count, ViaCost(layer_count));

    if (p_settings)
    {
        vias_allowed = p_settings->get_vias_allowed();
        for (int i = 0; i < layer_count; ++i)
        {
            layer_active[i] = p_settings->get_layer_active(i);
        }
        with_neckdown = p_settings->get_automatic_neckdown();
    }

    via_lower_bound = 0;
    via_upper_bound = layer_count;

    init_net(p_net_no, p_board, p_settings ? p_settings->get_via_costs() : 100);
}

AutorouteControl::~AutorouteControl() = default;

void AutorouteControl::set_layer_count(int p_count)
{
    layer_count = p_count;
    layer_active.resize(p_count, true);
    trace_half_width.resize(p_count, 200);
    compensated_trace_half_width.resize(p_count, 200);
    trace_costs.resize(p_count, ExpansionCostFactor(1.0, 1.0));
    via_radius_arr.resize(p_count, 200.0);
    add_via_costs.resize(p_count, ViaCost(p_count));
}

void AutorouteControl::init_net(int p_net_no, board::RoutingBoard* p_board, int p_via_costs)
{
    net_no = p_net_no;
    if (!p_board) return;

    // This is a simplified migration of the Java init_net logic
    // In a full implementation, we would query the rules and via rules
    for (int i = 0; i < layer_count; ++i)
    {
        trace_half_width[i] = 200; // Default
        compensated_trace_half_width[i] = 200;
        via_radius_arr[i] = 200;
    }

    max_via_radius = 200;
    min_normal_via_cost = p_via_costs * std::max(max_via_radius, 1.0);
    min_cheap_via_cost = 0.8 * min_normal_via_cost;
}

bool AutorouteControl::is_layer_active(int p_layer) const
{
    if (p_layer < 0 || p_layer >= static_cast<int>(layer_active.size()))
        return false;
    return layer_active[p_layer];
}

int AutorouteControl::get_trace_half_width(int p_layer) const
{
    if (p_layer < 0 || p_layer >= static_cast<int>(trace_half_width.size()))
        return 200;
    return trace_half_width[p_layer];
}

double AutorouteControl::get_via_cost(int p_from_layer, int p_to_layer) const
{
    if (p_from_layer < 0 || p_from_layer >= layer_count || p_to_layer < 0 || p_to_layer >= layer_count)
        return min_normal_via_cost;
    return min_normal_via_cost + add_via_costs[p_from_layer].to_layer[p_to_layer];
}

} // namespace freerouting::autoroute