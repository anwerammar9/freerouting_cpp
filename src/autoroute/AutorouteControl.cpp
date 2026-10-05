#include "autoroute/AutorouteControl.h"
#include "board/Pin.h"
#include "board/RoutingBoard.h"
#include "board/Padstack.h"
#include "geometry/planar/Shape.h"
#include "settings/RouterSettings.h"
#include "rules/BoardRules.h"
#include "rules/Net.h"
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
    // Java AutorouteControl takes the trace costs and the bend costs from the
    // router settings. The C++ settings are a stub returning an empty trace
    // cost array, so the neutral factor (1, 1) is used for every layer.
    trace_costs.assign(layer_count, ExpansionCostFactor(1.0, 1.0));
    bend_costs.assign(layer_count, 0.0);

    if (p_settings)
    {
        vias_allowed = p_settings->get_vias_allowed();
        for (int i = 0; i < layer_count; ++i)
        {
            layer_active[i] = p_settings->get_layer_active(i);
        }
        with_neckdown = p_settings->get_automatic_neckdown();
        std::vector<ExpansionCostFactor> settings_trace_costs = p_settings->get_trace_cost_arr();
        for (int i = 0; i < layer_count && i < static_cast<int>(settings_trace_costs.size()); ++i)
        {
            trace_costs[i] = settings_trace_costs[i];
        }
        for (int i = 0; i < layer_count; ++i)
        {
            bend_costs[i] = p_settings->get_bend_cost(i);
        }
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
    bend_costs.resize(p_count, 0.0);
    via_radius_arr.resize(p_count, 200.0);
    add_via_costs.resize(p_count, ViaCost(p_count));
}

void AutorouteControl::init_net(int p_net_no, board::RoutingBoard* p_board, int p_via_costs)
{
    net_no = p_net_no;
    if (!p_board) return;

    // Java init_net resolves the via rule of the current net class here, so
    // that rebuild_via_info() and the maze expansion engine can use it.
    rules::Net* current_net = p_board->get_rules().get_nets().get(p_net_no);
    if (current_net != nullptr && current_net->get_net_class() != nullptr)
    {
        via_rule = current_net->get_net_class()->get_via_rule();
    }
    else
    {
        via_rule = nullptr;
    }

    // This is a simplified migration of the Java init_net logic
    // In a full implementation, we would query the rules and via rules
    for (int i = 0; i < layer_count; ++i)
    {
        trace_half_width[i] = 200; // Default
        compensated_trace_half_width[i] = 200;
    }

    rebuild_via_info(p_board, p_via_costs, p_net_no);
}

namespace {

/**
 * Returns true, if every connectable item of p_net_no is an SMD pin, so that the
 * net has to be escaped from the component layer only. Port of Java
 * AutorouteControl.isPureSmdNet().
 */
bool is_pure_smd_net(board::RoutingBoard* p_board, int p_net_no)
{
    std::vector<board::Item*> net_items = p_board->get_connectable_items(p_net_no);
    if (net_items.empty())
    {
        return false;
    }
    for (board::Item* item : net_items)
    {
        auto* pin = dynamic_cast<board::Pin*>(item);
        if (pin == nullptr || pin->first_layer() != pin->last_layer())
        {
            return false;
        }
    }
    return true;
}

} // namespace

void AutorouteControl::rebuild_via_info(board::RoutingBoard* p_board, int p_via_costs, int p_net_no)
{
    // Port of Java AutorouteControl.rebuildViaInfo().
    const int via_count = via_rule != nullptr ? via_rule->via_count() : 0;

    if (via_count > 0)
    {
        auto first_via = via_rule->get_via(0);
        via_clearance_class = first_via != nullptr ? first_via->get_clearance_class() : 1;
    }
    else
    {
        via_clearance_class = 1;
    }

    via_info_arr.clear();
    attach_smd_allowed = false;
    // Java does not reset viaRadii and maxViaRadius here, the maxima accumulate
    // over repeated rebuilds of the same control, so they are not reset either.
    for (int i = 0; i < via_count; ++i)
    {
        auto current_via = via_rule->get_via(i);
        if (current_via == nullptr)
        {
            continue;
        }
        if (current_via->attach_smd_allowed())
        {
            attach_smd_allowed = true;
        }
        std::shared_ptr<board::Padstack> current_padstack = current_via->get_padstack();
        if (current_padstack == nullptr)
        {
            continue;
        }
        const int from_layer = current_padstack->get_from_layer();
        const int to_layer = current_padstack->get_to_layer();
        for (int j = from_layer; j <= to_layer; ++j)
        {
            if (j < 0 || j >= layer_count)
            {
                continue;
            }
            const geometry::planar::Shape* current_shape = current_padstack->get_shape(j);
            double current_radius = 0;
            if (current_shape != nullptr)
            {
                // Java casts the padstack shape to ConvexShape, which every
                // padstack shape of a board pad is.
                current_radius = 0.5 * current_shape->bounding_box().max_width();
            }
            via_radius_arr[j] = std::max(via_radius_arr[j], current_radius);
        }
        via_info_arr.emplace_back(from_layer, to_layer, current_via->attach_smd_allowed());
    }

    const bool pure_smd_net = p_board != nullptr && is_pure_smd_net(p_board, p_net_no);
    if (!attach_smd_allowed && layer_count > 1 && pure_smd_net)
    {
        // Pure SMD nets must still be able to escape their component layer,
        // even if the DSN marks every padstack as attach-off.
        attach_smd_allowed = true;
    }

    for (int j = 0; j < layer_count; ++j)
    {
        via_radius_arr[j] = std::max(via_radius_arr[j], static_cast<double>(trace_half_width[j]));
        max_via_radius = std::max(max_via_radius, via_radius_arr[j]);
    }

    double via_cost_factor = std::max(max_via_radius, 1.0);
    if (pure_smd_net)
    {
        // Pure SMD boards need a much cheaper via escape to avoid exhausting
        // the local pad channel before the search commits to a layer change.
        via_cost_factor *= 0.1;
    }
    min_normal_via_cost = static_cast<double>(p_via_costs) * via_cost_factor;
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