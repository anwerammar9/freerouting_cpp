#include "settings/OptimizerScoreSettings.h"

namespace freerouting::settings {

int copy_fields(const OptimizerScoreSettings& p_source, OptimizerScoreSettings& p_target)
{
    int changed = 0;
    changed += copy_enum_field(p_source.version, p_target.version);
    changed += copy_boxed_field(p_source.excess_wire_length_weight, p_target.excess_wire_length_weight);
    changed += copy_boxed_field(p_source.excess_via_weight, p_target.excess_via_weight);
    changed += copy_boxed_field(p_source.excess_bend_weight, p_target.excess_bend_weight);
    changed += copy_boxed_field(p_source.length_floor, p_target.length_floor);
    changed += copy_boxed_field(p_source.difficulty_scale_floor, p_target.difficulty_scale_floor);
    return changed;
}

template <>
const std::vector<SettingsField<OptimizerScoreSettings>>& settings_fields<OptimizerScoreSettings>()
{
    using S = OptimizerScoreSettings;
    static const std::vector<SettingsField<S>> fields = {
        settings_leaf_field("version", "version", &S::version),
        settings_leaf_field("excessWireLengthWeight", "excess_wire_length_weight", &S::excess_wire_length_weight),
        settings_leaf_field("excessViaWeight", "excess_via_weight", &S::excess_via_weight),
        settings_leaf_field("excessBendWeight", "excess_bend_weight", &S::excess_bend_weight),
        settings_leaf_field("lengthFloor", "length_floor", &S::length_floor),
        settings_leaf_field("difficultyScaleFloor", "difficulty_scale_floor", &S::difficulty_scale_floor),
    };
    return fields;
}

} // namespace freerouting::settings
