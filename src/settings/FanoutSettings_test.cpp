#include "settings/FanoutSettings.h"

#include <gtest/gtest.h>

namespace freerouting::settings {

TEST(FanoutSettingsTest, CopyFieldsCopiesOnlyNonNullValues)
{
    FanoutSettings source;
    source.max_milliseconds_per_pin = 2500LL;
    source.pin_sorting_order = "inner_first";
    FanoutSettings target;
    target.enabled = true;
    target.max_passes = 20;
    EXPECT_EQ(copy_fields(source, target), 2);
    EXPECT_EQ(target.enabled, true);
    EXPECT_EQ(target.max_passes, 20);
    EXPECT_EQ(target.max_milliseconds_per_pin, 2500LL);
    EXPECT_EQ(target.pin_sorting_order, "inner_first");
}

TEST(FanoutSettingsTest, EqualValueIsNotCounted)
{
    FanoutSettings source;
    source.enabled = true;
    FanoutSettings target;
    target.enabled = true;
    EXPECT_EQ(copy_fields(source, target), 0);
}

TEST(FanoutSettingsTest, FieldTableNamesAndAlternates)
{
    FanoutSettings settings;
    set_field_value(settings, "ripup_allowed", "0");
    EXPECT_EQ(settings.ripup_allowed, false);
    set_field_value(settings, "ripupAllowed", "1");
    EXPECT_EQ(settings.ripup_allowed, true);
    // The Java field timeoutString is serialized as "timeout".
    set_field_value(settings, "timeout", "5m");
    EXPECT_EQ(settings.timeout_string, "5m");
    set_field_value(settings, "max_escape_length_mm", "4.5");
    EXPECT_DOUBLE_EQ(*settings.max_escape_length_mm, 4.5);
    set_field_value(settings, "max_milliseconds_per_pin", "10000");
    EXPECT_EQ(settings.max_milliseconds_per_pin, 10000LL);
    EXPECT_THROW(set_field_value(settings, "max_passes", "abc"), std::invalid_argument);
    EXPECT_THROW(set_field_value(settings, "no_such_field", "1"), std::invalid_argument);
}

} // namespace freerouting::settings
