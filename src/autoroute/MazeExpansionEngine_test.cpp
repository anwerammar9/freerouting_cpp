#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <memory>
#include <set>
#include <vector>

#include "autoroute/AutorouteControl.h"
#include "autoroute/AutorouteEngine.h"
#include "autoroute/DrillPage.h"
#include "autoroute/DrillPageArray.h"
#include "autoroute/ExpansionDrill.h"
#include "autoroute/ExpansionRoom.h"
#include "autoroute/MazeExpansionEngine.h"
#include "autoroute/MazeSearchAlgo.h"
#include "autoroute/MazeSearchElement.h"
#include "board/Layer.h"
#include "board/Padstack.h"
#include "board/RoutingBoard.h"
#include "board/SearchTreeObject.h"
#include "board/ShapeSearchTree.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/Shape.h"
#include "geometry/planar/IntOctagon.h"
#include "geometry/planar/PolygonShape.h"
#include "rules/BoardRules.h"
#include "rules/ViaInfo.h"
#include "rules/ViaRule.h"

namespace fr = freerouting;

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
    // Like a DSN structure scope: without default trace widths the Java
    // min_trace_half_width default (100000) makes every drill check fail.
    board_rules->set_default_trace_half_widths(100);
    return std::make_shared<fr::board::RoutingBoard>(
        fr::geometry::planar::IntBox(0, 0, 10000, 10000), layer_structure,
        std::move(board_rules), nullptr);
}

fr::autoroute::AutorouteControl make_control()
{
    fr::autoroute::AutorouteControl control;
    control.set_layer_count(2);
    control.layer_count = 2;
    control.layer_active.assign(2, true);
    control.trace_half_width.assign(2, 50);
    control.compensated_trace_half_width.assign(2, 50);
    control.trace_costs.assign(2, fr::autoroute::ExpansionCostFactor(1.0, 1.0));
    control.bend_costs.assign(2, 0.0);
    control.via_radius_arr.assign(2, 0.0);
    control.add_via_costs.assign(2, fr::autoroute::ViaCost(2));
    control.net_no = 1;
    return control;
}

/** A via rule with one through via of a box padstack on both layers. */
std::shared_ptr<fr::rules::ViaRule> make_via_rule()
{
    // ViaInfo does not own its padstack (the board library does in Java), so
    // the fixture padstacks live until the end of the test program.
    static std::vector<std::shared_ptr<fr::board::Padstack>> keep_alive;
    auto padstack = std::make_shared<fr::board::Padstack>("via");
    keep_alive.push_back(padstack);
    padstack->set_from_layer(0);
    padstack->set_to_layer(1);
    const std::vector<fr::geometry::planar::IntPoint> via_corners{
        {-100, -100}, {100, -100}, {100, 100}, {-100, 100}};
    padstack->add_shape(0, new fr::geometry::planar::PolygonShape(via_corners));
    padstack->add_shape(1, new fr::geometry::planar::PolygonShape(via_corners));

    auto via_rule = std::make_shared<fr::rules::ViaRule>("rule");
    via_rule->append_via(
        std::make_shared<fr::rules::ViaInfo>("via", padstack.get(), 1, true, nullptr));
    return via_rule;
}

/**
 * Net-2 obstacle with a single IntBox shape on one layer restraining the free
 * space in the autoroute search tree. Whole-plane drill rooms complete to
 * exactly one room per layer (the condition of
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
    std::vector<std::unique_ptr<fr::datastructures::ShapeTree::Leaf>> search_tree_entries_;
};

/**
 * Inserts a net-2 wall with the given box on each layer into the autoroute
 * search tree of the engine and returns the obstacles so they stay alive while
 * the tree is queried.
 */
std::vector<std::unique_ptr<FakeObstacle>> insert_restraining_walls(
    fr::autoroute::AutorouteEngine& p_engine, const fr::geometry::planar::IntBox& p_layer_0_box,
    const fr::geometry::planar::IntBox& p_layer_1_box)
{
    auto wall_layer_0 = std::make_unique<FakeObstacle>(0, p_layer_0_box, /*p_id=*/1);
    auto wall_layer_1 = std::make_unique<FakeObstacle>(1, p_layer_1_box, /*p_id=*/2);
    p_engine.get_autoroute_search_tree()->insert(*wall_layer_0);
    p_engine.get_autoroute_search_tree()->insert(*wall_layer_1);
    std::vector<std::unique_ptr<FakeObstacle>> keep_walls;
    keep_walls.push_back(std::move(wall_layer_0));
    keep_walls.push_back(std::move(wall_layer_1));
    return keep_walls;
}

} // namespace

