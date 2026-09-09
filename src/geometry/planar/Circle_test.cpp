#include <gtest/gtest.h>

#include <memory>

#include "geometry/planar/FortyfiveDegreeBoundingDirections.h"
#include "geometry/planar/IntOctagon.h"
#include "geometry/planar/OrthogonalBoundingDirections.h"
#include "geometry/planar/Point.h"
#include "geometry/planar/Polyline.h"
#include "geometry/planar/Shape.h"
#include "geometry/planar/Simplex.h"

namespace freerouting {
namespace geometry {
namespace planar {
namespace {

using testing::Test;

class CircleTest : public Test
{
protected:
    const IntPoint origin{0, 0};
};

TEST_F(CircleTest, ConstructorNegatesNegativeRadius)
{
    const Circle circle(origin, -50);
    EXPECT_EQ(50, circle.radius);
}

TEST_F(CircleTest, ContainsInside)
{
    const Circle circle(origin, 5);
    EXPECT_TRUE(circle.contains_inside(IntPoint(0, 0)));
    EXPECT_TRUE(circle.contains_inside(IntPoint(1, 1)));
    EXPECT_FALSE(circle.contains_inside(IntPoint(5, 0)));
}

TEST_F(CircleTest, ContainsOnBorder)
{
    const Circle circle(origin, 5);
    EXPECT_TRUE(circle.contains_on_border(IntPoint(5, 0)));
    EXPECT_TRUE(circle.contains_on_border(IntPoint(0, -5)));
    EXPECT_FALSE(circle.contains_on_border(IntPoint(5, 5)));
    EXPECT_FALSE(circle.contains_on_border(IntPoint(0, 0)));
}

TEST_F(CircleTest, BoundingTileWithLargeMaxSegmentLengthIsOctagon)
{
    const Circle circle(origin, 100);
    const std::unique_ptr<TileShape> tile = circle.bounding_tile(1000);
    const auto* octagon = dynamic_cast<const IntOctagon*>(tile.get());
    ASSERT_NE(octagon, nullptr);
    EXPECT_TRUE(octagon->contains(IntPoint(0, 0)));
    EXPECT_TRUE(octagon->contains(IntPoint(70, 70)));
    EXPECT_TRUE(octagon->contains(IntPoint(100, 0)));
    EXPECT_FALSE(octagon->contains(IntPoint(150, 0)));
}

TEST_F(CircleTest, BoundingTileWithSmallMaxSegmentLengthIsSimplex)
{
    const Circle circle(origin, 100);
    const std::unique_ptr<TileShape> tile = circle.bounding_tile(10);
    const auto* simplex = dynamic_cast<const Simplex*>(tile.get());
    ASSERT_NE(simplex, nullptr);
    EXPECT_TRUE(simplex->contains(IntPoint(0, 0)));
}

TEST_F(CircleTest, SplitToConvexReturnsBoundingTile)
{
    const Circle circle(origin, 100);
    const std::vector<std::unique_ptr<TileShape>> tiles = circle.split_to_convex();
    ASSERT_EQ(1u, tiles.size());
    EXPECT_TRUE(tiles[0]->contains(IntPoint(0, 0)));
}

TEST_F(CircleTest, BoundingShapeOrthogonal)
{
    const Circle circle(origin, 100);
    const IntOctagon oct = circle.bounding_shape(OrthogonalBoundingDirections::INSTANCE);
    EXPECT_TRUE(oct.contains(IntPoint(0, 0)));
    EXPECT_TRUE(oct.contains(IntPoint(100, 0)));
    EXPECT_FALSE(oct.contains(IntPoint(200, 0)));
}

TEST_F(CircleTest, BoundingShapeFortyfiveDegree)
{
    const Circle circle(origin, 100);
    const IntOctagon oct = circle.bounding_shape(FortyfiveDegreeBoundingDirections::INSTANCE);
    EXPECT_TRUE(oct.contains(IntPoint(0, 0)));
    EXPECT_TRUE(oct.contains(IntPoint(70, 70)));
    EXPECT_EQ(circle.bounding_octagon(), oct);
}

TEST_F(CircleTest, GetBorderReturnsSelf)
{
    const Circle circle(IntPoint(3, 4), 7);
    const Circle border = circle.get_border();
    EXPECT_EQ(3, border.center.x);
    EXPECT_EQ(4, border.center.y);
    EXPECT_EQ(7, border.radius);
}

TEST_F(CircleTest, GetHolesIsEmpty)
{
    const Circle circle(origin, 5);
    EXPECT_TRUE(circle.get_holes().empty());
}

TEST_F(CircleTest, CornerApproxArrIsEmpty)
{
    const Circle circle(origin, 5);
    EXPECT_TRUE(circle.corner_approx_arr().empty());
}

TEST_F(CircleTest, CutoutReturnsEmpty)
{
    const Circle circle(origin, 5);
    const std::vector<Line> lines{Line(IntPoint(0, 0), IntPoint(10, 10))};
    const Polyline polyline(lines);
    EXPECT_TRUE(circle.cutout(polyline).empty());
}

} // namespace
} // namespace planar
} // namespace geometry
} // namespace freerouting