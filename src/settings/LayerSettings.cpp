#include "settings/LayerSettings.h"

namespace freerouting::settings {

int copy_fields(const LayerSettings& p_source, LayerSettings& p_target)
{
    int changed = 0;
    changed += copy_boxed_field(p_source.routable, p_target.routable);
    changed += copy_boxed_field(p_source.preferred_direction_horizontal, p_target.preferred_direction_horizontal);
    changed += copy_boxed_field(p_source.bend_cost, p_target.bend_cost);
    return changed;
}

template <>
const std::vector<SettingsField<LayerSettings>>& settings_fields<LayerSettings>()
{
    using S = LayerSettings;
    static const std::vector<SettingsField<S>> fields = {
        settings_leaf_field("routable", "routable", &S::routable),
        settings_leaf_field("preferredDirectionHorizontal", "preferred_direction_horizontal",
                            &S::preferred_direction_horizontal),
        settings_leaf_field("bendCost", "bend_cost", &S::bend_cost),
    };
    return fields;
}

} // namespace freerouting::settings
