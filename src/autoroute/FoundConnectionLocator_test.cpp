#include <gtest/gtest.h>

#include <map>
#include <memory>
#include <set>
#include <vector>

#include "autoroute/AutorouteControl.h"
#include "autoroute/ExpansionDrill.h"
#include "autoroute/ExpansionRoom.h"
#include "autoroute/FoundConnectionLocator.h"
#include "autoroute/FoundConnectionLocator45Degree.h"
#include "autoroute/FoundConnectionLocatorAnyAngle.h"
#include "autoroute/MazeSearchAlgo.h"
#include "board/FixedState.h"
#include "board/Layer.h"
#include "board/PolylineTrace.h"
#include "board/RoutingBoard.h"
#include "board/ShapeSearchTree.h"
#include "geometry/planar/FloatLine.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/Point.h"
#include "geometry/planar/Polyline.h"
#include "rules/BoardRules.h"

namespace fr = freerouting;
namespace fp = fr::geometry::planar;

namespace {

std::shared_ptr<fr::board::RoutingBoard> make_routing_board()
{
    std::vector<std::shared_ptr<fr::board::Layer>> layers;
    layers.push_back(std::make_shared<fr::board::Layer>("Top", true));
    layers.push_back(std::make_shared<fr::board::Layer>("Bottom", true));
    fr::board::LayerStructure layer_structure(layers);
    auto board_rules = std::make_unique<fr::rules::BoardRules>(layer_structure, 10);
    return std::make_shared<fr::board::RoutingBoard>(fp::IntBox(0, 0, 10000, 10000), layer_structure,
        std::move(board_rules), nullptr);
}

fr::board::PolylineTrace* insert_trace(fr::board::RoutingBoard* p_board,
    const std::vector<fp::IntPoint>& p_corners)
{
    auto polyline = std::make_shared<fp::Polyline>(fp::Polyline::from_corners(p_corners));
    return p_board->insert_trace_without_cleaning(polyline, 0, 50, {1}, 1,
        fr::board::FixedState::NOT_FIXED);
}

fr::autoroute::AutorouteControl make_control()
{
    fr::autoroute::AutorouteControl control;
    control.layer_count = 2;
    control.layer_active.assign(2, true);
    control.trace_half_width.assign(2, 50);
    control.compensated_trace_half_width.assign(2, 50);
    control.trace_costs.assign(2, fr::autoroute::ExpansionCostFactor(1.0, 1.0));
    control.bend_costs.assign(2, 0.0);
    control.net_no = 1;
    return control;
}

/**
 * A horizontal maze search path of a single layer: start item -- start room --
 * door1 -- middle room -- door2 -- destination room -- destination item.
 */
struct MazeFixture
{
    std::shared_ptr<fr::board::RoutingBoard> board = make_routing_board();
    std::unique_ptr<fr::board::ShapeSearchTree90Degree> tree;
    fr::board::PolylineTrace* start_trace = nullptr;
    fr::board::PolylineTrace* dest_trace = nullptr;
    fr::autoroute::CompleteFreeSpaceExpansionRoom start_room{0, 1, fp::IntBox(0, 450, 400, 550)};
    fr::autoroute::CompleteFreeSpaceExpansionRoom middle_room{0, 2, fp::IntBox(400, 450, 600, 550)};
    fr::autoroute::CompleteFreeSpaceExpansionRoom dest_room{0, 3, fp::IntBox(600, 450, 1000, 550)};
    std::unique_ptr<fr::autoroute::ExpansionDoor> door1;
    std::unique_ptr<fr::autoroute::ExpansionDoor> door2;
    std::unique_ptr<fr::autoroute::TargetItemExpansionDoor> start_door;
    std::unique_ptr<fr::autoroute::TargetItemExpansionDoor> dest_door;
    fr::autoroute::MazeSearchAlgo::Result result;