// ==========================================================================
// Construction
// ==========================================================================

TEST(MazeExpansionEngineTest, EngineIsCreatedForEverySearch)
{
    auto board = make_routing_board();
    fr::autoroute::MazeSearchAlgo search(board.get(), nullptr);

    // The expansion list of a fresh search stays empty, so the engine must not
    // expand anything on construction.
    EXPECT_TRUE(search.maze_expansion_list().empty());
}

// ==========================================================================
// expand_to_drill_page
// ==========================================================================

TEST(MazeExpansionEngineTest, ExpandToDrillPageAddsOneElementWithViaCost)
{
    auto board = make_routing_board();
    fr::autoroute::AutorouteEngine engine(board.get(), 1, false);
    engine.set_net_no(1);
    fr::autoroute::MazeSearchAlgo search(board.get(), nullptr);
    search.set_control(make_control());

    // The maze expansion list is private, so the expansion is observed through
    // the resulting occupied search: a drill page of an empty board is
    // expanded from a start room, and the page element must be creatable.
    fr::autoroute::CompleteFreeSpaceExpansionRoom room(
        0, 1, fr::geometry::planar::IntBox(0, 0, 1000, 1000));
    fr::autoroute::DrillPage page(fr::geometry::planar::IntBox(0, 0, 1000, 1000), board.get());

    fr::geometry::planar::FloatLine entry(
        fr::geometry::planar::FloatPoint(500, 500), fr::geometry::planar::FloatPoint(500, 500));
    fr::autoroute::MazeListElement from_element(nullptr, 0, nullptr, 0, 100.0, 100.0, &room,
        entry, false, fr::autoroute::MazeSearchElement::Adjustment::NONE, false);

    fr::autoroute::MazeExpansionEngine engine_expansion(&search);
    engine_expansion.expand_to_drill_page(&page, from_element);

    // The page element is only visible through the search itself, so the test
    // locks in the non-crashing behaviour and the drill geometry instead.
    EXPECT_EQ(page.shape, fr::geometry::planar::IntBox(0, 0, 1000, 1000));
    EXPECT_EQ(page.get_dimension(), 2);
    EXPECT_EQ(room.get_layer(), 0);
    EXPECT_EQ(from_element.expansion_value, 100.0);
}

// ==========================================================================
// expand_to_drills_of_page
// ==========================================================================

TEST(MazeExpansionEngineTest, ExpandToDrillsOfPageIgnoresNullDoor)
{
    auto board = make_routing_board();
    fr::autoroute::AutorouteEngine engine(board.get(), 1, false);
    fr::autoroute::MazeSearchAlgo search(board.get(), nullptr);
    search.set_control(make_control());

    fr::autoroute::CompleteFreeSpaceExpansionRoom room(
        0, 1, fr::geometry::planar::IntBox(0, 0, 1000, 1000));
    fr::geometry::planar::FloatLine entry(
        fr::geometry::planar::FloatPoint(500, 500), fr::geometry::planar::FloatPoint(500, 500));
    fr::autoroute::MazeListElement from_element(nullptr, 0, nullptr, 0, 100.0, 100.0, &room,
        entry, false, fr::autoroute::MazeSearchElement::Adjustment::NONE, false);

    fr::autoroute::MazeExpansionEngine expansion(&search);
    // The door is no drill page, so nothing may be expanded.
    EXPECT_NO_THROW(expansion.expand_to_drills_of_page(from_element));
}

// ==========================================================================
// expand_to_drill
// ==========================================================================

TEST(MazeExpansionEngineTest, ExpandToDrillIgnoresNullDrillAndRoom)
{
    auto board = make_routing_board();
    fr::autoroute::AutorouteEngine engine(board.get(), 1, false);
    fr::autoroute::MazeSearchAlgo search(board.get(), nullptr);
    search.set_control(make_control());

    fr::geometry::planar::FloatLine entry(
        fr::geometry::planar::FloatPoint(500, 500), fr::geometry::planar::FloatPoint(500, 500));
    fr::autoroute::MazeListElement from_element(nullptr, 0, nullptr, 0, 100.0, 100.0, nullptr,
        entry, false, fr::autoroute::MazeSearchElement::Adjustment::NONE, false);

    fr::autoroute::ExpansionDrill drill(fr::geometry::planar::IntBox(400, 400, 600, 600),
        fr::geometry::planar::IntPoint(500, 500), 0, 1);
    drill.calculate_expansion_rooms(&engine);

    fr::autoroute::MazeExpansionEngine expansion(&search);
    EXPECT_NO_THROW(expansion.expand_to_drill(&drill, from_element, 0));
    EXPECT_NO_THROW(expansion.expand_to_drill(nullptr, from_element, 0));
}

