#include "geometry/planar/LineSegment.h"

#include <gtest/gtest.h>

namespace freerouting::geometry::planar {
namespace {

// The diagonal x == y, cut by the anti-diagonal x + y == 1 at the rational point (0.5, 0.5).
const Line kDiagonal(IntPoint(0, 0), IntPoint(10, 10));
const Line kAntiDiagonal(IntPoint(0, 1), IntPoint(1, 0));
const Line kVerticalAtMinus10(IntPoint(-10, 0), IntPoint(-10, 10));
const Line kVerticalAt1(IntPoint(1, 0), IntPoint(1, 10));

TEST(LineSegmentTest, RationalEndPointIsKeptExact)
{
    const LineSegment segment(kVerticalAtMinus10, kDiagonal, kAntiDiagonal);
    const Point& end = segment.end_point_exact();
    // Java returns a RationalPoint; the C++ Line::intersection returns an exact FloatPoint here.
    EXPECT_EQ(nullptr, dynamic_cast<const IntPoint*>(&end));
    EXPECT_DOUBLE_EQ(0.5, end.to_float().x);
    EXPECT_DOUBLE_EQ(0.5, end.to_float().y);
    EXPECT_DOUBLE_EQ(0.5, segment.end_point_approx().x);
    // The rounded accessor uses Java Math.round (floor(x + 0.5)).
    EXPECT_EQ(IntPoint(1, 1), segment.end_point());
    EXPECT_EQ(IntPoint(-10, -10), segment.start_point());
}

TEST(LineSegmentTest, CollinearSegmentsTouchingAtARationalPointIntersectInOnePoint)
{
    // A ends and B starts at (0.5, 0.5); B ends at (1, 1), which is where A's end point would be
    // if it were rounded. Java compares the exact points and reports a single touching point.
    const LineSegment a(kVerticalAtMinus10, kDiagonal, kAntiDiagonal);
    const LineSegment b(kAntiDiagonal, kDiagonal, kVerticalAt1);
    const std::vector<Line> touching = a.intersection(b);
    ASSERT_EQ(1u, touching.size());
    EXPECT_EQ(kAntiDiagonal.a, touching[0].a);
    EXPECT_EQ(kAntiDiagonal.b, touching[0].b);
    EXPECT_FALSE(a.overlaps(b));
}

TEST(LineSegmentTest, CollinearOverlapReturnsBothBoundaryLines)
{
    const LineSegment a(kVerticalAtMinus10, kDiagonal, kVerticalAt1);
    const LineSegment b(kAntiDiagonal, kDiagonal, Line(IntPoint(5, 0), IntPoint(5, 10)));
    EXPECT_EQ(2u, a.intersection(b).size());
    EXPECT_TRUE(a.overlaps(b));
}

TEST(LineSegmentTest, SortEndpointsKeepsExactPoints)
{
    const LineSegment reversed(kAntiDiagonal, Line(IntPoint(10, 10), IntPoint(0, 0)), kVerticalAtMinus10);
    const LineSegment sorted = reversed.sort_endpoints_in_x_y();
    EXPECT_DOUBLE_EQ(-10.0, sorted.start_point_exact().to_float().x);
    EXPECT_DOUBLE_EQ(0.5, sorted.end_point_exact().to_float().x);
}

} // namespace
} // namespace freerouting::geometry::planar
