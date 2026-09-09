#include <gtest/gtest.h>

#include <optional>
#include <vector>

#include "geometry/planar/LineSegment.h"
#include "geometry/planar/Point.h"
#include "geometry/planar/Polyline.h"
#include "geometry/planar/Vector.h"

namespace freerouting {
namespace geometry {
namespace planar {
namespace {

using testing::Test;

class PolylineTest : public Test
{
};

static Polyline orthogonal_polyline()
{
    const std::vector<IntPoint> corners{
        IntPoint(0, 0), IntPoint(100, 0), IntPoint(100, 100),
        IntPoint(150, 100), IntPoint(150, 200)};
    return Polyline(corners);
}

TEST_F(PolylineTest, ConstructorRemovesConsecutiveParallelLines)
{
    // Similar to a non-cleanable polyline except that lines 1 and 2 are parallel.
    const std::vector<Line> lines{
        Line(IntPoint(0, 0), IntPoint(100, 0)),
        Line(IntPoint(100, 0), IntPoint(150, 50)),
        Line(IntPoint(150, 50), IntPoint(200, 100)),
        Line(IntPoint(200, 100), IntPoint(300, 100))};
    const Polyline polyline(lines);
    EXPECT_EQ(2, polyline.corner_count());
    EXPECT_EQ(IntPoint(100, 0), polyline.corner(0));
    EXPECT_EQ(IntPoint(200, 100), polyline.corner(1));
}

TEST_F(PolylineTest, ConstructorReturnsEmptyForTwoParallelLines)
{
    const std::vector<Line> lines{
        Line(IntPoint(0, 0), IntPoint(100, 0)),
        Line(IntPoint(100, 0), IntPoint(200, 0)),
        Line(IntPoint(200, 0), IntPoint(300, 0))};
    const Polyline polyline(lines);
    EXPECT_TRUE(polyline.is_empty());
}

TEST_F(PolylineTest, ConstructorKeepsCleanOrthogonalPolyline)
{
    const Polyline polyline = orthogonal_polyline();
    EXPECT_EQ(3, polyline.corner_count());
    EXPECT_EQ(IntPoint(100, 0), polyline.corner(0));
    EXPECT_EQ(IntPoint(100, 100), polyline.corner(1));
    EXPECT_EQ(IntPoint(150, 100), polyline.corner(2));
}

TEST_F(PolylineTest, ShortenCreatesRequestedLineCountAndLength)
{
    const Polyline polyline = orthogonal_polyline();
    const Polyline shortened = polyline.shorten(3, 50);
    EXPECT_EQ(2, shortened.corner_count());
    EXPECT_EQ(IntPoint(100, 0), shortened.corner(0));
    EXPECT_EQ(IntPoint(100, 50), shortened.corner(1));
    EXPECT_DOUBLE_EQ(50.0, shortened.length_approx(0, 1));
}

TEST_F(PolylineTest, ShortenWithTooLargeLineCountReturnsUnchanged)
{
    const Polyline polyline = orthogonal_polyline();
    const Polyline shortened = polyline.shorten(4, 50);
    EXPECT_EQ(polyline.corner_count(), shortened.corner_count());
}

TEST_F(PolylineTest, ProjectionLineFindsNearestSegment)
{
    const Polyline polyline = orthogonal_polyline();
    const std::optional<LineSegment> seg = polyline.projection_line(IntPoint(50, 50));
    ASSERT_TRUE(seg.has_value());
    EXPECT_TRUE(seg->get_end_closing_line().fast_equals(Line(IntPoint(100, 0), IntPoint(100, 100))));
    EXPECT_TRUE(seg->get_start_closing_line().fast_equals(Line(IntPoint(50, 50), IntPoint(50, 150))));
    EXPECT_TRUE(seg->get_line().side_of(IntPoint(50, 50)) == Side::COLLINEAR);
    EXPECT_EQ(seg->get_line().a.y, seg->get_line().b.y);
}

TEST_F(PolylineTest, ProjectionLinePerpendicularToMiddleSegment)
{
    const Polyline polyline = orthogonal_polyline();
    const std::optional<LineSegment> seg = polyline.projection_line(IntPoint(120, 110));
    ASSERT_TRUE(seg.has_value());
    EXPECT_TRUE(seg->get_end_closing_line().fast_equals(Line(IntPoint(100, 100), IntPoint(150, 100))));
    EXPECT_TRUE(seg->get_start_closing_line().fast_equals(Line(IntPoint(120, 110), IntPoint(170, 110))));
    EXPECT_TRUE(seg->get_line().side_of(IntPoint(120, 110)) == Side::COLLINEAR);
    EXPECT_EQ(seg->get_line().a.x, seg->get_line().b.x);
}

TEST_F(PolylineTest, ProjectionLineReturnsNulloptWhenProjectionOutside)
{
    const Polyline polyline = orthogonal_polyline();
    EXPECT_FALSE(polyline.projection_line(IntPoint(0, 300)).has_value());
    EXPECT_FALSE(polyline.projection_line(IntPoint(250, 110)).has_value());
}

} // namespace
} // namespace planar
} // namespace geometry
} // namespace freerouting