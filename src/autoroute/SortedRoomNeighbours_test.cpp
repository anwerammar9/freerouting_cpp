#include <gtest/gtest.h>


#include <memory>
#include <set>
#include <vector>

#include "autoroute/AutorouteEngine.h"
#include "autoroute/ExpansionRoom.h"
#include "autoroute/ItemAutorouteInfo.h"
#include "autoroute/SortedRoomNeighbours.h"
#include "board/FixedState.h"
#include "board/Layer.h"
#include "board/PolylineTrace.h"
#include "board/RoutingBoard.h"
#include "board/ShapeSearchTree.h"
#include "board/Trace.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/IntOctagon.h"
#include "geometry/planar/Point.h"
#include "geometry/planar/Polyline.h"
#include "geometry/planar/Simplex.h"
#include "geometry/planar/TileShape.h"
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
    return std::make_shared<fr::board::RoutingBoard>(fp::IntBox(-5000, -5000, 50000, 50000),
        layer_structure, std::move(board_rules), nullptr);
}

fr::board::PolylineTrace* insert_trace(fr::board::RoutingBoard* p_board,
    const std::vector<fp::IntPoint>& p_corners, const std::vector<int>& p_nets,
    fr::board::ShapeSearchTree* p_tree = nullptr)
{
    auto polyline = std::make_shared<fp::Polyline>(fp::Polyline::from_corners(p_corners));
    fr::board::PolylineTrace* trace = p_board->insert_trace_without_cleaning(polyline, 0, 50, p_nets, 1,
        fr::board::FixedState::NOT_FIXED);
    if (trace != nullptr && p_tree != nullptr)
    {
        p_tree->insert(trace);
    }
    return trace;
}

/** A 1 dimensional (segment) simplex from two collinear lines. */
fp::Simplex make_segment(const fp::IntPoint& p_from, const fp::IntPoint& p_to)
{
    return fp::Simplex({fp::Line(p_from, p_to), fp::Line(p_to, p_from)});
}

/** A 0 dimensional (point) simplex from three lines through one point. */
fp::Simplex make_point(const fp::IntPoint& p_point)
{
    return fp::Simplex({fp::Line(p_point, fp::IntPoint(p_point.x + 10, p_point.y)),
        fp::Line(p_point, fp::IntPoint(p_point.x, p_point.y + 10)),
        fp::Line(p_point, fp::IntPoint(p_point.x + 10, p_point.y + 10))});
}

/** Registers a complete free space room in the given search tree. */
void insert_free_space_room(fr::board::ShapeSearchTree* p_tree,
    fr::autoroute::CompleteFreeSpaceExpansionRoom* p_room)
{
    p_tree->insert(*p_room);
}

} // namespace

TEST(SortedRoomNeighboursTest, EmptyBoardHasNoNeighbours)
{
    auto board = make_routing_board();
    fr::board::ShapeSearchTree90Degree tree(board.get(), 0);

    fr::autoroute::IncompleteFreeSpaceExpansionRoom room(nullptr, 0, fp::IntBox(0, 0, 100, 100));
    auto neighbours = fr::autoroute::SortedRoomNeighbours::calculate_neighbours(&room, 1, &tree, 1);
    ASSERT_NE(neighbours, nullptr);

    EXPECT_TRUE(neighbours->get_sorted_neighbours().empty());
    EXPECT_TRUE(neighbours->get_own_net_objects().empty());
    // A new complete free space room is created for the incomplete from room.
    auto* completed_room = neighbours->get_completed_room();
    ASSERT_NE(completed_room, nullptr);
    EXPECT_EQ(completed_room->get_layer(), 0);
    EXPECT_EQ(completed_room->get_room_id(), 1);
    EXPECT_EQ(completed_room->get_shape(), fp::IntBox(0, 0, 100, 100));
}

TEST(SortedRoomNeighboursTest, UnexpectedRoomTypeReturnsNull)
{
    auto board = make_routing_board();
    fr::board::ShapeSearchTree90Degree tree(board.get(), 0);

    // A plain IncompleteExpansionRoom is not a supported from room type.
    fr::autoroute::IncompleteExpansionRoom room(nullptr, 0, fp::IntBox(0, 0, 100, 100));
    EXPECT_EQ(fr::autoroute::SortedRoomNeighbours::calculate_neighbours(&room, 1, &tree, 1), nullptr);
}

