#include <gtest/gtest.h>

#include <memory>
#include <set>
#include <vector>

#include "autoroute/AutorouteEngine.h"
#include "autoroute/DrillPage.h"
#include "autoroute/ExpansionRoom.h"
#include "board/Layer.h"
#include "board/RoutingBoard.h"
#include "board/SearchTreeObject.h"
#include "board/ShapeSearchTree.h"
#include "geometry/planar/IntBox.h"
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
    // These fixtures exercise the engine bookkeeping on the generic search tree; the Java
    // 45 degree tree (ShapeSearchTree45Degree, T506a) has its own tests.
    board_rules->set_trace_angle_restriction(fr::rules::AngleRestriction::NONE);
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

/** Net-2 obstacle used to restrain rooms in the engine database. */
class FakeObstacle : public fr::board::SearchTreeObject
{
public:
    FakeObstacle(int p_layer, fp::IntBox p_shape, int p_id)
        : layer_(p_layer), shape_(p_shape), id_(p_id)
    {
    }

    int tree_shape_count(const fr::datastructures::ShapeTree&) const override { return 1; }
    const fp::TileShape* get_tree_shape(const fr::datastructures::ShapeTree&, int) const override { return &shape_; }
    void set_search_tree_entries(std::vector<std::unique_ptr<fr::datastructures::ShapeTree::Leaf>> p_entries,
        fr::datastructures::ShapeTree&) override
    {
        // Own the leaves like board::Item does so the search tree keeps
        // referencing live nodes until the object is destroyed.
        tree_entries_ = std::move(p_entries);
    }

    int compare_to(const fr::datastructures::ShapeTree::Storable& p_other) const override
    {
        const auto* other_obstacle = dynamic_cast<const FakeObstacle*>(&p_other);
        if (other_obstacle == nullptr)
        {
            return -1;
        }
        if (id_ == other_obstacle->id_)
        {
            return 0;
        }
        return id_ < other_obstacle->id_ ? -1 : 1;
    }
    bool is_obstacle(int) const override { return true; }
    bool is_trace_obstacle(int p_net_no) const override { return p_net_no != 2; }
    int shape_layer(int) const override { return layer_; }
    int get_id() const override { return id_; }

private:
    int layer_;
    fp::IntBox shape_;
    int id_;
    std::vector<std::unique_ptr<fr::datastructures::ShapeTree::Leaf>> tree_entries_;
};

/** True if p_piece's interior does not overlap the interior of p_obstacle. */
bool avoids(const fp::IntBox& p_piece, const fp::IntBox& p_obstacle)
{
    return p_piece.to_Simplex().intersection(p_obstacle.to_Simplex()).dimension() < 2;
}

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
    // The room carries its exact shape as a Simplex (Java getTreeShape() returns
    // the room shape TileShape), the bounding box is the constructor box.
    EXPECT_EQ(tree_shape->bounding_box(), shape);
    EXPECT_EQ(tree_shape->dimension(), 2);
    EXPECT_EQ(room.get_shape_tile().lines.size(), shape.to_Simplex().lines.size());
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

