#include "settings/LayerSettings.h"

#include <gtest/gtest.h>

namespace freerouting::settings {

TEST(LayerSettingsTest, ConstructorsAndEquality)
{
    LayerSettings empty;
    EXPECT_FALSE(empty.routable.has_value());
    EXPECT_FALSE(empty.preferred_direction_horizontal.has_value());
    EXPECT_FALSE(empty.bend_cost.has_value());

    LayerSettings two(true, false);
    EXPECT_EQ(two.routable, true);
    EXPECT_EQ(two.preferred_direction_horizontal, false);
    EXPECT_FALSE(two.bend_cost.has_value());

    LayerSettings three(true, false, 1.5);
    EXPECT_NE(two, three);
    three.bend_cost.reset();
    EXPECT_EQ(two, three);
}

TEST(LayerSettingsTest, CopyFields)
{
    LayerSettings source(std::nullopt, true, 2.0);
    LayerSettings target(false, false);
    EXPECT_EQ(copy_fields(source, target), 2);
    EXPECT_EQ(target.routable, false);
    EXPECT_EQ(target.preferred_direction_horizontal, true);
    EXPECT_EQ(target.bend_cost, 2.0);
}

TEST(LayerSettingsTest, FieldTable)
{
    LayerSettings settings;
    set_field_value(settings, "preferred_direction_horizontal", "TRUE");
    set_field_value(settings, "bendCost", "0.5");
    EXPECT_EQ(settings.preferred_direction_horizontal, true);
    EXPECT_EQ(settings.bend_cost, 0.5);
    EXPECT_EQ(find_settings_field<LayerSettings>("bend_cost")->get(settings), "0.5");
}

} // namespace freerouting::settings
