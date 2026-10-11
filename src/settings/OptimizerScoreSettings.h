#pragma once

#include <optional>
#include <vector>

#include "settings/OptimizerScoringVersion.h"
#include "settings/SettingsFieldTable.h"

namespace freerouting::settings {

/** Nullable settings for the optimizer-board score. Mirrors Java settings/OptimizerScoreSettings.java. */
struct OptimizerScoreSettings
{
    /** JSON "version". */
    std::optional<OptimizerScoringVersion> version;
    /** JSON "excess_wire_length_weight". */
    std::optional<float> excess_wire_length_weight;
    /** JSON "excess_via_weight". */
    std::optional<float> excess_via_weight;
    /** JSON "excess_bend_weight". */
    std::optional<float> excess_bend_weight;
    /** JSON "length_floor". */
    std::optional<float> length_floor;
    /** JSON "difficulty_scale_floor". */
    std::optional<float> difficulty_scale_floor;
};

/** Java ReflectionUtil.copyFields(source, target) for OptimizerScoreSettings; returns the changed-field count. */
int copy_fields(const OptimizerScoreSettings& p_source, OptimizerScoreSettings& p_target);

template <>
const std::vector<SettingsField<OptimizerScoreSettings>>& settings_fields<OptimizerScoreSettings>();

} // namespace freerouting::settings