    /**
     * Creates the fixture. With p_overlap_rooms the neighbouring rooms overlap
     * by 100 units, which makes the doors two-dimensional, as they are for the
     * overlapping rooms of a real maze search. Doors between touching rooms are
     * one-dimensional, whose corners are duplicated in the IntBox representation.
     * p_room_half_height widens the rooms around the horizontal trace center line;
     * the any angle locator needs a door clearly wider than twice the trace half
     * width, otherwise no tangent line through the door exists.
     */
    explicit MazeFixture(bool p_overlap_rooms = false, int p_room_half_height = 50)
    {
        if (p_overlap_rooms || p_room_half_height != 50)
        {
            int y_min = 500 - p_room_half_height;
            int y_max = 500 + p_room_half_height;
            start_room.set_shape_tile(fp::IntBox(0, y_min, 400, y_max).to_Simplex());
            if (p_overlap_rooms)
            {
                middle_room.set_shape_tile(fp::IntBox(300, y_min, 700, y_max).to_Simplex());
                dest_room.set_shape_tile(fp::IntBox(600, y_min, 1000, y_max).to_Simplex());
            }
            else
            {
                middle_room.set_shape_tile(fp::IntBox(400, y_min, 600, y_max).to_Simplex());
                dest_room.set_shape_tile(fp::IntBox(600, y_min, 1000, y_max).to_Simplex());
            }
        }
    }

    /** Builds the maze search path; returns false if the setup is broken. */
    bool build()
    {
        tree = std::make_unique<fr::board::ShapeSearchTree90Degree>(board.get(), 0);
        start_trace = insert_trace(board.get(), {fp::IntPoint(50, 500), fp::IntPoint(350, 500)});
        dest_trace = insert_trace(board.get(), {fp::IntPoint(650, 500), fp::IntPoint(950, 500)});
        if (start_trace == nullptr || dest_trace == nullptr)
        {
            return false;
        }
        tree->insert(*start_trace);
        tree->insert(*dest_trace);

        door1 = std::make_unique<fr::autoroute::ExpansionDoor>(&start_room, &middle_room, 1);
        door2 = std::make_unique<fr::autoroute::ExpansionDoor>(&middle_room, &dest_room, 1);
        start_door = std::make_unique<fr::autoroute::TargetItemExpansionDoor>(&start_room, start_trace, 0,
            tree.get());
        dest_door = std::make_unique<fr::autoroute::TargetItemExpansionDoor>(&dest_room, dest_trace, 0, tree.get());

        // Allocate the sections of both doors and connect the backtrack chain:
        // destination door -> door2 -> door1 -> start door.
        if (door1->get_section_segments(50).empty() || door2->get_section_segments(50).empty())
        {
            return false;
        }

        fr::autoroute::MazeSearchElement* dest_element = dest_door->get_maze_search_element(0);
        fr::autoroute::MazeSearchElement* start_element = start_door->get_maze_search_element(0);
        fr::autoroute::MazeSearchElement* door1_element = door1->get_maze_search_element(0);
        fr::autoroute::MazeSearchElement* door2_element = door2->get_maze_search_element(0);
        if (dest_element == nullptr || start_element == nullptr || door1_element == nullptr
            || door2_element == nullptr)
        {
            return false;
        }
        dest_element->backtrack_door = door2.get();
        dest_element->section_no_of_backtrack_door = 0;
        door2_element->backtrack_door = door1.get();
        door2_element->section_no_of_backtrack_door = 0;
        door1_element->backtrack_door = start_door.get();
        door1_element->section_no_of_backtrack_door = 0;
        start_element->backtrack_door = nullptr;

        result.destination_door = dest_door.get();
        result.section_no_of_door = 0;
        return true;
    }
};

} // namespace

// ==========================================================================
// get_instance dispatch
// ==========================================================================

TEST(FoundConnectionLocatorTest, GetInstanceRejectsMissingResult)
{
    auto board = make_routing_board();
    fr::board::ShapeSearchTree90Degree tree(board.get(), 0);
    fr::autoroute::AutorouteControl control = make_control();

    EXPECT_EQ(fr::autoroute::FoundConnectionLocator::get_instance(nullptr, control, &tree,
        fr::board::AngleRestriction::FORTYFIVE_DEGREE), nullptr);
}

