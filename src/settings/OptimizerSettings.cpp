#include "settings/OptimizerSettings.h"

namespace freerouting::settings {

int copy_fields(const OptimizerSettings& p_source, OptimizerSettings& p_target)
{
    int changed = 0;
    changed += copy_boxed_field(p_source.enabled, p_target.enabled);
    changed += copy_boxed_field(p_source.algorithm, p_target.algorithm);
    changed += copy_boxed_field(p_source.max_passes, p_target.max_passes);
    changed += copy_boxed_field(p_source.max_items, p_target.max_items);
    changed += copy_boxed_field(p_source.max_threads, p_target.max_threads);
    changed += copy_boxed_field(p_source.optimization_improvement_threshold,
                                p_target.optimization_improvement_threshold);
    changed += copy_boxed_field(p_source.enable_preflight_guards, p_target.enable_preflight_guards);
    changed += copy_boxed_field(p_source.max_consecutive_failures, p_target.max_consecutive_failures);
    changed += copy_boxed_field(p_source.max_consecutive_failures_pass1, p_target.max_consecutive_failures_pass1);
    changed += copy_boxed_field(p_source.additional_ripup_cost_factor_at_start,
                                p_target.additional_ripup_cost_factor_at_start);
    changed += copy_boxed_field(p_source.trace_ripup_cost_factor, p_target.trace_ripup_cost_factor);
    changed += copy_boxed_field(p_source.max_autoroute_passes, p_target.max_autoroute_passes);
    // Java parity: transient fields are public and therefore copied by copyFields.
    changed += copy_enum_field(p_source.board_update_strategy, p_target.board_update_strategy);
    changed += copy_enum_field(p_source.item_selection_strategy, p_target.item_selection_strategy);
    changed += copy_boxed_field(p_source.timeout_string, p_target.timeout_string);
    return changed;
}

template <>
const std::vector<SettingsField<OptimizerSettings>>& settings_fields<OptimizerSettings>()
{
    using S = OptimizerSettings;
    static const std::vector<SettingsField<S>> fields = {
        settings_leaf_field("enabled", "enabled", &S::enabled),
        settings_leaf_field("algorithm", "algorithm", &S::algorithm),
        settings_leaf_field("maxPasses", "max_passes", &S::max_passes),
        settings_leaf_field("maxItems", "max_items", &S::max_items),
        settings_leaf_field("maxThreads", "max_threads", &S::max_threads),
        settings_leaf_field("optimizationImprovementThreshold", "improvement_threshold",
                            &S::optimization_improvement_threshold),
        settings_leaf_field("enablePreflightGuards", "enable_preflight_guards", &S::enable_preflight_guards),
        settings_leaf_field("maxConsecutiveFailures", "max_consecutive_failures", &S::max_consecutive_failures),
        settings_leaf_field("maxConsecutiveFailuresPass1", "max_consecutive_failures_pass1",
                            &S::max_consecutive_failures_pass1),
        settings_leaf_field("additionalRipupCostFactorAtStart", "additional_ripup_cost_factor_at_start",
                            &S::additional_ripup_cost_factor_at_start),
        settings_leaf_field("traceRipupCostFactor", "trace_ripup_cost_factor", &S::trace_ripup_cost_factor),
        settings_leaf_field("maxAutoroutePasses", "max_autoroute_passes", &S::max_autoroute_passes),
        settings_transient(
            settings_leaf_field("boardUpdateStrategy", "board_update_strategy", &S::board_update_strategy)),
        settings_transient(
            settings_leaf_field("itemSelectionStrategy", "item_selection_strategy", &S::item_selection_strategy)),
        settings_leaf_field("timeoutString", "timeout", &S::timeout_string),
    };
    return fields;
}

} // namespace freerouting::settings
