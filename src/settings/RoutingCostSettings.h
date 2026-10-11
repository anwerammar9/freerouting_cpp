#pragma once

#include <optional>
#include <vector>

#include "settings/SettingsFieldTable.h"

namespace freerouting::settings {

/**
 * Costs used by the router's search and rip-up algorithms. Mirrors Java
 * settings/RoutingCostSettings.java (the former C++ RouterScoringSettings, which baked in the
 * DefaultSettings values; defaults now come from sources::DefaultSettings). Copying the struct is
 * Java clone() (the per-layer arrays are deep-copied).
 */
struct RoutingCostSettings
{
    /** Cost of 1 mm of trace in the preferred direction, per layer. Java transient. JSON "preferred_direction_trace_cost". */
    std::optional<std::vector<double>> preferred_direction_trace_cost;
    /** Cost of 1 mm of trace in the undesired direction, per layer. Java transient. JSON "undesired_direction_trace_cost". */
    std::optional<std::vector<double>> undesired_direction_trace_cost;
    /** JSON "default_preferred_direction_trace_cost". */
    std::optional<double> default_preferred_direction_trace_cost;
    /** JSON "default_undesired_direction_trace_cost". */
    std::optional<double> default_undesired_direction_trace_cost;
    /** Cost of a via on a regular net. JSON "via_costs" (alternate "viaCosts"). */
    std::optional<int> via_costs;
    /** Cost of a via placed on a plane. JSON "plane_via_costs". */
    std::optional<int> plane_via_costs;
    /** Base rip-up cost of the first rip-up pass. JSON "start_ripup_costs" (alternate "startRipupCosts"). */
    std::optional<int> start_ripup_costs;
    /** Default bend cost per direction change on a layer. JSON "default_bend_cost". */
    std::optional<double> default_bend_cost;
    /** Legacy board-score weight. JSON "unrouted_net_penalty". */
    std::optional<float> unrouted_net_penalty;
    /** Legacy board-score weight. JSON "clearance_violation_penalty". */
    std::optional<float> clearance_violation_penalty;
    /** Legacy board-score weight. JSON "bend_penalty". */
    std::optional<float> bend_penalty;
};

/** Java ReflectionUtil.copyFields(source, target) for RoutingCostSettings; returns the changed-field count. */
int copy_fields(const RoutingCostSettings& p_source, RoutingCostSettings& p_target);

template <>
const std::vector<SettingsField<RoutingCostSettings>>& settings_fields<RoutingCostSettings>();

} // namespace freerouting::settings
