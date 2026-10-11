// Tests for the Java-parity number formatting helpers. The expected strings are
// the output of Double.toString / Float.toString on JDK 25.

#include "datastructures/JavaFormat.h"

#include <gtest/gtest.h>

#include <limits>

namespace freerouting::datastructures {
namespace {

TEST(JavaFormatTest, DoublePlainNotation)
{
    EXPECT_EQ("0.0", java_double_to_string(0.0));
    EXPECT_EQ("-0.0", java_double_to_string(-0.0));
    EXPECT_EQ("1.0", java_double_to_string(1.0));
    EXPECT_EQ("2.0", java_double_to_string(2.0));
    EXPECT_EQ("0.1", java_double_to_string(0.1));
    EXPECT_EQ("0.001", java_double_to_string(0.001));
    EXPECT_EQ("338.5", java_double_to_string(338.5));
    EXPECT_EQ("-2.5", java_double_to_string(-2.5));
    EXPECT_EQ("0.3333333333333333", java_double_to_string(1.0 / 3.0));
    EXPECT_EQ("9999999.0", java_double_to_string(9999999.0));
}

TEST(JavaFormatTest, DoubleScientificNotation)
{
    EXPECT_EQ("1.0E7", java_double_to_string(1.0e7));
    EXPECT_EQ("1.0E-4", java_double_to_string(1.0e-4));
    EXPECT_EQ("1.23456789E8", java_double_to_string(123456789.0));
    EXPECT_EQ("1.2345678E7", java_double_to_string(12345678.0));
    EXPECT_EQ("1.5E-7", java_double_to_string(1.5e-7));
    EXPECT_EQ("1.0E10", java_double_to_string(1.0e10));
    EXPECT_EQ("1.0E20", java_double_to_string(1.0e20));
    EXPECT_EQ("1.0E-300", java_double_to_string(1.0e-300));
}

TEST(JavaFormatTest, DoubleSpecialValues)
{
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();
    EXPECT_EQ("NaN", java_double_to_string(nan));
    EXPECT_EQ("Infinity", java_double_to_string(inf));
    EXPECT_EQ("-Infinity", java_double_to_string(-inf));
}

TEST(JavaFormatTest, FloatPlainNotation)
{
    EXPECT_EQ("0.0", java_float_to_string(0.0f));
    EXPECT_EQ("-0.0", java_float_to_string(-0.0f));
    EXPECT_EQ("1.0", java_float_to_string(1.0f));
    EXPECT_EQ("0.1", java_float_to_string(0.1f));
    EXPECT_EQ("0.33333334", java_float_to_string(1.0f / 3.0f));
    EXPECT_EQ("9999999.0", java_float_to_string(9999999.0f));
}

TEST(JavaFormatTest, FloatScientificNotation)
{
    EXPECT_EQ("1.0E7", java_float_to_string(1.0e7f));
    EXPECT_EQ("1.0E-4", java_float_to_string(1.0e-4f));
    EXPECT_EQ("1.2345679E8", java_float_to_string(123456789.0f));
    EXPECT_EQ("1.0E10", java_float_to_string(1.0e10f));
}

} // namespace
} // namespace freerouting::datastructures
