#include "settings/RouterScoreSettings.h"

namespace freerouting::settings {

int copy_fields(const RouterScoreSettings& p_source, RouterScoreSettings& p_target)
{
    int changed = 0;
    changed += copy_enum_field(p_source.version, p_target.version);
    changed += copy_boxed_field(p_source.unrouted_connection_weight, p_target.unrouted_connection_weight);
    changed += copy_boxed_field(p_source.unrouted_free_fraction, p_target.unrouted_free_fraction);
    changed += copy_boxed_field(p_source.unrouted_first_half_weight, p_target.unrouted_first_half_weight);
    changed += copy_boxed_field(p_source.unrouted_second_half_weight, p_target.unrouted_second_half_weight);
    changed += copy_boxed_field(p_source.clearance_violation_count_weight, p_target.clearance_violation_count_weight);
    changed += copy_boxed_field(p_source.clearance_violation_depth_weight, p_target.clearance_violation_depth_weight);
    changed += copy_boxed_field(p_source.clearance_violation_depth_scale, p_target.clearance_violation_depth_scale);
    return changed;
}

template <>
const std::vector<SettingsField<RouterScoreSettings>>& settings_fields<RouterScoreSettings>()
{
    using S = RouterScoreSettings;
    static const std::vector<SettingsField<S>> fields = {
        settings_leaf_field("version", "version", &S::version),
        settings_leaf_field("unroutedConnectionWeight", "unrouted_connection_weight", &S::unrouted_connection_weight),
        settings_leaf_field("unroutedFreeFraction", "unrouted_free_fraction", &S::unrouted_free_fraction),
        settings_leaf_field("unroutedFirstHalfWeight", "unrouted_first_half_weight", &S::unrouted_first_half_weight),
        settings_leaf_field("unroutedSecondHalfWeight", "unrouted_second_half_weight",
                            &S::unrouted_second_half_weight),
        settings_leaf_field("clearanceViolationCountWeight", "clearance_violation_count_weight",
                            &S::clearance_violation_count_weight),
        settings_leaf_field("clearanceViolationDepthWeight", "clearance_violation_depth_weight",
                            &S::clearance_violation_depth_weight),
        settings_leaf_field("clearanceViolationDepthScale", "clearance_violation_depth_scale",
                            &S::clearance_violation_depth_scale),
    };
    return fields;
}

} // namespace freerouting::settings
