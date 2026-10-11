#include "settings/AutorouterSettings.h"

namespace freerouting::settings {

int copy_fields(const AutorouterSettings& p_source, AutorouterSettings& p_target)
{
    int changed = 0;
    changed += copy_boxed_field(p_source.enabled, p_target.enabled);
    changed += copy_boxed_field(p_source.algorithm, p_target.algorithm);
    changed += copy_boxed_field(p_source.max_passes, p_target.max_passes);
    changed += copy_boxed_field(p_source.max_items, p_target.max_items);
    changed += copy_boxed_field(p_source.max_threads, p_target.max_threads);
    changed += copy_boxed_field(p_source.save_intermediate_stages, p_target.save_intermediate_stages);
    changed += copy_array_field(p_source.ignore_net_classes, p_target.ignore_net_classes);
    return changed;
}

template <>
const std::vector<SettingsField<AutorouterSettings>>& settings_fields<AutorouterSettings>()
{
    using S = AutorouterSettings;
    static const std::vector<SettingsField<S>> fields = {
        settings_leaf_field("enabled", "enabled", &S::enabled),
        settings_leaf_field("algorithm", "algorithm", &S::algorithm),
        settings_leaf_field("maxPasses", "max_passes", &S::max_passes),
        settings_leaf_field("maxItems", "max_items", &S::max_items),
        settings_leaf_field("maxThreads", "max_threads", &S::max_threads),
        settings_leaf_field("saveIntermediateStages", "save_intermediate_stages", &S::save_intermediate_stages),
        settings_leaf_field("ignoreNetClasses", "ignore_net_classes", &S::ignore_net_classes),
    };
    return fields;
}

} // namespace freerouting::settings
