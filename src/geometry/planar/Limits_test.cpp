/**
 * @file Limits_test.cpp
 * @brief Unit tests for Limits utility class
 */

#include "Limits.h"
#include <gtest/gtest.h>
#include <string>
#include <limits>

namespace freerouting {
namespace geometry {
namespace planar {

class LimitsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Test setup if needed
    }
};

// Test constant values
TEST_F(LimitsTest, ConstantValues) {
    // Verify CRIT_INT is 2^25
    EXPECT_EQ(Limits::CRIT_INT, 33554432);
    EXPECT_EQ(Limits::CRIT_INT, 1 << 25);
    
    // Verify CRIT_DOUBLE is 2^53
    EXPECT_DOUBLE_EQ(Limits::CRIT_DOUBLE, 9007199254740992.0);
    
    // Verify SQRT2
    EXPECT_DOUBLE_EQ(Limits::SQRT2, std::sqrt(2.0));
    EXPECT_GT(Limits::SQRT2, 1.414);
    EXPECT_LT(Limits::SQRT2, 1.415);
}

// Test critical integer bounds checking
TEST_F(LimitsTest, CritIntBoundsChecking) {
    // Test values within bounds
    EXPECT_TRUE(Limits::is_within_crit_int(0));
    EXPECT_TRUE(Limits::is_within_crit_int(1000));
    EXPECT_TRUE(Limits::is_within_crit_int(Limits::CRIT_INT - 1));
    EXPECT_TRUE(Limits::is_within_crit_int(-Limits::CRIT_INT + 1));
    
    // Test values at bounds
    EXPECT_FALSE(Limits::is_within_crit_int(Limits::CRIT_INT));
    EXPECT_FALSE(Limits::is_within_crit_int(-Limits::CRIT_INT));
    
    // Test values outside bounds
    EXPECT_FALSE(Limits::is_within_crit_int(Limits::CRIT_INT + 1));
    EXPECT_FALSE(Limits::is_within_crit_int(-Limits::CRIT_INT - 1));
    EXPECT_FALSE(Limits::is_within_crit_int(std::numeric_limits<int>::max()));
    EXPECT_FALSE(Limits::is_within_crit_int(std::numeric_limits<int>::min()));
}

// Test critical double bounds checking
TEST_F(LimitsTest, CritDoubleBoundsChecking) {
    // Test values within bounds
    EXPECT_TRUE(Limits::is_within_crit_double(0.0));
    EXPECT_TRUE(Limits::is_within_crit_double(1000.0));
    EXPECT_TRUE(Limits::is_within_crit_double(Limits::CRIT_DOUBLE - 1.0));
    EXPECT_TRUE(Limits::is_within_crit_double(-Limits::CRIT_DOUBLE + 1.0));
    
    // Test values at bounds
    EXPECT_FALSE(Limits::is_within_crit_double(Limits::CRIT_DOUBLE));
    EXPECT_FALSE(Limits::is_within_crit_double(-Limits::CRIT_DOUBLE));
    
    // Test values outside bounds
    EXPECT_FALSE(Limits::is_within_crit_double(Limits::CRIT_DOUBLE + 1.0));
    EXPECT_FALSE(Limits::is_within_crit_double(-Limits::CRIT_DOUBLE - 1.0));
    EXPECT_FALSE(Limits::is_within_crit_double(std::numeric_limits<double>::max()));
    EXPECT_FALSE(Limits::is_within_crit_double(-std::numeric_limits<double>::max()));
}

// Test description functions
TEST_F(LimitsTest, Descriptions) {
    std::string int_desc = Limits::get_crit_int_description();
    std::string double_desc = Limits::get_crit_double_description();
    
    // Should not be empty
    EXPECT_FALSE(int_desc.empty());
    EXPECT_FALSE(double_desc.empty());
    
    // Should contain key information
    EXPECT_NE(int_desc.find("2^25"), std::string::npos);
    EXPECT_NE(int_desc.find("33554432"), std::string::npos);
    EXPECT_NE(int_desc.find("double"), std::string::npos);
    
    EXPECT_NE(double_desc.find("2^53"), std::string::npos);
    EXPECT_NE(double_desc.find("9007199254740992"), std::string::npos);
    EXPECT_NE(double_desc.find("integer"), std::string::npos);
}

// Test mathematical properties
TEST_F(LimitsTest, MathematicalProperties) {
    // Test that CRIT_INT * CRIT_INT < CRIT_DOUBLE
    // This is important for the purpose described in comments
    double product = static_cast<double>(Limits::CRIT_INT) * static_cast<double>(Limits::CRIT_INT);
    EXPECT_LT(product, Limits::CRIT_DOUBLE);
    
    // Test that SQRT2 has expected properties
    EXPECT_GT(Limits::SQRT2 * Limits::SQRT2, 1.999);
    EXPECT_LT(Limits::SQRT2 * Limits::SQRT2, 2.001);
}

// Test edge cases
TEST_F(LimitsTest, EdgeCases) {
    // Test boundary conditions
    EXPECT_TRUE(Limits::is_within_crit_int(Limits::CRIT_INT - 1));
    EXPECT_FALSE(Limits::is_within_crit_int(Limits::CRIT_INT));
    
    EXPECT_TRUE(Limits::is_within_crit_double(Limits::CRIT_DOUBLE - 1.0));
    EXPECT_FALSE(Limits::is_within_crit_double(Limits::CRIT_DOUBLE));
    
    // Test negative values
    EXPECT_TRUE(Limits::is_within_crit_int(-1));
    EXPECT_TRUE(Limits::is_within_crit_int(-1000));
    EXPECT_TRUE(Limits::is_within_crit_int(-Limits::CRIT_INT + 1));
    EXPECT_FALSE(Limits::is_within_crit_int(-Limits::CRIT_INT));
    
    EXPECT_TRUE(Limits::is_within_crit_double(-1.0));
    EXPECT_TRUE(Limits::is_within_crit_double(-1000.0));
    EXPECT_TRUE(Limits::is_within_crit_double(-Limits::CRIT_DOUBLE + 1.0));
    EXPECT_FALSE(Limits::is_within_crit_double(-Limits::CRIT_DOUBLE));
}

// Test compilation-time evaluation
TEST_F(LimitsTest, CompileTimeEvaluation) {
    // These should be evaluated at compile-time
    constexpr int test_int = Limits::CRIT_INT;
    constexpr double test_double = Limits::CRIT_DOUBLE;
    constexpr double test_sqrt2 = Limits::SQRT2;
    
    constexpr bool within_int = Limits::is_within_crit_int(1000);
    constexpr bool within_double = Limits::is_within_crit_double(1000.0);
    
    EXPECT_EQ(test_int, 33554432);
    EXPECT_DOUBLE_EQ(test_double, 9007199254740992.0);
    EXPECT_DOUBLE_EQ(test_sqrt2, std::sqrt(2.0));
    EXPECT_TRUE(within_int);
    EXPECT_TRUE(within_double);
}

// Test practical usage scenarios
TEST_F(LimitsTest, PracticalUsage) {
    // Test scenario: checking if coordinate multiplication is safe
    int coord1 = 10000;
    int coord2 = 20000;
    
    // Check if individual coordinates are within bounds
    EXPECT_TRUE(Limits::is_within_crit_int(coord1));
    EXPECT_TRUE(Limits::is_within_crit_int(coord2));
    
    // Check if product would be within double precision
    double product = static_cast<double>(coord1) * static_cast<double>(coord2);
    EXPECT_TRUE(Limits::is_within_crit_double(product));
    
    // Test with values that would exceed bounds
    int large_coord = Limits::CRIT_INT;
    EXPECT_FALSE(Limits::is_within_crit_int(large_coord));
}

} // namespace planar
} // namespace geometry
} // namespace freerouting
