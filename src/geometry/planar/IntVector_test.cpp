/**
 * @file IntVector_test.cpp
 * @brief Unit tests for Vector::get_instance and IntVector::change_length_approx
 */

#include "geometry/planar/Vector.h"

#include <gtest/gtest.h>

namespace freerouting::geometry::planar {

TEST(IntVectorTest, GetInstanceCreatesIntVector)
{
    auto vec = Vector::get_instance(3, -4);
    ASSERT_NE(vec, nullptr);
    const auto* int_vec = dynamic_cast<const IntVector*>(vec.get());
    ASSERT_NE(int_vec, nullptr);
    EXPECT_EQ(int_vec->x, 3);
    EXPECT_EQ(int_vec->y, -4);
}

TEST(IntVectorTest, ChangeLengthApprox)
{
    const IntVector v(3, 4);
    const IntVector scaled = v.change_length_approx(10.0);
    EXPECT_EQ(scaled.x, 6);
    EXPECT_EQ(scaled.y, 8);
}

TEST(IntVectorTest, ChangeLengthApproxZeroVector)
{
    const IntVector scaled = IntVector::ZERO.change_length_approx(10.0);
    EXPECT_EQ(scaled.x, 0);
    EXPECT_EQ(scaled.y, 0);
}

TEST(IntVectorTest, ChangeLengthApproxRounds)
{
    const IntVector v(1, 0);
    EXPECT_EQ(v.change_length_approx(0.4).x, 0);
    EXPECT_EQ(v.change_length_approx(0.6).x, 1);
}

} // namespace freerouting::geometry::planar