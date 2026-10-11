#include "settings/FanoutSettings.h"

namespace freerouting::settings {

int copy_fields(const FanoutSettings& p_source, FanoutSettings& p_target)
{
    int changed = 0;
    changed += copy_boxed_field(p_source.enabled, p_target.enabled);
    changed += copy_boxed_field(p_source.max_passes, p_target.max_passes);
    changed += copy_boxed_field(p_source.max_items, p_target.max_items);
    changed += copy_boxed_field(p_source.max_milliseconds_per_pin, p_target.max_milliseconds_per_pin);
    changed += copy_boxed_field(p_source.ripup_allowed, p_target.ripup_allowed);
    changed += copy_boxed_field(p_source.min_escape_length_mm, p_target.min_escape_length_mm);
    changed += copy_boxed_field(p_source.max_escape_length_mm, p_target.max_escape_length_mm);
    changed += copy_boxed_field(p_source.start_via_diameter_mm, p_target.start_via_diameter_mm);
    changed += copy_boxed_field(p_source.end_via_diameter_mm, p_target.end_via_diameter_mm);
    changed += copy_boxed_field(p_source.pin_sorting_order, p_target.pin_sorting_order);
    changed += copy_boxed_field(p_source.fallback_to_board_vias, p_target.fallback_to_board_vias);
    changed += copy_boxed_field(p_source.timeout_string, p_target.timeout_string);
    return changed;
}

template <>
const std::vector<SettingsField<FanoutSettings>>& settings_fields<FanoutSettings>()
{
    using S = FanoutSettings;
    static const std::vector<SettingsField<S>> fields = {
        settings_leaf_field("enabled", "enabled", &S::enabled),
        settings_leaf_field("maxPasses", "max_passes", &S::max_passes),
        settings_leaf_field("maxItems", "max_items", &S::max_items),
        settings_leaf_field("maxMillisecondsPerPin", "max_milliseconds_per_pin", &S::max_milliseconds_per_pin),
        settings_leaf_field("ripupAllowed", "ripup_allowed", &S::ripup_allowed, {"ripupAllowed"}),
        settings_leaf_field("minEscapeLengthMm", "min_escape_length_mm", &S::min_escape_length_mm),
        settings_leaf_field("maxEscapeLengthMm", "max_escape_length_mm", &S::max_escape_length_mm),
        settings_leaf_field("startViaDiameterMm", "start_via_diameter_mm", &S::start_via_diameter_mm),
        settings_leaf_field("endViaDiameterMm", "end_via_diameter_mm", &S::end_via_diameter_mm),
        settings_leaf_field("pinSortingOrder", "pin_sorting_order", &S::pin_sorting_order),
        settings_leaf_field("fallbackToBoardVias", "fallback_to_board_vias", &S::fallback_to_board_vias),
        settings_leaf_field("timeoutString", "timeout", &S::timeout_string),
    };
    return fields;
}

} // namespace freerouting::settings