TEST(FoundConnectionLocatorTest, GetInstanceDispatchesOnAngleRestriction)
{
    MazeFixture fixture;
    ASSERT_TRUE(fixture.build());
    fr::autoroute::AutorouteControl control = make_control();

    std::unique_ptr<fr::autoroute::FoundConnectionLocator> forty_five =
        fr::autoroute::FoundConnectionLocator::get_instance(&fixture.result, control, fixture.tree.get(),
            fr::board::AngleRestriction::FORTYFIVE_DEGREE);
    ASSERT_NE(forty_five, nullptr);
    EXPECT_NE(dynamic_cast<fr::autoroute::FoundConnectionLocator45Degree*>(forty_five.get()), nullptr);
    EXPECT_EQ(dynamic_cast<fr::autoroute::FoundConnectionLocatorAnyAngle*>(forty_five.get()), nullptr);

    std::unique_ptr<fr::autoroute::FoundConnectionLocator> ninety =
        fr::autoroute::FoundConnectionLocator::get_instance(&fixture.result, control, fixture.tree.get(),
            fr::board::AngleRestriction::NINETY_DEGREE);
    ASSERT_NE(ninety, nullptr);
    EXPECT_NE(dynamic_cast<fr::autoroute::FoundConnectionLocator45Degree*>(ninety.get()), nullptr);

    std::unique_ptr<fr::autoroute::FoundConnectionLocator> any_angle =
        fr::autoroute::FoundConnectionLocator::get_instance(&fixture.result, control, fixture.tree.get(),
            fr::board::AngleRestriction::NONE);
    ASSERT_NE(any_angle, nullptr);
    EXPECT_NE(dynamic_cast<fr::autoroute::FoundConnectionLocatorAnyAngle*>(any_angle.get()), nullptr);
    EXPECT_EQ(dynamic_cast<fr::autoroute::FoundConnectionLocator45Degree*>(any_angle.get()), nullptr);
}

TEST(FoundConnectionLocatorTest, EmptyMazeResultIsNotValid)
{
    // Java raises an exception when the destination door is null, which the
    // caller of getInstance() maps to a null result. The C++ port reports the
    // same condition through is_valid().
    auto board = make_routing_board();
    fr::board::ShapeSearchTree90Degree tree(board.get(), 0);
    fr::autoroute::AutorouteControl control = make_control();
    fr::autoroute::MazeSearchAlgo::Result empty;

    std::unique_ptr<fr::autoroute::FoundConnectionLocator> locator =
        fr::autoroute::FoundConnectionLocator::get_instance(&empty, control, &tree,
            fr::board::AngleRestriction::FORTYFIVE_DEGREE);

    ASSERT_NE(locator, nullptr);
    EXPECT_FALSE(locator->is_valid());
    EXPECT_EQ(locator->start_item, nullptr);
    EXPECT_EQ(locator->target_item, nullptr);
    EXPECT_TRUE(locator->connection_items.empty());
}

TEST(FoundConnectionLocatorTest, ExpansionDrillDestinationWithoutStartDoorIsNotValid)
{
    // A drill as the destination door is the fanout end of a connection. If
    // the backtrack of such a fanout connection does not end in a start door,
    // the Java locator raises an exception, which the C++ port reports through
    // is_valid() instead.
    auto board = make_routing_board();
    fr::autoroute::AutorouteControl control = make_control();
    fr::autoroute::CompleteFreeSpaceExpansionRoom fanout_room(1, 7, fp::IntBox(0, 450, 400, 550));
    fr::autoroute::ExpansionDrill drill(fp::IntBox(2000, 2000, 2001, 2001), fp::IntPoint(2000, 2000), 0, 1);
    drill.room_arr.assign(2, &fanout_room);
    ASSERT_NE(drill.get_maze_search_element(0), nullptr);

    fr::autoroute::MazeSearchAlgo::Result result;
    result.destination_door = &drill;
    result.section_no_of_door = 0;

    std::unique_ptr<fr::autoroute::FoundConnectionLocator> locator =
        fr::autoroute::FoundConnectionLocator::get_instance(&result, control, nullptr,
            fr::board::AngleRestriction::FORTYFIVE_DEGREE);

    ASSERT_NE(locator, nullptr);
    EXPECT_FALSE(locator->is_valid());
    EXPECT_EQ(locator->target_item, nullptr);
    EXPECT_EQ(locator->target_layer, 0);
    ASSERT_EQ(locator->backtrack_array().size(), 1u);
    EXPECT_EQ(locator->backtrack_array()[0].door, static_cast<fr::autoroute::ExpandableObject*>(&drill));
    EXPECT_EQ(locator->backtrack_array()[0].next_room,
        static_cast<fr::autoroute::CompleteExpansionRoom*>(&fanout_room));
}