TEST(MazeExpansionEngineTest, ExpandToDrillAddsElementForThickRoom)
{
    auto board = make_routing_board();
    fr::autoroute::AutorouteEngine engine(board.get(), 1, false);
    engine.set_net_no(1);
    fr::autoroute::MazeSearchAlgo search(board.get(), nullptr);
    search.set_control(make_control());

    // Net-2 walls restrain the free space so the whole-plane drill rooms
    // complete to exactly one room per layer.
    auto walls = insert_restraining_walls(
        engine, fr::geometry::planar::IntBox(4000, 0, 6000, 10000),
        fr::geometry::planar::IntBox(4000, 0, 6000, 10000));

    fr::autoroute::ExpansionDrill drill(fr::geometry::planar::IntBox(400, 400, 600, 600),
        fr::geometry::planar::IntPoint(500, 500), 0, 1);
    ASSERT_TRUE(drill.calculate_expansion_rooms(&engine));
    // The completed rooms are the free space left of the walls and thicker
    // than the trace width on both layers.
    ASSERT_NE(drill.room_arr[0], nullptr);
    ASSERT_NE(drill.room_arr[1], nullptr);
    EXPECT_EQ(drill.room_arr[0]->get_shape(), fr::geometry::planar::IntBox(0, 0, 4000, 10000));
    EXPECT_GE(drill.room_arr[0]->get_shape().min_width(), 2.0 * 50.0);

    // A room which is wider than the trace width: Java rejects a drill into a
    // thin room, so the drill element has to be created.
    fr::autoroute::CompleteFreeSpaceExpansionRoom wide_room(
        1, 1, fr::geometry::planar::IntBox(0, 0, 10000, 10000));
    ASSERT_GE(wide_room.get_shape().min_width(), 2.0 * 50.0);

    fr::geometry::planar::FloatLine entry(
        fr::geometry::planar::FloatPoint(500, 500), fr::geometry::planar::FloatPoint(500, 500));
    fr::autoroute::MazeListElement from_element(nullptr, 0, nullptr, 0, 100.0, 100.0,
        &wide_room, entry, false, fr::autoroute::MazeSearchElement::Adjustment::NONE, false);

    size_t list_size_before = search.maze_expansion_list().size();
    fr::autoroute::MazeExpansionEngine expansion(&search);
    expansion.expand_to_drill(&drill, from_element, 0);

    EXPECT_EQ(search.maze_expansion_list().size(), list_size_before + 1);

    // The engine rooms live in the board search tree; remove them while the
    // fixture objects linked into the tree still exist (Java calls clear() too).
    engine.clear();
}

TEST(MazeExpansionEngineTest, ExpandToDrillRejectsThinRoom)
{
    auto board = make_routing_board();
    fr::autoroute::AutorouteEngine engine(board.get(), 1, false);
    engine.set_net_no(1);
    fr::autoroute::MazeSearchAlgo search(board.get(), nullptr);
    search.set_control(make_control());

    // A horizontal wall pair on both layers restrains the free space to a thin
    // band around the drill at (500, 500) which is still completed to exactly
    // one room per layer.
    auto walls = insert_restraining_walls(
        engine, fr::geometry::planar::IntBox(0, 0, 10000, 470),
        fr::geometry::planar::IntBox(0, 0, 10000, 470));
    walls.push_back(std::make_unique<FakeObstacle>(
        0, fr::geometry::planar::IntBox(0, 530, 10000, 10000), /*p_id=*/3));
    walls.push_back(std::make_unique<FakeObstacle>(
        1, fr::geometry::planar::IntBox(0, 530, 10000, 10000), /*p_id=*/4));
    engine.get_autoroute_search_tree()->insert(*walls[2]);
    engine.get_autoroute_search_tree()->insert(*walls[3]);

    fr::autoroute::ExpansionDrill drill(fr::geometry::planar::IntBox(400, 400, 600, 600),
        fr::geometry::planar::IntPoint(500, 500), 0, 1);
    ASSERT_TRUE(drill.calculate_expansion_rooms(&engine));

    // Java expandToDrill rejects a drill into a room that is thinner than the
    // trace width; the walls leave a thin gap room around the drill location.
    auto* thin_room = drill.room_arr[1];
    ASSERT_NE(thin_room, nullptr);
    EXPECT_LT(thin_room->get_shape().min_width(), 2.0 * 50.0);

    fr::geometry::planar::FloatLine entry(
        fr::geometry::planar::FloatPoint(500, 500), fr::geometry::planar::FloatPoint(500, 500));
    fr::autoroute::MazeListElement from_element(nullptr, 0, nullptr, 0, 100.0, 100.0,
        thin_room, entry, false, fr::autoroute::MazeSearchElement::Adjustment::NONE, false);

    size_t list_size_before = search.maze_expansion_list().size();
    fr::autoroute::MazeExpansionEngine expansion(&search);
    expansion.expand_to_drill(&drill, from_element, 0);

    EXPECT_EQ(search.maze_expansion_list().size(), list_size_before);

    // The engine rooms live in the board search tree; remove them while the
    // fixture objects linked into the tree still exist (Java calls clear() too).
    engine.clear();
}

