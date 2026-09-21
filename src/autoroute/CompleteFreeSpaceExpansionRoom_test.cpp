#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include "autoroute/AutorouteEngine.h"
#include "autoroute/DrillPage.h"
#include "autoroute/ExpansionRoom.h"
#include "board/Layer.h"
#include "board/RoutingBoard.h"
#include "board/ShapeSearchTree.h"
#include "geometry/planar/IntBox.h"
#include "rules/BoardRules.h"

namespace fr = freerouting;

namespace {

std::shared_ptr<fr::board::RoutingBoard> make_routing_board()
{
    std::vector<std::shared_ptr<fr::board::Layer>> layers;
    layers.push_back(std::make_shared<fr::board::Layer>("Top", true));
    layers.push_back(std::make_shared<fr::board::Layer>("Bottom", true));
    fr::board::LayerStructure layer_structure(layers);
    auto board_rules = std::make_unique<fr::rules::BoardRules>(layer_structure, 10);
    return std::make_shared<fr::board::RoutingBoard>(
        fr::geometry::planar::IntBox(0, 0, 10000, 10000), layer_structure,
        std::move(board_rules), nullptr);
}

/** Minimal non-room ShapeTree::Storable to verify the room compare_to ordering. */
class FakeStorable : public fr::datastructures::ShapeTree::Storable
{
public:
    int tree_shape_count(const fr::datastructures::ShapeTree&) const override { return 0; }
    const fr::geometry::planar::TileShape* get_tree_shape(const fr::datastructures::ShapeTree&, int) const override { return nullptr; }
    void set_search_tree_entries(std::vector<std::unique_ptr<fr::datastructures::ShapeTree::Leaf>>, fr::datastructures::ShapeTree&) override {}
    int compare_to(const fr::datastructures::ShapeTree::Storable&) const override { return 0; }
};

} // namespace

TEST(CompleteFreeSpaceExpansionRoomTest, StorableContract)
{
    auto board = make_routing_board();
    fr::board::ShapeSearchTree90Degree tree(board.get(), 0);

    fr::geometry::planar::IntBox shape(0, 0, 100, 100);
    fr::autoroute::CompleteFreeSpaceExpansionRoom room(3, 7, shape);

    EXPECT_EQ(room.tree_shape_count(tree), 1);
    const fr::geometry::planar::TileShape* tree_shape = room.get_tree_shape(tree, 0);
    ASSERT_NE(tree_shape, nullptr);
    const auto* box_shape = dynamic_cast<const fr::geometry::planar::IntBox*>(tree_shape);
    ASSERT_NE(box_shape, nullptr);
    EXPECT_EQ(*box_shape, shape);
    EXPECT_EQ(room.get_tree_shape(tree, 1), nullptr);
    EXPECT_EQ(room.shape_layer(0), 3);
    EXPECT_EQ(room.shape_layer(1), 3);
    EXPECT_TRUE(room.is_obstacle(1));
    EXPECT_TRUE(room.is_obstacle(-1));
    EXPECT_TRUE(room.is_trace_obstacle(5));
    EXPECT_FALSE(room.is_net_dependent());
    room.set_net_dependent();
    EXPECT_TRUE(room.is_net_dependent());
}

TEST(CompleteFreeSpaceExpansionRoomTest, CompareToOrdersByReversedId)
{
    fr::geometry::planar::IntBox shape(0, 0, 10, 10);
    fr::autoroute::CompleteFreeSpaceExpansionRoom room_a(0, 1, shape);
    fr::autoroute::CompleteFreeSpaceExpansionRoom room_b(0, 2, shape);

    // room_a(id 1).compareTo(room_b(id 2)) = 2 - 1 = 1.
    EXPECT_EQ(room_a.compare_to(room_b), 1);
    EXPECT_EQ(room_b.compare_to(room_a), -1);
    EXPECT_EQ(room_a.compare_to(room_a), 0);

    // Rooms sort before any non-room storable.
    FakeStorable fake;
    EXPECT_LT(room_a.compare_to(fake), 0);
}