TEST(SortedRoomNeighboursTest, OneDimensionalNeighbourCreatesDoor)
{
    auto board = make_routing_board();
    fr::board::ShapeSearchTree90Degree tree(board.get(), 0);

    fr::autoroute::CompleteFreeSpaceExpansionRoom left_room(0, 2, fp::IntBox(-200, -200, 0, 200));
    insert_free_space_room(&tree, &left_room);

    // The incomplete room touches the left room on its left edge line.
    fr::autoroute::IncompleteFreeSpaceExpansionRoom room(nullptr, 0, fp::IntBox(0, 0, 200, 200));
    auto neighbours = fr::autoroute::SortedRoomNeighbours::calculate_neighbours(&room, 1, &tree, 1);
    ASSERT_NE(neighbours, nullptr);
    ASSERT_EQ(neighbours->get_sorted_neighbours().size(), 1u);

    // The intersection is the shared edge x == 0 between y == 0 and y == 200.
    EXPECT_EQ(neighbours->get_sorted_neighbours().front().intersection->bounding_box(),
        fp::IntBox(0, 0, 0, 200));

    // A 1-dimensional door is created on both sides.
    auto* completed_room = neighbours->get_completed_room();
    ASSERT_NE(completed_room, nullptr);
    ASSERT_EQ(completed_room->get_doors().size(), 1u);
    EXPECT_EQ(left_room.get_doors().size(), 1u);
    EXPECT_TRUE(completed_room->door_exists(&left_room));
    EXPECT_TRUE(left_room.door_exists(completed_room));
    EXPECT_EQ(completed_room->get_doors().front()->get_dimension(), 1);

    // The completed room is owned by the SortedRoomNeighbours until released.
    fr::autoroute::CompleteFreeSpaceExpansionRoom* released = neighbours->release_completed_room();
    EXPECT_EQ(released, completed_room);
    EXPECT_EQ(neighbours->release_completed_room(), nullptr);
}

TEST(SortedRoomNeighboursTest, TwoDimensionalOverlapOnFreeSpaceRoomIsIgnored)
{
    auto board = make_routing_board();
    fr::board::ShapeSearchTree90Degree tree(board.get(), 0);

    fr::autoroute::CompleteFreeSpaceExpansionRoom inner_room(0, 2, fp::IntBox(20, 20, 80, 80));
    insert_free_space_room(&tree, &inner_room);

    fr::autoroute::IncompleteFreeSpaceExpansionRoom room(nullptr, 0, fp::IntBox(0, 0, 100, 100));
    auto neighbours = fr::autoroute::SortedRoomNeighbours::calculate_neighbours(&room, 1, &tree, 1);
    ASSERT_NE(neighbours, nullptr);

    // Only an ObstacleExpansionRoom may have a 2-dimensional overlap.
    EXPECT_TRUE(neighbours->get_sorted_neighbours().empty());
    EXPECT_TRUE(neighbours->get_completed_room()->get_doors().empty());
    EXPECT_TRUE(inner_room.get_doors().empty());
}

TEST(SortedRoomNeighboursTest, PointTouchCreatesCornerNeighbourWithoutDoor)
{
    auto board = make_routing_board();
    fr::board::ShapeSearchTree90Degree tree(board.get(), 0);

    fr::autoroute::CompleteFreeSpaceExpansionRoom diagonal_room(0, 2, fp::IntBox(100, 100, 300, 300));
    insert_free_space_room(&tree, &diagonal_room);

    fr::autoroute::IncompleteFreeSpaceExpansionRoom room(nullptr, 0, fp::IntBox(0, 0, 100, 100));
    auto neighbours = fr::autoroute::SortedRoomNeighbours::calculate_neighbours(&room, 1, &tree, 1);
    ASSERT_NE(neighbours, nullptr);

    ASSERT_EQ(neighbours->get_sorted_neighbours().size(), 1u);
    const auto& neighbour = neighbours->get_sorted_neighbours().front();
    EXPECT_EQ(neighbour.intersection->dimension(), 0);
    EXPECT_TRUE(neighbour.room_touch_is_corner);
    EXPECT_TRUE(neighbour.neighbour_room_touch_is_corner);
    // A point touch is no door: the door shape has dimension 1.
    EXPECT_TRUE(neighbours->get_completed_room()->get_doors().empty());
    EXPECT_TRUE(diagonal_room.get_doors().empty());
}

