#include "settings/RoutingCostSettings.h"

namespace freerouting::settings {

int copy_fields(const RoutingCostSettings& p_source, RoutingCostSettings& p_target)
{
    int changed = 0;
    // Java parity: the transient per-layer arrays are public and therefore copied by copyFields.
    changed += copy_array_field(p_source.preferred_direction_trace_cost, p_target.preferred_direction_trace_cost);
    changed += copy_array_field(p_source.undesired_direction_trace_cost, p_target.undesired_direction_trace_cost);
    changed += copy_boxed_field(p_source.default_preferred_direction_trace_cost,
                                p_target.default_preferred_direction_trace_cost);
    changed += copy_boxed_field(p_source.default_undesired_direction_trace_cost,
                                p_target.default_undesired_direction_trace_cost);
    changed += copy_boxed_field(p_source.via_costs, p_target.via_costs);
    changed += copy_boxed_field(p_source.plane_via_costs, p_target.plane_via_costs);
    changed += copy_boxed_field(p_source.start_ripup_costs, p_target.start_ripup_costs);
    changed += copy_boxed_field(p_source.default_bend_cost, p_target.default_bend_cost);
    changed += copy_boxed_field(p_source.unrouted_net_penalty, p_target.unrouted_net_penalty);
    changed += copy_boxed_field(p_source.clearance_violation_penalty, p_target.clearance_violation_penalty);
    changed += copy_boxed_field(p_source.bend_penalty, p_target.bend_penalty);
    return changed;
}

template <>
const std::vector<SettingsField<RoutingCostSettings>>& settings_fields<RoutingCostSettings>()
{
    using S = RoutingCostSettings;
    static const std::vector<SettingsField<S>> fields = {
        settings_transient(settings_leaf_field("preferredDirectionTraceCost", "preferred_direction_trace_cost",
                                               &S::preferred_direction_trace_cost)),
        settings_transient(settings_leaf_field("undesiredDirectionTraceCost", "undesired_direction_trace_cost",
                                               &S::undesired_direction_trace_cost)),
        settings_leaf_field("defaultPreferredDirectionTraceCost", "default_preferred_direction_trace_cost",
                            &S::default_preferred_direction_trace_cost),
        settings_leaf_field("defaultUndesiredDirectionTraceCost", "default_undesired_direction_trace_cost",
                            &S::default_undesired_direction_trace_cost),
        settings_leaf_field("viaCosts", "via_costs", &S::via_costs, {"viaCosts"}),
        settings_leaf_field("planeViaCosts", "plane_via_costs", &S::plane_via_costs),
        settings_leaf_field("startRipupCosts", "start_ripup_costs", &S::start_ripup_costs, {"startRipupCosts"}),
        settings_leaf_field("defaultBendCost", "default_bend_cost", &S::default_bend_cost),
        settings_leaf_field("unroutedNetPenalty", "unrouted_net_penalty", &S::unrouted_net_penalty),
        settings_leaf_field("clearanceViolationPenalty", "clearance_violation_penalty",
                            &S::clearance_violation_penalty),
        settings_leaf_field("bendPenalty", "bend_penalty", &S::bend_penalty),
    };
    return fields;
}

} // namespace freerouting::settings
