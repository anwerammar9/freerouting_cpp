#include "settings/RoutingCostSettings.h"

#include <gtest/gtest.h>

namespace freerouting::settings {

TEST(RoutingCostSettingsTest, DoubleArrayCopyRules)
{
    RoutingCostSettings source;
    source.preferred_direction_trace_cost = std::vector<double>{1.0, 2.0};
    source.undesired_direction_trace_cost = std::vector<double>{};
    RoutingCostSettings target;
    target.undesired_direction_trace_cost = std::vector<double>{3.0};
    // Non-empty array copied, empty array not copied over a non-null target.
    EXPECT_EQ(copy_fields(source, target), 1);
    EXPECT_EQ(*target.preferred_direction_trace_cost, (std::vector<double>{1.0, 2.0}));
    EXPECT_EQ(*target.undesired_direction_trace_cost, std::vector<double>{3.0});
}

TEST(RoutingCostSettingsTest, CopyIsDeepClone)
{
    RoutingCostSettings original;
    original.preferred_direction_trace_cost = std::vector<double>{1.0};
    RoutingCostSettings copy = original;
    (*copy.preferred_direction_trace_cost)[0] = 5.0;
    EXPECT_EQ((*original.preferred_direction_trace_cost)[0], 1.0);
}

TEST(RoutingCostSettingsTest, FieldTableAlternates)
{
    RoutingCostSettings settings;
    set_field_value(settings, "viaCosts", "40");
    set_field_value(settings, "start_ripup_costs", "+90");
    set_field_value(settings, "preferred_direction_trace_cost", "1.0, 2.5");
    EXPECT_EQ(settings.via_costs, 40);
    EXPECT_EQ(settings.start_ripup_costs, 90);
    EXPECT_EQ(*settings.preferred_direction_trace_cost, (std::vector<double>{1.0, 2.5}));
    EXPECT_EQ(find_settings_field<RoutingCostSettings>("preferred_direction_trace_cost")->get(settings), "1.0,2.5");
    EXPECT_TRUE(find_settings_field<RoutingCostSettings>("preferred_direction_trace_cost")->is_transient);
}

} // namespace freerouting::settings
