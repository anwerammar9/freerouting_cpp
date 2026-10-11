#pragma once

#include <optional>
#include <string>
#include <vector>

#include "settings/SettingsFieldTable.h"

namespace freerouting::settings {

/**
 * Configuration for the SMD-pin fanout pre-pass that runs before the main batch autorouter.
 * Mirrors Java settings/FanoutSettings.java; null means "this source has no opinion", defaults
 * live in DefaultSettings. Copying the struct is Java clone().
 */
struct FanoutSettings
{
    /** Whether to run the fanout pre-pass at all. JSON "enabled". */
    std::optional<bool> enabled;
    /** Maximum number of fanout passes. JSON "max_passes". */
    std::optional<int> max_passes;
    /** Maximum number of escape/fanout routing attempts. JSON "max_items". */
    std::optional<int> max_items;
    /** Base time budget per SMD pin in pass 1 (scaled by the pass number). JSON "max_milliseconds_per_pin". */
    std::optional<long long> max_milliseconds_per_pin;
    /** Whether fanout may rip up existing traces. JSON "ripup_allowed" (alternate "ripupAllowed"). */
    std::optional<bool> ripup_allowed;
    /** Minimum escape wire length in mm. JSON "min_escape_length_mm". */
    std::optional<double> min_escape_length_mm;
    /** Maximum escape wire length in mm. JSON "max_escape_length_mm". */
    std::optional<double> max_escape_length_mm;
    /** Diameter of starting/escape vias in mm. JSON "start_via_diameter_mm". */
    std::optional<double> start_via_diameter_mm;
    /** Diameter of landing/end vias in mm. JSON "end_via_diameter_mm". */
    std::optional<double> end_via_diameter_mm;
    /** "inner_first", "outer_first" or "unsorted". JSON "pin_sorting_order". */
    std::optional<std::string> pin_sorting_order;
    /** Fall back to board-wide via rules for nets without via rules. JSON "fallback_to_board_vias". */
    std::optional<bool> fallback_to_board_vias;
    /** Timeout for the fanout stage (e.g. "5m"). JSON "timeout". */
    std::optional<std::string> timeout_string;
};

/** Java ReflectionUtil.copyFields(source, target) for FanoutSettings; returns the changed-field count. */
int copy_fields(const FanoutSettings& p_source, FanoutSettings& p_target);

template <>
const std::vector<SettingsField<FanoutSettings>>& settings_fields<FanoutSettings>();

} // namespace freerouting::settings
