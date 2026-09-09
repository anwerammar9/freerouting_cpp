/**
 * @file IntBox_test.cpp
 * @brief Unit tests for IntBox::nearest_point, nearest_border_projections,
 * distance and intersects(Circle)
 */

#include "geometry/planar/IntBox.h"

#include <gtest/gtest.h>

#include <vector>

#include "geometry/planar/Shape.h"

namespace freerouting::geometry::planar {

namespace {
constexpr double kEps = 1e-9;
}

TEST(IntBoxTest, NearestPointClampsToBox)
{
    const IntBox box(IntPoint(0, 0), IntPoint(100, 100));
    const FloatPoint inside = box.nearest_point(FloatPoint(40.0, 50.0));
    EXPECT_NEAR(inside.x, 40.0, kEps);
    EXPECT_NEAR(inside.y, 50.0, kEps);

    const FloatPoint beyond = box.nearest_point(FloatPoint(200.0, -50.0));
    EXPECT_NEAR(beyond.x, 100.0, kEps);
    EXPECT_NEAR(beyond.y, 0.0, kEps);

    const FloatPoint x_beyond = box.nearest_point(FloatPoint(-10.0, 30.0));
    EXPECT_NEAR(x_beyond.x, 0.0, kEps);
    EXPECT_NEAR(x_beyond.y, 30.0, kEps);
}

TEST(IntBoxTest, Distance)
{
    const IntBox box(IntPoint(0, 0), IntPoint(100, 100));
    EXPECT_NEAR(box.distance(FloatPoint(200.0, 0.0)), 100.0, kEps);
    EXPECT_NEAR(box.distance(FloatPoint(20.0, 30.0)), 0.0, kEps);
    EXPECT_NEAR(box.distance(FloatPoint(100.0, 150.0)), 50.0, kEps);
}

TEST(IntBoxTest, NearestBorderProjections)
{
    const IntBox box(IntPoint(0, 0), IntPoint(100, 100));
    const std::vector<IntPoint> projections =
        box.nearest_border_projections(IntPoint(40, 30), 2);
    ASSERT_EQ(projections.size(), 2u);
    EXPECT_EQ(projections[0], IntPoint(40, 0));
    EXPECT_EQ(projections[1], IntPoint(0, 30));

    const std::vector<IntPoint> center =
        box.nearest_border_projections(IntPoint(50, 50), 2);
    ASSERT_EQ(center.size(), 2u);
    EXPECT_EQ(center[0], IntPoint(0, 50));
    EXPECT_EQ(center[1], IntPoint(100, 50));
}

TEST(IntBoxTest, NearestBorderProjectionsMaxPointsZeroOrNegative)
{
    const IntBox box(IntPoint(0, 0), IntPoint(100, 100));
    EXPECT_TRUE(box.nearest_border_projections(IntPoint(50, 50), 0).empty());
    EXPECT_TRUE(box.nearest_border_projections(IntPoint(50, 50), -3).empty());
}

TEST(IntBoxTest, IntersectsCircle)
{
    const IntBox box(IntPoint(0, 0), IntPoint(100, 100));
    const Circle inside(IntPoint(50, 50), 10);
    EXPECT_TRUE(box.intersects(inside));

    const Circle far(IntPoint(200, 50), 10);
    EXPECT_FALSE(box.intersects(far));

    const Circle touching(IntPoint(110, 50), 10);
    EXPECT_TRUE(box.intersects(touching));
}

} // namespace freerouting::geometry::planar