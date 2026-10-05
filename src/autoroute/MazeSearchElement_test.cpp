#include "autoroute/MazeSearchElement.h"

#include "autoroute/ExpansionRoom.h"
#include "geometry/planar/FloatLine.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/Point.h"

#include <gtest/gtest.h>

#include <set>
#include <vector>

namespace freerouting {
namespace autoroute {

namespace {

geometry::planar::FloatLine line_at(double p_x, double p_y)
{
    geometry::planar::FloatPoint point(p_x, p_y);
    return geometry::planar::FloatLine(point, point);
}

/** Three rooms and two doors with different room pairs, hence different ids. */
struct DoorFixture
{
    CompleteFreeSpaceExpansionRoom room_a{0, 11, geometry::planar::IntBox(0, 0, 100, 100)};
    CompleteFreeSpaceExpansionRoom room_b{0, 22, geometry::planar::IntBox(100, 0, 200, 100)};
    CompleteFreeSpaceExpansionRoom room_c{0, 33, geometry::planar::IntBox(200, 0, 300, 100)};
    ExpansionDoor door_a{&room_a, &room_b, 1};
    ExpansionDoor door_b{&room_b, &room_c, 1};

    int64_t id_a = door_a.get_id();
    int64_t id_b = door_b.get_id();
};

} // namespace

// ==========================================================================
// MazeSearchElement
// ==========================================================================

TEST(MazeSearchElementTest, DefaultState) {
    MazeSearchElement element;
    EXPECT_FALSE(element.is_occupied);
    EXPECT_EQ(element.backtrack_door, nullptr);
    EXPECT_EQ(element.section_no_of_backtrack_door, 0);
    EXPECT_FALSE(element.room_ripped);
    EXPECT_EQ(element.ripup_cost, 0);
    EXPECT_EQ(element.adjustment, MazeSearchElement::Adjustment::NONE);
}

TEST(MazeSearchElementTest, ResetClearsBacktrackState) {
    MazeSearchElement element;
    element.is_occupied = true;
    element.room_ripped = true;
    element.ripup_cost = 7;
    element.adjustment = MazeSearchElement::Adjustment::LEFT;
    element.section_no_of_backtrack_door = 3;

    element.reset();

    EXPECT_FALSE(element.is_occupied);
    EXPECT_EQ(element.backtrack_door, nullptr);
    EXPECT_EQ(element.section_no_of_backtrack_door, 0);
    EXPECT_FALSE(element.room_ripped);
    EXPECT_EQ(element.ripup_cost, 0);
    EXPECT_EQ(element.adjustment, MazeSearchElement::Adjustment::NONE);
}

TEST(MazeSearchElementTest, ResetSectionArray) {
    std::vector<MazeSearchElement> arr(3);
    for (auto& element : arr) {
        element.is_occupied = true;
        element.ripup_cost = 5;
    }

    MazeSearchElement::reset_section_array(arr);

    for (const auto& element : arr) {
        EXPECT_FALSE(element.is_occupied);
        EXPECT_EQ(element.ripup_cost, 0);
    }
}

// ==========================================================================
// MazeListElement
// ==========================================================================

TEST(MazeListElementTest, ConstructorStoresAllFields) {
    DoorFixture fixture;
    MazeListElement element(&fixture.door_a, 2, &fixture.door_b, 1, 3.5, 4.5, &fixture.room_b, line_at(10, 20),
        true, MazeSearchElement::Adjustment::RIGHT, true);

    EXPECT_EQ(element.door, &fixture.door_a);
    EXPECT_EQ(element.section_no_of_door, 2);
    EXPECT_EQ(element.backtrack_door, &fixture.door_b);
    EXPECT_EQ(element.section_no_of_backtrack_door, 1);
    EXPECT_DOUBLE_EQ(element.expansion_value, 3.5);
    EXPECT_DOUBLE_EQ(element.sorting_value, 4.5);
    EXPECT_EQ(element.next_room, &fixture.room_b);
    EXPECT_TRUE(element.room_ripped);
    EXPECT_EQ(element.adjustment, MazeSearchElement::Adjustment::RIGHT);
    EXPECT_TRUE(element.already_checked);
    EXPECT_EQ(element.ripup_cost, 0);
}

TEST(MazeListElementTest, CompareToSortsBySortingValueFirst) {
    DoorFixture fixture;
    MazeListElement low(&fixture.door_b, 0, nullptr, 0, 10.0, 10.0, &fixture.room_b, line_at(0, 0), false,
        MazeSearchElement::Adjustment::NONE, false);
    MazeListElement high(&fixture.door_a, 0, nullptr, 0, 1.0, 20.0, &fixture.room_a, line_at(0, 0), false,
        MazeSearchElement::Adjustment::NONE, false);

    EXPECT_LT(low.compare_to(high), 0);
    EXPECT_GT(high.compare_to(low), 0);
}

TEST(MazeListElementTest, CompareToBreaksTieWithExpansionValue) {
    DoorFixture fixture;
    MazeListElement low(&fixture.door_b, 0, nullptr, 0, 5.0, 10.0, &fixture.room_b, line_at(0, 0), false,
        MazeSearchElement::Adjustment::NONE, false);
    MazeListElement high(&fixture.door_a, 0, nullptr, 0, 6.0, 10.0, &fixture.room_a, line_at(0, 0), false,
        MazeSearchElement::Adjustment::NONE, false);

    EXPECT_LT(low.compare_to(high), 0);
    EXPECT_GT(high.compare_to(low), 0);
}

TEST(MazeListElementTest, CompareToBreaksTieWithDoorId) {
    DoorFixture fixture;
    ASSERT_LT(fixture.id_a, fixture.id_b);
    MazeListElement by_a(&fixture.door_a, 0, nullptr, 0, 1.0, 1.0, &fixture.room_b, line_at(0, 0), false,
        MazeSearchElement::Adjustment::NONE, false);
    MazeListElement by_b(&fixture.door_b, 0, nullptr, 0, 1.0, 1.0, &fixture.room_b, line_at(0, 0), false,
        MazeSearchElement::Adjustment::NONE, false);

    EXPECT_LT(by_a.compare_to(by_b), 0);
    EXPECT_GT(by_b.compare_to(by_a), 0);
}

TEST(MazeListElementTest, CompareToBreaksTieWithSectionNumber) {
    DoorFixture fixture;
    MazeListElement first(&fixture.door_a, 0, nullptr, 0, 1.0, 1.0, &fixture.room_b, line_at(0, 0), false,
        MazeSearchElement::Adjustment::NONE, false);
    MazeListElement second(&fixture.door_a, 1, nullptr, 0, 1.0, 1.0, &fixture.room_b, line_at(0, 0), false,
        MazeSearchElement::Adjustment::NONE, false);

    EXPECT_LT(first.compare_to(second), 0);
    EXPECT_GT(second.compare_to(first), 0);
}

TEST(MazeListElementTest, CompareToIsZeroForIdenticalEntries) {
    DoorFixture fixture;
    MazeListElement one(&fixture.door_a, 3, nullptr, 0, 1.0, 1.0, &fixture.room_b, line_at(0, 0), false,
        MazeSearchElement::Adjustment::NONE, false);
    MazeListElement two(&fixture.door_a, 3, nullptr, 0, 1.0, 1.0, &fixture.room_b, line_at(0, 0), false,
        MazeSearchElement::Adjustment::NONE, false);

    EXPECT_EQ(one.compare_to(two), 0);
}

TEST(MazeListElementTest, EqualEntriesCollapseInAJavaTreeSet) {
    // Java uses a TreeSet<MazeListElement>, so comparing equal elements are the
    // same list entry. The C++ port uses a std::set, which must behave the same.
    DoorFixture fixture;
    MazeListElement one(&fixture.door_a, 0, nullptr, 0, 1.0, 1.0, &fixture.room_b, line_at(0, 0), false,
        MazeSearchElement::Adjustment::NONE, false);
    MazeListElement two(&fixture.door_a, 0, nullptr, 0, 1.0, 1.0, &fixture.room_b, line_at(0, 0), false,
        MazeSearchElement::Adjustment::NONE, false);

    std::set<MazeListElement> list;
    list.insert(one);
    list.insert(two);
    EXPECT_EQ(list.size(), 1u);
}

TEST(MazeListElementTest, SetIsOrderedBySortingValue) {
    DoorFixture fixture;
    std::set<MazeListElement> list;
    list.insert(MazeListElement(&fixture.door_b, 0, nullptr, 0, 9.0, 30.0, &fixture.room_b, line_at(0, 0),
        false, MazeSearchElement::Adjustment::NONE, false));
    list.insert(MazeListElement(&fixture.door_b, 1, nullptr, 0, 1.0, 10.0, &fixture.room_b, line_at(0, 0),
        false, MazeSearchElement::Adjustment::NONE, false));
    list.insert(MazeListElement(&fixture.door_b, 2, nullptr, 0, 2.0, 20.0, &fixture.room_b, line_at(0, 0),
        false, MazeSearchElement::Adjustment::NONE, false));

    ASSERT_EQ(list.size(), 3u);
    auto it = list.begin();
    EXPECT_DOUBLE_EQ(it->sorting_value, 10.0);
    ++it;
    EXPECT_DOUBLE_EQ(it->sorting_value, 20.0);
    ++it;
    EXPECT_DOUBLE_EQ(it->sorting_value, 30.0);
}

TEST(MazeListElementTest, OrderingIsDeterministicForEqualSortingValues) {
    // The Java search relies on a deterministic list order, otherwise equal
    // expansion paths would be visited in an arbitrary order.
    DoorFixture fixture;
    auto make = [&fixture](ExpandableObject* p_door, int p_section) {
        return MazeListElement(p_door, p_section, nullptr, 0, 1.0, 1.0, &fixture.room_b, line_at(0, 0), false,
            MazeSearchElement::Adjustment::NONE, false);
    };

    std::set<MazeListElement> first_set;
    first_set.insert(make(&fixture.door_b, 1));
    first_set.insert(make(&fixture.door_a, 0));
    first_set.insert(make(&fixture.door_a, 1));

    std::set<MazeListElement> second_set;
    second_set.insert(make(&fixture.door_a, 1));
    second_set.insert(make(&fixture.door_b, 1));
    second_set.insert(make(&fixture.door_a, 0));

    ASSERT_EQ(first_set.size(), second_set.size());
    auto first_it = first_set.begin();
    auto second_it = second_set.begin();
    while (first_it != first_set.end() && second_it != second_set.end()) {
        EXPECT_EQ(first_it->door->get_id(), second_it->door->get_id());
        EXPECT_EQ(first_it->section_no_of_door, second_it->section_no_of_door);
        ++first_it;
        ++second_it;
    }
}

} // namespace autoroute
} // namespace freerouting