// ==========================================================================
// calculate_additional_corner
// ==========================================================================

TEST(FoundConnectionLocatorTest, AdditionalCornerNinetyDegree)
{
    fp::FloatPoint from(0.0, 0.0);
    fp::FloatPoint to(100.0, 50.0);

    EXPECT_EQ(fr::autoroute::FoundConnectionLocator::calculate_additional_corner(from, to, true,
        fr::board::AngleRestriction::NINETY_DEGREE), fp::FloatPoint(100.0, 0.0));
    EXPECT_EQ(fr::autoroute::FoundConnectionLocator::calculate_additional_corner(from, to, false,
        fr::board::AngleRestriction::NINETY_DEGREE), fp::FloatPoint(0.0, 50.0));
}

TEST(FoundConnectionLocatorTest, AdditionalCornerFortyFiveDegree)
{
    fp::FloatPoint from(0.0, 0.0);
    fp::FloatPoint to(100.0, 50.0);

    // abs(dx) > abs(dy): horizontal first keeps the y of the from point.
    EXPECT_EQ(fr::autoroute::FoundConnectionLocator::calculate_additional_corner(from, to, true,
        fr::board::AngleRestriction::FORTYFIVE_DEGREE), fp::FloatPoint(50.0, 0.0));
    EXPECT_EQ(fr::autoroute::FoundConnectionLocator::calculate_additional_corner(from, to, false,
        fr::board::AngleRestriction::FORTYFIVE_DEGREE), fp::FloatPoint(50.0, 50.0));

    fp::FloatPoint steep(0.0, 0.0);
    fp::FloatPoint steep_to(50.0, 100.0);
    // abs(dx) <= abs(dy): vertical first keeps the x of the from point.
    EXPECT_EQ(fr::autoroute::FoundConnectionLocator::calculate_additional_corner(steep, steep_to, true,
        fr::board::AngleRestriction::FORTYFIVE_DEGREE), fp::FloatPoint(50.0, 50.0));
}

TEST(FoundConnectionLocatorTest, AdditionalCornerWithoutRestrictionIsTheTarget)
{
    fp::FloatPoint from(0.0, 0.0);
    fp::FloatPoint to(100.0, 50.0);

    EXPECT_EQ(fr::autoroute::FoundConnectionLocator::calculate_additional_corner(from, to, true,
        fr::board::AngleRestriction::NONE), to);
}

// ==========================================================================
// 45 degree locator
// ==========================================================================

