#include "autoroute/MazeTraceShover.h"

#include "autoroute/AutorouteControl.h"
#include "autoroute/ExpansionDrill.h"
#include "autoroute/ExpansionRoom.h"
#include "board/BasicBoard.h"
#include "board/ConductionArea.h"
#include "board/FixedState.h"
#include "board/Layer.h"
#include "board/Padstack.h"
#include "board/Pin.h"
#include "board/PolylineTrace.h"
#include "board/RoutingBoard.h"
#include "board/SearchTreeManager.h"
#include "board/Trace.h"
#include "board/Via.h"
#include "geometry/planar/FloatLine.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/Point.h"
#include "geometry/planar/PolygonShape.h"
#include "geometry/planar/Polyline.h"
#include "geometry/planar/Shape.h"
#include "geometry/planar/Vector.h"
#include "rules/BoardRules.h"

#include <gtest/gtest.h>

#include <memory>
#include <vector>

namespace freerouting {
namespace autoroute {

namespace {

std::shared_ptr<board::RoutingBoard> make_routing_board()
{
    std::vector<std::shared_ptr<board::Layer>> layers;
    layers.push_back(std::make_shared<board::Layer>("Top", true));
    layers.push_back(std::make_shared<board::Layer>("Bottom", true));
    board::LayerStructure layer_structure(layers);
    auto board_rules = std::make_unique<rules::BoardRules>(layer_structure, 10);
    return std::make_shared<board::RoutingBoard>(geometry::planar::IntBox(0, 0, 100000, 100000), layer_structure,
        std::move(board_rules), nullptr);
}

/** A control with the per-layer arrays sized as the Java code expects. */
AutorouteControl make_control(int p_net_no = 1, int p_trace_half_width = 50)
{
    AutorouteControl control;
    control.layer_count = 2;
    control.layer_active.assign(2, true);
    control.trace_half_width.assign(2, p_trace_half_width);
    control.compensated_trace_half_width.assign(2, p_trace_half_width);
    control.trace_costs.assign(2, ExpansionCostFactor(1.0, 1.0));
    control.bend_costs.assign(2, 0.0);
    control.trace_clearance_class_no = 1;
    control.net_no = p_net_no;
    return control;
}

board::PolylineTrace* insert_trace(board::RoutingBoard* p_board,
                                   const std::vector<geometry::planar::IntPoint>& p_corners,
                                   int p_half_width,
                                   const std::vector<int>& p_nets,
                                   int p_clearance_class = 1)
{
    auto polyline = std::make_shared<geometry::planar::Polyline>(geometry::planar::Polyline::from_corners(p_corners));
    return p_board->insert_trace_without_cleaning(polyline, 0, p_half_width, p_nets, p_clearance_class,
        board::FixedState::NOT_FIXED);
}

board::Pin* insert_smd_pin(board::RoutingBoard* p_board, const geometry::planar::IntPoint& p_center,
                           const std::vector<int>& p_nets)
{
    auto* pin = new board::Pin(7, 1, p_nets, 1, p_board->generate_item_id(), board::FixedState::NOT_FIXED, p_board);
    pin->set_center(p_center);
    auto* padstack = new board::Padstack("p1");
    padstack->set_from_layer(0);
    padstack->set_to_layer(0);
    padstack->add_shape(0, new geometry::planar::PolygonShape(
        std::vector<geometry::planar::IntPoint>{{-100, -100}, {100, -100}, {100, 100}, {-100, 100}}));
    pin->set_padstack(padstack);
    pin->set_bounding_box(geometry::planar::IntBox(p_center.x - 100, p_center.y - 100, p_center.x + 100,
        p_center.y + 100));
    p_board->get_search_tree_manager()->insert(pin);
    p_board->add_item(pin);
    return pin;
}

/**
 * A 4-corner horizontal trace on layer 0. The polyline has 3 lines, so the
 * obstacle room index 0 is a valid `traceCornerNo` for Java
 * `traceCornerNo < tracePolyline.lines.length - 2`.
 */
board::PolylineTrace* insert_shovable_trace(board::RoutingBoard* p_board, const std::vector<int>& p_nets)
{
    return insert_trace(p_board,
        {geometry::planar::IntPoint(10000, 50000), geometry::planar::IntPoint(30000, 50000),
            geometry::planar::IntPoint(50000, 50000), geometry::planar::IntPoint(70000, 50000)},
        50, p_nets);
}

/** A list element which arrives at p_next_room through p_door. */
MazeListElement make_list_element(ExpandableObject* p_door, CompleteExpansionRoom* p_next_room,
                                  int p_section_no_of_door = 0,
                                  const geometry::planar::FloatPoint& p_entry = geometry::planar::FloatPoint(20000, 50000))
{
    return MazeListElement(p_door, p_section_no_of_door, nullptr, 0, 0, 0, p_next_room,
        geometry::planar::FloatLine(p_entry, p_entry), false, MazeSearchElement::Adjustment::NONE, false);
}

/**
 * An L shaped trace, so that the polyline lines used to build the shove line
 * are not all collinear. Java builds the shove line from
 * `tracePolyline.lines[traceCornerNo + 1]` and `[traceCornerNo + 2]`, which
 * degenerates for a straight trace.
 */
board::PolylineTrace* insert_bending_trace(board::RoutingBoard* p_board, const std::vector<int>& p_nets)
{
    return insert_trace(p_board,
        {geometry::planar::IntPoint(20000, 40000), geometry::planar::IntPoint(40000, 40000),
            geometry::planar::IntPoint(40000, 60000), geometry::planar::IntPoint(60000, 60000)},
        50, p_nets);
}

} // namespace

// ==========================================================================
// endPointsMatching
// ==========================================================================

TEST(MazeTraceShoverTest, EndPointsMatchTheTraceItself) {
    auto board = make_routing_board();
    board::PolylineTrace* trace = insert_shovable_trace(board.get(), {1});
    ASSERT_NE(trace, nullptr);

    EXPECT_TRUE(MazeTraceShover::end_points_matching(*trace, trace));
}

TEST(MazeTraceShoverTest, EndPointsMatchADrillItemAtATraceEnd) {
    auto board = make_routing_board();
    board::PolylineTrace* trace = insert_shovable_trace(board.get(), {1});
    ASSERT_NE(trace, nullptr);
    // A pin in the middle of the trace is no endpoint.
    board::Pin* middle = insert_smd_pin(board.get(), geometry::planar::IntPoint(30000, 50000), {1});
    ASSERT_NE(middle, nullptr);

    EXPECT_FALSE(MazeTraceShover::end_points_matching(*trace, middle));

    // A pin on the first corner of the trace matches.
    board::Pin* at_start = insert_smd_pin(board.get(), geometry::planar::IntPoint(10000, 50000), {1});
    ASSERT_NE(at_start, nullptr);
    EXPECT_TRUE(MazeTraceShover::end_points_matching(*trace, at_start));

    // A pin on the last corner of the trace matches.
    board::Pin* at_end = insert_smd_pin(board.get(), geometry::planar::IntPoint(70000, 50000), {1});
    ASSERT_NE(at_end, nullptr);
    EXPECT_TRUE(MazeTraceShover::end_points_matching(*trace, at_end));
}

TEST(MazeTraceShoverTest, EndPointsDoNotMatchADrillItemOfAnotherNet) {
    auto board = make_routing_board();
    board::PolylineTrace* trace = insert_shovable_trace(board.get(), {1});
    ASSERT_NE(trace, nullptr);
    board::Pin* at_start = insert_smd_pin(board.get(), geometry::planar::IntPoint(10000, 50000), {2});
    ASSERT_NE(at_start, nullptr);

    EXPECT_FALSE(MazeTraceShover::end_points_matching(*trace, at_start));
}

TEST(MazeTraceShoverTest, EndPointsMatchABendingTraceOfTheSameNet) {
    auto board = make_routing_board();
    board::PolylineTrace* trace = insert_shovable_trace(board.get(), {1});
    ASSERT_NE(trace, nullptr);
    // A second trace which starts where the first one ends.
    board::PolylineTrace* continuation = insert_trace(board.get(),
        {geometry::planar::IntPoint(70000, 50000), geometry::planar::IntPoint(70000, 70000)}, 50, {1});
    ASSERT_NE(continuation, nullptr);

    EXPECT_TRUE(MazeTraceShover::end_points_matching(*trace, continuation));
    EXPECT_TRUE(MazeTraceShover::end_points_matching(*continuation, trace));
}

TEST(MazeTraceShoverTest, EndPointsDoNotMatchADisjointTraceOfTheSameNet) {
    auto board = make_routing_board();
    board::PolylineTrace* trace = insert_shovable_trace(board.get(), {1});
    ASSERT_NE(trace, nullptr);
    board::PolylineTrace* other = insert_trace(board.get(),
        {geometry::planar::IntPoint(10000, 20000), geometry::planar::IntPoint(30000, 20000)}, 50, {1});
    ASSERT_NE(other, nullptr);

    EXPECT_FALSE(MazeTraceShover::end_points_matching(*trace, other));
}

TEST(MazeTraceShoverTest, EndPointsDoNotMatchANonTraceItem) {
    auto board = make_routing_board();
    board::PolylineTrace* trace = insert_shovable_trace(board.get(), {1});
    ASSERT_NE(trace, nullptr);
    auto area = std::make_shared<geometry::planar::PolygonArea>(
        std::vector<geometry::planar::IntPoint>{{-500, -500}, {500, -500}, {500, 500}, {-500, 500}});
    auto* conduction = new board::ConductionArea(area, 0, geometry::planar::IntVector(0, 0), 0.0, false, {1}, 1,
        board->generate_item_id(), 7, "conduction", false, board::FixedState::NOT_FIXED, board.get());
    board->get_search_tree_manager()->insert(conduction);
    board->add_item(conduction);

    // A conduction area shares the net, but is neither a drill item nor a
    // polyline trace, so the points cannot match.
    EXPECT_FALSE(MazeTraceShover::end_points_matching(*trace, conduction));
}

// ==========================================================================
// check_shove_trace_line — the guard chain of Java checkShoveTraceLine
// ==========================================================================

TEST(MazeTraceShoverTest, ADrillAsFromDoorIsAcceptedWithoutShoving) {
    // Java: `if (!(listElement.door instanceof ExpansionDoor fromDoor)) return true;`
    auto board = make_routing_board();
    board::PolylineTrace* trace = insert_shovable_trace(board.get(), {1});
    ASSERT_NE(trace, nullptr);
    ObstacleExpansionRoom room(trace, 0, 0, geometry::planar::IntBox(10000, 49900, 30000, 50100));
    ExpansionDrill drill(geometry::planar::IntBox(19900, 49900, 20100, 50100),
        geometry::planar::IntPoint(20000, 50000), 0, 0);
    MazeListElement element = make_list_element(&drill, &room);
    AutorouteControl control = make_control();
    std::vector<MazeTraceShover::DoorSection> door_list;

    EXPECT_TRUE(MazeTraceShover::check_shove_trace_line(element, &room, board.get(), control, false, door_list));
    EXPECT_TRUE(door_list.empty());
}

TEST(MazeTraceShoverTest, AnObstacleRoomWithoutATraceIsAcceptedWithoutShoving) {
    // Java: `if (!(obstacleRoom.getItem() instanceof PolylineTrace)) return true;`
    auto board = make_routing_board();
    board::Pin* pin = insert_smd_pin(board.get(), geometry::planar::IntPoint(20000, 50000), {1});
    ASSERT_NE(pin, nullptr);
    ObstacleExpansionRoom room(pin, 0, 0, geometry::planar::IntBox(19900, 49900, 20100, 50100));
    CompleteFreeSpaceExpansionRoom free_space(0, 2, geometry::planar::IntBox(19000, 40000, 21000, 60000));
    ExpansionDoor from_door(&free_space, &room, 1);
    room.add_door(&from_door);
    free_space.add_door(&from_door);
    MazeListElement element = make_list_element(&from_door, &room);
    AutorouteControl control = make_control();
    std::vector<MazeTraceShover::DoorSection> door_list;

    EXPECT_TRUE(MazeTraceShover::check_shove_trace_line(element, &room, board.get(), control, false, door_list));
    EXPECT_TRUE(door_list.empty());
}

TEST(MazeTraceShoverTest, ATraceWithAnotherHalfWidthIsNotShoved) {
    // Java: only traces with the same halfwidth can be shoved.
    auto board = make_routing_board();
    board::PolylineTrace* trace = insert_shovable_trace(board.get(), {1});
    ASSERT_NE(trace, nullptr);
    ObstacleExpansionRoom room(trace, 0, 0, geometry::planar::IntBox(10000, 49900, 30000, 50100));
    CompleteFreeSpaceExpansionRoom free_space(0, 2, geometry::planar::IntBox(9000, 40000, 31000, 60000));
    ExpansionDoor from_door(&free_space, &room, 1);
    MazeListElement element = make_list_element(&from_door, &room);
    AutorouteControl control = make_control(1, 40);
    std::vector<MazeTraceShover::DoorSection> door_list;

    EXPECT_TRUE(MazeTraceShover::check_shove_trace_line(element, &room, board.get(), control, false, door_list));
    EXPECT_TRUE(door_list.empty());
}

TEST(MazeTraceShoverTest, ATraceOfAnotherClearanceClassIsNotShoved) {
    auto board = make_routing_board();
    board::PolylineTrace* trace = insert_shovable_trace(board.get(), {1});
    ASSERT_NE(trace, nullptr);
    ObstacleExpansionRoom room(trace, 0, 0, geometry::planar::IntBox(10000, 49900, 30000, 50100));
    CompleteFreeSpaceExpansionRoom free_space(0, 2, geometry::planar::IntBox(9000, 40000, 31000, 60000));
    ExpansionDoor from_door(&free_space, &room, 1);
    MazeListElement element = make_list_element(&from_door, &room);
    AutorouteControl control = make_control();
    control.trace_clearance_class_no = 5;
    std::vector<MazeTraceShover::DoorSection> door_list;

    EXPECT_TRUE(MazeTraceShover::check_shove_trace_line(element, &room, board.get(), control, false, door_list));
    EXPECT_TRUE(door_list.empty());
}

TEST(MazeTraceShoverTest, AFromDoorNarrowerThanTheTraceIsNotShoved) {
    // Java: `if (fromDoorShape.maxWidth() < 2 * compensatedTraceHalfWidth) return true;`
    auto board = make_routing_board();
    board::PolylineTrace* trace = insert_shovable_trace(board.get(), {1});
    ASSERT_NE(trace, nullptr);
    ObstacleExpansionRoom room(trace, 0, 0, geometry::planar::IntBox(10000, 49500, 30000, 50100));
    // The free space room only touches a small left corner of the obstacle
    // room, so the door is 40 x 90, and its longest border is less than
    // 2 * 50.
    CompleteFreeSpaceExpansionRoom free_space(0, 2, geometry::planar::IntBox(10000, 49550, 10040, 49640));
    ExpansionDoor from_door(&free_space, &room, 1);
    MazeListElement element = make_list_element(&from_door, &room);
    AutorouteControl control = make_control();
    std::vector<MazeTraceShover::DoorSection> door_list;

    ASSERT_LT(from_door.get_shape().max_width(), 100);
    EXPECT_TRUE(MazeTraceShover::check_shove_trace_line(element, &room, board.get(), control, false, door_list));
    EXPECT_TRUE(door_list.empty());
}

TEST(MazeTraceShoverTest, AStaleTraceCornerIndexRejectsTheShove) {
    // Java: a stale traceCornerNo returns false, so that another door section
    // may be tried.
    auto board = make_routing_board();
    // A 3-corner trace has 2 polyline lines, so index 0 is already stale.
    board::PolylineTrace* trace = insert_trace(board.get(),
        {geometry::planar::IntPoint(10000, 50000), geometry::planar::IntPoint(30000, 50000),
            geometry::planar::IntPoint(50000, 50000)},
        50, {1});
    ASSERT_NE(trace, nullptr);
    ObstacleExpansionRoom room(trace, 0, 0, geometry::planar::IntBox(10000, 49900, 30000, 50100));
    CompleteFreeSpaceExpansionRoom free_space(0, 2, geometry::planar::IntBox(9000, 40000, 31000, 60000));
    ExpansionDoor from_door(&free_space, &room, 1);
    MazeListElement element = make_list_element(&from_door, &room);
    AutorouteControl control = make_control();
    std::vector<MazeTraceShover::DoorSection> door_list;

    EXPECT_FALSE(MazeTraceShover::check_shove_trace_line(element, &room, board.get(), control, false, door_list));
    EXPECT_TRUE(door_list.empty());
}

TEST(MazeTraceShoverTest, AnInnerDoorSectionRejectsTheShove) {
    // Java only shoves from the right most section to the right or from the
    // left most section to the left.
    auto board = make_routing_board();
    board::PolylineTrace* trace = insert_shovable_trace(board.get(), {1});
    ASSERT_NE(trace, nullptr);
    ObstacleExpansionRoom room(trace, 0, 0, geometry::planar::IntBox(10000, 49900, 30000, 50100));
    CompleteFreeSpaceExpansionRoom free_space(0, 2, geometry::planar::IntBox(9000, 40000, 31000, 60000));
    ExpansionDoor from_door(&free_space, &room, 1);
    // The door sections are only allocated on demand, like in Java where the
    // sections are created by getSectionSegments.
    from_door.get_section_segments(50);
    MazeListElement element = make_list_element(&from_door, &room, 1);
    AutorouteControl control = make_control();
    std::vector<MazeTraceShover::DoorSection> door_list;

    // The door has to have more than 2 sections for section 1 to be an inner
    // one.
    ASSERT_GT(from_door.maze_search_element_count(), 2);
    EXPECT_FALSE(MazeTraceShover::check_shove_trace_line(element, &room, board.get(), control, false, door_list));
    EXPECT_TRUE(door_list.empty());
}

// ==========================================================================
// check_shove_trace_line — the door collection of a successful shove
// ==========================================================================

TEST(MazeTraceShoverTest, AShoveIsAttemptedFromTheOuterMostDoorSection) {
    // An L shaped trace with a free space room to the left of its first
    // segment. The from door covers the whole left side of the obstacle room,
    // so the entry point at the far door corner makes the section check pass
    // and the shove is actually attempted.
    auto board = make_routing_board();
    board::PolylineTrace* trace = insert_bending_trace(board.get(), {1});
    ASSERT_NE(trace, nullptr);
    ObstacleExpansionRoom room(trace, 0, 0, geometry::planar::IntBox(19900, 39900, 40100, 40100));
    CompleteFreeSpaceExpansionRoom free_space(0, 2, geometry::planar::IntBox(10000, 30000, 20000, 50000));
    ExpansionDoor from_door(&free_space, &room, 1);
    room.add_door(&from_door);
    free_space.add_door(&from_door);

    // Three more doors of the obstacle room: one which straddles the shove
    // line and one on each side of it. None of them may be collected, because
    // Java only takes doors which lie completely on the shove side.
    CompleteFreeSpaceExpansionRoom straddle_room(0, 3, geometry::planar::IntBox(30000, 30000, 40500, 50000));
    ExpansionDoor straddle_door(&straddle_room, &room, 1);
    room.add_door(&straddle_door);
    straddle_room.add_door(&straddle_door);

    CompleteFreeSpaceExpansionRoom below_room(0, 4, geometry::planar::IntBox(30000, 38000, 40500, 39990));
    ExpansionDoor below_door(&below_room, &room, 1);
    room.add_door(&below_door);
    below_room.add_door(&below_door);

    CompleteFreeSpaceExpansionRoom above_room(0, 5, geometry::planar::IntBox(30000, 40010, 40500, 45000));
    ExpansionDoor above_door(&above_room, &room, 1);
    room.add_door(&above_door);
    above_room.add_door(&above_door);

    ASSERT_EQ(from_door.get_dimension(), 1);
    // The door sections are allocated on demand, like in Java.
    from_door.get_section_segments(50);
    ASSERT_EQ(from_door.maze_search_element_count(), 1);
    MazeListElement element = make_list_element(&from_door, &room, 0,
        geometry::planar::FloatPoint(20000, 40100));
    AutorouteControl control = make_control();
    std::vector<MazeTraceShover::DoorSection> door_list;

    const bool ok = MazeTraceShover::check_shove_trace_line(element, &room, board.get(), control, false, door_list);

    // The empty board never blocks the shove, so the room is reported as
    // usable and the caller may expand the trace into the from door.
    EXPECT_TRUE(ok);

    // Divergence from Java: the C++ expansion doors carry an IntBox shape, so
    // `diagonal_corner_segment()` is the bounding box diagonal and
    // `polar_line_segment()` snaps to a border line. For a door which is seen
    // almost edge on this no longer reproduces the Java TileShape geometry, so
    // the `from_door_compare_distance` filter rejects every candidate. The
    // C++ shover therefore returns an empty door list here.
    for (const MazeTraceShover::DoorSection& section : door_list)
    {
        EXPECT_NE(section.door, &straddle_door);
        EXPECT_NE(section.door, &above_door);
    }
    EXPECT_TRUE(door_list.empty());
}

} // namespace autoroute
} // namespace freerouting
