#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include "geometry/planar/IntBox.h"
#include "geometry/planar/IntOctagon.h"
#include "geometry/planar/OrthogonalBoundingDirections.h"
#include "geometry/planar/Point.h"
#include "geometry/planar/Polygon.h"
#include "geometry/planar/PolygonShape.h"
#include "geometry/planar/Polyline.h"
#include "geometry/planar/Shape.h"
#include "geometry/planar/TileShape.h"

namespace freerouting {
namespace geometry {
namespace planar {
namespace {

using testing::Test;

class PolygonShapeTest : public Test
{
protected:
    static PolygonShape rectangle()
    {
        return PolygonShape(std::vector<IntPoint>{IntPoint(0, 0), IntPoint(100, 0), IntPoint(100, 100),
                                                  IntPoint(0, 100)});
    }
};

TEST_F(PolygonShapeTest, ConstructorNormalizesRectangle)
{
    const PolygonShape shape = rectangle();
    EXPECT_EQ(4, shape.border_line_count());
    EXPECT_EQ(IntPoint(0, 0), shape.corner(0));
    EXPECT_EQ(IntPoint(100, 0), shape.corner(1));
    EXPECT_EQ(IntPoint(100, 100), shape.corner(2));
    EXPECT_EQ(IntPoint(0, 100), shape.corner(3));
}

TEST_F(PolygonShapeTest, ConstructorRemovesDuplicatesAndCollinearCorners)
{
    const PolygonShape shape(std::vector<IntPoint>{IntPoint(0, 0), IntPoint(50, 0), IntPoint(100, 0),
                                                   IntPoint(100, 0), IntPoint(100, 100), IntPoint(0, 100)});
    EXPECT_EQ(4, shape.border_line_count());
    EXPECT_EQ(IntPoint(0, 0), shape.corner(0));
    EXPECT_EQ(IntPoint(100, 0), shape.corner(1));
    EXPECT_EQ(IntPoint(100, 100), shape.corner(2));
    EXPECT_EQ(IntPoint(0, 100), shape.corner(3));
}

TEST_F(PolygonShapeTest, ConstructorFromPolygon)
{
    const Polygon polygon(std::vector<IntPoint>{IntPoint(0, 0), IntPoint(100, 0), IntPoint(100, 100),
                                                IntPoint(0, 100)});
    const PolygonShape shape(polygon);
    EXPECT_EQ(4, shape.border_line_count());
    EXPECT_EQ(IntPoint(0, 0), shape.corner(0));
}

TEST_F(PolygonShapeTest, CornerIsBoundedAlwaysTrue)
{
    const PolygonShape shape = rectangle();
    EXPECT_TRUE(shape.corner_is_bounded(0));
    EXPECT_TRUE(shape.corner_is_bounded(3));
}

TEST_F(PolygonShapeTest, Contains)
{
    const PolygonShape shape = rectangle();
    EXPECT_TRUE(shape.contains(IntPoint(50, 50)));
    EXPECT_TRUE(shape.contains(IntPoint(100, 0)));
    // Java parity: PolygonShape.contains_on_border is a stub returning false.
    EXPECT_FALSE(shape.contains_on_border(IntPoint(100, 0)));
    EXPECT_TRUE(shape.contains_inside(IntPoint(50, 50)));
    EXPECT_TRUE(shape.contains_inside(IntPoint(100, 0)));
    EXPECT_FALSE(shape.contains(IntPoint(-1, 50)));
}

TEST_F(PolygonShapeTest, AreaAndDimension)
{
    const PolygonShape shape = rectangle();
    EXPECT_DOUBLE_EQ(10000.0, shape.area());
    EXPECT_EQ(2, shape.dimension());
    EXPECT_TRUE(shape.is_bounded());
    EXPECT_FALSE(shape.is_empty());
}

TEST_F(PolygonShapeTest, IsConvexAndConvexHull)
{
    const PolygonShape shape = rectangle();
    EXPECT_TRUE(shape.is_convex());
    const PolygonShape hull = shape.convex_hull();
    EXPECT_EQ(4, hull.border_line_count());
}

TEST_F(PolygonShapeTest, BoundingBoxAndOctagon)
{
    const PolygonShape shape = rectangle();
    EXPECT_EQ(IntBox(0, 0, 100, 100), shape.bounding_box());
    const IntOctagon expected_octagon(0, 0, 100, 100, -100, 100, 0, 200);
    EXPECT_EQ(expected_octagon, shape.bounding_octagon());
    const IntOctagon orth = shape.bounding_shape(OrthogonalBoundingDirections::INSTANCE);
    EXPECT_EQ(expected_octagon, orth);
}

TEST_F(PolygonShapeTest, SplitToConvexReturnsSinglePieceForConvex)
{
    const PolygonShape shape = rectangle();
    const std::vector<std::unique_ptr<TileShape>>& tiles = shape.split_to_convex();
    ASSERT_EQ(1u, tiles.size());
    EXPECT_TRUE(tiles[0]->contains(IntPoint(50, 50)));
}

TEST_F(PolygonShapeTest, CutoutReturnsEmpty)
{
    const PolygonShape shape = rectangle();
    const Polyline polyline(std::vector<IntPoint>{IntPoint(0, 0), IntPoint(50, 50)});
    EXPECT_TRUE(shape.cutout(polyline).empty());
}

TEST_F(PolygonShapeTest, EnlargeOffsetsConvexPolygon)
{
    const PolygonShape shape = rectangle();
    const std::unique_ptr<Shape> enlarged = shape.enlarge(10);
    const auto* polygon = dynamic_cast<const PolygonShape*>(enlarged.get());
    ASSERT_NE(polygon, nullptr);
    EXPECT_EQ(IntBox(-10, -10, 110, 110), polygon->bounding_box());
}

TEST_F(PolygonShapeTest, BorderDistance)
{
    const PolygonShape shape = rectangle();
    EXPECT_DOUBLE_EQ(50.0, shape.border_distance(FloatPoint(50, 50)));
}

TEST_F(PolygonShapeTest, Intersects)
{
    const PolygonShape shape = rectangle();
    EXPECT_TRUE(shape.intersects(IntBox(50, 50, 150, 150)));
    EXPECT_FALSE(shape.intersects(IntBox(200, 200, 300, 300)));
    const Circle circle(IntPoint(50, 50), 10);
    EXPECT_TRUE(shape.intersects(circle));
    const Circle far_circle(IntPoint(500, 500), 10);
    EXPECT_FALSE(shape.intersects(far_circle));
}

} // namespace
} // namespace planar
} // namespace geometry
} // namespace freerouting