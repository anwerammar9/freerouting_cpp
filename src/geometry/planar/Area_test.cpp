#include <cmath>
#include <memory>

#include "geometry/planar/IntBox.h"
#include "geometry/planar/PolygonShape.h"
#include "geometry/planar/PolylineArea.h"
#include "geometry/planar/Shape.h"
#include "geometry/planar/TileShape.h"
#include "geometry/planar/Vector.h"
#include "gtest/gtest.h"

using namespace freerouting::geometry::planar;

namespace
{

PolygonArea make_square_area()
{
    return PolygonArea({IntPoint(0, 0), IntPoint(10, 0), IntPoint(10, 10), IntPoint(0, 10)});
}

} // namespace

TEST(AreaTest, PolygonAreaIsBoundedAndDimension)
{
    PolygonArea area = make_square_area();
    EXPECT_TRUE(area.is_bounded());
    EXPECT_EQ(area.dimension(), 2);

    PolygonArea empty;
    EXPECT_FALSE(empty.is_bounded());
    EXPECT_EQ(empty.dimension(), -1);
}

TEST(AreaTest, PolygonAreaContainsFloatPoint)
{
    PolygonArea area = make_square_area();
    EXPECT_TRUE(area.contains(FloatPoint(5.0, 5.0)));
    EXPECT_FALSE(area.contains(FloatPoint(11.0, 5.0)));
    EXPECT_FALSE(area.contains(FloatPoint(0.0, 0.0)));
}

TEST(AreaTest, PolygonAreaIsContainedIn)
{
    PolygonArea area = make_square_area();
    EXPECT_TRUE(area.is_contained_in(IntBox(-5, -5, 15, 15)));
    EXPECT_FALSE(area.is_contained_in(IntBox(0, 0, 10, 5)));
}

TEST(AreaTest, PolygonAreaNearestPointApprox)
{
    PolygonArea area = make_square_area();
    auto nearest = area.nearest_point_approx(FloatPoint(5.0, 15.0));
    ASSERT_TRUE(nearest.has_value());
    EXPECT_EQ(*nearest, FloatPoint(5.0, 10.0));

    PolygonArea empty;
    EXPECT_FALSE(empty.nearest_point_approx(FloatPoint(0.0, 0.0)).has_value());
}

TEST(AreaTest, PolygonAreaCornerApproxArr)
{
    PolygonArea area = make_square_area();
    std::vector<FloatPoint> corners = area.corner_approx_arr();
    ASSERT_EQ(corners.size(), 4u);
    EXPECT_EQ(corners[0], FloatPoint(0.0, 0.0));
    EXPECT_EQ(corners[3], FloatPoint(0.0, 10.0));
}

TEST(AreaTest, PolygonAreaSplitToConvexIsCachedAndTiled)
{
    PolygonArea area = make_square_area();
    const auto& first = area.split_to_convex();
    const auto& second = area.split_to_convex();
    ASSERT_EQ(first.size(), 1u);
    EXPECT_EQ(&first, &second);
    EXPECT_EQ(first[0]->dimension(), 2);
    EXPECT_DOUBLE_EQ(first[0]->area(), 100.0);
}

TEST(AreaTest, PolygonAreaTransformsThroughAreaInterface)
{
    PolygonArea area = make_square_area();

    std::unique_ptr<Area> shifted = area.translate_by(IntVector(1, 2));
    EXPECT_EQ(shifted->bounding_box(), IntBox(1, 2, 11, 12));

    std::unique_ptr<Area> turned = area.turn_90_degree(1, IntPoint(5, 5));
    EXPECT_EQ(turned->bounding_box(), IntBox(0, 0, 10, 10));

    std::unique_ptr<Area> rotated = area.rotate_approx(M_PI / 2.0, FloatPoint(5.0, 5.0));
    EXPECT_NEAR(rotated->bounding_box().area(), 100.0, 1e-6);

    std::unique_ptr<Area> mirrored_h = area.mirror_horizontal(IntPoint(0, 0));
    EXPECT_EQ(mirrored_h->bounding_box(), IntBox(0, -10, 10, 0));

    std::unique_ptr<Area> mirrored_v = area.mirror_vertical(IntPoint(0, 0));
    EXPECT_EQ(mirrored_v->bounding_box(), IntBox(-10, 0, 0, 10));
}

TEST(AreaTest, PolygonAreaVirtualDispatchViaBase)
{
    auto area = std::make_unique<PolygonArea>(make_square_area());
    const Area& base = *area;

    EXPECT_TRUE(base.contains(IntPoint(5, 5)));
    EXPECT_FALSE(base.is_empty());
    EXPECT_EQ(base.bounding_box(), IntBox(0, 0, 10, 10));

    std::unique_ptr<Area> clone = base.clone();
    EXPECT_FALSE(clone->is_empty());
    EXPECT_EQ(clone->bounding_box(), IntBox(0, 0, 10, 10));
}

TEST(AreaTest, PolylineAreaTransformsReturnConcreteAreaThroughInterface)
{
    PolylineArea area(
        std::make_shared<PolygonShape>(std::vector<IntPoint>{IntPoint(0, 0), IntPoint(0, 10), IntPoint(10, 10), IntPoint(10, 0)}),
        {});

    std::unique_ptr<Area> shifted = area.translate_by(IntVector(1, 2));
    EXPECT_EQ(shifted->bounding_box(), IntBox(1, 2, 11, 12));
    EXPECT_TRUE(shifted->contains(IntPoint(2, 10)));

    const Area& base = area;
    std::unique_ptr<Area> turned = base.turn_90_degree(1, IntPoint(0, 0));
    EXPECT_FALSE(turned->is_empty());
}

TEST(AreaTest, PolygonAreaTranslateByZeroReturnsCopy)
{
    PolygonArea area = make_square_area();
    std::unique_ptr<Area> shifted = area.translate_by(IntVector(0, 0));
    ASSERT_NE(shifted, nullptr);
    EXPECT_EQ(shifted->bounding_box(), IntBox(0, 0, 10, 10));
}