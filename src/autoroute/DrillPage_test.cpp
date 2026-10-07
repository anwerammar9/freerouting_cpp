#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>
#include <memory>
#include <vector>

#include "autoroute/AutorouteEngine.h"
#include "autoroute/DrillPage.h"
#include "autoroute/ExpansionDrill.h"
#include "autoroute/ExpansionRoom.h"
#include "board/Layer.h"
#include "board/PolylineTrace.h"
#include "board/RoutingBoard.h"
#include "board/SearchTreeObject.h"
#include "board/ShapeSearchTree.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/Polyline.h"
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

fr::board::PolylineTrace* insert_trace(
    fr::board::RoutingBoard* p_board,
    const std::vector<fr::geometry::planar::IntPoint>& p_corners,
    const std::vector<int>& p_nets)
{
    auto polyline = std::make_shared<fr::geometry::planar::Polyline>(
        fr::geometry::planar::Polyline::from_corners(p_corners));
    return p_board->insert_trace_without_cleaning(polyline, 0, 50, p_nets, 1,
                                                  fr::board::FixedState::NOT_FIXED);
}

/**
 * Net-2 obstacle restraining the free space in the autoroute search tree.
 * Whole-plane drill rooms complete to exactly one room (the condition of
 * ExpansionDrill.calculateExpansionRooms) only when the free space around the
 * drill location is restrained by obstacles.
 */
class FakeObstacle : public fr::board::SearchTreeObject
{
public:
    FakeObstacle(int p_layer, fr::geometry::planar::IntBox p_shape, int p_id)
        : layer_(p_layer), shape_(p_shape), id_(p_id)
    {
    }

    int tree_shape_count(const fr::datastructures::ShapeTree&) const override { return 1; }
    const fr::geometry::planar::TileShape* get_tree_shape(const fr::datastructures::ShapeTree&, int) const override { return &shape_; }
    void set_search_tree_entries(std::vector<std::unique_ptr<fr::datastructures::ShapeTree::Leaf>> p_entries,
                                fr::datastructures::ShapeTree&) override
    {
        // Own the leaves like board::Item does so the search tree keeps
        // referencing live nodes until the object is destroyed.
        search_tree_entries_ = std::move(p_entries);
    }
    std::vector<std::unique_ptr<fr::datastructures::ShapeTree::Leaf>> search_tree_entries_;

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
    fr::geometry::planar::IntBox shape_;
    int id_;
};

} // namespace

TEST(DrillPageTest, EmptyPageProducesSingleFreeDrillWithRoomsOnAllLayers)
{
    auto board = make_routing_board();
    fr::autoroute::AutorouteEngine engine(board.get(), 1, false);
    engine.set_net_no(1);
    fr::autoroute::DrillPage page(fr::geometry::planar::IntBox(0, 0, 1000, 1000), board.get());

    // Whole-plane drill rooms only complete to exactly one room per layer (the
    // condition of ExpansionDrill.calculateExpansionRooms) when the free space
    // is restrained; the net-2 walls on both layers leave the free space left
    // of them as the room around the drill at (500, 500).
    fr::board::ShapeSearchTree* tree = engine.get_autoroute_search_tree();
    ASSERT_NE(tree, nullptr);
    FakeObstacle wall_layer_0(0, fr::geometry::planar::IntBox(4000, 0, 6000, 10000), /*p_id=*/1);
    FakeObstacle wall_layer_1(1, fr::geometry::planar::IntBox(4000, 0, 6000, 10000), /*p_id=*/2);
    tree->insert(wall_layer_0);
    tree->insert(wall_layer_1);

    auto drills = page.get_drills(&engine, false);
    ASSERT_EQ(drills.size(), 1u);

    fr::autoroute::ExpansionDrill* drill = drills[0];
    EXPECT_EQ(drill->first_layer, 0);
    EXPECT_EQ(drill->last_layer, 1);
    EXPECT_EQ(drill->get_shape(), page.shape);
    EXPECT_EQ(drill->location, fr::geometry::planar::IntPoint(500, 500));

    // One expansion room per layer, all 2-dimensional and matching the layers.
    ASSERT_EQ(drill->room_arr.size(), 2u);
    EXPECT_NE(drill->room_arr[0], nullptr);
    EXPECT_NE(drill->room_arr[1], nullptr);
    EXPECT_EQ(drill->room_arr[0]->get_layer(), 0);
    EXPECT_EQ(drill->room_arr[1]->get_layer(), 1);
    EXPECT_EQ(drill->room_arr[0]->get_shape().dimension(), 2);
    EXPECT_EQ(drill->room_arr[1]->get_shape().dimension(), 2);
    // The completed room is the free space left of the wall.
    EXPECT_EQ(drill->room_arr[0]->get_shape(), fr::geometry::planar::IntBox(0, 0, 4000, 10000));
    EXPECT_EQ(drill->room_arr[1]->get_shape(), fr::geometry::planar::IntBox(0, 0, 4000, 10000));
}

