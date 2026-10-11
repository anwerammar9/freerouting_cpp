#pragma once

#include <optional>
#include <vector>

#include "settings/RouterScoringVersion.h"
#include "settings/SettingsFieldTable.h"

namespace freerouting::settings {

/** Nullable settings for the router-board score. Mirrors Java settings/RouterScoreSettings.java. */
struct RouterScoreSettings
{
    /** JSON "version". */
    std::optional<RouterScoringVersion> version;
    /** JSON "unrouted_connection_weight". */
    std::optional<float> unrouted_connection_weight;
    /** JSON "unrouted_free_fraction". */
    std::optional<float> unrouted_free_fraction;
    /** JSON "unrouted_first_half_weight". */
    std::optional<float> unrouted_first_half_weight;
    /** JSON "unrouted_second_half_weight". */
    std::optional<float> unrouted_second_half_weight;
    /** JSON "clearance_violation_count_weight". */
    std::optional<float> clearance_violation_count_weight;
    /** JSON "clearance_violation_depth_weight". */
    std::optional<float> clearance_violation_depth_weight;
    /** JSON "clearance_violation_depth_scale". */
    std::optional<float> clearance_violation_depth_scale;
};

/** Java ReflectionUtil.copyFields(source, target) for RouterScoreSettings; returns the changed-field count. */
int copy_fields(const RouterScoreSettings& p_source, RouterScoreSettings& p_target);

template <>
const std::vector<SettingsField<RouterScoreSettings>>& settings_fields<RouterScoreSettings>();

} // namespace freerouting::settings