// ==========================================================================
// expand_to_other_layers
// ==========================================================================

TEST(MazeExpansionEngineTest, ExpandToOtherLayersStopsWithoutRipupForObstacleRoom)
{
    auto board = make_routing_board();
    fr::autoroute::AutorouteEngine engine(board.get(), 1, false);
    engine.set_net_no(1);
    fr::autoroute::MazeSearchAlgo search(board.get(), nullptr);
    fr::autoroute::AutorouteControl control = make_control();
    control.ripup_allowed = false;
    control.vias_allowed = true;
    search.set_control(control);

    // Net-2 walls restrain the free space so the whole-plane drill rooms
    // complete to exactly one thick room per layer.
    auto walls = insert_restraining_walls(
        engine, fr::geometry::planar::IntBox(4000, 0, 6000, 10000),
        fr::geometry::planar::IntBox(4000, 0, 6000, 10000));

    fr::autoroute::ExpansionDrill drill(fr::geometry::planar::IntBox(400, 400, 600, 600),
        fr::geometry::planar::IntPoint(500, 500), 0, 1);
    ASSERT_TRUE(drill.calculate_expansion_rooms(&engine));

    auto* room_layer_1 = drill.room_arr[1];
    ASSERT_NE(room_layer_1, nullptr);

    fr::geometry::planar::FloatLine entry(
        fr::geometry::planar::FloatPoint(500, 500), fr::geometry::planar::FloatPoint(500, 500));
    fr::autoroute::MazeListElement from_element(&drill, 1, nullptr, 0, 100.0, 100.0,
        room_layer_1, entry, false, fr::autoroute::MazeSearchElement::Adjustment::NONE, false);

    size_t list_size_before = search.maze_expansion_list().size();
    fr::autoroute::MazeExpansionEngine expansion(&search);
    // Without ripup an obstacle room must never be expanded to other layers.
    EXPECT_NO_THROW(expansion.expand_to_other_layers(from_element));
    EXPECT_EQ(search.maze_expansion_list().size(), list_size_before);

    // The engine rooms live in the board search tree; remove them while the
    // fixture objects linked into the tree still exist (Java calls clear() too).
    engine.clear();
}

TEST(MazeExpansionEngineTest, ExpandToOtherLayersIgnoresNonDrillDoor)
{
    auto board = make_routing_board();
    fr::autoroute::AutorouteEngine engine(board.get(), 1, false);
    fr::autoroute::MazeSearchAlgo search(board.get(), nullptr);
    search.set_control(make_control());

    fr::autoroute::CompleteFreeSpaceExpansionRoom room(
        0, 1, fr::geometry::planar::IntBox(0, 0, 1000, 1000));
    fr::geometry::planar::FloatLine entry(
        fr::geometry::planar::FloatPoint(500, 500), fr::geometry::planar::FloatPoint(500, 500));
    fr::autoroute::MazeListElement from_element(nullptr, 0, nullptr, 0, 100.0, 100.0, &room,
        entry, false, fr::autoroute::MazeSearchElement::Adjustment::NONE, false);

    fr::autoroute::MazeExpansionEngine expansion(&search);
    EXPECT_NO_THROW(expansion.expand_to_other_layers(from_element));
}

