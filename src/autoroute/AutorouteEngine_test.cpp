#include <gtest/gtest.h>

#include <memory>
#include <set>
#include <vector>

#include "autoroute/AutorouteEngine.h"
#include "autoroute/ExpansionRoom.h"
#include "board/FixedState.h"
#include "board/Layer.h"
#include "board/PolylineTrace.h"
#include "board/RoutingBoard.h"
#include "board/ShapeSearchTree.h"
#include "geometry/planar/IntBox.h"
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
    return std::make_shared<fr::board::RoutingBoard>(fp::IntBox(0, 0, 10000, 10000), layer_structure,
        std::move(board_rules), nullptr);
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

/**
 * ExpansionDoor::other_room() must be called with an ExpansionRoom pointer, as
 * in Java: the CompleteExpansionRoom overload returns nullptr for a door to an
 * incomplete room.
 */
fr::autoroute::ExpansionRoom* other_room(fr::autoroute::ExpansionDoor* p_door, fr::autoroute::ExpansionRoom* p_room)
{
    return p_door->other_room(p_room);
}

/** Completes a single room on layer 0 through the engine room database. */
fr::autoroute::CompleteFreeSpaceExpansionRoom* complete_room(fr::autoroute::AutorouteEngine& p_engine,
    const fp::IntBox& p_shape)
{
    fr::autoroute::IncompleteFreeSpaceExpansionRoom incomplete(nullptr, 0, p_shape);
    std::vector<fr::autoroute::CompleteFreeSpaceExpansionRoom*> rooms =
        p_engine.complete_expansion_room(&incomplete);
    if (rooms.size() != 1u)
    {
        return nullptr;
    }
    return rooms[0];
}

} // namespace

// ==========================================================================
// AutorouteEngine.getFirstIncompleteExpansionRoom
// ==========================================================================

TEST(AutorouteEngineTest, FirstIncompleteExpansionRoomIsNullWhenDatabaseIsEmpty)
{
    auto board = make_routing_board();
    fr::autoroute::AutorouteEngine engine(board.get(), 1, true);

    EXPECT_EQ(engine.get_first_incomplete_expansion_room(), nullptr);

    std::vector<fp::Line> half_plane{fp::Line(fp::IntPoint(100, 0), fp::IntPoint(100, 100))};
    fr::autoroute::IncompleteFreeSpaceExpansionRoom* added =
        engine.add_incomplete_expansion_room(fp::Simplex::get_instance(half_plane), 0,
            fp::IntBox(0, 0, 10, 10).to_Simplex());
    ASSERT_NE(added, nullptr);
    EXPECT_EQ(engine.get_first_incomplete_expansion_room(), added);

    engine.clear();
    EXPECT_EQ(engine.get_first_incomplete_expansion_room(), nullptr);
}

// ==========================================================================
// AutorouteEngine.removeCompleteExpansionRoom
// ==========================================================================

TEST(AutorouteEngineTest, RemoveCompleteExpansionRoomCreatesIncompleteNeighbourRoom)
{
    // Java AutorouteEngine.removeCompleteExpansionRoom(): a neighbour room
    // touching the removed room along a 1-dimensional door gets a new
    // incomplete expansion room behind its border line, so the free space of
    // the removed room stays reachable.
    auto board = make_routing_board();
    fr::autoroute::AutorouteEngine engine(board.get(), 1, true);
    engine.set_net_no(1);
    fr::board::ShapeSearchTree* tree = engine.get_autoroute_search_tree();
    ASSERT_NE(tree, nullptr);
    ASSERT_EQ(tree->size(), 0);

    fr::autoroute::CompleteFreeSpaceExpansionRoom* room = complete_room(engine, fp::IntBox(1000, 1000, 2000, 2000));
    ASSERT_NE(room, nullptr);
    // The shape is needed after the removal, because the room object itself is
    // owned (and destroyed) by the engine.
    const fp::Simplex removed_room_shape = room->get_shape_tile();
    ASSERT_EQ(tree->size(), 1);
    ASSERT_EQ(engine.get_first_incomplete_expansion_room(), nullptr);

    // A neighbour room sharing the right border line of the removed room.
    fr::autoroute::CompleteFreeSpaceExpansionRoom neighbour(0, 999, fp::IntBox(2000, 1000, 3000, 2000));
    auto* door = new fr::autoroute::ExpansionDoor(room, &neighbour, 1);
    room->add_door(door);
    neighbour.add_door(door);
    ASSERT_EQ(room->get_doors().size(), 1u);
    ASSERT_EQ(neighbour.get_doors().size(), 1u);

    engine.remove_complete_expansion_room(room);

    // The removed room is gone from the search tree and from the room database.
    EXPECT_EQ(tree->size(), 0);
    EXPECT_TRUE(tree->overlapping_objects(removed_room_shape.bounding_box(), -1).empty());

    // The neighbour keeps exactly one door, the new 1-dimensional door to the
    // new incomplete room.
    ASSERT_EQ(neighbour.get_doors().size(), 1u);
    fr::autoroute::ExpansionDoor* new_door = neighbour.get_doors().front();
    EXPECT_EQ(new_door->get_dimension(), 1);

    fr::autoroute::IncompleteFreeSpaceExpansionRoom* new_room = engine.get_first_incomplete_expansion_room();
    ASSERT_NE(new_room, nullptr);
    EXPECT_EQ(new_room->get_layer(), 0);
    EXPECT_EQ(other_room(new_door, &neighbour), static_cast<fr::autoroute::ExpansionRoom*>(new_room));
    ASSERT_EQ(new_room->get_doors().size(), 1u);
    EXPECT_EQ(new_room->get_doors().front(), new_door);

    // The new incomplete room is a half plane covering the whole removed room,
    // and it overlaps the neighbour only on the common border line.
    const fp::Simplex new_room_shape = new_room->get_shape_tile();
    EXPECT_EQ(new_room_shape.dimension(), 2);
    EXPECT_EQ(fp::TileShape::intersection_of(new_room_shape, removed_room_shape)->dimension(), 2);
    EXPECT_EQ(fp::TileShape::intersection_of(new_room_shape, neighbour.get_shape_tile())->dimension(), 1);
    EXPECT_EQ(new_room->get_contained_shape_tile().dimension(), 1);
}

