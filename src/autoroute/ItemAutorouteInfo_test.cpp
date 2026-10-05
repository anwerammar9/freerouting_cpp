#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include "autoroute/AutorouteEngine.h"
#include "autoroute/ExpansionRoom.h"
#include "autoroute/ItemAutorouteInfo.h"
#include "board/FixedState.h"
#include "board/Layer.h"
#include "board/PolylineTrace.h"
#include "board/RoutingBoard.h"
#include "board/ShapeSearchTree.h"
#include "board/Trace.h"
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
    return std::make_shared<fr::board::RoutingBoard>(fp::IntBox(0, 0, 10000, 10000),
        layer_structure, std::move(board_rules), nullptr);
}

fr::board::PolylineTrace* insert_trace(fr::board::RoutingBoard* p_board,
    const std::vector<fp::IntPoint>& p_corners, const std::vector<int>& p_nets)
{
    auto polyline = std::make_shared<fp::Polyline>(fp::Polyline::from_corners(p_corners));
    return p_board->insert_trace_without_cleaning(polyline, 0, 50, p_nets, 1,
        fr::board::FixedState::NOT_FIXED);
}

/** Storable that is not a board item, to check the connectable guard. */
class FakeStorable : public fr::datastructures::ShapeTree::Storable
{
public:
    int tree_shape_count(const fr::datastructures::ShapeTree&) const override { return 0; }
    const fr::geometry::planar::TileShape* get_tree_shape(const fr::datastructures::ShapeTree&, int) const override { return nullptr; }
    void set_search_tree_entries(std::vector<std::unique_ptr<fr::datastructures::ShapeTree::Leaf>>, fr::datastructures::ShapeTree&) override {}
    int compare_to(const fr::datastructures::ShapeTree::Storable&) const override { return 0; }
};

} // namespace

TEST(ItemAutorouteInfoTest, GetExpansionRoomCreatesAndCachesRoom)
{
    auto board = make_routing_board();
    fr::board::ShapeSearchTree90Degree tree(board.get(), 0);
    fr::board::PolylineTrace* trace = insert_trace(board.get(),
        {fp::IntPoint(0, 0), fp::IntPoint(200, 0)}, {1});
    ASSERT_NE(trace, nullptr);

    std::shared_ptr<fr::autoroute::ItemAutorouteInfo> info = trace->get_autoroute_info();
    ASSERT_NE(info, nullptr);

    fr::autoroute::ObstacleExpansionRoom* room = info->get_expansion_room(0, &tree);
    ASSERT_NE(room, nullptr);
    EXPECT_EQ(room->get_item(), trace);
    EXPECT_EQ(room->get_index_in_item(), 0);
    EXPECT_EQ(room->get_layer(), 0);

    // The room covers the tree shape of the trace.
    const fp::TileShape* tree_shape = trace->get_tree_shape(tree, 0);
    ASSERT_NE(tree_shape, nullptr);
    EXPECT_EQ(room->get_shape(), tree_shape->bounding_box());

    // The room id is derived from the item id and the shape index.
    EXPECT_EQ(room->get_room_id(), (trace->get_id() << 10) | 0);

    // A second call returns the same cached room.
    EXPECT_EQ(info->get_expansion_room(0, &tree), room);
}

TEST(ItemAutorouteInfoTest, GetExpansionRoomOutOfRangeReturnsNull)
{
    auto board = make_routing_board();
    fr::board::ShapeSearchTree90Degree tree(board.get(), 0);
    fr::board::PolylineTrace* trace = insert_trace(board.get(),
        {fp::IntPoint(0, 0), fp::IntPoint(200, 0)}, {1});
    ASSERT_NE(trace, nullptr);

    std::shared_ptr<fr::autoroute::ItemAutorouteInfo> info = trace->get_autoroute_info();
    ASSERT_NE(info, nullptr);
    EXPECT_EQ(info->get_expansion_room(1, &tree), nullptr);
    EXPECT_EQ(info->get_expansion_room(-1, &tree), nullptr);
    EXPECT_EQ(info->get_expansion_room(0, nullptr), nullptr);
}

TEST(ItemAutorouteInfoTest, ExpansionRoomForEachTraceShape)
{
    // A two-segment trace has two tree shapes and therefore two rooms, one
    // per shape index.
    auto board = make_routing_board();
    fr::board::ShapeSearchTree90Degree tree(board.get(), 0);
    fr::board::PolylineTrace* trace = insert_trace(board.get(),
        {fp::IntPoint(0, 0), fp::IntPoint(200, 0), fp::IntPoint(200, 200)}, {1});
    ASSERT_NE(trace, nullptr);
    EXPECT_EQ(trace->tree_shape_count(tree), 2);

    std::shared_ptr<fr::autoroute::ItemAutorouteInfo> info = trace->get_autoroute_info();
    ASSERT_NE(info, nullptr);
    fr::autoroute::ObstacleExpansionRoom* room0 = info->get_expansion_room(0, &tree);
    fr::autoroute::ObstacleExpansionRoom* room1 = info->get_expansion_room(1, &tree);
    ASSERT_NE(room0, nullptr);
    ASSERT_NE(room1, nullptr);
    EXPECT_EQ(room0->get_index_in_item(), 0);
    EXPECT_EQ(room1->get_index_in_item(), 1);
    EXPECT_NE(room0, room1);
}