TEST(FoundConnectionLocatorTest, FortyFiveDegreeLocatorBuildsTraceThroughRooms)
{
    MazeFixture fixture;
    ASSERT_TRUE(fixture.build());
    fr::autoroute::AutorouteControl control = make_control();

    std::unique_ptr<fr::autoroute::FoundConnectionLocator> locator =
        fr::autoroute::FoundConnectionLocator::get_instance(&fixture.result, control, fixture.tree.get(),
            fr::board::AngleRestriction::FORTYFIVE_DEGREE);

    ASSERT_NE(locator, nullptr);
    ASSERT_TRUE(locator->is_valid());
    EXPECT_EQ(locator->start_item, static_cast<fr::board::Item*>(fixture.start_trace));
    EXPECT_EQ(locator->target_item, static_cast<fr::board::Item*>(fixture.dest_trace));
    EXPECT_EQ(locator->start_layer, 0);
    EXPECT_EQ(locator->target_layer, 0);

    // The backtrack array runs from the destination door to the start door.
    const auto& backtrack = locator->backtrack_array();
    ASSERT_EQ(backtrack.size(), 4u);
    EXPECT_EQ(backtrack[0].door, static_cast<fr::autoroute::ExpandableObject*>(fixture.dest_door.get()));
    EXPECT_EQ(backtrack[1].door, static_cast<fr::autoroute::ExpandableObject*>(fixture.door2.get()));
    EXPECT_EQ(backtrack[2].door, static_cast<fr::autoroute::ExpandableObject*>(fixture.door1.get()));
    EXPECT_EQ(backtrack[3].door, static_cast<fr::autoroute::ExpandableObject*>(fixture.start_door.get()));
    EXPECT_EQ(backtrack[0].next_room, static_cast<fr::autoroute::CompleteExpansionRoom*>(&fixture.dest_room));
    EXPECT_EQ(backtrack[1].next_room, static_cast<fr::autoroute::CompleteExpansionRoom*>(&fixture.middle_room));
    EXPECT_EQ(backtrack[2].next_room, static_cast<fr::autoroute::CompleteExpansionRoom*>(&fixture.start_room));
    EXPECT_EQ(backtrack[3].next_room, nullptr);

    // A single layer connection produces exactly one trace item on layer 0.
    ASSERT_EQ(locator->connection_items.size(), 1u);
    const auto& trace = locator->connection_items.front();
    EXPECT_EQ(trace.layer, 0);
    ASSERT_GE(trace.corners.size(), 3u);

    // The trace starts inside the destination room and ends inside the start
    // item, crossing both doors.
    EXPECT_GT(trace.corners.front().x, 600);
    EXPECT_LE(trace.corners.back().x, 400);
    bool crosses_door2 = false;
    bool crosses_door1 = false;
    for (size_t i = 1; i < trace.corners.size(); i++)
    {
        int64_t from_x = trace.corners[i - 1].x;
        int64_t to_x = trace.corners[i].x;
        if ((from_x - 600) * (to_x - 600) <= 0)
        {
            crosses_door2 = true;
        }
        if ((from_x - 400) * (to_x - 400) <= 0)
        {
            crosses_door1 = true;
        }
    }
    EXPECT_TRUE(crosses_door2);
    EXPECT_TRUE(crosses_door1);

    // Every 45 degree step is either horizontal or vertical or 45 degrees.
    for (size_t i = 1; i < trace.corners.size(); i++)
    {
        int64_t dx = trace.corners[i].x - trace.corners[i - 1].x;
        int64_t dy = trace.corners[i].y - trace.corners[i - 1].y;
        EXPECT_TRUE(dx == 0 || dy == 0 || std::abs(dx) == std::abs(dy));
    }
}

TEST(FoundConnectionLocatorTest, NinetyDegreeLocatorProducesOrthogonalSteps)
{
    MazeFixture fixture;
    ASSERT_TRUE(fixture.build());
    fr::autoroute::AutorouteControl control = make_control();

    std::unique_ptr<fr::autoroute::FoundConnectionLocator> locator =
        fr::autoroute::FoundConnectionLocator::get_instance(&fixture.result, control, fixture.tree.get(),
            fr::board::AngleRestriction::NINETY_DEGREE);

    ASSERT_NE(locator, nullptr);
    ASSERT_TRUE(locator->is_valid());
    ASSERT_EQ(locator->connection_items.size(), 1u);
    const auto& trace = locator->connection_items.front();
    ASSERT_GE(trace.corners.size(), 2u);
    for (size_t i = 1; i < trace.corners.size(); i++)
    {
        int64_t dx = trace.corners[i].x - trace.corners[i - 1].x;
        int64_t dy = trace.corners[i].y - trace.corners[i - 1].y;
        EXPECT_TRUE(dx == 0 || dy == 0);
    }
}

TEST(FoundConnectionLocatorTest, AnyAngleLocatorProducesTraceThroughRooms)
{
    MazeFixture fixture(true, 100);
    ASSERT_TRUE(fixture.build());
    fr::autoroute::AutorouteControl control = make_control();

    std::unique_ptr<fr::autoroute::FoundConnectionLocator> locator =
        fr::autoroute::FoundConnectionLocator::get_instance(&fixture.result, control, fixture.tree.get(),
            fr::board::AngleRestriction::NONE);

    ASSERT_NE(locator, nullptr);
    ASSERT_TRUE(locator->is_valid());
    EXPECT_EQ(locator->start_item, static_cast<fr::board::Item*>(fixture.start_trace));
    EXPECT_EQ(locator->target_item, static_cast<fr::board::Item*>(fixture.dest_trace));
    ASSERT_EQ(locator->connection_items.size(), 1u);
    const auto& trace = locator->connection_items.front();
    EXPECT_EQ(trace.layer, 0);
    ASSERT_GE(trace.corners.size(), 2u);
    EXPECT_GT(trace.corners.front().x, 600);
    EXPECT_LE(trace.corners.back().x, 400);
}