TEST(AutorouteEngineTest, RemoveCompleteExpansionRoomSkipsOverlappingNeighbour)
{
    // A neighbour overlapping the removed room with a 2-dimensional door gets
    // no new incomplete room: the free space it lost is already covered by the
    // overlapping room itself.
    auto board = make_routing_board();
    fr::autoroute::AutorouteEngine engine(board.get(), 1, true);
    engine.set_net_no(1);
    fr::board::ShapeSearchTree* tree = engine.get_autoroute_search_tree();
    ASSERT_NE(tree, nullptr);

    fr::autoroute::CompleteFreeSpaceExpansionRoom* room = complete_room(engine, fp::IntBox(1000, 1000, 2000, 2000));
    ASSERT_NE(room, nullptr);
    const fp::Simplex removed_room_shape = room->get_shape_tile();

    fr::autoroute::CompleteFreeSpaceExpansionRoom neighbour(0, 999, fp::IntBox(1500, 1000, 3000, 2000));
    auto* door = new fr::autoroute::ExpansionDoor(room, &neighbour, 2);
    room->add_door(door);
    neighbour.add_door(door);

    engine.remove_complete_expansion_room(room);

    // No incomplete room is created and the door of the removed room is gone
    // from the neighbour as well.
    EXPECT_EQ(engine.get_first_incomplete_expansion_room(), nullptr);
    EXPECT_TRUE(neighbour.get_doors().empty());
    EXPECT_EQ(tree->size(), 0);
    EXPECT_TRUE(tree->overlapping_objects(removed_room_shape.bounding_box(), -1).empty());
}

TEST(AutorouteEngineTest, RemoveCompleteExpansionRoomIgnoresNullRoom)
{
    auto board = make_routing_board();
    fr::autoroute::AutorouteEngine engine(board.get(), 1, true);
    engine.set_net_no(1);
    fr::board::ShapeSearchTree* tree = engine.get_autoroute_search_tree();
    ASSERT_NE(tree, nullptr);

    fr::autoroute::CompleteFreeSpaceExpansionRoom* room = complete_room(engine, fp::IntBox(1000, 1000, 2000, 2000));
    ASSERT_NE(room, nullptr);
    ASSERT_EQ(tree->size(), 1);

    engine.remove_complete_expansion_room(nullptr);

    // The database is untouched.
    EXPECT_EQ(tree->size(), 1);
    EXPECT_EQ(engine.get_first_incomplete_expansion_room(), nullptr);
}

TEST(AutorouteEngineTest, RemoveCompleteExpansionRoomHandlesUnregisteredRoom)
{
    // Java logs a warning when the room is not part of the complete room
    // database; the C++ port must not crash and must still clear the room.
    auto board = make_routing_board();
    fr::autoroute::AutorouteEngine engine(board.get(), 1, true);
    engine.set_net_no(1);
    fr::board::ShapeSearchTree* tree = engine.get_autoroute_search_tree();
    ASSERT_NE(tree, nullptr);

    // The room lives on the stack and was never registered in the engine.
    fr::autoroute::CompleteFreeSpaceExpansionRoom room(0, 42, fp::IntBox(1000, 1000, 2000, 2000));
    tree->insert(room);
    ASSERT_EQ(tree->size(), 1);

    engine.remove_complete_expansion_room(&room);

    EXPECT_EQ(tree->size(), 0);
    EXPECT_TRUE(room.get_doors().empty());
    EXPECT_EQ(engine.get_first_incomplete_expansion_room(), nullptr);
}