TEST(CompleteFreeSpaceExpansionRoomTest, RoomIsRestrainedAroundObstacle)
{
    // A 2-dimensional room containing a net-2 obstacle on its layer must be
    // completed through ShapeSearchTree::complete_shape (WP-10 Increment C):
    // the obstacle restrains the room, so the completion is split into pieces
    // that do not overlap the obstacle interior instead of returning the raw
    // room box.
    auto board = make_routing_board();
    fr::autoroute::AutorouteEngine engine(board.get(), 1, true);
    engine.set_net_no(1);
    fr::board::ShapeSearchTree* tree = engine.get_autoroute_search_tree();
    ASSERT_NE(tree, nullptr);

    fp::IntBox wall(4000, 0, 6000, 10000);
    FakeObstacle obstacle(0, wall, /*p_id=*/1);
    tree->insert(obstacle);

    // The room box covers the whole wall. The completion must not return a
    // single room covering the raw box.
    fr::autoroute::IncompleteFreeSpaceExpansionRoom incomplete(
        nullptr, 0, fp::IntBox(0, 0, 10000, 10000));
    std::vector<fr::autoroute::CompleteFreeSpaceExpansionRoom*> rooms =
        engine.complete_expansion_room(&incomplete);

    ASSERT_FALSE(rooms.empty()) << "complete_shape must complete a room despite the obstacle";
    bool some_room_restrained = false;
    for (fr::autoroute::CompleteFreeSpaceExpansionRoom* room : rooms)
    {
        ASSERT_NE(room, nullptr);
        EXPECT_EQ(room->get_layer(), 0);
        fp::IntBox shape = room->get_shape();
        EXPECT_EQ(shape.dimension(), 2);
        EXPECT_TRUE(avoids(shape, wall))
            << "completed room overlaps the obstacle interior: " << shape.to_string();
        if (shape != fp::IntBox(0, 0, 10000, 10000))
        {
            some_room_restrained = true;
        }
    }
    EXPECT_TRUE(some_room_restrained);
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

    // The engines of one board share its autoroute search tree (Java
    // SearchTreeManager.getAutorouteTree), so the second engine gets its own board.
    auto purging_board = make_routing_board();
    fr::autoroute::AutorouteEngine purging_engine(purging_board.get(), 1, true);
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

    // Whole-plane drill rooms only complete to exactly one free space room
    // (Java calculateExpansionRooms requires size == 1) when the free space is
    // restrained; on an obstacle-free tree the board is divided into sections
    // and the drill would be dropped. The net-2 walls restrain the free space
    // on both layers.
    FakeObstacle wall_layer_0(0, fp::IntBox(4000, 0, 6000, 10000), /*p_id=*/1);
    FakeObstacle wall_layer_1(1, fp::IntBox(4000, 0, 6000, 10000), /*p_id=*/2);
    tree->insert(wall_layer_0);
    tree->insert(wall_layer_1);
    ASSERT_EQ(tree->size(), 2);

    fr::autoroute::DrillPage page(fr::geometry::planar::IntBox(0, 0, 1000, 1000), board.get());
    auto drills = page.get_drills(&engine, false);
    ASSERT_EQ(drills.size(), 1u);
    ASSERT_EQ(drills[0]->room_arr.size(), 2u);

    // The rooms completed for the drill are stored in the search tree next to
    // the two wall obstacles.
    EXPECT_EQ(tree->size(), 4);
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

    // The engine rooms live in the board search tree; remove them while the
    // fixture objects linked into the tree still exist (Java calls clear() too).
    engine.clear();
}

// ==========================================================================
// Whole-plane (Java null shape) rooms
// ==========================================================================

TEST(CompleteFreeSpaceExpansionRoomTest, WholePlaneRoomCompletesToRestrainedFreeSpace)
{
    // Java IncompleteFreeSpaceExpansionRoom(null, layer, containedShape): the
    // completion starts from the whole board bounding box, so the completed
    // room is the maximal free space containing the contained shape.
    auto board = make_routing_board();
    fr::autoroute::AutorouteEngine engine(board.get(), 1, true);
    engine.set_net_no(1);
    fr::board::ShapeSearchTree* tree = engine.get_autoroute_search_tree();
    ASSERT_NE(tree, nullptr);
    FakeObstacle wall(0, fp::IntBox(4000, 0, 6000, 10000), /*p_id=*/1);
    tree->insert(wall);

    fr::autoroute::IncompleteFreeSpaceExpansionRoom room(
        nullptr, 0, fr::autoroute::kWholePlaneShape, fp::IntBox(500, 500, 501, 501).to_Simplex());
    EXPECT_TRUE(room.is_whole_plane());
    EXPECT_EQ(room.get_shape(), fp::IntBox::EMPTY);

    std::vector<fr::autoroute::CompleteFreeSpaceExpansionRoom*> rooms =
        engine.complete_expansion_room(&room);
    ASSERT_EQ(rooms.size(), 1u);
    EXPECT_EQ(rooms[0]->get_shape(), fp::IntBox(0, 0, 4000, 10000));
}

TEST(CompleteFreeSpaceExpansionRoomTest, WholePlaneRoomNeverCompletesToOneRoomOnObstacleFreeTree)
{
    // On a tree without obstacles the whole-board free space is larger than
    // half of the board in both directions and ShapeSearchTree
    // divide_large_room splits it into sections (Java divideLargeRoom), so the
    // completion never yields exactly one room. ExpansionDrill
    // calculateExpansionRooms requires size == 1 and therefore drops the
    // drill — the Java behaviour on obstacle-free boards.
    auto board = make_routing_board();
    fr::autoroute::AutorouteEngine engine(board.get(), 1, true);
    engine.set_net_no(1);
    engine.get_autoroute_search_tree();

    fr::autoroute::IncompleteFreeSpaceExpansionRoom room(
        nullptr, 0, fr::autoroute::kWholePlaneShape, fp::IntBox(500, 500, 501, 501).to_Simplex());
    std::vector<fr::autoroute::CompleteFreeSpaceExpansionRoom*> rooms =
        engine.complete_expansion_room(&room);
    EXPECT_NE(rooms.size(), 1u);
    EXPECT_GE(rooms.size(), 2u);
    for (fr::autoroute::CompleteFreeSpaceExpansionRoom* completed : rooms)
    {
        ASSERT_NE(completed, nullptr);
        EXPECT_EQ(completed->get_shape().dimension(), 2);
    }
}
