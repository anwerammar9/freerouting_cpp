#pragma once

#include <optional>
#include <vector>

#include "settings/SettingsFieldTable.h"

namespace freerouting::settings {

/** Settings configuration for a single board layer. Mirrors Java settings/LayerSettings.java. */
struct LayerSettings
{
    LayerSettings() = default;

    LayerSettings(std::optional<bool> p_routable, std::optional<bool> p_preferred_direction_horizontal)
        : routable(p_routable)
        , preferred_direction_horizontal(p_preferred_direction_horizontal)
    {
    }

    LayerSettings(std::optional<bool> p_routable, std::optional<bool> p_preferred_direction_horizontal,
                  std::optional<double> p_bend_cost)
        : routable(p_routable)
        , preferred_direction_horizontal(p_preferred_direction_horizontal)
        , bend_cost(p_bend_cost)
    {
    }

    /** JSON "routable". */
    std::optional<bool> routable;
    /** JSON "preferred_direction_horizontal". */
    std::optional<bool> preferred_direction_horizontal;
    /**
     * Per-layer bend cost added each time the router changes direction on this layer. Null means
     * "use the board default"; valid range 0.0 .. 9.9. JSON "bend_cost".
     */
    std::optional<double> bend_cost;

    /** Java LayerSettings.equals (Objects.equals on all three fields). */
    bool operator==(const LayerSettings&) const = default;
};

/** Java ReflectionUtil.copyFields(source, target) for LayerSettings; returns the changed-field count. */
int copy_fields(const LayerSettings& p_source, LayerSettings& p_target);

template <>
const std::vector<SettingsField<LayerSettings>>& settings_fields<LayerSettings>();

} // namespace freerouting::settings
