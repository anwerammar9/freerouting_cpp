#include "settings/SettingsFieldTable.h"

#include <gtest/gtest.h>

namespace freerouting::settings {

TEST(SettingsFieldTableTest, SnakeToLowerCamel)
{
    EXPECT_EQ(snake_to_lower_camel("max_passes"), "maxPasses");
    EXPECT_EQ(snake_to_lower_camel("MAX_PASSES"), "maxPasses");
    EXPECT_EQ(snake_to_lower_camel("maxPasses"), "maxPasses");
    EXPECT_EQ(snake_to_lower_camel("max_consecutive_failures_pass1"), "maxConsecutiveFailuresPass1");
}

TEST(SettingsFieldTableTest, SplitPropertyPath)
{
    EXPECT_EQ(split_property_path("router.optimizer.max_passes"),
              (std::vector<std::string>{"router", "optimizer", "max_passes"}));
    EXPECT_EQ(split_property_path("optimizer:max-passes"),
              (std::vector<std::string>{"optimizer", "max", "passes"}));
}

TEST(SettingsFieldTableTest, JavaNumberParsing)
{
    EXPECT_EQ(parse_java_integer<int>("42"), 42);
    EXPECT_EQ(parse_java_integer<int>("-7"), -7);
    EXPECT_EQ(parse_java_integer<int>("+7"), 7);
    EXPECT_THROW(parse_java_integer<int>(" 7"), std::invalid_argument);
    EXPECT_THROW(parse_java_integer<int>("7.0"), std::invalid_argument);
    EXPECT_THROW(parse_java_integer<int>(""), std::invalid_argument);

    EXPECT_DOUBLE_EQ(parse_java_floating<double>(" 2.5 "), 2.5);
    EXPECT_DOUBLE_EQ(parse_java_floating<double>("1e3"), 1000.0);
    EXPECT_DOUBLE_EQ(parse_java_floating<double>("3d"), 3.0);
    EXPECT_THROW(parse_java_floating<double>("abc"), std::invalid_argument);

    EXPECT_TRUE(parse_java_boolean("1"));
    EXPECT_TRUE(parse_java_boolean("TRUE"));
    EXPECT_FALSE(parse_java_boolean("0"));
    EXPECT_FALSE(parse_java_boolean("yes")); // Boolean.parseBoolean
}

TEST(SettingsFieldTableTest, FloatingFormatting)
{
    EXPECT_EQ(format_java_floating(1.0), "1.0");
    EXPECT_EQ(format_java_floating(2.5), "2.5");
    EXPECT_EQ(format_java_floating(0.6F), "0.6");
}

TEST(SettingsFieldTableTest, StringArrayParsingDropsTrailingEmptyTokens)
{
    using Converter = SettingsValueConverter<std::vector<std::string>>;
    EXPECT_TRUE(Converter::parse("  ").empty());
    EXPECT_EQ(Converter::parse("a, ,b,,"), (std::vector<std::string>{"a", "", "b"}));
}

} // namespace freerouting::settings
