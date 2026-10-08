#include <gtest/gtest.h>

#include "board/Component.h"

namespace freerouting::board {
namespace {

using geometry::planar::IntPoint;

TEST(ComponentTest, RotationIsNormalizedAndPackageFollowsTheSide)
{
    Packages packages;
    Package* front = packages.add("P", {}, {}, {}, {}, {}, {}, {}, true);
    Package* back = packages.add("P", {}, {}, {}, {}, {}, {}, {}, false);
    Component component("U1", IntPoint(100, 200), -90.0, true, front, back, 1, false, "PN1");
    EXPECT_EQ(270.0, component.get_rotation());
    EXPECT_EQ(front, component.get_package());
    EXPECT_EQ("PN1", component.get_part_number());

    component.change_side(IntPoint(0, 0));
    EXPECT_FALSE(component.is_placed_on_front());
    EXPECT_EQ(back, component.get_package());
    EXPECT_EQ(IntPoint(-100, 200), component.get_location());
}

TEST(ComponentTest, TurnAndTranslateMoveTheLocation)
{
    Component component("U1", IntPoint(100, 0), 270.0, true, nullptr, nullptr, 1, false);
    component.turn_90_degree(1, IntPoint(0, 0));
    EXPECT_EQ(0.0, component.get_rotation());
    EXPECT_EQ(IntPoint(0, 100), component.get_location());
    component.translate_by(geometry::planar::IntVector(5, 5));
    EXPECT_EQ(IntPoint(5, 105), component.get_location());
}

TEST(ComponentTest, UnplacedComponentHasNoLocation)
{
    const Component component("J1", std::nullopt, 0.0, true, nullptr, nullptr, 1, false);
    EXPECT_FALSE(component.is_placed());
}

TEST(ComponentsTest, IdsAreOneBasedLikeJava)
{
    Components components;
    Component* u1 = components.add("U1", IntPoint(0, 0), 0.0, true, nullptr, nullptr, false);
    Component* u2 = components.add("U2", IntPoint(0, 0), 0.0, true, nullptr, nullptr, false);
    EXPECT_EQ(1, u1->get_id());
    EXPECT_EQ(2, u2->get_id());
    EXPECT_EQ(u1, components.get(1));
    EXPECT_EQ(u2, components.get(2));
    EXPECT_EQ(nullptr, components.get(0));
    EXPECT_EQ(nullptr, components.get(3));
    EXPECT_EQ(u2, components.get_by_name("U2"));
    EXPECT_EQ(2, components.component_count());
}

} // namespace
} // namespace freerouting::board