TEST(FoundConnectionLocatorTest, AdjustStartCornerStaysInsideTheRoom)
{
    // A start point outside of the shrunk start room is moved to the nearest
    // point inside it, so that the trace does not leave the room.
    MazeFixture fixture;
    ASSERT_TRUE(fixture.build());
    fr::autoroute::AutorouteControl control = make_control();

    std::unique_ptr<fr::autoroute::FoundConnectionLocator> locator =
        fr::autoroute::FoundConnectionLocator::get_instance(&fixture.result, control, fixture.tree.get(),
            fr::board::AngleRestriction::FORTYFIVE_DEGREE);

    ASSERT_NE(locator, nullptr);
    ASSERT_TRUE(locator->is_valid());
    const auto& trace = locator->connection_items.front();
    ASSERT_GE(trace.corners.size(), 2u);
    for (const fp::IntPoint& corner : trace.corners)
    {
        // The whole trace is inside the union of the three rooms.
        fp::IntBox rooms = fixture.start_room.get_shape().union_with(fixture.middle_room.get_shape());
        fp::IntBox all_rooms = rooms.union_with(fixture.dest_room.get_shape());
        EXPECT_TRUE(all_rooms.contains(corner));
    }
}

TEST(FoundConnectionLocatorTest, BacktrackCollectsRippedObstacleRooms)
{
    // A ripped obstacle room on the backtrack path is reported through the
    // ripped item list and the ripup costs map, like in the Java locator.
    auto board = make_routing_board();
    fr::autoroute::AutorouteControl control = make_control();
    fr::board::ShapeSearchTree90Degree tree(board.get(), 0);
    fr::board::PolylineTrace* obstacle_trace =
        insert_trace(board.get(), {fp::IntPoint(100, 100), fp::IntPoint(300, 100)});
    fr::board::PolylineTrace* dest_trace =
        insert_trace(board.get(), {fp::IntPoint(700, 100), fp::IntPoint(900, 100)});
    ASSERT_NE(obstacle_trace, nullptr);
    ASSERT_NE(dest_trace, nullptr);
    tree.insert(*obstacle_trace);
    tree.insert(*dest_trace);

    fr::autoroute::ObstacleExpansionRoom obstacle_room(obstacle_trace, 0, 0, fp::IntBox(50, 50, 350, 150));
    fr::autoroute::CompleteFreeSpaceExpansionRoom obstacle_front_room(0, 2, fp::IntBox(350, 50, 600, 150));
    fr::autoroute::CompleteFreeSpaceExpansionRoom dest_room(0, 3, fp::IntBox(600, 50, 1000, 150));
    fr::autoroute::ExpansionDoor obstacle_door(&obstacle_front_room, &obstacle_room, 1);
    fr::autoroute::ExpansionDoor dest_door_plain(&obstacle_front_room, &dest_room, 1);
    ASSERT_FALSE(obstacle_door.get_section_segments(50).empty());
    ASSERT_FALSE(dest_door_plain.get_section_segments(50).empty());

    // The destination of this path is a target door of the destination item,
    // its room is the free space room behind the ripped obstacle room.
    fr::autoroute::TargetItemExpansionDoor dest_door(&dest_room, dest_trace, 0, &tree);

    fr::autoroute::MazeSearchElement* obstacle_door_element = obstacle_door.get_maze_search_element(0);
    ASSERT_NE(obstacle_door_element, nullptr);
    obstacle_door_element->backtrack_door = nullptr;
    obstacle_door_element->room_ripped = true;
    obstacle_door_element->ripup_cost = 17;

    fr::autoroute::MazeSearchElement* plain_door_element = dest_door_plain.get_maze_search_element(0);
    ASSERT_NE(plain_door_element, nullptr);
    plain_door_element->backtrack_door = &obstacle_door;
    plain_door_element->section_no_of_backtrack_door = 0;
    plain_door_element->room_ripped = true;
    plain_door_element->ripup_cost = 17;

    fr::autoroute::MazeSearchElement* dest_element = dest_door.get_maze_search_element(0);
    ASSERT_NE(dest_element, nullptr);
    dest_element->backtrack_door = &dest_door_plain;
    dest_element->section_no_of_backtrack_door = 0;

    fr::autoroute::MazeSearchAlgo::Result result;
    result.destination_door = &dest_door;
    result.section_no_of_door = 0;

    fr::board::ItemSet ripped_items;
    std::map<fr::board::Item*, int> ripup_costs;
    std::unique_ptr<fr::autoroute::FoundConnectionLocator> locator =
        fr::autoroute::FoundConnectionLocator::get_instance(&result, control, &tree,
            fr::board::AngleRestriction::FORTYFIVE_DEGREE, &ripped_items, &ripup_costs);

    ASSERT_NE(locator, nullptr);
    // The backtrack ends in a door, not in a target door, so the locator is invalid.
    EXPECT_FALSE(locator->is_valid());
    const auto& backtrack = locator->backtrack_array();
    ASSERT_EQ(backtrack.size(), 3u);
    EXPECT_EQ(backtrack[0].door, static_cast<fr::autoroute::ExpandableObject*>(&dest_door));
    EXPECT_EQ(backtrack[1].door, static_cast<fr::autoroute::ExpandableObject*>(&dest_door_plain));
    EXPECT_EQ(backtrack[2].door, static_cast<fr::autoroute::ExpandableObject*>(&obstacle_door));
    EXPECT_EQ(backtrack[2].next_room, static_cast<fr::autoroute::CompleteExpansionRoom*>(&obstacle_room));
    // The obstacle room in front of the ripped door is reported as ripped.
    EXPECT_EQ(ripped_items.count(obstacle_trace), 1u);
    ASSERT_EQ(ripup_costs.count(obstacle_trace), 1u);
    EXPECT_EQ(ripup_costs[obstacle_trace], 17);
}