TEST(DrillPageTest, DrillsAreCachedByNetAndRecomputedAfterInvalidate)
{
    auto board = make_routing_board();
    fr::autoroute::AutorouteEngine engine(board.get(), 1, false);
    engine.set_net_no(1);
    fr::autoroute::DrillPage page(fr::geometry::planar::IntBox(0, 0, 1000, 1000), board.get());

    // Net-2 walls restrain the free space on both layers so the whole-plane
    // drill rooms complete to exactly one room and the drill is kept.
    fr::board::ShapeSearchTree* tree = engine.get_autoroute_search_tree();
    ASSERT_NE(tree, nullptr);
    FakeObstacle wall_layer_0(0, fr::geometry::planar::IntBox(4000, 0, 6000, 10000), /*p_id=*/1);
    FakeObstacle wall_layer_1(1, fr::geometry::planar::IntBox(4000, 0, 6000, 10000), /*p_id=*/2);
    tree->insert(wall_layer_0);
    tree->insert(wall_layer_1);

    auto drills = page.get_drills(&engine, false);
    ASSERT_EQ(drills.size(), 1u);

    // Same net without invalidation returns the cached drills.
    auto cached = page.get_drills(&engine, false);
    ASSERT_EQ(cached.size(), 1u);
    EXPECT_EQ(cached[0], drills[0]);

    // A new obstacle is not visible while the drills are cached.
    fr::board::PolylineTrace* trace =
        insert_trace(board.get(),
                     {fr::geometry::planar::IntPoint(0, 300), fr::geometry::planar::IntPoint(1000, 300)},
                     {2});
    auto still_cached = page.get_drills(&engine, false);
    ASSERT_EQ(still_cached.size(), 1u);
    EXPECT_EQ(still_cached[0], drills[0]);

    // Maintain the autoroute search tree the way the router would when new
    // items are added to the board, then invalidate so the drills are
    // recalculated and avoid the obstacle.
    engine.get_autoroute_search_tree()->insert(trace);
    page.invalidate();
    auto recomputed = page.get_drills(&engine, false);
    EXPECT_GT(recomputed.size(), 1u);
    for (fr::autoroute::ExpansionDrill* drill : recomputed)
    {
        // The obstacle trace has half width 50 around the center line y = 300.
        EXPECT_TRUE(drill->location.y <= 250 || drill->location.y >= 350);
    }
}

TEST(DrillPageTest, OverlappingObstacleCreatesMultipleDrillShapes)
{
    auto board = make_routing_board();
    fr::autoroute::AutorouteEngine engine(board.get(), 1, false);
    engine.set_net_no(1);

    // Obstacle on net 2 crossing the page horizontally through its center.
    fr::board::PolylineTrace* trace =
        insert_trace(board.get(),
                     {fr::geometry::planar::IntPoint(0, 500), fr::geometry::planar::IntPoint(1000, 500)},
                     {2});

    // The tree is built lazily with the trace above already on the board; the
    // net-2 walls then restrain the free space so every drill piece completes
    // to exactly one room per layer (whole-plane rooms are dropped otherwise).
    fr::board::ShapeSearchTree* tree = engine.get_autoroute_search_tree();
    ASSERT_NE(tree, nullptr);
    FakeObstacle wall_layer_0(0, fr::geometry::planar::IntBox(4000, 0, 6000, 10000), /*p_id=*/1);
    FakeObstacle wall_layer_1(1, fr::geometry::planar::IntBox(4000, 0, 6000, 10000), /*p_id=*/2);
    tree->insert(wall_layer_0);
    tree->insert(wall_layer_1);

    fr::autoroute::DrillPage page(fr::geometry::planar::IntBox(0, 0, 1000, 1000), board.get());
    auto drills = page.get_drills(&engine, false);

    // The page is split into at least 2 free pieces (above/below the trace).
    EXPECT_GE(drills.size(), 2u);
    // No drill location lies inside the obstacle band.
    for (fr::autoroute::ExpansionDrill* drill : drills)
    {
        EXPECT_TRUE(drill->location.y <= 450 || drill->location.y >= 550);
    }

    // All drills still get valid expansion rooms on every layer.
    for (fr::autoroute::ExpansionDrill* drill : drills)
    {
        EXPECT_EQ(drill->room_arr.size(), 2u);
        EXPECT_NE(drill->room_arr[0], nullptr);
        EXPECT_NE(drill->room_arr[1], nullptr);
    }

    // The obstacle item must be present in the engine search tree for the
    // cutout; sanity-check the via the drill locations avoid it.
    EXPECT_NE(trace, nullptr);
}

