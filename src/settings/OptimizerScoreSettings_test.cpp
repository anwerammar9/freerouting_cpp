#include "settings/OptimizerScoreSettings.h"

#include <gtest/gtest.h>

namespace freerouting::settings {

TEST(OptimizerScoreSettingsTest, EnumIsAlwaysCountedWhenSet)
{
    OptimizerScoreSettings source;
    source.version = OptimizerScoringVersion::V2_LOWER_BOUND;
    OptimizerScoreSettings target;
    target.version = OptimizerScoringVersion::V2_LOWER_BOUND;
    // Java copyFields copies and counts every non-null enum, even an equal one.
    EXPECT_EQ(copy_fields(source, target), 1);
    EXPECT_EQ(target.version, OptimizerScoringVersion::V2_LOWER_BOUND);
}

TEST(OptimizerScoreSettingsTest, CopyFieldsWeights)
{
    OptimizerScoreSettings source;
    source.excess_via_weight = 2000.0F;
    source.length_floor = 1.0F;
    OptimizerScoreSettings target;
    EXPECT_EQ(copy_fields(source, target), 2);
    EXPECT_EQ(target.excess_via_weight, 2000.0F);
    EXPECT_EQ(target.length_floor, 1.0F);
    EXPECT_FALSE(target.version.has_value());
}

TEST(OptimizerScoreSettingsTest, FieldTableParsesEnumCaseInsensitive)
{
    OptimizerScoreSettings settings;
    set_field_value(settings, "version", " v1_legacy ");
    EXPECT_EQ(settings.version, OptimizerScoringVersion::V1_LEGACY);
    EXPECT_EQ(find_settings_field<OptimizerScoreSettings>("version")->get(settings), "V1_LEGACY");
    EXPECT_THROW(set_field_value(settings, "version", "V3"), std::invalid_argument);
    set_field_value(settings, "excess_bend_weight", "500");
    EXPECT_EQ(settings.excess_bend_weight, 500.0F);
}

} // namespace freerouting::settings