TEST(SortedRoomNeighboursTest, NeighboursAreSortedByTouchingSide)
{
    auto board = make_routing_board();
    fr::board::ShapeSearchTree90Degree tree(board.get(), 0);

    fr::autoroute::CompleteFreeSpaceExpansionRoom left_room(0, 2, fp::IntBox(-200, 0, 0, 100));
    fr::autoroute::CompleteFreeSpaceExpansionRoom right_room(0, 3, fp::IntBox(100, 0, 300, 100));
    fr::autoroute::CompleteFreeSpaceExpansionRoom top_room(0, 4, fp::IntBox(0, 100, 100, 300));
    insert_free_space_room(&tree, &right_room);
    insert_free_space_room(&tree, &top_room);
    insert_free_space_room(&tree, &left_room);

    fr::autoroute::IncompleteFreeSpaceExpansionRoom room(nullptr, 0, fp::IntBox(0, 0, 100, 100));
    auto neighbours = fr::autoroute::SortedRoomNeighbours::calculate_neighbours(&room, 1, &tree, 1);
    ASSERT_NE(neighbours, nullptr);

    const auto& sorted = neighbours->get_sorted_neighbours();
    ASSERT_EQ(sorted.size(), 3u);
    for (size_t i = 1; i < sorted.size(); i++)
    {
        EXPECT_LE(sorted[i - 1].compare_to(sorted[i]), 0);
        EXPECT_LE(sorted[i - 1].touching_side_no_of_room, sorted[i].touching_side_no_of_room);
    }
    // All three neighbours are distinct tree objects and each has its own door.
    std::set<const fr::board::SearchTreeObject*> neighbour_objects;
    for (const auto& neighbour : sorted)
    {
        neighbour_objects.insert(neighbour.search_tree_object);
    }
    EXPECT_EQ(neighbour_objects.size(), 3u);
    EXPECT_EQ(neighbours->get_completed_room()->get_doors().size(), 3u);
}

TEST(SortedRoomNeighboursTest, OwnNetObjectsAreCollectedForTargetDoors)
{
    auto board = make_routing_board();
    fr::board::ShapeSearchTree90Degree tree(board.get(), 0);

    // A trace of the routing net is no trace obstacle, so the door calculation
    // for it is delayed (own net objects).
    fr::board::PolylineTrace* own_net_trace = insert_trace(board.get(),
        {fp::IntPoint(0, 0), fp::IntPoint(80, 0)}, {1}, &tree);
    // A trace of another net touching the room border is a trace obstacle and
    // becomes a sorted neighbour.
    fr::board::PolylineTrace* other_net_trace = insert_trace(board.get(),
        {fp::IntPoint(0, 350), fp::IntPoint(80, 350)}, {2}, &tree);
    ASSERT_NE(own_net_trace, nullptr);
    ASSERT_NE(other_net_trace, nullptr);

    fr::autoroute::IncompleteFreeSpaceExpansionRoom room(nullptr, 0, fp::IntBox(-100, -100, 100, 300));
    auto neighbours = fr::autoroute::SortedRoomNeighbours::calculate_neighbours(&room, 1, &tree, 1);
    ASSERT_NE(neighbours, nullptr);

    ASSERT_EQ(neighbours->get_own_net_objects().size(), 1u);
    EXPECT_EQ(neighbours->get_own_net_objects().front().object, own_net_trace);
    EXPECT_EQ(neighbours->get_own_net_objects().front().shape_index_in_object, 0);

    ASSERT_EQ(neighbours->get_sorted_neighbours().size(), 1u);
    EXPECT_EQ(neighbours->get_sorted_neighbours().front().search_tree_object, other_net_trace);
    EXPECT_EQ(neighbours->get_completed_room()->get_doors().size(), 1u);
}