TEST(ItemAutorouteInfoTest, OverlapDoorCreatedBetweenTracesOnSameNet)
{
    auto board = make_routing_board();
    fr::board::ShapeSearchTree90Degree tree(board.get(), 0);
    fr::board::PolylineTrace* horizontal = insert_trace(board.get(),
        {fp::IntPoint(0, 0), fp::IntPoint(200, 0)}, {1});
    fr::board::PolylineTrace* vertical = insert_trace(board.get(),
        {fp::IntPoint(100, -100), fp::IntPoint(100, 100)}, {1});
    ASSERT_NE(horizontal, nullptr);
    ASSERT_NE(vertical, nullptr);

    fr::autoroute::ObstacleExpansionRoom room_a(horizontal, 0, &tree);
    fr::autoroute::ObstacleExpansionRoom room_b(vertical, 0, &tree);

    EXPECT_TRUE(room_a.create_overlap_door(&room_b));
    EXPECT_TRUE(room_a.door_exists(&room_b));
    EXPECT_TRUE(room_b.door_exists(&room_a));
    EXPECT_EQ(room_a.get_doors().size(), 1u);
    EXPECT_EQ(room_b.get_doors().size(), 1u);

    // The door exists on both sides, so a second creation attempt is rejected.
    EXPECT_FALSE(room_a.create_overlap_door(&room_b));
    EXPECT_EQ(room_a.get_doors().size(), 1u);
}

TEST(ItemAutorouteInfoTest, OverlapDoorDeniedForDifferentNets)
{
    auto board = make_routing_board();
    fr::board::ShapeSearchTree90Degree tree(board.get(), 0);
    fr::board::PolylineTrace* net1 = insert_trace(board.get(),
        {fp::IntPoint(0, 0), fp::IntPoint(200, 0)}, {1});
    fr::board::PolylineTrace* net2 = insert_trace(board.get(),
        {fp::IntPoint(100, -100), fp::IntPoint(100, 100)}, {2});
    ASSERT_NE(net1, nullptr);
    ASSERT_NE(net2, nullptr);

    fr::autoroute::ObstacleExpansionRoom room_a(net1, 0, &tree);
    fr::autoroute::ObstacleExpansionRoom room_b(net2, 0, &tree);

    EXPECT_FALSE(room_a.create_overlap_door(&room_b));
    EXPECT_FALSE(room_a.door_exists(&room_b));
}

TEST(ItemAutorouteInfoTest, OverlapDoorBetweenConsecutiveShapesOfSameTrace)
{
    // Two shapes of the same trace (a polyline) are only connected when their
    // indices are consecutive.
    auto board = make_routing_board();
    fr::board::ShapeSearchTree90Degree tree(board.get(), 0);
    fr::board::PolylineTrace* trace = insert_trace(board.get(),
        {fp::IntPoint(0, 0), fp::IntPoint(200, 0), fp::IntPoint(200, 200)}, {1});
    ASSERT_NE(trace, nullptr);

    fr::autoroute::ObstacleExpansionRoom room0(trace, 0, &tree);
    fr::autoroute::ObstacleExpansionRoom room1(trace, 1, &tree);
    fr::autoroute::ObstacleExpansionRoom room0_again(trace, 0, &tree);

    EXPECT_TRUE(room0.create_overlap_door(&room1));
    EXPECT_TRUE(room1.door_exists(&room0));
    // A room of the same trace whose index is not adjacent (0 to 0) is not
    // connected.
    EXPECT_FALSE(room0.create_overlap_door(&room0_again));
    // Consecutive shapes are always connected, even through a fresh room.
    EXPECT_TRUE(room0_again.create_overlap_door(&room1));
    EXPECT_TRUE(room1.door_exists(&room0_again));
}

