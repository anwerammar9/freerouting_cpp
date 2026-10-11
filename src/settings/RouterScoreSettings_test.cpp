#include "settings/RouterScoreSettings.h"

#include <gtest/gtest.h>

namespace freerouting::settings {

TEST(RouterScoreSettingsTest, CopyFields)
{
    RouterScoreSettings source;
    source.version = RouterScoringVersion::V1_LEGACY;
    source.clearance_violation_depth_scale = 1000.0F;
    RouterScoreSettings target;
    target.version = RouterScoringVersion::V2_CONTINUOUS;
    target.unrouted_connection_weight = 1000.0F;
    EXPECT_EQ(copy_fields(source, target), 2);
    EXPECT_EQ(target.version, RouterScoringVersion::V1_LEGACY);
    EXPECT_EQ(target.unrouted_connection_weight, 1000.0F);
    EXPECT_EQ(target.clearance_violation_depth_scale, 1000.0F);
}

TEST(RouterScoreSettingsTest, FieldTable)
{
    RouterScoreSettings settings;
    set_field_value(settings, "version", "V2_CONTINUOUS");
    set_field_value(settings, "unroutedFreeFraction", "0.5");
    EXPECT_EQ(settings.version, RouterScoringVersion::V2_CONTINUOUS);
    EXPECT_EQ(settings.unrouted_free_fraction, 0.5F);
    EXPECT_EQ(settings_fields<RouterScoreSettings>().size(), 8U);
}

} // namespace freerouting::settings