TEST(SortedRoomNeighboursTest, ObstacleRoomCreatesOverlapDoorForSameNetItem)
{
    auto board = make_routing_board();
    fr::board::ShapeSearchTree90Degree tree(board.get(), 0);

    fr::board::PolylineTrace* own_room_trace = insert_trace(board.get(),
        {fp::IntPoint(0, 0), fp::IntPoint(200, 0)}, {1}, &tree);
    fr::board::PolylineTrace* overlapping_trace = insert_trace(board.get(),
        {fp::IntPoint(100, -100), fp::IntPoint(100, 100)}, {1}, &tree);
    ASSERT_NE(own_room_trace, nullptr);
    ASSERT_NE(overlapping_trace, nullptr);

    fr::autoroute::ObstacleExpansionRoom room(own_room_trace, 0, &tree);
    auto neighbours = fr::autoroute::SortedRoomNeighbours::calculate_neighbours(&room, 1, &tree, 1);
    ASSERT_NE(neighbours, nullptr);

    // The completed room of an obstacle room is the obstacle room itself.
    EXPECT_EQ(neighbours->get_completed_room(), &room);
    // A 2-dimensional overlap with a routable item of the same net creates a
    // 2-dimensional overlap door.
    ASSERT_EQ(room.get_doors().size(), 1u);
    EXPECT_EQ(room.get_doors().front()->get_dimension(), 2);
    EXPECT_EQ(neighbours->release_completed_room(), nullptr);
}

TEST(SortedRoomNeighboursTest, ObstacleRoomOverlapWithOtherNetIsIgnored)
{
    auto board = make_routing_board();
    fr::board::ShapeSearchTree90Degree tree(board.get(), 0);

    fr::board::PolylineTrace* own_room_trace = insert_trace(board.get(),
        {fp::IntPoint(0, 0), fp::IntPoint(200, 0)}, {1}, &tree);
    fr::board::PolylineTrace* other_net_trace = insert_trace(board.get(),
        {fp::IntPoint(100, -100), fp::IntPoint(100, 100)}, {2}, &tree);
    ASSERT_NE(own_room_trace, nullptr);
    ASSERT_NE(other_net_trace, nullptr);

    fr::autoroute::ObstacleExpansionRoom room(own_room_trace, 0, &tree);
    auto neighbours = fr::autoroute::SortedRoomNeighbours::calculate_neighbours(&room, 1, &tree, 1);
    ASSERT_NE(neighbours, nullptr);
    EXPECT_TRUE(room.get_doors().empty());
}

TEST(SortedRoomNeighboursTest, ObstacleRoomGetsOneDimensionalDoorToSameNetItemRoom)
{
    auto board = make_routing_board();
    fr::board::ShapeSearchTree90Degree tree(board.get(), 0);

    // The second trace of the same net only touches the border of the first
    // room of the own room trace.
    fr::board::PolylineTrace* own_room_trace = insert_trace(board.get(),
        {fp::IntPoint(0, 0), fp::IntPoint(200, 0)}, {1}, &tree);
    fr::board::PolylineTrace* same_net_trace = insert_trace(board.get(),
        {fp::IntPoint(100, 100), fp::IntPoint(300, 100)}, {1}, &tree);
    fr::board::PolylineTrace* other_net_trace = insert_trace(board.get(),
        {fp::IntPoint(100, 300), fp::IntPoint(300, 300)}, {2}, &tree);
    ASSERT_NE(own_room_trace, nullptr);
    ASSERT_NE(same_net_trace, nullptr);
    ASSERT_NE(other_net_trace, nullptr);

    fr::autoroute::ObstacleExpansionRoom room(own_room_trace, 0, &tree);
    auto neighbours = fr::autoroute::SortedRoomNeighbours::calculate_neighbours(&room, 1, &tree, 1);
    ASSERT_NE(neighbours, nullptr);

    // A 1-dimensional door to the expansion room of the same net item is
    // created; the other net item is only 1-dimensional as well, but the two
    // obstacle rooms do not share a net.
    ASSERT_EQ(room.get_doors().size(), 1u);
    EXPECT_EQ(room.get_doors().front()->get_dimension(), 1);
    EXPECT_EQ(room.get_doors().front()->other_room(&room)->get_room_id(),
        (same_net_trace->get_id() << 10) | 0);
}

