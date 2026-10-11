#pragma once

#include <optional>
#include <string>
#include <vector>

#include "settings/SettingsFieldTable.h"

namespace freerouting::settings {

/**
 * Execution knobs for the batch autorouter stage (fanout and optimizer stay in their own objects).
 * Mirrors Java settings/AutorouterSettings.java. All fields are nullable so SettingsMerger can tell
 * "this source sets the field" from "this source has no opinion". Copying the struct is Java clone().
 */
struct AutorouterSettings
{
    /** Whether the autorouter stage runs after fanout. JSON "enabled". */
    std::optional<bool> enabled;
    /** Algorithm identifier (for example RouterSettings::ALGORITHM_CURRENT). JSON "algorithm". */
    std::optional<std::string> algorithm;
    /** Maximum autorouter passes. 0 means no limit. JSON "max_passes". */
    std::optional<int> max_passes;
    /** Maximum items attempted in the autorouter stage. JSON "max_items". */
    std::optional<int> max_items;
    /** Maximum worker threads for a multi-thread autorouter pass. JSON "max_threads". */
    std::optional<int> max_threads;
    /** When true, intermediate board snapshots are saved between passes. JSON "save_intermediate_stages". */
    std::optional<bool> save_intermediate_stages;
    /** Net class names the autorouter should skip. JSON "ignore_net_classes". */
    std::optional<std::vector<std::string>> ignore_net_classes;
};

/** Java ReflectionUtil.copyFields(source, target) for AutorouterSettings; returns the changed-field count. */
int copy_fields(const AutorouterSettings& p_source, AutorouterSettings& p_target);

template <>
const std::vector<SettingsField<AutorouterSettings>>& settings_fields<AutorouterSettings>();

} // namespace freerouting::settings
