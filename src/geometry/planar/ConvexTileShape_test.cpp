#include <gtest/gtest.h>

#include <cmath>
#include <memory>
#include <numbers>

#include "geometry/planar/ConvexTileShape.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/IntOctagon.h"
#include "geometry/planar/Shape.h"
#include "geometry/planar/Simplex.h"

namespace freerouting::geometry::planar {
namespace {

TEST(TileShapeTransformTest, BoxTurn90KeepsBoxType)
{
    const IntBox box(0, 0, 100, 50);
    const std::unique_ptr<TileShape> turned = static_cast<const TileShape&>(box).turn_90_degree(1, IntPoint(0, 0));
    const auto* turned_box = dynamic_cast<const IntBox*>(turned.get());
    ASSERT_NE(nullptr, turned_box);
    // (x, y) -> (-y, x)
    EXPECT_EQ(IntBox(-50, 0, 0, 100).ll, turned_box->ll);
    EXPECT_EQ(IntBox(-50, 0, 0, 100).ur, turned_box->ur);
}

TEST(TileShapeTransformTest, SimplexTurn90MatchesBoxGeometry)
{
    const Simplex simplex = IntBox(0, 0, 100, 50).to_Simplex();
    const auto turned = static_cast<const TileShape&>(simplex).turn_90_degree(1, IntPoint(0, 0));
    const IntBox bbox = turned->bounding_box();
    EXPECT_EQ(IntPoint(-50, 0), bbox.ll);
    EXPECT_EQ(IntPoint(0, 100), bbox.ur);
    EXPECT_NEAR(5000.0, turned->area(), 1e-6);
}

TEST(TileShapeTransformTest, MirrorVerticalReflectsX)
{
    const IntOctagon oct = IntBox(10, 0, 30, 20).to_IntOctagon();
    const auto mirrored = static_cast<const TileShape&>(oct).mirror_vertical(IntPoint(0, 0));
    const IntBox bbox = mirrored->bounding_box();
    EXPECT_EQ(IntPoint(-30, 0), bbox.ll);
    EXPECT_EQ(IntPoint(-10, 20), bbox.ur);
}

TEST(TileShapeTransformTest, RotateApproxByZeroIsIdentityAnd45DegreesKeepsArea)
{
    const IntBox box(-100, -100, 100, 100);
    const auto same = static_cast<const TileShape&>(box).rotate_approx(0, FloatPoint(0, 0));
    EXPECT_NEAR(box.area(), same->area(), 1e-6);

    const auto rotated = static_cast<const TileShape&>(box).rotate_approx(std::numbers::pi / 4, FloatPoint(0, 0));
    // A square rotated by 45 degrees: corners on the axes at distance 100*sqrt(2).
    const IntBox bbox = rotated->bounding_box();
    EXPECT_NEAR(141, bbox.ur.x, 1);
    EXPECT_NEAR(-141, bbox.ll.y, 1);
    EXPECT_NEAR(box.area(), rotated->area(), 400.0); // integer rounding of the corners
}

TEST(TileShapeTransformTest, TranslateByKeepsConcreteType)
{
    const IntOctagon oct = IntBox(0, 0, 10, 10).to_IntOctagon();
    const auto moved = static_cast<const TileShape&>(oct).translate_by(IntVector(5, -5));
    ASSERT_NE(nullptr, dynamic_cast<const IntOctagon*>(moved.get()));
    EXPECT_EQ(IntPoint(5, -5), moved->bounding_box().ll);
}

TEST(ConvexTileShapeTest, DelegatesQueriesToTheTile)
{
    const ConvexTileShape shape(std::make_unique<IntBox>(0, 0, 100, 40));
    EXPECT_EQ(IntPoint(0, 0), shape.bounding_box().ll);
    EXPECT_TRUE(shape.contains_inside(IntPoint(50, 20)));
    EXPECT_FALSE(shape.contains_inside(IntPoint(0, 20)));
    EXPECT_TRUE(shape.contains_on_border(IntPoint(0, 20)));
    EXPECT_NEAR(4000.0, shape.area(), 1e-6);
    EXPECT_NEAR(40.0, shape.min_width(), 1e-6);
    EXPECT_NEAR(20.0, shape.border_distance(FloatPoint(50, 20)), 1e-6);
}

TEST(ConvexTileShapeTest, IntersectsOtherTilesAndCircles)
{
    const ConvexTileShape box(std::make_unique<IntBox>(0, 0, 100, 100));
    const ConvexTileShape near_oct(std::make_unique<IntOctagon>(IntBox(50, 50, 150, 150).to_IntOctagon()));
    const ConvexTileShape far_simplex(std::make_unique<Simplex>(IntBox(500, 500, 600, 600).to_Simplex()));
    EXPECT_TRUE(box.intersects(near_oct));
    EXPECT_FALSE(box.intersects(far_simplex));
    EXPECT_TRUE(box.intersects(Circle(IntPoint(120, 50), 30)));
    EXPECT_FALSE(box.intersects(Circle(IntPoint(300, 300), 30)));
}

TEST(ConvexTileShapeTest, EnlargeOffsetsTheTile)
{
    const ConvexTileShape shape(std::make_unique<IntBox>(0, 0, 100, 100));
    const std::unique_ptr<Shape> enlarged = shape.enlarge(10);
    EXPECT_LE(enlarged->bounding_box().ll.x, -9);
    EXPECT_GE(enlarged->bounding_box().ur.x, 109);
}

} // namespace
} // namespace freerouting::geometry::planar
