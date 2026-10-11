// Tests for the CircleArea adapter (Java Circle used as an Area).

#include "geometry/planar/CircleArea.h"

#include <gtest/gtest.h>

#include "geometry/planar/IntBox.h"
#include "geometry/planar/TileShape.h"

namespace freerouting::geometry::planar {
namespace {

TEST(CircleAreaTest, DelegatesToTheCircle)
{
    const CircleArea area(Circle(IntPoint(100, 200), 50));
    EXPECT_EQ(IntBox(50, 150, 150, 250), area.bounding_box());
    EXPECT_TRUE(area.contains(IntPoint(100, 200)));
    EXPECT_FALSE(area.contains(IntPoint(200, 200)));
    EXPECT_EQ(2, area.dimension());
    ASSERT_EQ(1u, area.split_to_convex().size()); // Java Circle.splitToConvex: the bounding tile
    EXPECT_EQ(area.split_to_convex()[0]->bounding_box(), area.bounding_box());
}

TEST(CircleAreaTest, ZeroRadiusIsNotAnArea)
{
    EXPECT_EQ(0, CircleArea(Circle(IntPoint(0, 0), 0)).dimension());
}

TEST(CircleAreaTest, TransformsReturnCircleAreas)
{
    const CircleArea area(Circle(IntPoint(10, 0), 5));
    const auto moved = area.translate_by(IntVector(5, 5));
    EXPECT_EQ(IntBox(10, 0, 20, 10), moved->bounding_box());
    EXPECT_NE(nullptr, dynamic_cast<const CircleArea*>(moved.get()));
}

} // namespace
} // namespace freerouting::geometry::planar