TEST(ItemAutorouteInfoTest, TargetDoorsCalculatedForNetItemOverlappingRoom)
{
    auto board = make_routing_board();
    fr::board::ShapeSearchTree90Degree tree(board.get(), 0);
    fr::board::PolylineTrace* trace = insert_trace(board.get(),
        {fp::IntPoint(0, 0), fp::IntPoint(200, 0)}, {1});
    ASSERT_NE(trace, nullptr);

    fr::autoroute::CompleteFreeSpaceExpansionRoom room(0, 5, fp::IntBox(-60, -60, 260, 60));
    fr::datastructures::ShapeTree::TreeEntry own_net_entry(trace, 0);

    room.calculate_target_doors(own_net_entry, /*p_net_no=*/1, &tree);

    // Touching a net object makes the room net dependent.
    EXPECT_TRUE(room.is_net_dependent());
    ASSERT_EQ(room.target_doors().size(), 1u);
    auto* target_door = dynamic_cast<fr::autoroute::TargetItemExpansionDoor*>(room.target_doors()[0]);
    ASSERT_NE(target_door, nullptr);
    EXPECT_EQ(target_door->item, trace);
    EXPECT_EQ(target_door->tree_entry_no, 0);
    EXPECT_EQ(target_door->get_room(), &room);

    // The door shape is the room clipped to the item shape.
    const fp::TileShape* tree_shape = trace->get_tree_shape(tree, 0);
    ASSERT_NE(tree_shape, nullptr);
    EXPECT_EQ(target_door->get_shape(), fp::IntBox(-60, -60, 260, 60).intersection(tree_shape->bounding_box()));
    EXPECT_EQ(target_door->get_dimension(), 2);
}

TEST(ItemAutorouteInfoTest, TargetDoorsSkippedForDisjointRoomOtherNetAndNonItem)
{
    auto board = make_routing_board();
    fr::board::ShapeSearchTree90Degree tree(board.get(), 0);
    fr::board::PolylineTrace* trace = insert_trace(board.get(),
        {fp::IntPoint(0, 0), fp::IntPoint(200, 0)}, {1});
    ASSERT_NE(trace, nullptr);

    // A room far away from the trace gets no target door.
    fr::autoroute::CompleteFreeSpaceExpansionRoom disjoint_room(0, 6, fp::IntBox(1000, 1000, 2000, 2000));
    fr::datastructures::ShapeTree::TreeEntry trace_entry(trace, 0);
    disjoint_room.calculate_target_doors(trace_entry, 1, &tree);
    EXPECT_TRUE(disjoint_room.target_doors().empty());

    // An item not carrying the net gets no target door.
    fr::board::PolylineTrace* other_net = insert_trace(board.get(),
        {fp::IntPoint(0, 0), fp::IntPoint(200, 0)}, {2});
    ASSERT_NE(other_net, nullptr);
    fr::autoroute::CompleteFreeSpaceExpansionRoom room(0, 7, fp::IntBox(-60, -60, 260, 60));
    fr::datastructures::ShapeTree::TreeEntry other_entry(other_net, 0);
    room.calculate_target_doors(other_entry, 1, &tree);
    EXPECT_TRUE(room.target_doors().empty());

    // A Storable that is not a board item gets no target door.
    FakeStorable fake;
    fr::datastructures::ShapeTree::TreeEntry fake_entry(&fake, 0);
    room.calculate_target_doors(fake_entry, 1, &tree);
    EXPECT_TRUE(room.target_doors().empty());
}

TEST(AutorouteEngineTest, RemoveAllDoorsClearsBothDoorSides)
{
    auto board = make_routing_board();
    fr::autoroute::AutorouteEngine engine(board.get(), 1, true);

    fr::autoroute::CompleteFreeSpaceExpansionRoom complete_room(0, 7, fp::IntBox(0, 0, 100, 100));
    fr::autoroute::IncompleteFreeSpaceExpansionRoom incomplete_room(nullptr, 0, fp::IntBox(0, 0, 100, 100));
    auto* door = new fr::autoroute::ExpansionDoor(&complete_room, &incomplete_room, 2);
    complete_room.add_door(door);
    incomplete_room.add_door(door);
    EXPECT_EQ(complete_room.get_doors().size(), 1u);
    EXPECT_EQ(incomplete_room.get_doors().size(), 1u);

    engine.remove_all_doors(&complete_room);

    EXPECT_TRUE(complete_room.get_doors().empty());
    EXPECT_TRUE(incomplete_room.get_doors().empty());
}

TEST(AutorouteEngineTest, CompletingRoomRemovesDoorsAndErasesIncompleteRoom)
{
    auto board = make_routing_board();
    fr::autoroute::AutorouteEngine engine(board.get(), 1, true);

    // The incomplete room is registered in (and owned by) the engine.
    auto* incomplete_room = new fr::autoroute::IncompleteFreeSpaceExpansionRoom(
        nullptr, 0, fp::IntBox(100, 200, 300, 400));
    engine.add_incomplete_expansion_room(incomplete_room);

    fr::autoroute::CompleteFreeSpaceExpansionRoom complete_room(0, 7, fp::IntBox(0, 0, 100, 100));
    auto* door = new fr::autoroute::ExpansionDoor(&complete_room, incomplete_room, 2);
    complete_room.add_door(door);
    incomplete_room->add_door(door);

    // Completing the registered incomplete room mirrors Java
    // completeExpansionRoom(): the doors of the (removed) incomplete room are
    // removed from both sides.
    std::vector<fr::autoroute::CompleteFreeSpaceExpansionRoom*> completed =
        engine.complete_expansion_room(incomplete_room);
    ASSERT_EQ(completed.size(), 1u);
    EXPECT_TRUE(complete_room.get_doors().empty());
}