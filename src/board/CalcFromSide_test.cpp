#include <gtest/gtest.h>

#include "board/CalcFromSide.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/IntOctagon.h"
#include "geometry/planar/LineSegment.h"
#include "geometry/planar/Polyline.h"

using namespace freerouting::geometry::planar;
using freerouting::board::CalcFromSide;

namespace
{

const IntBox kSquare(IntPoint(0, 0), IntPoint(100, 100));

} // namespace

TEST(CalcFromSideTest, NotCalculated)
{
    EXPECT_EQ(CalcFromSide::NOT_CALCULATED.no, -1);
}

TEST(CalcFromSideTest, ValuesConstructor)
{
    const FloatPoint intersection(12.5, 17.25);
    const CalcFromSide cfs(3, intersection);
    EXPECT_EQ(cfs.no, 3);
    EXPECT_DOUBLE_EQ(cfs.border_intersection.x, 12.5);
    EXPECT_DOUBLE_EQ(cfs.border_intersection.y, 17.25);
}

TEST(CalcFromSideTest, PolylineEntersSquareFromLeft)
{
    // Segment between the two middle corners crosses the square from x = -100
    // to x = 200 at y = 50. Enters at the left border (edge 3).
    const Polyline polyline(
        std::vector<IntPoint>{IntPoint(-100, 0), IntPoint(-100, 50), IntPoint(200, 50),
                              IntPoint(200, 150)});
    const CalcFromSide cfs(polyline, 1, kSquare);
    EXPECT_EQ(cfs.no, 3);
    EXPECT_DOUBLE_EQ(cfs.border_intersection.x, 0.0);
    EXPECT_DOUBLE_EQ(cfs.border_intersection.y, 50.0);
}

TEST(CalcFromSideTest, PolylineLeavesSquareAtTop)
{
    // Segment between corners (50, 50) and (50, 150) leaves the square
    // through the top border (edge 2).
    const Polyline polyline(
        std::vector<IntPoint>{IntPoint(-100, 50), IntPoint(50, 50), IntPoint(50, 150),
                              IntPoint(200, 150)});
    const CalcFromSide cfs(polyline, 1, kSquare);
    EXPECT_EQ(cfs.no, 2);
    EXPECT_DOUBLE_EQ(cfs.border_intersection.x, 50.0);
    EXPECT_DOUBLE_EQ(cfs.border_intersection.y, 100.0);
}

TEST(CalcFromSideTest, PolylineStartsInsideSquare)
{
    // No line segment crosses the border; the nearest intersection of the
    // second polyline line with the shape border is used instead.
    const Polyline polyline(
        std::vector<IntPoint>{IntPoint(10, 10), IntPoint(40, 10), IntPoint(40, 40),
                              IntPoint(90, 40)});
    const CalcFromSide cfs(polyline, 1, kSquare);
    EXPECT_EQ(cfs.no, 0);
    EXPECT_DOUBLE_EQ(cfs.border_intersection.x, 40.0);
    EXPECT_DOUBLE_EQ(cfs.border_intersection.y, 0.0);
}

TEST(CalcFromSideTest, FromPointBelowSquare)
{
    const CalcFromSide cfs(IntPoint(50, -50), kSquare);
    EXPECT_EQ(cfs.no, 0);
    EXPECT_DOUBLE_EQ(cfs.border_intersection.x, 50.0);
    EXPECT_DOUBLE_EQ(cfs.border_intersection.y, 0.0);
}

TEST(CalcFromSideTest, FromPointAtCenter)
{
    // (50, 50) is equidistant from all four edges; the projection loop visits
    // the left border first, so edge 3 is selected.
    const CalcFromSide cfs(IntPoint(50, 50), kSquare);
    EXPECT_EQ(cfs.no, 3);
    EXPECT_DOUBLE_EQ(cfs.border_intersection.x, 0.0);
    EXPECT_DOUBLE_EQ(cfs.border_intersection.y, 50.0);
}

TEST(CalcFromSideTest, FromPointLeftOfSquare)
{
    const CalcFromSide cfs(IntPoint(-10, 50), kSquare);
    EXPECT_EQ(cfs.no, 3);
    EXPECT_DOUBLE_EQ(cfs.border_intersection.x, 0.0);
    EXPECT_DOUBLE_EQ(cfs.border_intersection.y, 50.0);
}

TEST(CalcFromSideTest, LineSegmentHorizontalCrossing)
{
    const LineSegment cross = LineSegment::from_endpoints(IntPoint(-50, 50), IntPoint(150, 50));
    const CalcFromSide cfs_left(cross, kSquare, true);
    EXPECT_EQ(cfs_left.no, 1);
    EXPECT_DOUBLE_EQ(cfs_left.border_intersection.x, 100.0);
    EXPECT_DOUBLE_EQ(cfs_left.border_intersection.y, 50.0);
    const CalcFromSide cfs_right(cross, kSquare, false);
    EXPECT_EQ(cfs_right.no, 1);
    EXPECT_DOUBLE_EQ(cfs_right.border_intersection.x, 100.0);
    EXPECT_DOUBLE_EQ(cfs_right.border_intersection.y, 50.0);
}

TEST(CalcFromSideTest, LineSegmentVerticalCrossing)
{
    const LineSegment cross = LineSegment::from_endpoints(IntPoint(25, -50), IntPoint(75, 150));
    const CalcFromSide cfs_left(cross, kSquare, true);
    EXPECT_EQ(cfs_left.no, 2);
    EXPECT_DOUBLE_EQ(cfs_left.border_intersection.x, 50.0);
    EXPECT_DOUBLE_EQ(cfs_left.border_intersection.y, 100.0);
    const CalcFromSide cfs_right(cross, kSquare, false);
    EXPECT_EQ(cfs_right.no, 2);
    EXPECT_DOUBLE_EQ(cfs_right.border_intersection.x, 50.0);
    EXPECT_DOUBLE_EQ(cfs_right.border_intersection.y, 100.0);
}

TEST(CalcFromSideTest, OctagonShoveDirectionAsymmetry)
{
    // Octagon with 45-degree corner cuts on the square (0, 0)-(100, 100).
    // The crossing segment enters at the left face (edge 6); the shove
    // direction selects the opposite side, edge 0 for a left shove and
    // edge 4 for a right shove.
    const IntOctagon octagon(0, 0, 100, 100, -80, 80, 20, 180);
    const LineSegment cross = LineSegment::from_endpoints(IntPoint(-50, 50), IntPoint(150, 50));
    const CalcFromSide cfs_left(cross, octagon, true);
    EXPECT_EQ(cfs_left.no, 0);
    EXPECT_DOUBLE_EQ(cfs_left.border_intersection.x, 50.0);
    EXPECT_DOUBLE_EQ(cfs_left.border_intersection.y, 0.0);
    const CalcFromSide cfs_right(cross, octagon, false);
    EXPECT_EQ(cfs_right.no, 4);
    EXPECT_DOUBLE_EQ(cfs_right.border_intersection.x, 50.0);
    EXPECT_DOUBLE_EQ(cfs_right.border_intersection.y, 100.0);
}