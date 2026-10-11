#pragma once

#include <array>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "autoroute/BoardUpdateStrategy.h"
#include "autoroute/ItemSelectionStrategy.h"
#include "settings/SettingsFieldTable.h"

namespace freerouting::settings {

/**
 * Settings for the route optimizer which runs after auto-routing to reduce vias and trace length.
 * Mirrors Java settings/OptimizerSettings.java. Copying the struct is Java clone().
 */
struct OptimizerSettings
{
    /** Whether the route optimizer is enabled. JSON "enabled". */
    std::optional<bool> enabled;
    /** Optimization algorithm identifier (e.g. "freerouting-optimizer"). JSON "algorithm". */
    std::optional<std::string> algorithm;
    /** Maximum number of full optimization passes. JSON "max_passes". */
    std::optional<int> max_passes;
    /** Maximum number of item optimization attempts. JSON "max_items". */
    std::optional<int> max_items;
    /** Maximum number of threads for parallel route optimization. JSON "max_threads". */
    std::optional<int> max_threads;
    /** Relative pass-improvement threshold in percent (2.5 = 2.5%). JSON "improvement_threshold". */
    std::optional<float> optimization_improvement_threshold;
    /** Whether pre-flight optimization guards are enabled. JSON "enable_preflight_guards". */
    std::optional<bool> enable_preflight_guards;
    /** Consecutive item failures before the current pass is aborted. JSON "max_consecutive_failures". */
    std::optional<int> max_consecutive_failures;
    /** Consecutive item failures before pass 1 is aborted. JSON "max_consecutive_failures_pass1". */
    std::optional<int> max_consecutive_failures_pass1;
    /** Multiplier of the base ripup cost at the start of optimization. JSON "additional_ripup_cost_factor_at_start". */
    std::optional<int> additional_ripup_cost_factor_at_start;
    /** Ripup cost discount for traces. JSON "trace_ripup_cost_factor". */
    std::optional<float> trace_ripup_cost_factor;
    /** Autoroute passes per ripped-up item during optimization. JSON "max_autoroute_passes". */
    std::optional<int> max_autoroute_passes;
    /** Java transient. JSON "board_update_strategy". */
    std::optional<autoroute::BoardUpdateStrategy> board_update_strategy;
    /** Java transient. JSON "item_selection_strategy". */
    std::optional<autoroute::ItemSelectionStrategy> item_selection_strategy;
    /** Timeout for the optimizer stage (e.g. "5m"). JSON "timeout". */
    std::optional<std::string> timeout_string;
};

/** Java ReflectionUtil.copyFields(source, target) for OptimizerSettings; returns the changed-field count. */
int copy_fields(const OptimizerSettings& p_source, OptimizerSettings& p_target);

template <>
const std::vector<SettingsField<OptimizerSettings>>& settings_fields<OptimizerSettings>();

/**
 * Java enum autoroute.BoardUpdateStrategy has the single constant GLOBAL_OPTIMAL; the extra C++
 * constants (GREEDY, HYBRID) have no Java name and cannot be parsed from settings text.
 */
template <>
struct SettingsEnumNames<autoroute::BoardUpdateStrategy>
{
    static constexpr std::array<std::pair<autoroute::BoardUpdateStrategy, std::string_view>, 1> names{{
        {autoroute::BoardUpdateStrategy::GLOBAL_OPTIMAL, "GLOBAL_OPTIMAL"},
    }};
};

/** Java enum autoroute.ItemSelectionStrategy: SEQUENTIAL, PRIORITIZED (C++ RANDOM has no Java name). */
template <>
struct SettingsEnumNames<autoroute::ItemSelectionStrategy>
{
    static constexpr std::array<std::pair<autoroute::ItemSelectionStrategy, std::string_view>, 2> names{{
        {autoroute::ItemSelectionStrategy::SEQUENTIAL, "SEQUENTIAL"},
        {autoroute::ItemSelectionStrategy::PRIORITIZED, "PRIORITIZED"},
    }};
};

} // namespace freerouting::settings
