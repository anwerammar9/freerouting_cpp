/**
 * @file IntOctagon_test.cpp
 * @brief Unit tests for IntOctagon::nearest_border_projections
 */

#include "geometry/planar/IntOctagon.h"

#include <gtest/gtest.h>

#include <cmath>

namespace freerouting::geometry::planar {

TEST(IntOctagonTest, NearestBorderProjectionsSquare)
{
    // square box (0,0)-(100,100) expressed as octagon
    const IntOctagon oct(0, 0, 100, 100, -100, 100, 0, 200);
    const std::vector<IntPoint> projections =
        oct.nearest_border_projections(IntPoint(50, 50), 2);
    ASSERT_EQ(projections.size(), 2u);
    const FloatPoint center(50.0, 50.0);
    EXPECT_NEAR(center.distance_square(projections[0].to_float()), 2500.0, 1e-6);
    EXPECT_NEAR(center.distance_square(projections[1].to_float()), 2500.0, 1e-6);
}

TEST(IntOctagonTest, NearestBorderProjectionsCorner)
{
    const IntOctagon oct(0, 0, 100, 100, -100, 100, 0, 200);
    const std::vector<IntPoint> projections =
        oct.nearest_border_projections(IntPoint(5, 5), 2);
    ASSERT_EQ(projections.size(), 2u);
    const bool p0_is_left = (projections[0] == IntPoint(0, 5));
    const bool p0_is_down = (projections[0] == IntPoint(5, 0));
    const bool p1_is_down = (projections[1] == IntPoint(5, 0));
    const bool p1_is_left = (projections[1] == IntPoint(0, 5));
    EXPECT_TRUE((p0_is_left && p1_is_down) || (p0_is_down && p1_is_left));
}

TEST(IntOctagonTest, NearestBorderProjectionsChamferedOctagon)
{
    // octagon chamfered along the 45-degree borders so the diagonal
    // directions are strictly nearer than the flat sides
    const IntOctagon oct(0, 0, 100, 100, -30, 30, 70, 130);
    const std::vector<IntPoint> projections =
        oct.nearest_border_projections(IntPoint(50, 50), 2);
    ASSERT_EQ(projections.size(), 2u);
    EXPECT_EQ(projections[0], IntPoint(65, 65));
    EXPECT_EQ(projections[1], IntPoint(35, 65));
}

TEST(IntOctagonTest, NearestBorderProjectionsOutside)
{
    const IntOctagon oct(0, 0, 100, 100, -100, 100, 0, 100);
    EXPECT_TRUE(oct.nearest_border_projections(IntPoint(500, 500), 2).empty());
}

TEST(IntOctagonTest, NearestBorderProjectionsMaxPointsZeroOrNegative)
{
    const IntOctagon oct(0, 0, 100, 100, -100, 100, 0, 100);
    EXPECT_TRUE(oct.nearest_border_projections(IntPoint(50, 50), 0).empty());
    EXPECT_TRUE(oct.nearest_border_projections(IntPoint(50, 50), -1).empty());
}

} // namespace freerouting::geometry::planar