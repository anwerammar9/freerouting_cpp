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

// Golden pieces printed by Java PolygonShape.splitToConvex (freerouting 2.1.0 jar). The start
// corner of the split comes from java.util.Random(99); the old C++ LCG used multiplier
// 0x5DEECE66 instead of 0x5DEECE66D and split these shapes differently.
std::vector<std::vector<IntPoint>> split_corners(const PolygonShape& p_shape)
{
    std::vector<std::vector<IntPoint>> result;
    for (const auto& piece : p_shape.split_to_convex())
    {
        std::vector<IntPoint> corners;
        for (int i = 0; i < piece->border_line_count(); ++i)
        {
            corners.push_back(piece->corner(i));
        }
        result.push_back(corners);
    }
    return result;
}

TEST(PolygonShapeSplitTest, MatchesJavaForComb)
{
    const PolygonShape comb({IntPoint(0, 0), IntPoint(70, 0), IntPoint(70, 40), IntPoint(60, 40), IntPoint(60, 10),
        IntPoint(40, 10), IntPoint(40, 40), IntPoint(30, 40), IntPoint(30, 10), IntPoint(10, 10), IntPoint(10, 40),
        IntPoint(0, 40)});
    const std::vector<std::vector<IntPoint>> expected = {
        {IntPoint(0, 10), IntPoint(10, 10), IntPoint(10, 40), IntPoint(0, 40)},
        {IntPoint(0, 0), IntPoint(30, 0), IntPoint(30, 10), IntPoint(0, 10)},
        {IntPoint(30, 10), IntPoint(40, 10), IntPoint(40, 40), IntPoint(30, 40)},
        {IntPoint(30, 0), IntPoint(60, 0), IntPoint(60, 10), IntPoint(30, 10)},
        {IntPoint(60, 0), IntPoint(70, 0), IntPoint(70, 40), IntPoint(60, 40)},
    };
    EXPECT_EQ(expected, split_corners(comb));
}

TEST(PolygonShapeSplitTest, MatchesJavaForLShape)
{
    const PolygonShape l_shape({IntPoint(0, 0), IntPoint(40, 0), IntPoint(40, 10), IntPoint(10, 10), IntPoint(10, 30),
        IntPoint(0, 30)});
    const std::vector<std::vector<IntPoint>> expected = {
        {IntPoint(0, 10), IntPoint(10, 10), IntPoint(10, 30), IntPoint(0, 30)},
        {IntPoint(0, 0), IntPoint(40, 0), IntPoint(40, 10), IntPoint(0, 10)},
    };
    EXPECT_EQ(expected, split_corners(l_shape));
}

TEST(PolygonShapeSplitTest, MatchesJavaForArrow)
{
    const PolygonShape arrow({IntPoint(0, 0), IntPoint(50, 20), IntPoint(0, 40), IntPoint(15, 20)});
    const std::vector<std::vector<IntPoint>> expected = {
        {IntPoint(0, 0), IntPoint(15, 6), IntPoint(15, 20)},
        {IntPoint(15, 6), IntPoint(50, 20), IntPoint(15, 34)},
        {IntPoint(15, 20), IntPoint(15, 34), IntPoint(0, 40)},
    };
    EXPECT_EQ(expected, split_corners(arrow));
}

} // namespace
} // namespace planar
} // namespace geometry
} // namespace freerouting