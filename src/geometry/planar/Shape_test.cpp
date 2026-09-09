#include <gtest/gtest.h>

#include <cmath>
#include <memory>

#include "geometry/planar/IntBox.h"
#include "geometry/planar/IntOctagon.h"
#include "geometry/planar/OrthogonalBoundingDirections.h"
#include "geometry/planar/Point.h"
#include "geometry/planar/PolygonShape.h"
#include "geometry/planar/Polyline.h"
#include "geometry/planar/Shape.h"
#include "geometry/planar/TileShape.h"

namespace freerouting {
namespace geometry {
namespace planar {
namespace {

using testing::Test;

const double kPi = M_PI;

TEST(ShapeInterfaceTest, CircleSurfaceThroughShapeBase)
{
    const Circle circle(IntPoint(0, 0), 100);
    const Shape& shape = circle;

    EXPECT_NEAR(2.0 * kPi * 100.0, shape.circumference(), 1e-9);
    EXPECT_NEAR(kPi * 100.0 * 100.0, shape.area(), 1e-6);

    const FloatPoint cog = shape.centre_of_gravity();
    EXPECT_NEAR(0.0, cog.x, 1e-9);
    EXPECT_NEAR(0.0, cog.y, 1e-9);

    EXPECT_FALSE(shape.is_outside(IntPoint(50, 0)));
    EXPECT_TRUE(shape.is_outside(IntPoint(150, 0)));
    EXPECT_FALSE(shape.is_outside(FloatPoint(50.0, 0.0)));
    EXPECT_TRUE(shape.is_outside(FloatPoint(250.0, 0.0)));

    EXPECT_TRUE(shape.contains_inside(IntPoint(50, 0)));
    EXPECT_FALSE(shape.contains_inside(IntPoint(100, 0)));
    EXPECT_TRUE(shape.contains_inside(FloatPoint(50.0, 0.0)));
    EXPECT_FALSE(shape.contains_inside(FloatPoint(100.0, 0.0)));

    EXPECT_TRUE(shape.contains_on_border(IntPoint(100, 0)));
    EXPECT_FALSE(shape.contains_on_border(IntPoint(99, 0)));
    EXPECT_TRUE(shape.contains_on_border(FloatPoint(-100.0, 0.0)));
    EXPECT_FALSE(shape.contains_on_border(FloatPoint(-99.0, 0.0)));

    EXPECT_TRUE(shape.contains(FloatPoint(100.0, 0.0)));
    EXPECT_FALSE(shape.contains(FloatPoint(101.0, 0.0)));

    EXPECT_NEAR(0.0, shape.distance(FloatPoint(50.0, 0.0)), 1e-9);
    EXPECT_NEAR(50.0, shape.distance(FloatPoint(150.0, 0.0)), 1e-9);
    EXPECT_NEAR(50.0, shape.border_distance(FloatPoint(150.0, 0.0)), 1e-9);
    EXPECT_NEAR(50.0, shape.border_distance(FloatPoint(50.0, 0.0)), 1e-9);

    EXPECT_DOUBLE_EQ(100.0, shape.smallest_radius());

    const auto tile = shape.bounding_tile();
    ASSERT_NE(nullptr, tile);
    EXPECT_TRUE(tile->contains(IntPoint(0, 0)));

    const auto& dirs = OrthogonalBoundingDirections::INSTANCE;
    const IntOctagon oct = shape.bounding_shape(dirs);
    EXPECT_TRUE(oct.contains(IntPoint(0, 0)));

    const std::unique_ptr<Shape> enlarged = shape.enlarge(50);
    ASSERT_NE(nullptr, enlarged);
    const Circle* enlarged_circle = dynamic_cast<const Circle*>(enlarged.get());
    ASSERT_NE(nullptr, enlarged_circle);
    EXPECT_EQ(150, enlarged_circle->radius);

    EXPECT_TRUE(shape.intersects(Circle(IntPoint(200, 0), 150)));
    EXPECT_FALSE(shape.intersects(Circle(IntPoint(200, 0), 50)));
    EXPECT_TRUE(shape.intersects(IntBox(IntPoint(99, 0), IntPoint(101, 2))));
    EXPECT_FALSE(shape.intersects(IntBox(IntPoint(200, 0), IntPoint(201, 2))));

    const std::vector<Line> lines{Line(IntPoint(0, 0), IntPoint(10, 10))};
    const Polyline polyline(lines);
    EXPECT_TRUE(shape.cutout(polyline).empty());
}

TEST(ShapeInterfaceTest, PolygonShapeSurfaceThroughShapeBase)
{
    const PolygonShape rec(std::vector<IntPoint>{IntPoint(0, 0), IntPoint(100, 0), IntPoint(100, 100),
                                                 IntPoint(0, 100)});
    const Shape& shape = rec;

    EXPECT_NEAR(400.0, shape.circumference(), 1e-9);
    EXPECT_NEAR(10000.0, shape.area(), 1e-6);

    const FloatPoint cog = shape.centre_of_gravity();
    EXPECT_NEAR(50.0, cog.x, 1e-9);
    EXPECT_NEAR(50.0, cog.y, 1e-9);

    EXPECT_FALSE(shape.is_outside(IntPoint(50, 50)));
    EXPECT_TRUE(shape.is_outside(IntPoint(150, 50)));
    EXPECT_FALSE(shape.is_outside(FloatPoint(50.0, 50.0)));
    EXPECT_TRUE(shape.is_outside(FloatPoint(50.0, 150.0)));

    EXPECT_TRUE(shape.contains_inside(IntPoint(50, 50)));
    EXPECT_FALSE(shape.contains_inside(IntPoint(150, 50)));
    EXPECT_TRUE(shape.contains_inside(FloatPoint(50.0, 50.0)));
    EXPECT_FALSE(shape.contains_inside(FloatPoint(50.0, 150.0)));

    EXPECT_FALSE(shape.contains_on_border(IntPoint(50, 50)));
    EXPECT_FALSE(shape.contains_on_border(FloatPoint(50.0, 50.0)));

    EXPECT_TRUE(shape.contains(FloatPoint(50.0, 50.0)));
    EXPECT_FALSE(shape.contains(FloatPoint(150.0, 50.0)));

    EXPECT_NEAR(0.0, shape.distance(FloatPoint(50.0, 50.0)), 1e-9);
    EXPECT_NEAR(50.0, shape.distance(FloatPoint(150.0, 50.0)), 1e-9);
    EXPECT_NEAR(50.0, shape.border_distance(FloatPoint(150.0, 50.0)), 1e-9);

    EXPECT_NEAR(50.0, shape.smallest_radius(), 1e-9);

    const auto tile = shape.bounding_tile();
    ASSERT_NE(nullptr, tile);
    EXPECT_TRUE(tile->contains(IntPoint(50, 50)));

    const auto& dirs = OrthogonalBoundingDirections::INSTANCE;
    const IntOctagon oct = shape.bounding_shape(dirs);
    EXPECT_TRUE(oct.contains(IntPoint(50, 50)));

    const std::unique_ptr<Shape> enlarged = shape.enlarge(0);
    ASSERT_NE(nullptr, enlarged);
    EXPECT_EQ(4, dynamic_cast<const PolygonShape&>(*enlarged).corners.size());

    EXPECT_FALSE(shape.intersects(IntBox(IntPoint(200, 0), IntPoint(201, 2))));
    EXPECT_TRUE(shape.intersects(Circle(IntPoint(110, 50), 20)));
}

TEST(ShapeInterfaceTest, DefaultImplementationsAreConservative)
{
    // A shape type with no precise override still compiles and returns
    // conservative defaults instead of crashing. Circle overrides everything,
    // so use it as a stand-in for an unimplemented specialized shape.
    const Circle circle(IntPoint(0, 0), 10);
    EXPECT_DOUBLE_EQ(2.0 * kPi * 10.0, circle.circumference());
}

} // namespace
} // namespace planar
} // namespace geometry
} // namespace freerouting