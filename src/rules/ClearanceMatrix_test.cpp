// Tests for the port of Java rules.ClearanceMatrix.

#include "rules/ClearanceMatrix.h"

#include <gtest/gtest.h>

#include <memory>

namespace freerouting::rules {
namespace {

board::LayerStructure layers(int p_count)
{
    std::vector<std::shared_ptr<board::Layer>> result;
    for (int i = 0; i < p_count; ++i)
    {
        result.push_back(std::make_shared<board::Layer>("L" + std::to_string(i), true));
    }
    return board::LayerStructure(result);
}

TEST(ClearanceMatrixTest, DefaultInstanceLeavesTheNullClassAtZero)
{
    const ClearanceMatrix matrix = ClearanceMatrix::get_default_instance(layers(2), 200);
    EXPECT_EQ(2, matrix.get_class_count());
    EXPECT_EQ(200, matrix.get_value(1, 1, 0));
    EXPECT_EQ(0, matrix.get_value(0, 1, 0));
    EXPECT_EQ(216, matrix.get_value(1, 1, 1, true));
    EXPECT_EQ(1, matrix.get_no("DEFAULT"));
    EXPECT_EQ(0, matrix.get_value(1, 5, 0));
}

TEST(ClearanceMatrixTest, ValuesAreMadeEvenAndAreNotSymmetricByThemselves)
{
    ClearanceMatrix matrix = ClearanceMatrix::get_default_instance(layers(2), 0);
    ASSERT_TRUE(matrix.append_class("wide"));
    matrix.set_value(2, 1, 0, 301);
    EXPECT_EQ(302, matrix.get_value(2, 1, 0));
    EXPECT_EQ(0, matrix.get_value(1, 2, 0)); // Java stores row[j].column[i]
    EXPECT_EQ(0, matrix.get_value(2, 1, 1)); // per layer
    EXPECT_TRUE(matrix.is_layer_dependent(2, 1));
    EXPECT_EQ(302, matrix.get_max_value_on_layer(0));
    EXPECT_EQ(302, matrix.max_value(1, 0));
}

TEST(ClearanceMatrixTest, CompensationIsHalfTheOwnClearanceRoundedUp)
{
    ClearanceMatrix matrix = ClearanceMatrix::get_default_instance(layers(1), 250);
    // 250 is even; the compensation is (250 + 1) / 2 = 125
    EXPECT_EQ(125, matrix.get_clearance_compensation_value(1, 0));
}

TEST(ClearanceMatrixTest, AppendedClassCopiesTheDefaultValues)
{
    ClearanceMatrix matrix = ClearanceMatrix::get_default_instance(layers(2), 100);
    EXPECT_TRUE(matrix.append_class("smd"));
    EXPECT_FALSE(matrix.append_class("SMD"));
    EXPECT_EQ(3, matrix.get_class_count());
    EXPECT_EQ(100, matrix.get_value(2, 2, 1));
    EXPECT_EQ(100, matrix.get_value(1, 2, 0));
    EXPECT_EQ(100, matrix.get_value(2, 1, 0));
    EXPECT_TRUE(matrix.is_equal(1, 2));
}

TEST(ClearanceMatrixTest, RemoveClassShiftsTheFollowingClasses)
{
    ClearanceMatrix matrix = ClearanceMatrix::get_default_instance(layers(1), 100);
    matrix.append_class("a");
    matrix.append_class("b");
    matrix.set_value(3, 3, 0, 500);
    matrix.remove_class(2);
    EXPECT_EQ(3, matrix.get_class_count());
    EXPECT_EQ("b", matrix.get_name(2));
    EXPECT_EQ(500, matrix.get_value(2, 2, 0));
}

TEST(ClearanceMatrixTest, InnerValuesSkipTheOuterLayers)
{
    ClearanceMatrix matrix = ClearanceMatrix::get_default_instance(layers(4), 100);
    matrix.set_inner_value(1, 1, 300);
    EXPECT_EQ(100, matrix.get_value(1, 1, 0));
    EXPECT_EQ(300, matrix.get_value(1, 1, 1));
    EXPECT_EQ(300, matrix.get_value(1, 1, 2));
    EXPECT_EQ(100, matrix.get_value(1, 1, 3));
    EXPECT_FALSE(matrix.is_inner_layer_dependent(1, 1));
}

} // namespace
} // namespace freerouting::rules
