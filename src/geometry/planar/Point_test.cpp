#include <gtest/gtest.h>

#include <cmath>
#include <optional>
#include <vector>

#include "geometry/planar/Point.h"

namespace freerouting {
namespace geometry {
namespace planar {
namespace {

constexpr double kTolerance = 1e-6;

TEST(PointTangentialTest, TangentialPointsAreOnTheCircleAroundTheTarget)
{
    const FloatPoint pole(0, 0);
    const FloatPoint circle_center(200, 0);
    const double radius = 50;

    std::vector<FloatPoint> points = pole.tangential_points(circle_center, radius);

    ASSERT_EQ(points.size(), 2u);
    for (const FloatPoint& point : points)
    {
        EXPECT_NEAR(radius, point.distance(circle_center), kTolerance);
    }
}

TEST(PointTangentialTest, TangentialPointsLieOnTheCircleAndOnTheTangentLine)
{
    const FloatPoint pole(0, 0);
    const FloatPoint circle_center(200, 50);
    const double radius = 30;

    std::vector<FloatPoint> points = pole.tangential_points(circle_center, radius);

    ASSERT_EQ(points.size(), 2u);
    for (const FloatPoint& point : points)
    {
        // The radius to a tangential point is perpendicular to the tangent line.
        const double radius_x = point.x - circle_center.x;
        const double radius_y = point.y - circle_center.y;
        const double line_x = point.x - pole.x;
        const double line_y = point.y - pole.y;
        EXPECT_NEAR(0.0, radius_x * line_x + radius_y * line_y, kTolerance);
    }
}

TEST(PointTangentialTest, TangentialPointsAreEmptyForAnInsidePole)
{
    const FloatPoint pole(0, 0);
    const FloatPoint circle_center(10, 0);

    // The pole lies inside the circle, so no tangent line exists.
    EXPECT_TRUE(pole.tangential_points(circle_center, 50).empty());
    EXPECT_FALSE(pole.right_tangential_point(circle_center, 50).has_value());
    EXPECT_FALSE(pole.left_tangential_point(circle_center, 50).has_value());
}

TEST(PointTangentialTest, LeftAndRightTangentialPointsAreOnDifferentSides)
{
    const FloatPoint pole(0, 0);
    const FloatPoint circle_center(200, 0);
    const double radius = 50;

    std::optional<FloatPoint> left = pole.left_tangential_point(circle_center, radius);
    std::optional<FloatPoint> right = pole.right_tangential_point(circle_center, radius);

    ASSERT_TRUE(left.has_value());
    ASSERT_TRUE(right.has_value());
    // The line from the pole to the circle center runs along the x axis, so the
    // left tangential point is above and the right tangential point below it.
    EXPECT_GT(left->y, 0.0);
    EXPECT_LT(right->y, 0.0);
    EXPECT_NEAR(left->x, right->x, kTolerance);
}

TEST(PointTangentialTest, TangentialPointsWorkForAVerticallyDominatingDifference)
{
    // dy > dx, so the implementation turns the situation by 90 degrees.
    const FloatPoint pole(0, 0);
    const FloatPoint circle_center(50, 200);
    const double radius = 40;

    std::vector<FloatPoint> points = pole.tangential_points(circle_center, radius);

    ASSERT_EQ(points.size(), 2u);
    for (const FloatPoint& point : points)
    {
        EXPECT_NEAR(radius, point.distance(circle_center), kTolerance);
    }
    std::optional<FloatPoint> left = pole.left_tangential_point(circle_center, radius);
    std::optional<FloatPoint> right = pole.right_tangential_point(circle_center, radius);
    ASSERT_TRUE(left.has_value());
    ASSERT_TRUE(right.has_value());
    EXPECT_LT(left->x, right->x);
}

TEST(PointTangentialTest, TangentialPointSidesAreMirrored)
{
    // The same situation seen from the other side must not exchange the sides.
    const FloatPoint pole(200, 0);
    const FloatPoint circle_center(0, 0);
    const double radius = 50;

    std::optional<FloatPoint> left = pole.left_tangential_point(circle_center, radius);
    std::optional<FloatPoint> right = pole.right_tangential_point(circle_center, radius);

    ASSERT_TRUE(left.has_value());
    ASSERT_TRUE(right.has_value());
    EXPECT_LT(left->y, 0.0);
    EXPECT_GT(right->y, 0.0);
}

} // namespace
} // namespace planar
} // namespace geometry
} // namespace freerouting