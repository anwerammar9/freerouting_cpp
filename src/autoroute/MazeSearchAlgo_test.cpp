#include "autoroute/MazeSearchAlgo.h"

#include "autoroute/AutorouteControl.h"
#include "autoroute/AutorouteEngine.h"
#include "autoroute/ExpansionDrill.h"
#include "autoroute/MazeFanoutDiagnostics.h"
#include "autoroute/ExpansionRoom.h"
#include "board/Communication.h"
#include "board/Layer.h"
#include "board/Padstack.h"
#include "board/Pin.h"
#include "board/PolylineTrace.h"
#include "board/RoutingBoard.h"
#include "board/SearchTreeManager.h"
#include "board/ShapeSearchTree.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/Point.h"
#include "geometry/planar/PolygonShape.h"
#include "geometry/planar/Polyline.h"
#include "rules/BoardRules.h"

#include <gtest/gtest.h>

#include <memory>
#include <set>
#include <vector>

namespace freerouting {
namespace autoroute {

namespace {

/**
 * The board communication of the fanout tests: a resolution of 1 mil, which
 * Java scales to micrometers with Unit.scale(1, Unit.UM, Unit.MIL) = 1/25.4.
 */
board::Communication board_communication = [] {
    board::Communication comm;
    comm.set_unit(board::Unit::MIL);
    comm.set_resolution(1);
    return comm;
}();

std::shared_ptr<board::RoutingBoard> make_routing_board()
{
    std::vector<std::shared_ptr<board::Layer>> layers;
    layers.push_back(std::make_shared<board::Layer>("Top", true));
    layers.push_back(std::make_shared<board::Layer>("Bottom", true));
    board::LayerStructure layer_structure(layers);
    auto board_rules = std::make_unique<rules::BoardRules>(layer_structure, 10);
    return std::make_shared<board::RoutingBoard>(geometry::planar::IntBox(0, 0, 10000, 10000), layer_structure,
        std::move(board_rules), nullptr);
}

/** A control with the per-layer arrays sized as the Java code expects. */
AutorouteControl make_control()
{
    AutorouteControl control;
    control.layer_count = 2;
    control.layer_active.assign(2, true);
    control.trace_half_width.assign(2, 50);
    control.compensated_trace_half_width.assign(2, 50);
    control.trace_costs.assign(2, ExpansionCostFactor(1.0, 1.0));
    control.bend_costs.assign(2, 0.0);
    control.net_no = 1;
    return control;
}

/** Inserts an SMD tie pin of p_nets at p_center on layer 0 into the board tree. */
board::Pin* insert_tie_pin(board::RoutingBoard* p_board, const geometry::planar::IntPoint& p_center,
    const std::vector<int>& p_nets, int p_pad_half_width)
{
    auto* pin = new board::Pin(7, 1, p_nets, 1, p_board->generate_item_id(),
        board::FixedState::NOT_FIXED, p_board);
    pin->set_center(p_center);
    auto* padstack = new board::Padstack("p1");
    padstack->set_from_layer(0);
    padstack->set_to_layer(0);
    padstack->add_shape(0, new geometry::planar::PolygonShape(
        std::vector<geometry::planar::IntPoint>{
            geometry::planar::IntPoint(-p_pad_half_width, -p_pad_half_width),
            geometry::planar::IntPoint(p_pad_half_width, -p_pad_half_width),
            geometry::planar::IntPoint(p_pad_half_width, p_pad_half_width),
            geometry::planar::IntPoint(-p_pad_half_width, p_pad_half_width)}));
    pin->set_padstack(padstack);
    pin->set_bounding_box(geometry::planar::IntBox(p_center.x - p_pad_half_width,
        p_center.y - p_pad_half_width, p_center.x + p_pad_half_width,
        p_center.y + p_pad_half_width));
    p_board->get_search_tree_manager()->insert(pin);
    p_board->add_item(pin);
    return pin;
}

/** Inserts a trace of p_nets on layer 0 into the board tree. */
board::PolylineTrace* insert_trace(board::RoutingBoard* p_board,
    const std::vector<geometry::planar::IntPoint>& p_corners, const std::vector<int>& p_nets)
{
    auto polyline =
        std::make_shared<geometry::planar::Polyline>(geometry::planar::Polyline::from_corners(p_corners));
    return p_board->insert_trace_without_cleaning(polyline, 0, 50, p_nets, 1,
        board::FixedState::NOT_FIXED);
}

} // namespace

// ==========================================================================
// Result
// ==========================================================================

TEST(MazeSearchAlgoTest, ResultDefaultsAreEmpty) {
    MazeSearchAlgo::Result result;
    EXPECT_EQ(result.destination_door, nullptr);
    EXPECT_EQ(result.section_no_of_door, 0);
}

// ==========================================================================
// Construction / control
// ==========================================================================

TEST(MazeSearchAlgoTest, AlreadyRippedCostMatchesJava) {
    EXPECT_EQ(MazeSearchAlgo::ALREADY_RIPPED_COSTS, 1);
}

TEST(MazeSearchAlgoTest, ConstructionLeavesSearchUninitialized) {
    auto board = make_routing_board();
    MazeSearchAlgo algo(board.get(), nullptr);

    EXPECT_FALSE(algo.is_initialized());
    EXPECT_TRUE(algo.maze_expansion_list().empty());
}

TEST(MazeSearchAlgoTest, ControlIsConfigurable) {
    auto board = make_routing_board();
    MazeSearchAlgo algo(board.get(), nullptr);

    algo.set_net_no(7);
    EXPECT_EQ(algo.get_control().net_no, 7);

    AutorouteControl control = make_control();
    algo.set_control(control);
    EXPECT_EQ(algo.get_control().layer_count, 2);
    EXPECT_EQ(algo.get_control().net_no, 1);
}

// ==========================================================================
// get_instance / init
// ==========================================================================

TEST(MazeSearchAlgoTest, GetInstanceRejectsMissingEngine) {
    AutorouteControl control = make_control();
    board::ItemSet items;

    EXPECT_EQ(MazeSearchAlgo::get_instance(items, items, nullptr, control), nullptr);
}

TEST(MazeSearchAlgoTest, GetInstanceFailsWithoutStartDoors) {
    // Door calculation is not ported yet, so no start door can be created and
    // init() must report a failure instead of returning a half initialised
    // search. This locks in the documented pending behaviour.
    auto board = make_routing_board();
    AutorouteEngine engine(board.get(), 1, true);
    AutorouteControl control = make_control();
    board::ItemSet items;

    std::unique_ptr<MazeSearchAlgo> algo =
        MazeSearchAlgo::get_instance(items, items, &engine, control, nullptr);

    EXPECT_EQ(algo, nullptr);
}

TEST(MazeSearchAlgoTest, InitFailsWithoutEngine) {
    auto board = make_routing_board();
    MazeSearchAlgo algo(board.get(), nullptr);
    board::ItemSet items;

    EXPECT_FALSE(algo.init(items, items));
    EXPECT_FALSE(algo.is_initialized());
}

TEST(MazeSearchAlgoTest, InitReducesForeignTraceShapesAtTiePins) {
    // Port of the reduceTraceShapesAtTiePins calls at the start of Java
    // MazeSearchEngine.init: traces of foreign nets connected to a tie pin
    // lose the pin center in their search shape, own net traces stay intact.
    namespace fp = geometry::planar;
    auto board = make_routing_board();
    board::Pin* pin = insert_tie_pin(board.get(), fp::IntPoint(2000, 2000), { 1, 2 }, 300);
    board::PolylineTrace* foreign_trace =
        insert_trace(board.get(), { fp::IntPoint(2000, 2000), fp::IntPoint(6000, 2000) }, { 2 });
    board::PolylineTrace* own_trace =
        insert_trace(board.get(), { fp::IntPoint(2000, 2000), fp::IntPoint(2000, 6000) }, { 1 });
    ASSERT_NE(pin, nullptr);
    ASSERT_NE(foreign_trace, nullptr);
    ASSERT_NE(own_trace, nullptr);

    AutorouteEngine engine(board.get(), 1, true);
    board::ShapeSearchTree* tree = engine.get_autoroute_search_tree();
    ASSERT_NE(tree, nullptr);

    const fp::TileShape* foreign_before = foreign_trace->get_tree_shape(*tree, 0);
    const fp::TileShape* own_before = own_trace->get_tree_shape(*tree, 0);
    ASSERT_NE(foreign_before, nullptr);
    ASSERT_NE(own_before, nullptr);
    ASSERT_TRUE(foreign_before->contains(fp::IntPoint(2000, 2000)));

    AutorouteControl control = make_control();
    board::ItemSet start_items{ pin };
    board::ItemSet dest_items{ foreign_trace };

    // The door calculation is not ported yet, so the instance creation may
    // fail; the tie-pin reduction runs at the start of init regardless.
    std::unique_ptr<MazeSearchAlgo> algo =
        MazeSearchAlgo::get_instance(start_items, dest_items, &engine, control, nullptr);
    (void)algo;

    const fp::TileShape* foreign_after = foreign_trace->get_tree_shape(*tree, 0);
    const fp::TileShape* own_after = own_trace->get_tree_shape(*tree, 0);
    ASSERT_NE(foreign_after, nullptr);
    ASSERT_NE(own_after, nullptr);
    // The foreign trace lost the pin center and keeps the piece with its far
    // corner ...
    EXPECT_NE(foreign_after, foreign_before);
    EXPECT_FALSE(foreign_after->contains(fp::IntPoint(2000, 2000)));
    EXPECT_TRUE(foreign_after->contains(fp::FloatPoint(6000.0, 2000.0)));
    // ... while the own net trace is untouched.
    EXPECT_EQ(own_after, own_before);
    EXPECT_TRUE(tree->validate_entries(foreign_trace));
    EXPECT_TRUE(tree->validate_entries(own_trace));
}

// ==========================================================================
// find_connection / reconstruct_path
// ==========================================================================

TEST(MazeSearchAlgoTest, FindConnectionWithoutExpansionListReportsNoDestination) {
    auto board = make_routing_board();
    MazeSearchAlgo algo(board.get(), nullptr);

    MazeSearchAlgo::Result result = algo.find_connection();

    EXPECT_EQ(result.destination_door, nullptr);
    EXPECT_EQ(result.section_no_of_door, 0);
}

TEST(MazeSearchAlgoTest, OccupyNextElementOnEmptyListReturnsFalse) {
    auto board = make_routing_board();
    MazeSearchAlgo algo(board.get(), nullptr);

    EXPECT_FALSE(algo.occupy_next_element());
}

TEST(MazeSearchAlgoTest, ReconstructPathOfEmptyResultIsEmpty) {
    auto board = make_routing_board();
    MazeSearchAlgo algo(board.get(), nullptr);
    MazeSearchAlgo::Result result;

    EXPECT_TRUE(algo.reconstruct_path(result).empty());
}

TEST(MazeSearchAlgoTest, ReconstructPathFollowsBacktrackChain) {
    // The backtrack chain of a found connection is walked from the destination
    // door back to the start door and returned in traversal order.
    auto board = make_routing_board();
    MazeSearchAlgo algo(board.get(), nullptr);

    CompleteFreeSpaceExpansionRoom start_room(0, 1, geometry::planar::IntBox(0, 0, 100, 100));
    CompleteFreeSpaceExpansionRoom middle_room(0, 2, geometry::planar::IntBox(100, 0, 200, 100));
    CompleteFreeSpaceExpansionRoom end_room(0, 3, geometry::planar::IntBox(200, 0, 300, 100));

    ExpansionDoor start_door(&start_room, &middle_room, 1);
    ExpansionDoor end_door(&middle_room, &end_room, 1);
    // The sections of a door are allocated by get_section_segments(), exactly
    // as in the search algorithm.
    const size_t start_section_count = start_door.get_section_segments(50).size();
    const size_t end_section_count = end_door.get_section_segments(50).size();
    ASSERT_EQ(start_section_count, static_cast<size_t>(start_door.maze_search_element_count()));
    ASSERT_EQ(end_section_count, static_cast<size_t>(end_door.maze_search_element_count()));

    auto* start_section = start_door.get_maze_search_element(0);
    ASSERT_NE(start_section, nullptr);
    start_section->backtrack_door = nullptr;

    auto* end_section = end_door.get_maze_search_element(0);
    ASSERT_NE(end_section, nullptr);
    end_section->backtrack_door = &start_door;
    end_section->section_no_of_backtrack_door = 0;

    MazeSearchAlgo::Result result;
    result.destination_door = &end_door;
    result.section_no_of_door = 0;

    std::vector<ExpandableObject*> path = algo.reconstruct_path(result);

    ASSERT_EQ(path.size(), 2u);
    EXPECT_EQ(path[0], &start_door);
    EXPECT_EQ(path[1], &end_door);
}

TEST(MazeSearchAlgoTest, ReconstructPathStopsOnSelfReference) {
    auto board = make_routing_board();
    MazeSearchAlgo algo(board.get(), nullptr);

    CompleteFreeSpaceExpansionRoom room_a(0, 1, geometry::planar::IntBox(0, 0, 100, 100));
    CompleteFreeSpaceExpansionRoom room_b(0, 2, geometry::planar::IntBox(100, 0, 200, 100));
    ExpansionDoor door(&room_a, &room_b, 1);
    const size_t section_count = door.get_section_segments(50).size();
    ASSERT_EQ(section_count, static_cast<size_t>(door.maze_search_element_count()));

    auto* section = door.get_maze_search_element(0);
    ASSERT_NE(section, nullptr);
    section->backtrack_door = &door;
    section->section_no_of_backtrack_door = 0;

    MazeSearchAlgo::Result result;
    result.destination_door = &door;
    result.section_no_of_door = 0;

    std::vector<ExpandableObject*> path = algo.reconstruct_path(result);

    ASSERT_EQ(path.size(), 1u);
    EXPECT_EQ(path[0], &door);
}

// ==========================================================================
// Fanout expansion list filter
// ==========================================================================

namespace {

/**
 * Returns a control for a fanout pass of the pin at p_pin_center on layer
 * p_start_layer, as RoutingBoard.fanout() sets it up.
 */
AutorouteControl make_fanout_control(const geometry::planar::FloatPoint& p_pin_center,
                                     int p_start_layer)
{
    AutorouteControl control = make_control();
    control.is_fanout = true;
    control.fanout_start_pin_name = "U27-1";
    control.fanout_start_pin_center = p_pin_center;
    control.fanout_start_pin_layer = p_start_layer;
    control.remove_unconnected_vias = false;
    return control;
}

/** Returns an expansion element with p_entry_point as shape entry. */
MazeListElement make_element(ExpandableObject* p_door, CompleteExpansionRoom* p_next_room,
                             const geometry::planar::FloatPoint& p_entry_point)
{
    return MazeListElement(p_door, 0, nullptr, 0, 0.0, 0.0, p_next_room,
        geometry::planar::FloatLine(p_entry_point, p_entry_point), false,
        MazeSearchElement::Adjustment::NONE, false);
}

} // namespace

TEST(MazeSearchAlgoTest, FanoutFilterAllowsEverythingOutsideOfAFanoutPass) {
    auto board = make_routing_board();
    MazeSearchAlgo algo(board.get(), nullptr);
    CompleteFreeSpaceExpansionRoom start_layer_room(0, 1, geometry::planar::IntBox(0, 0, 100, 100));
    CompleteFreeSpaceExpansionRoom other_layer_room(1, 2, geometry::planar::IntBox(0, 0, 100, 100));
    const geometry::planar::FloatPoint far_away(90000.0, 90000.0);

    EXPECT_TRUE(algo.passes_fanout_filter(
        make_element(nullptr, &start_layer_room, far_away)));
    EXPECT_TRUE(algo.passes_fanout_filter(
        make_element(nullptr, &other_layer_room, far_away)));
}

TEST(MazeSearchAlgoTest, FanoutFilterWithoutPinCenterAllowsEverything) {
    auto board = make_routing_board();
    MazeSearchAlgo algo(board.get(), nullptr);
    AutorouteControl control = make_control();
    control.is_fanout = true;
    algo.set_control(control);
    CompleteFreeSpaceExpansionRoom start_layer_room(0, 1, geometry::planar::IntBox(0, 0, 100, 100));

    EXPECT_TRUE(algo.passes_fanout_filter(
        make_element(nullptr, &start_layer_room, geometry::planar::FloatPoint(90000.0, 90000.0))));
}

TEST(MazeSearchAlgoTest, FanoutFilterRejectsRoomsFarAwayOnTheStartLayer) {
    auto board = make_routing_board();
    board->set_communication(&board_communication);
    MazeSearchAlgo algo(board.get(), nullptr);
    algo.set_control(make_fanout_control(geometry::planar::FloatPoint(1000.0, 1000.0), 0));
    CompleteFreeSpaceExpansionRoom start_layer_room(0, 1, geometry::planar::IntBox(0, 0, 100, 100));
    CompleteFreeSpaceExpansionRoom other_layer_room(1, 2, geometry::planar::IntBox(0, 0, 100, 100));

    // The board resolution is 1 mil, which Java scales with
    // Unit.scale(1, Unit.UM, Unit.MIL) = 1/25.4 to micrometers. With the Java
    // default of 3000 um this leaves an escape radius of about 118 units.
    EXPECT_TRUE(algo.passes_fanout_filter(
        make_element(nullptr, &start_layer_room, geometry::planar::FloatPoint(1100.0, 1000.0))));
    EXPECT_FALSE(algo.passes_fanout_filter(
        make_element(nullptr, &start_layer_room, geometry::planar::FloatPoint(1200.0, 1000.0))));
    // Rooms on other layers are not limited by the escape length.
    EXPECT_TRUE(algo.passes_fanout_filter(
        make_element(nullptr, &other_layer_room, geometry::planar::FloatPoint(9000.0, 9000.0))));
}

TEST(MazeSearchAlgoTest, FanoutFilterUsesTheConfiguredEscapeLengths) {
    auto board = make_routing_board();
    board->set_communication(&board_communication);
    MazeSearchAlgo algo(board.get(), nullptr);
    AutorouteControl control = make_fanout_control(geometry::planar::FloatPoint(1000.0, 1000.0), 0);
    control.max_escape_length_um = 254.0; // 10 units with the resolution above
    control.min_escape_length_um = 25.4;  // 1 unit with the resolution above
    algo.set_control(control);
    CompleteFreeSpaceExpansionRoom start_layer_room(0, 1, geometry::planar::IntBox(0, 0, 100, 100));

    EXPECT_TRUE(algo.passes_fanout_filter(
        make_element(nullptr, &start_layer_room, geometry::planar::FloatPoint(1010.0, 1000.0))));
    EXPECT_FALSE(algo.passes_fanout_filter(
        make_element(nullptr, &start_layer_room, geometry::planar::FloatPoint(1011.0, 1000.0))));
}

TEST(MazeSearchAlgoTest, FanoutFilterRejectsDrillsCloseToThePin) {
    auto board = make_routing_board();
    board->set_communication(&board_communication);
    MazeSearchAlgo algo(board.get(), nullptr);
    AutorouteControl control = make_fanout_control(geometry::planar::FloatPoint(1000.0, 1000.0), 0);
    control.min_escape_length_um = 25.4; // 1 unit with the resolution above
    algo.set_control(control);
    CompleteFreeSpaceExpansionRoom start_layer_room(0, 1, geometry::planar::IntBox(0, 0, 100, 100));
    // The drill sits on the fanout pin, so it is closer to it than the
    // minimum escape length of one unit.
    ExpansionDrill near_drill(geometry::planar::IntBox(980, 980, 1020, 1020),
        geometry::planar::IntPoint(1000, 1000), 0, 1);
    ExpansionDrill far_drill(geometry::planar::IntBox(1000, 1000, 1020, 1020),
        geometry::planar::IntPoint(2000, 1000), 0, 1);

    EXPECT_FALSE(algo.passes_fanout_filter(
        make_element(&near_drill, &start_layer_room, geometry::planar::FloatPoint(1000.0, 1000.0))));
    EXPECT_TRUE(algo.passes_fanout_filter(
        make_element(&far_drill, &start_layer_room, geometry::planar::FloatPoint(1000.0, 1000.0))));
}

TEST(MazeSearchAlgoTest, FanoutDiagnosticsFollowTheControlOfTheSearch) {
    auto board = make_routing_board();
    MazeSearchAlgo algo(board.get(), nullptr);

    AutorouteControl control = make_control();
    control.is_fanout = true;
    control.fanout_start_pin_name = "U27-1";
    algo.set_control(control);
    EXPECT_TRUE(algo.fanout_diagnostics().enabled());
    EXPECT_EQ(algo.fanout_diagnostics().label(), "U27-1");

    // Another pin turns the targeted diagnostics off again.
    AutorouteControl other_control = make_control();
    other_control.is_fanout = true;
    other_control.fanout_start_pin_name = "U1-1";
    algo.set_control(other_control);
    EXPECT_FALSE(algo.fanout_diagnostics().enabled());
    EXPECT_EQ(algo.fanout_diagnostics().label(), "U1-1");

    // Without a fanout pin name the diagnostics use the net number.
    AutorouteControl unnamed_control = make_control();
    unnamed_control.is_fanout = true;
    unnamed_control.net_no = 7;
    algo.set_control(unnamed_control);
    EXPECT_FALSE(algo.fanout_diagnostics().enabled());
    EXPECT_EQ(algo.fanout_diagnostics().label(), "fanout-pin(net=7)");
}

} // namespace autoroute
} // namespace freerouting