TEST(FoundConnectionLocatorTest, BacktrackSectionNumberIsClampedToTheDoorSectionCount)
{
    // A section number beyond the section count of the backtrack door is
    // clamped, like in the Java locator.
    auto board = make_routing_board();
    fr::autoroute::AutorouteControl control = make_control();

    fr::autoroute::CompleteFreeSpaceExpansionRoom room_a(0, 1, fp::IntBox(0, 0, 100, 100));
    fr::autoroute::CompleteFreeSpaceExpansionRoom room_b(0, 2, fp::IntBox(100, 0, 200, 100));
    fr::autoroute::CompleteFreeSpaceExpansionRoom room_c(0, 3, fp::IntBox(200, 0, 300, 100));
    fr::autoroute::ExpansionDoor door1(&room_a, &room_b, 1);
    fr::autoroute::ExpansionDoor door2(&room_b, &room_c, 1);
    ASSERT_FALSE(door1.get_section_segments(50).empty());
    ASSERT_FALSE(door2.get_section_segments(50).empty());

    fr::autoroute::MazeSearchElement* door1_element = door1.get_maze_search_element(0);
    ASSERT_NE(door1_element, nullptr);
    door1_element->backtrack_door = nullptr;

    // The section number of the backtrack door is far too large and gets
    // clamped to the last section of door1.
    fr::autoroute::MazeSearchElement* door2_element = door2.get_maze_search_element(0);
    ASSERT_NE(door2_element, nullptr);
    door2_element->backtrack_door = &door1;
    door2_element->section_no_of_backtrack_door = 99;

    fr::autoroute::MazeSearchAlgo::Result result;
    result.destination_door = &door2;
    result.section_no_of_door = 0;

    std::unique_ptr<fr::autoroute::FoundConnectionLocator> locator =
        fr::autoroute::FoundConnectionLocator::get_instance(&result, control, nullptr,
            fr::board::AngleRestriction::FORTYFIVE_DEGREE);

    ASSERT_NE(locator, nullptr);
    EXPECT_FALSE(locator->is_valid());
    ASSERT_EQ(locator->backtrack_array().size(), 2u);
    EXPECT_EQ(locator->backtrack_array()[1].section_no_of_door, 0);
    EXPECT_EQ(locator->backtrack_array()[1].door, static_cast<fr::autoroute::ExpandableObject*>(&door1));
}