// ==========================================================================
// AutorouteEngine.completeNeighbourRooms
// ==========================================================================

TEST(AutorouteEngineTest, CompleteNeighbourRoomsCompletesIncompleteFreeSpaceNeighbour)
{
    // Java AutorouteEngine.completeNeighbourRooms(): an incomplete free space
    // neighbour is completed, so it disappears from the incomplete room
    // database and the completed room is stored in the search tree.
    auto board = make_routing_board();
    fr::autoroute::AutorouteEngine engine(board.get(), 1, true);
    engine.set_net_no(1);
    fr::board::ShapeSearchTree* tree = engine.get_autoroute_search_tree();
    ASSERT_NE(tree, nullptr);

    fr::autoroute::CompleteFreeSpaceExpansionRoom* room = complete_room(engine, fp::IntBox(1000, 1000, 2000, 2000));
    ASSERT_NE(room, nullptr);
    ASSERT_EQ(tree->size(), 1);

    // An unregistered incomplete free space room touching the completed room
    // along its right border line.
    fr::autoroute::IncompleteFreeSpaceExpansionRoom neighbour_incomplete(nullptr, 0,
        fp::IntBox(2000, 1000, 3000, 2000));
    auto* door = new fr::autoroute::ExpansionDoor(room, &neighbour_incomplete, 1);
    room->add_door(door);
    neighbour_incomplete.add_door(door);
    ASSERT_EQ(room->get_doors().size(), 1u);
    ASSERT_EQ(engine.get_first_incomplete_expansion_room(), nullptr);

    engine.complete_neighbour_rooms(room);

    // The incomplete neighbour was completed and registered in the search tree.
    EXPECT_EQ(tree->size(), 2);
    EXPECT_EQ(engine.get_first_incomplete_expansion_room(), nullptr);

    // The door of the removed incomplete room is gone and the completed
    // neighbour room is reachable through a new 1-dimensional door.
    ASSERT_EQ(room->get_doors().size(), 1u);
    fr::autoroute::ExpansionDoor* new_door = room->get_doors().front();
    EXPECT_NE(new_door, door);
    EXPECT_EQ(new_door->get_dimension(), 1);
    auto* completed_neighbour = dynamic_cast<fr::autoroute::CompleteFreeSpaceExpansionRoom*>(
        other_room(new_door, room));
    ASSERT_NE(completed_neighbour, nullptr);
    EXPECT_EQ(completed_neighbour->get_shape(), fp::IntBox(2000, 1000, 3000, 2000));
}

TEST(AutorouteEngineTest, CompleteNeighbourRoomsCalculatesObstacleNeighbourDoors)
{
    // Java AutorouteEngine.completeNeighbourRooms(): an obstacle room neighbour
    // gets its doors calculated only once.
    auto board = make_routing_board();
    fr::autoroute::AutorouteEngine engine(board.get(), 1, true);
    engine.set_net_no(1);
    fr::board::ShapeSearchTree* tree = engine.get_autoroute_search_tree();
    ASSERT_NE(tree, nullptr);

    fr::autoroute::CompleteFreeSpaceExpansionRoom* room = complete_room(engine, fp::IntBox(1000, 1000, 2000, 2000));
    ASSERT_NE(room, nullptr);

    // A net-1 trace obstacle providing the tree shape of the obstacle room.
    fr::board::PolylineTrace* trace =
        insert_trace(board.get(), {fp::IntPoint(3000, 500), fp::IntPoint(3000, 2500)}, {1}, tree);
    ASSERT_NE(trace, nullptr);

    fr::autoroute::ObstacleExpansionRoom obstacle_room(trace, 0, 0, fp::IntBox(2950, 500, 3050, 2500));
    ASSERT_FALSE(obstacle_room.all_doors_calculated());
    auto* door = new fr::autoroute::ExpansionDoor(room, &obstacle_room, 1);
    room->add_door(door);
    obstacle_room.add_door(door);

    engine.complete_neighbour_rooms(room);

    EXPECT_TRUE(obstacle_room.all_doors_calculated());
    // The door of the obstacle room is kept.
    ASSERT_EQ(room->get_doors().size(), 1u);
    EXPECT_EQ(room->get_doors().front(), door);

    // A second call is a no-op for the already calculated obstacle room.
    const size_t obstacle_door_count = obstacle_room.get_doors().size();
    engine.complete_neighbour_rooms(room);
    EXPECT_EQ(obstacle_room.get_doors().size(), obstacle_door_count);
}

TEST(AutorouteEngineTest, CompleteNeighbourRoomsIgnoresNullRoom)
{
    auto board = make_routing_board();
    fr::autoroute::AutorouteEngine engine(board.get(), 1, true);
    engine.set_net_no(1);
    engine.complete_neighbour_rooms(nullptr);
    EXPECT_EQ(engine.get_first_incomplete_expansion_room(), nullptr);
}