TEST(SortedRoomNeighboursTest, InsertDoorOkChecksDoorAndNets)
{
    auto board = make_routing_board();
    fr::board::ShapeSearchTree90Degree tree(board.get(), 0);

    fr::board::PolylineTrace* net1 = insert_trace(board.get(),
        {fp::IntPoint(0, 0), fp::IntPoint(200, 0)}, {1});
    fr::board::PolylineTrace* net2 = insert_trace(board.get(),
        {fp::IntPoint(0, 400), fp::IntPoint(200, 400)}, {2});
    ASSERT_NE(net1, nullptr);
    ASSERT_NE(net2, nullptr);

    fr::autoroute::ObstacleExpansionRoom room_net1(net1, 0, &tree);
    fr::autoroute::ObstacleExpansionRoom room_net1_again(net1, 0, &tree);
    fr::autoroute::ObstacleExpansionRoom room_net2(net2, 0, &tree);
    fr::autoroute::CompleteFreeSpaceExpansionRoom free_room_a(0, 1, fp::IntBox(0, 0, 10, 10));
    fr::autoroute::CompleteFreeSpaceExpansionRoom free_room_b(0, 2, fp::IntBox(20, 0, 30, 10));

    // A 1-dimensional door shape between two boxes.
    fp::Simplex door_shape = make_segment(fp::IntPoint(10, 0), fp::IntPoint(20, 0));
    ASSERT_EQ(door_shape.dimension(), 1);

    // Two obstacle rooms are only connected if their items share a net.
    EXPECT_TRUE(fr::autoroute::SortedRoomNeighbours::insert_door_ok(&room_net1, &room_net1_again, door_shape));
    EXPECT_FALSE(fr::autoroute::SortedRoomNeighbours::insert_door_ok(&room_net1, &room_net2, door_shape));

    // Two free space rooms are always connected.
    EXPECT_TRUE(fr::autoroute::SortedRoomNeighbours::insert_door_ok(&free_room_a, &free_room_b, door_shape));

    // A point shaped door has no door line, so no door is inserted.
    fp::Simplex point_door_shape = make_point(fp::IntPoint(15, 5));
    ASSERT_EQ(point_door_shape.dimension(), 0);
    EXPECT_FALSE(fr::autoroute::SortedRoomNeighbours::insert_door_ok(&room_net1, &free_room_a,
        point_door_shape));

    // An existing door is not inserted twice.
    auto* door = new fr::autoroute::ExpansionDoor(&room_net1, &free_room_a, 1);
    room_net1.add_door(door);
    free_room_a.add_door(door);
    EXPECT_FALSE(fr::autoroute::SortedRoomNeighbours::insert_door_ok(&room_net1, &free_room_a, door_shape));
    delete door;
}

TEST(SortedRoomNeighboursTest, InsertDoorOkRespectsTraceForkParallelity)
{
    auto board = make_routing_board();
    fr::board::ShapeSearchTree90Degree tree(board.get(), 0);

    // The room of the first tree shape of a two segment trace is a fork entry.
    fr::board::PolylineTrace* trace = insert_trace(board.get(),
        {fp::IntPoint(0, 0), fp::IntPoint(200, 0), fp::IntPoint(200, 200)}, {1});
    ASSERT_NE(trace, nullptr);
    ASSERT_EQ(trace->tile_shape_count(), 2);

    fr::autoroute::ObstacleExpansionRoom first_room(trace, 0, &tree);
    fr::autoroute::ObstacleExpansionRoom second_room(trace, 1, &tree);
    fr::autoroute::CompleteFreeSpaceExpansionRoom free_room(0, 1, fp::IntBox(0, 0, 10, 10));

    // The first polyline line is horizontal, so a parallel horizontal door line
    // is accepted and a vertical one is rejected.
    fp::Line horizontal = fp::Line(fp::IntPoint(0, 0), fp::IntPoint(100, 0));
    fp::Line vertical = fp::Line(fp::IntPoint(0, 0), fp::IntPoint(0, 100));
    EXPECT_TRUE(fr::autoroute::SortedRoomNeighbours::insert_door_ok(&first_room, horizontal));
    EXPECT_FALSE(fr::autoroute::SortedRoomNeighbours::insert_door_ok(&first_room, vertical));

    // The room of the second tree shape is the other fork entry, which is
    // described by the second polyline line (vertical).
    EXPECT_TRUE(fr::autoroute::SortedRoomNeighbours::insert_door_ok(&second_room, vertical));
    EXPECT_FALSE(fr::autoroute::SortedRoomNeighbours::insert_door_ok(&second_room, horizontal));
}