TEST(DrillPageTest, AttachSmdKeepsSameDrillShapeWithCentreOfGravityLocation)
{
    auto board = make_routing_board();
    fr::autoroute::AutorouteEngine engine(board.get(), 1, false);
    engine.set_net_no(1);

    // With no drillable pin item available (bare Pin objects need a component
    // library which the test board does not provide) attach_smd only changes
    // the drill location: calc_pin_center_in_drill() falls back to the centre
    // of gravity, so both modes agree on an obstacle-free page. The net-2
    // walls restrain the free space so the whole-plane rooms complete to
    // exactly one room per layer and the drills are kept.
    fr::autoroute::DrillPage page(fr::geometry::planar::IntBox(0, 0, 1000, 1000), board.get());

    fr::board::ShapeSearchTree* tree = engine.get_autoroute_search_tree();
    ASSERT_NE(tree, nullptr);
    FakeObstacle wall_layer_0(0, fr::geometry::planar::IntBox(4000, 0, 6000, 10000), /*p_id=*/1);
    FakeObstacle wall_layer_1(1, fr::geometry::planar::IntBox(4000, 0, 6000, 10000), /*p_id=*/2);
    tree->insert(wall_layer_0);
    tree->insert(wall_layer_1);

    auto drills_smd = page.get_drills(&engine, true);
    ASSERT_EQ(drills_smd.size(), 1u);
    EXPECT_EQ(drills_smd[0]->location, fr::geometry::planar::IntPoint(500, 500));
    EXPECT_EQ(drills_smd[0]->room_arr.size(), 2u);

    // attach_smd does not change the drill shape of an obstacle split page.
    fr::board::PolylineTrace* trace =
        insert_trace(board.get(),
                     {fr::geometry::planar::IntPoint(0, 500), fr::geometry::planar::IntPoint(1000, 500)},
                     {2});
    engine.get_autoroute_search_tree()->insert(trace);
    page.invalidate();
    auto drills_split_smd = page.get_drills(&engine, true);
    EXPECT_GE(drills_split_smd.size(), 2u);
    for (fr::autoroute::ExpansionDrill* drill : drills_split_smd)
    {
        EXPECT_EQ(drill->room_arr.size(), 2u);
        EXPECT_NE(drill->room_arr[0], nullptr);
        EXPECT_NE(drill->room_arr[1], nullptr);
    }
}

TEST(DrillPageTest, MazeSearchElementsPerLayer)
{
    auto board = make_routing_board();
    fr::autoroute::DrillPage page(fr::geometry::planar::IntBox(0, 0, 500, 500), board.get());

    EXPECT_EQ(page.maze_search_element_count(), 2);
    EXPECT_NE(page.get_maze_search_element(0), nullptr);
    EXPECT_NE(page.get_maze_search_element(1), nullptr);
    EXPECT_EQ(page.get_maze_search_element(2), nullptr);
    EXPECT_EQ(page.get_maze_search_element(-1), nullptr);

    page.reset();
    EXPECT_NE(page.get_maze_search_element(0), nullptr);
    EXPECT_NE(page.get_maze_search_element(1), nullptr);
}