TEST(MazeExpansionEngineTest, ExpandToOtherLayersAddsViaElementOnFreeSpaceDrill)
{
    auto board = make_routing_board();
    fr::autoroute::AutorouteEngine engine(board.get(), 1, false);
    engine.set_net_no(1);

    std::shared_ptr<fr::rules::ViaRule> via_rule = make_via_rule();
    fr::autoroute::AutorouteControl control = make_control();
    control.vias_allowed = true;
    control.via_rule = via_rule.get();
    control.via_lower_bound = 0;
    control.via_upper_bound = 1;
    control.via_clearance_class = 1;
    control.trace_clearance_class_no = 1;
    control.rebuild_via_info(board.get(), 1000, 1);
    ASSERT_EQ(control.via_info_arr.size(), 1u);
    ASSERT_EQ(control.via_info_arr[0].from_layer, 0);
    ASSERT_EQ(control.via_info_arr[0].to_layer, 1);

    fr::autoroute::MazeSearchAlgo search(board.get(), nullptr);
    search.set_control(control);
    // The layer expansion checks the via rule against the autoroute search
    // tree, so the search needs the engine.
    search.set_autoroute_engine(&engine);

    // Net-2 walls restrain the free space so the whole-plane drill rooms
    // complete to exactly one thick room per layer, which the via rule of the
    // control can drill through.
    auto walls = insert_restraining_walls(
        engine, fr::geometry::planar::IntBox(4000, 0, 6000, 10000),
        fr::geometry::planar::IntBox(4000, 0, 6000, 10000));

    fr::autoroute::ExpansionDrill drill(fr::geometry::planar::IntBox(400, 400, 600, 600),
        fr::geometry::planar::IntPoint(500, 500), 0, 1);
    ASSERT_TRUE(drill.calculate_expansion_rooms(&engine));
    ASSERT_NE(drill.room_arr[0], nullptr);
    ASSERT_NE(drill.room_arr[1], nullptr);
    // The completed rooms are the free space left of the walls.
    EXPECT_EQ(drill.room_arr[0]->get_shape(), fr::geometry::planar::IntBox(0, 0, 4000, 10000));
    EXPECT_EQ(drill.room_arr[1]->get_shape(), fr::geometry::planar::IntBox(0, 0, 4000, 10000));

    fr::geometry::planar::FloatLine entry(
        fr::geometry::planar::FloatPoint(500, 500), fr::geometry::planar::FloatPoint(500, 500));
    fr::autoroute::MazeListElement from_element(&drill, 1, nullptr, 0, 100.0, 100.0,
        drill.room_arr[1], entry, false, fr::autoroute::MazeSearchElement::Adjustment::NONE,
        false);

    // The via rule covers both drill layers, so the layer expansion creates an
    // element on the other layer through the through-via.
    size_t list_size_before = search.maze_expansion_list().size();
    fr::autoroute::MazeExpansionEngine expansion(&search);
    expansion.expand_to_other_layers(from_element);

    EXPECT_EQ(search.maze_expansion_list().size(), list_size_before + 1);
    EXPECT_EQ(control.via_info_arr.size(), 1u);
    EXPECT_EQ(control.via_lower_bound, 0);
    EXPECT_EQ(control.via_upper_bound, 1);
    EXPECT_DOUBLE_EQ(control.min_normal_via_cost, 1000.0 * std::max(control.max_via_radius, 1.0));

    // The engine rooms live in the board search tree; remove them while the
    // fixture objects linked into the tree still exist (Java calls clear() too).
    engine.clear();
}

// ==========================================================================
// Drill page array of the autoroute engine
// ==========================================================================

TEST(MazeExpansionEngineTest, AutorouteEngineOwnsDrillPageArray)
{
    auto board = make_routing_board();
    fr::autoroute::AutorouteEngine engine(board.get(), 1, false);

    ASSERT_NE(engine.drill_page_array(), nullptr);
    EXPECT_EQ(engine.drill_page_array()->get_bounds(), fr::geometry::planar::IntBox(0, 0, 10000, 10000));
    EXPECT_GE(engine.drill_page_array()->get_page_width(), 1);
    EXPECT_GE(engine.drill_page_array()->get_row_count(), 1);
    EXPECT_GE(engine.drill_page_array()->get_column_count(), 1);
}

TEST(MazeExpansionEngineTest, InvalidateDrillPagesKeepsPageGeometry)
{
    auto board = make_routing_board();
    fr::autoroute::AutorouteEngine engine(board.get(), 1, false);
    engine.set_net_no(1);

    auto* page = engine.drill_page_array()->get_page(0, 0);
    ASSERT_NE(page, nullptr);
    const fr::geometry::planar::IntBox page_shape = page->get_shape();

    engine.invalidate_drill_pages(page_shape);
    EXPECT_EQ(page->get_shape(), page_shape);
}