TEST(CompleteFreeSpaceExpansionRoomTest, CompletedRoomIsStoredInSearchTree)
{
    auto board = make_routing_board();
    fr::autoroute::AutorouteEngine engine(board.get(), 1, false);
    fr::board::ShapeSearchTree* tree = engine.get_autoroute_search_tree();
    ASSERT_NE(tree, nullptr);
    EXPECT_EQ(tree->size(), 0);

    fr::autoroute::IncompleteFreeSpaceExpansionRoom incomplete(
        nullptr, 0, fr::geometry::planar::IntBox(100, 200, 300, 400));
    std::vector<fr::autoroute::CompleteFreeSpaceExpansionRoom*> rooms =
        engine.complete_expansion_room(&incomplete);
    ASSERT_EQ(rooms.size(), 1u);

    // The completed room is inserted into the autoroute search tree.
    EXPECT_EQ(tree->size(), 1);

    // The room is discoverable by overlapping objects at its location.
    std::set<fr::board::SearchTreeObject*, fr::board::TreeStorableLess> overlaps =
        tree->overlapping_objects(fr::geometry::planar::IntBox(150, 250, 250, 350), -1);
    ASSERT_EQ(overlaps.size(), 1u);
    EXPECT_EQ(overlaps.count(static_cast<fr::board::SearchTreeObject*>(rooms[0])), 1u);

    // clear() removes the room from the tree again.
    engine.clear();
    EXPECT_EQ(tree->size(), 0);
    EXPECT_TRUE(tree->overlapping_objects(fr::geometry::planar::IntBox(150, 250, 250, 350), -1).empty());
}

TEST(CompleteFreeSpaceExpansionRoomTest, NetDependentRoomPurgedOnNetChange)
{
    // A retained (maintain_database) engine keeps non-net-dependent rooms
    // across net changes and purges net-dependent ones.
    auto board = make_routing_board();

    fr::autoroute::AutorouteEngine retained_engine(board.get(), 1, true);
    fr::autoroute::IncompleteFreeSpaceExpansionRoom incomplete_a(
        nullptr, 0, fr::geometry::planar::IntBox(100, 200, 300, 400));
    retained_engine.complete_expansion_room(&incomplete_a);
    retained_engine.init_connection(2, nullptr, 0);
    EXPECT_EQ(retained_engine.get_autoroute_search_tree()->size(), 1);

    fr::autoroute::AutorouteEngine purging_engine(board.get(), 1, true);
    fr::autoroute::IncompleteFreeSpaceExpansionRoom incomplete_b(
        nullptr, 0, fr::geometry::planar::IntBox(100, 200, 300, 400));
    std::vector<fr::autoroute::CompleteFreeSpaceExpansionRoom*> rooms =
        purging_engine.complete_expansion_room(&incomplete_b);
    ASSERT_EQ(rooms.size(), 1u);
    rooms[0]->set_net_dependent();
    EXPECT_EQ(purging_engine.get_autoroute_search_tree()->size(), 1);

    purging_engine.init_connection(2, nullptr, 0);
    EXPECT_EQ(purging_engine.get_autoroute_search_tree()->size(), 0);
}

TEST(CompleteFreeSpaceExpansionRoomTest, DrillRoomIsStoredInTreeAndReusedAfterRecalculation)
{
    auto board = make_routing_board();
    fr::autoroute::AutorouteEngine engine(board.get(), 1, true);
    engine.set_net_no(1);
    fr::board::ShapeSearchTree* tree = engine.get_autoroute_search_tree();
    ASSERT_NE(tree, nullptr);

    fr::autoroute::DrillPage page(fr::geometry::planar::IntBox(0, 0, 1000, 1000), board.get());
    auto drills = page.get_drills(&engine, false);
    ASSERT_EQ(drills.size(), 1u);
    ASSERT_EQ(drills[0]->room_arr.size(), 2u);

    // The rooms completed for the drill are stored in the search tree.
    EXPECT_EQ(tree->size(), 2);
    fr::autoroute::CompleteFreeSpaceExpansionRoom* room_l0 =
        dynamic_cast<fr::autoroute::CompleteFreeSpaceExpansionRoom*>(drills[0]->room_arr[0]);
    ASSERT_NE(room_l0, nullptr);
    std::set<fr::board::SearchTreeObject*, fr::board::TreeStorableLess> overlaps =
        tree->overlapping_objects(room_l0->get_shape(), -1);
    EXPECT_EQ(overlaps.count(static_cast<fr::board::SearchTreeObject*>(room_l0)), 1u);

    // The same drill location on the same net is reused after invalidation.
    page.invalidate();
    auto cached = page.get_drills(&engine, false);
    ASSERT_EQ(cached.size(), 1u);
    EXPECT_EQ(cached[0]->room_arr[0], drills[0]->room_arr[0]);
    EXPECT_EQ(cached[0]->room_arr[1], drills[0]->room_arr[1]);

    // Net change purges nothing, but a fresh net keeps the database intact.
    engine.init_connection(2, nullptr, 0);
    page.invalidate();
    auto after_net_change = page.get_drills(&engine, false);
    ASSERT_EQ(after_net_change.size(), 1u);
    EXPECT_EQ(after_net_change[0]->room_arr[0], drills[0]->room_arr[0]);
}