TEST(SortedRoomNeighboursTest, IncompleteRoomsWithEmptyNeighbours)
{
    auto board = make_routing_board();
    fr::board::ShapeSearchTree90Degree tree(board.get(), 0);
    fr::autoroute::AutorouteEngine engine(board.get(), 1, true);

    // A trace far away from everything else: its room has no neighbours.
    fr::board::PolylineTrace* trace = insert_trace(board.get(),
        {fp::IntPoint(2000, 2000), fp::IntPoint(2400, 2000)}, {1}, &tree);
    ASSERT_NE(trace, nullptr);

    fr::autoroute::ObstacleExpansionRoom room(trace, 0, &tree);
    ASSERT_TRUE(room.get_doors().empty());

    fr::autoroute::SortedRoomNeighbours::calculate_incomplete_rooms_with_empty_neighbours(&room, &engine);

    // Only the border lines parallel to the trace line following the room get a
    // door, because entering a fork room from the wrong side causes check
    // ripup problems. The room shape of a straight horizontal trace is an
    // octagon with 8 border lines, 2 of which are horizontal.
    ASSERT_EQ(room.get_doors().size(), 2u);
    for (auto* door : room.get_doors())
    {
        ASSERT_NE(door, nullptr);
        EXPECT_EQ(door->get_dimension(), 1);
        // The CompleteExpansionRoom overload of other_room() would be preferred
        // for an ObstacleExpansionRoom argument and would return nullptr for
        // the incomplete room, so the base overload is selected explicitly.
        auto* neighbour = door->other_room(static_cast<fr::autoroute::ExpansionRoom*>(&room));
        ASSERT_NE(neighbour, nullptr);
        auto* incomplete = dynamic_cast<fr::autoroute::IncompleteFreeSpaceExpansionRoom*>(neighbour);
        ASSERT_NE(incomplete, nullptr);
        // The shape of the new room is the half plane behind the border line,
        // the contained shape the part of the room shape.
        EXPECT_EQ(incomplete->get_shape_tile().dimension(), 2);
        EXPECT_FALSE(incomplete->get_shape_tile().is_bounded());
    }
    engine.remove_all_doors(&room);
    EXPECT_TRUE(room.get_doors().empty());
}

TEST(SortedRoomNeighboursTest, IncompleteRoomsWithEmptyNeighboursSkippedForNullArguments)
{
    // The Java method does not guard against null; the C++ port returns early
    // instead of dereferencing.
    fr::autoroute::SortedRoomNeighbours::calculate_incomplete_rooms_with_empty_neighbours(nullptr, nullptr);
}

TEST(SortedRoomNeighboursTest, SortedRoomNeighbourCornerAccessors)
{
    auto board = make_routing_board();
    fr::board::ShapeSearchTree90Degree tree(board.get(), 0);

    fr::autoroute::CompleteFreeSpaceExpansionRoom left_room(0, 2, fp::IntBox(-200, 0, 0, 100));
    insert_free_space_room(&tree, &left_room);

    fr::autoroute::IncompleteFreeSpaceExpansionRoom room(nullptr, 0, fp::IntBox(0, 0, 100, 100));
    auto neighbours = fr::autoroute::SortedRoomNeighbours::calculate_neighbours(&room, 1, &tree, 1);
    ASSERT_NE(neighbours, nullptr);
    ASSERT_EQ(neighbours->get_sorted_neighbours().size(), 1u);

    const auto& neighbour = neighbours->get_sorted_neighbours().front();
    // A full edge touch: the first and the last corner are the two endpoints of
    // the shared edge x == 0, in ascending order around the room.
    const fp::IntPoint& first = neighbour.first_corner();
    const fp::IntPoint& last = neighbour.last_corner();
    EXPECT_TRUE((first == fp::IntPoint(0, 0) && last == fp::IntPoint(0, 100))
        || (first == fp::IntPoint(0, 100) && last == fp::IntPoint(0, 0)));
    // The accessors are cached, so repeated calls are stable.
    EXPECT_EQ(neighbour.first_corner(), first);
    EXPECT_EQ(neighbour.last_corner(), last);
}

