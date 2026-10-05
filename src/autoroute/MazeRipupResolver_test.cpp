#include "autoroute/MazeRipupResolver.h"

#include "autoroute/AutorouteControl.h"
#include "autoroute/AutorouteEngine.h"
#include "autoroute/ExpansionRoom.h"
#include "autoroute/MazeRandom.h"
#include "autoroute/MazeSearchAlgo.h"
#include "autoroute/MazeSearchElement.h"
#include "board/BasicBoard.h"
#include "board/FixedState.h"
#include "board/Layer.h"
#include "board/PolylineTrace.h"
#include "board/RoutingBoard.h"
#include "board/Trace.h"
#include "geometry/planar/FloatLine.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/Point.h"
#include "geometry/planar/Polyline.h"
#include "rules/BoardRules.h"

#include <gtest/gtest.h>

#include <memory>
#include <set>
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
    return std::make_shared<board::RoutingBoard>(geometry::planar::IntBox(0, 0, 10000, 10000), layer_structure,
        std::move(board_rules), nullptr);
}

/** A control with the per-layer arrays sized as the Java code expects. */
AutorouteControl make_control(int p_ripup_costs = 1000, int p_ripup_pass_no = 1)
{
    AutorouteControl control;
    control.layer_count = 2;
    control.layer_active.assign(2, true);
    control.trace_half_width.assign(2, 50);
    control.compensated_trace_half_width.assign(2, 50);
    control.trace_costs.assign(2, ExpansionCostFactor(1.0, 1.0));
    control.bend_costs.assign(2, 0.0);
    control.net_no = 1;
    control.ripup_allowed = true;
    control.ripup_costs = p_ripup_costs;
    control.ripup_pass_no = p_ripup_pass_no;
    return control;
}

board::PolylineTrace* insert_trace(board::RoutingBoard* p_board,
                                   const std::vector<geometry::planar::IntPoint>& p_corners,
                                   int p_half_width,
                                   const std::vector<int>& p_nets,
                                   board::FixedState p_fixed_state = board::FixedState::NOT_FIXED)
{
    auto polyline = std::make_shared<geometry::planar::Polyline>(geometry::planar::Polyline::from_corners(p_corners));
    return p_board->insert_trace_without_cleaning(polyline, 0, p_half_width, p_nets, 1, p_fixed_state);
}

/** A list element which arrives at p_next_room through p_door without a ripup. */
MazeListElement make_list_element(ExpandableObject* p_door, CompleteExpansionRoom* p_next_room)
{
    geometry::planar::FloatPoint entry(0, 0);
    return MazeListElement(p_door, 0, nullptr, 0, 0, 0, p_next_room,
        geometry::planar::FloatLine(entry, entry), false, MazeSearchElement::Adjustment::NONE, false);
}

} // namespace

// ==========================================================================
// MazeRandom — the values below are the output of java.util.Random with the
// JDK 25 of this workspace, for the same seeds (verified with
// `new Random(seed).nextDouble()`).
// ==========================================================================

TEST(MazeRandomTest, NextDoubleMatchesJavaForSeed1000) {
    MazeRandom random(1000);

    EXPECT_DOUBLE_EQ(random.next_double(), 0.7101849056320707);
    EXPECT_DOUBLE_EQ(random.next_double(), 0.574836350385667);
    EXPECT_DOUBLE_EQ(random.next_double(), 0.9464192094792073);
}

TEST(MazeRandomTest, NextDoubleMatchesJavaForSeveralSeeds) {
    MazeRandom random_zero(0);
    MazeRandom random_one(1);
    MazeRandom random_two(2);
    MazeRandom random_hundred(100);
    MazeRandom random_large(123456789);

    EXPECT_DOUBLE_EQ(random_zero.next_double(), 0.730967787376657);
    EXPECT_DOUBLE_EQ(random_one.next_double(), 0.7308781907032909);
    EXPECT_DOUBLE_EQ(random_two.next_double(), 0.7311469360199058);
    EXPECT_DOUBLE_EQ(random_hundred.next_double(), 0.7220096548596434);
    EXPECT_DOUBLE_EQ(random_large.next_double(), 0.664038103272266);
}

TEST(MazeRandomTest, NextMatchesJavaForTheRequestedBitCount) {
    MazeRandom random(1000);

    EXPECT_EQ(random.next(32), -1244746321);
    EXPECT_EQ(random.next(32), 1060493871);
    EXPECT_EQ(random.next(32), -1826063944);
}

TEST(MazeRandomTest, NextEightMatchesJava) {
    MazeRandom random(1000);

    EXPECT_EQ(random.next(8), 181);
    EXPECT_EQ(random.next(8), 63);
    EXPECT_EQ(random.next(8), 147);
    EXPECT_EQ(random.next(8), 117);
}

TEST(MazeRandomTest, ReseedWithTheSameSeedRepeatsTheSequence) {
    MazeRandom random(1000);
    const double first = random.next_double();
    const double second = random.next_double();

    random.set_seed(1000);

    EXPECT_DOUBLE_EQ(random.next_double(), first);
    EXPECT_DOUBLE_EQ(random.next_double(), second);
}

TEST(MazeRandomTest, DefaultSeedMatchesJavaSeedZero) {
    MazeRandom random;
    MazeRandom reference(0);

    EXPECT_DOUBLE_EQ(random.next_double(), reference.next_double());
}

TEST(MazeRandomTest, NextDoubleStaysInTheUnitInterval) {
    MazeRandom random(4711);

    for (int i = 0; i < 1000; i++)
    {
        const double value = random.next_double();
        EXPECT_GE(value, 0.0);
        EXPECT_LT(value, 1.0);
    }
}

TEST(MazeRandomTest, ControlChangeReseedsTheGeneratorWithTheRipupCosts) {
    // Java MazeSearchEngine seeds randomGenerator with ctrl.ripupCosts to keep
    // the ripup randomization deterministic across passes.
    auto board = make_routing_board();
    MazeSearchAlgo algo(board.get(), nullptr);

    algo.set_control(make_control(1000));
    EXPECT_DOUBLE_EQ(algo.random_generator().next_double(), 0.7101849056320707);

    algo.set_control(make_control(1001));
    EXPECT_DOUBLE_EQ(algo.random_generator().next_double(), 0.7100953387610269);
}

TEST(MazeSearchAlgoTest, SearchOwnsARipupResolver) {
    auto board = make_routing_board();
    MazeSearchAlgo algo(board.get(), nullptr);

    ASSERT_NE(algo.ripup_resolver(), nullptr);
    EXPECT_EQ(algo.search_tree(), nullptr);
}

// ==========================================================================
// calc_fanout_via_ripup_cost_factor
// ==========================================================================

TEST(MazeRipupResolverTest, FanoutFactorOfAMissingTraceIsOne) {
    EXPECT_DOUBLE_EQ(MazeRipupResolver::calc_fanout_via_ripup_cost_factor(nullptr), 1.0);
}

TEST(MazeRipupResolverTest, FanoutFactorOfAnUnprotectedTraceIsOne) {
    // The trace ends in free space, so neither end has a single contact which
    // could be a fanout via.
    auto board = make_routing_board();
    board::PolylineTrace* trace = insert_trace(board.get(),
        {geometry::planar::IntPoint(0, 0), geometry::planar::IntPoint(1000, 0)}, 50, {1});
    ASSERT_NE(trace, nullptr);
    ASSERT_TRUE(trace->get_start_contacts().empty());
    ASSERT_TRUE(trace->get_end_contacts().empty());

    EXPECT_DOUBLE_EQ(MazeRipupResolver::calc_fanout_via_ripup_cost_factor(trace), 1.0);
}

TEST(MazeRipupResolverTest, FanoutFactorIsOneForANormalContactTrace) {
    // A single contact which is not shove fixed does not protect a fanout via.
    auto board = make_routing_board();
    board::PolylineTrace* trace = insert_trace(board.get(),
        {geometry::planar::IntPoint(0, 0), geometry::planar::IntPoint(1000, 0)}, 50, {1});
    board::PolylineTrace* contact = insert_trace(board.get(),
        {geometry::planar::IntPoint(-1000, 0), geometry::planar::IntPoint(0, 0)}, 50, {1});
    ASSERT_NE(trace, nullptr);
    ASSERT_NE(contact, nullptr);
    ASSERT_EQ(trace->get_start_contacts().size(), 1u);

    EXPECT_DOUBLE_EQ(MazeRipupResolver::calc_fanout_via_ripup_cost_factor(trace), 1.0);
}

TEST(MazeRipupResolverTest, FanoutFactorProtectsShoveFixedContact) {
    // A shove fixed contact with exactly 2 corners is the trace of a fanout via,
    // which is expensive to rip up: (half_width / length)^2 * FANOUT_COST_CONSTANT
    // = (50 / 1000)^2 * 20000 = 50.
    auto board = make_routing_board();
    board::PolylineTrace* trace = insert_trace(board.get(),
        {geometry::planar::IntPoint(0, 0), geometry::planar::IntPoint(1000, 0)}, 50, {1});
    board::PolylineTrace* contact = insert_trace(board.get(),
        {geometry::planar::IntPoint(-1000, 0), geometry::planar::IntPoint(0, 0)}, 50, {1},
        board::FixedState::SHOVE_FIXED);
    ASSERT_NE(trace, nullptr);
    ASSERT_NE(contact, nullptr);
    ASSERT_EQ(trace->get_start_contacts().size(), 1u);
    ASSERT_EQ(contact->corner_count(), 2);

    EXPECT_NEAR(MazeRipupResolver::calc_fanout_via_ripup_cost_factor(trace), 50.0, 1e-9);
}

TEST(MazeRipupResolverTest, FanoutFactorProtectsShoveFixedContactAtTheEnd) {
    auto board = make_routing_board();
    board::PolylineTrace* trace = insert_trace(board.get(),
        {geometry::planar::IntPoint(0, 0), geometry::planar::IntPoint(1000, 0)}, 50, {1});
    board::PolylineTrace* contact = insert_trace(board.get(),
        {geometry::planar::IntPoint(1000, 0), geometry::planar::IntPoint(2000, 0)}, 50, {1},
        board::FixedState::SHOVE_FIXED);
    ASSERT_NE(trace, nullptr);
    ASSERT_NE(contact, nullptr);
    ASSERT_TRUE(trace->get_start_contacts().empty());
    ASSERT_EQ(trace->get_end_contacts().size(), 1u);

    EXPECT_NEAR(MazeRipupResolver::calc_fanout_via_ripup_cost_factor(trace), 50.0, 1e-9);
}

TEST(MazeRipupResolverTest, FanoutFactorIgnoresACorneredShoveFixedContact) {
    // A shove fixed contact with 3 corners is no fanout via trace. The contact
    // corner has to be an end corner of the contact, because the ported
    // Trace::get_normal_contacts() only detects contacts at the corners of a
    // trace, exactly like the Java original.
    auto board = make_routing_board();
    board::PolylineTrace* trace = insert_trace(board.get(),
        {geometry::planar::IntPoint(0, 0), geometry::planar::IntPoint(1000, 0)}, 50, {1});
    board::PolylineTrace* contact = insert_trace(board.get(),
        {geometry::planar::IntPoint(0, 0), geometry::planar::IntPoint(-500, 0), geometry::planar::IntPoint(-500, -500)}, 50,
        {1}, board::FixedState::SHOVE_FIXED);
    ASSERT_NE(trace, nullptr);
    ASSERT_NE(contact, nullptr);
    ASSERT_EQ(trace->get_start_contacts().size(), 1u);
    ASSERT_EQ(contact->corner_count(), 3);

    EXPECT_DOUBLE_EQ(MazeRipupResolver::calc_fanout_via_ripup_cost_factor(trace), 1.0);
}

TEST(MazeRipupResolverTest, FanoutCostConstantMatchesJava) {
    EXPECT_DOUBLE_EQ(MazeRipupResolver::FANOUT_COST_CONSTANT, 20000.0);
}

// ==========================================================================
// check_ripup
// ==========================================================================

TEST(MazeRipupResolverTest, RipupOfAMissingItemIsRejected) {
    auto board = make_routing_board();
    MazeSearchAlgo algo(board.get(), nullptr);
    CompleteFreeSpaceExpansionRoom room(0, 1, geometry::planar::IntBox(0, 0, 100, 100));
    ExpansionDoor door(&room, &room, 1);
    MazeListElement element = make_list_element(&door, &room);

    EXPECT_EQ(algo.ripup_resolver()->check_ripup(element, nullptr, false), -1);
}

TEST(MazeRipupResolverTest, RipupThroughAThickDoorIsRejected) {
    // enter_through_small_door() only accepts 1-dimensional doors, so a small
    // door of a 2-dimensional door shape cannot be entered after a ripup.
    auto board = make_routing_board();
    board::PolylineTrace* trace = insert_trace(board.get(),
        {geometry::planar::IntPoint(0, 0), geometry::planar::IntPoint(1000, 0)}, 50, {1});
    ASSERT_NE(trace, nullptr);

    MazeSearchAlgo algo(board.get(), nullptr);
    algo.set_control(make_control());
    CompleteFreeSpaceExpansionRoom free_room(0, 1, geometry::planar::IntBox(0, 0, 100, 100));
    ObstacleExpansionRoom obstacle_room(trace, 0, 0, geometry::planar::IntBox(0, 0, 1000, 100));
    ExpansionDoor door(&free_room, &obstacle_room, 2);
    MazeListElement element = make_list_element(&door, &obstacle_room);

    EXPECT_EQ(algo.ripup_resolver()->check_ripup(element, trace, true), -1);
}

TEST(MazeRipupResolverTest, RipupOfTheAlreadyRippedRoomCostsOne) {
    // Entering the same item through a second door of one of its rooms means
    // that the item was already ripped, which is the cheapest ripup case.
    auto board = make_routing_board();
    board::PolylineTrace* trace = insert_trace(board.get(),
        {geometry::planar::IntPoint(0, 0), geometry::planar::IntPoint(1000, 0)}, 50, {1});
    ASSERT_NE(trace, nullptr);

    MazeSearchAlgo algo(board.get(), nullptr);
    algo.set_control(make_control());
    ObstacleExpansionRoom obstacle_room_a(trace, 0, 0, geometry::planar::IntBox(0, 0, 500, 100));
    ObstacleExpansionRoom obstacle_room_b(trace, 1, 0, geometry::planar::IntBox(500, 0, 1000, 100));
    ExpansionDoor door(&obstacle_room_a, &obstacle_room_b, 1);
    MazeListElement element = make_list_element(&door, &obstacle_room_b);

    EXPECT_EQ(algo.ripup_resolver()->check_ripup(element, trace, false), MazeSearchAlgo::ALREADY_RIPPED_COSTS);
    EXPECT_EQ(MazeSearchAlgo::ALREADY_RIPPED_COSTS, 1);
}

TEST(MazeRipupResolverTest, PlainTraceRipupCostMatchesJava) {
    // Java: ripupCosts * half_width, divided by the detour of the connection
    // (1 for an uncalculated connection) => 1000 * 50.
    auto board = make_routing_board();
    board::PolylineTrace* trace = insert_trace(board.get(),
        {geometry::planar::IntPoint(0, 0), geometry::planar::IntPoint(1000, 0)}, 50, {1});
    ASSERT_NE(trace, nullptr);

    MazeSearchAlgo algo(board.get(), nullptr);
    algo.set_control(make_control(1000, 1));
    CompleteFreeSpaceExpansionRoom free_room(0, 1, geometry::planar::IntBox(0, 0, 100, 100));
    ObstacleExpansionRoom obstacle_room(trace, 0, 0, geometry::planar::IntBox(0, 0, 1000, 100));
    ExpansionDoor door(&free_room, &obstacle_room, 1);
    MazeListElement element = make_list_element(&door, &obstacle_room);

    EXPECT_EQ(algo.ripup_resolver()->check_ripup(element, trace, false), 50000);
}

TEST(MazeRipupResolverTest, RipupCostOfAPassWithoutRandomizationIsStable) {
    // ripupPassNo 6 is not randomized (pass % 3 == 0), so the plain cost returns.
    auto board = make_routing_board();
    board::PolylineTrace* trace = insert_trace(board.get(),
        {geometry::planar::IntPoint(0, 0), geometry::planar::IntPoint(1000, 0)}, 50, {1});
    ASSERT_NE(trace, nullptr);

    MazeSearchAlgo algo(board.get(), nullptr);
    algo.set_control(make_control(1000, 6));
    CompleteFreeSpaceExpansionRoom free_room(0, 1, geometry::planar::IntBox(0, 0, 100, 100));
    ObstacleExpansionRoom obstacle_room(trace, 0, 0, geometry::planar::IntBox(0, 0, 1000, 100));
    ExpansionDoor door(&free_room, &obstacle_room, 1);
    MazeListElement element = make_list_element(&door, &obstacle_room);

    EXPECT_EQ(algo.ripup_resolver()->check_ripup(element, trace, false), 50000);
}

TEST(MazeRipupResolverTest, RandomizedRipupCostMatchesJava) {
    // Java from pass 4 on randomizes the detour with
    // (0.5 + random^2) of a Random seeded with the ripup costs:
    // 1000 * 50 / (0.5 + 0.7101849056320707^2) = 49782.
    auto board = make_routing_board();
    board::PolylineTrace* trace = insert_trace(board.get(),
        {geometry::planar::IntPoint(0, 0), geometry::planar::IntPoint(1000, 0)}, 50, {1});
    ASSERT_NE(trace, nullptr);

    MazeSearchAlgo algo(board.get(), nullptr);
    algo.set_control(make_control(1000, 4));
    CompleteFreeSpaceExpansionRoom free_room(0, 1, geometry::planar::IntBox(0, 0, 100, 100));
    ObstacleExpansionRoom obstacle_room(trace, 0, 0, geometry::planar::IntBox(0, 0, 1000, 100));
    ExpansionDoor door(&free_room, &obstacle_room, 1);
    MazeListElement element = make_list_element(&door, &obstacle_room);

    EXPECT_EQ(algo.ripup_resolver()->check_ripup(element, trace, false), 49782);
}

TEST(MazeRipupResolverTest, RandomizedRipupCostDependsOnTheSeed) {
    // A different ripup cost seeds the randomizer differently:
    // 1001 * 50 / (0.5 + 0.7100953387610269^2) = 49838.
    auto board = make_routing_board();
    board::PolylineTrace* trace = insert_trace(board.get(),
        {geometry::planar::IntPoint(0, 0), geometry::planar::IntPoint(1000, 0)}, 50, {1});
    ASSERT_NE(trace, nullptr);

    MazeSearchAlgo algo(board.get(), nullptr);
    algo.set_control(make_control(1001, 4));
    CompleteFreeSpaceExpansionRoom free_room(0, 1, geometry::planar::IntBox(0, 0, 100, 100));
    ObstacleExpansionRoom obstacle_room(trace, 0, 0, geometry::planar::IntBox(0, 0, 1000, 100));
    ExpansionDoor door(&free_room, &obstacle_room, 1);
    MazeListElement element = make_list_element(&door, &obstacle_room);

    EXPECT_EQ(algo.ripup_resolver()->check_ripup(element, trace, false), 49838);
}

TEST(MazeRipupResolverTest, RandomizedRipupCostIsReproducible) {
    // The same control must always produce the same ripup cost, so that a pass
    // of the autorouter is deterministic.
    auto board = make_routing_board();
    board::PolylineTrace* trace = insert_trace(board.get(),
        {geometry::planar::IntPoint(0, 0), geometry::planar::IntPoint(1000, 0)}, 50, {1});
    ASSERT_NE(trace, nullptr);

    CompleteFreeSpaceExpansionRoom free_room(0, 1, geometry::planar::IntBox(0, 0, 100, 100));
    ObstacleExpansionRoom obstacle_room(trace, 0, 0, geometry::planar::IntBox(0, 0, 1000, 100));
    ExpansionDoor door(&free_room, &obstacle_room, 1);
    MazeListElement element = make_list_element(&door, &obstacle_room);

    MazeSearchAlgo first(board.get(), nullptr);
    first.set_control(make_control(1000, 4));
    MazeSearchAlgo second(board.get(), nullptr);
    second.set_control(make_control(1000, 4));

    EXPECT_EQ(first.ripup_resolver()->check_ripup(element, trace, false),
        second.ripup_resolver()->check_ripup(element, trace, false));
}

TEST(MazeRipupResolverTest, FanoutProtectionMultipliesTheRipupCost) {
    // With unconnected vias kept, ripup costs at most twice the start ripup
    // costs protect fanout vias: 2 * 50 * 50 = 5000.
    auto board = make_routing_board();
    board::PolylineTrace* trace = insert_trace(board.get(),
        {geometry::planar::IntPoint(0, 0), geometry::planar::IntPoint(1000, 0)}, 50, {1});
    board::PolylineTrace* fanout_contact = insert_trace(board.get(),
        {geometry::planar::IntPoint(-1000, 0), geometry::planar::IntPoint(0, 0)}, 50, {1},
        board::FixedState::SHOVE_FIXED);
    ASSERT_NE(trace, nullptr);
    ASSERT_NE(fanout_contact, nullptr);
    ASSERT_EQ(trace->get_start_contacts().size(), 1u);

    MazeSearchAlgo algo(board.get(), nullptr);
    AutorouteControl control = make_control(2, 1);
    control.remove_unconnected_vias = false;
    algo.set_control(control);
    CompleteFreeSpaceExpansionRoom free_room(0, 1, geometry::planar::IntBox(0, 0, 100, 100));
    ObstacleExpansionRoom obstacle_room(trace, 0, 0, geometry::planar::IntBox(0, 0, 1000, 100));
    ExpansionDoor door(&free_room, &obstacle_room, 1);
    MazeListElement element = make_list_element(&door, &obstacle_room);

    EXPECT_EQ(algo.ripup_resolver()->check_ripup(element, trace, false), 5000);
}

TEST(MazeRipupResolverTest, FanoutProtectionIsDroppedWithHighRipupCosts) {
    // Ripup costs above twice the start ripup costs no longer protect fanout
    // vias, so the plain cost of 4 * 50 is returned.
    auto board = make_routing_board();
    board::PolylineTrace* trace = insert_trace(board.get(),
        {geometry::planar::IntPoint(0, 0), geometry::planar::IntPoint(1000, 0)}, 50, {1});
    board::PolylineTrace* fanout_contact = insert_trace(board.get(),
        {geometry::planar::IntPoint(-1000, 0), geometry::planar::IntPoint(0, 0)}, 50, {1},
        board::FixedState::SHOVE_FIXED);
    ASSERT_NE(trace, nullptr);
    ASSERT_NE(fanout_contact, nullptr);
    ASSERT_EQ(trace->get_start_contacts().size(), 1u);

    MazeSearchAlgo algo(board.get(), nullptr);
    AutorouteControl control = make_control(4, 1);
    control.remove_unconnected_vias = false;
    algo.set_control(control);
    CompleteFreeSpaceExpansionRoom free_room(0, 1, geometry::planar::IntBox(0, 0, 100, 100));
    ObstacleExpansionRoom obstacle_room(trace, 0, 0, geometry::planar::IntBox(0, 0, 1000, 100));
    ExpansionDoor door(&free_room, &obstacle_room, 1);
    MazeListElement element = make_list_element(&door, &obstacle_room);

    EXPECT_EQ(algo.ripup_resolver()->check_ripup(element, trace, false), 200);
}

TEST(MazeRipupResolverTest, RipupOfARoomBehindAShovedRoomOfTheSameNetIsRejected) {
    // A shoved room of a trace of the same net may not be entered through an
    // obstacle of the same net, because the shove would destroy the trace.
    auto board = make_routing_board();
    board::PolylineTrace* previous_trace = insert_trace(board.get(),
        {geometry::planar::IntPoint(0, 0), geometry::planar::IntPoint(500, 0)}, 50, {1});
    board::PolylineTrace* next_trace = insert_trace(board.get(),
        {geometry::planar::IntPoint(500, 0), geometry::planar::IntPoint(1000, 0)}, 50, {1});
    ASSERT_NE(previous_trace, nullptr);
    ASSERT_NE(next_trace, nullptr);
    ASSERT_TRUE(previous_trace->shares_net(next_trace));

    MazeSearchAlgo algo(board.get(), nullptr);
    algo.set_control(make_control());
    ObstacleExpansionRoom previous_room(previous_trace, 0, 0, geometry::planar::IntBox(0, 0, 500, 100));
    ObstacleExpansionRoom next_room(next_trace, 0, 0, geometry::planar::IntBox(500, 0, 1000, 100));
    ExpansionDoor door(&previous_room, &next_room, 1);
    geometry::planar::FloatPoint entry(500, 0);
    MazeListElement element(&door, 0, nullptr, 0, 0, 0, &next_room,
        geometry::planar::FloatLine(entry, entry), false, MazeSearchElement::Adjustment::LEFT, false);

    EXPECT_EQ(algo.ripup_resolver()->check_ripup(element, next_trace, false), -1);
}

TEST(MazeRipupResolverTest, RipupBehindAShovedRoomOfAnotherNetIsAllowed) {
    auto board = make_routing_board();
    board::PolylineTrace* previous_trace = insert_trace(board.get(),
        {geometry::planar::IntPoint(0, 0), geometry::planar::IntPoint(500, 0)}, 50, {1});
    board::PolylineTrace* next_trace = insert_trace(board.get(),
        {geometry::planar::IntPoint(500, 500), geometry::planar::IntPoint(1000, 500)}, 50, {2});
    ASSERT_NE(previous_trace, nullptr);
    ASSERT_NE(next_trace, nullptr);
    ASSERT_FALSE(previous_trace->shares_net(next_trace));

    MazeSearchAlgo algo(board.get(), nullptr);
    algo.set_control(make_control(1000, 1));
    ObstacleExpansionRoom previous_room(previous_trace, 0, 0, geometry::planar::IntBox(0, 0, 500, 100));
    ObstacleExpansionRoom next_room(next_trace, 0, 0, geometry::planar::IntBox(500, 500, 1000, 600));
    ExpansionDoor door(&previous_room, &next_room, 1);
    geometry::planar::FloatPoint entry(500, 500);
    MazeListElement element(&door, 0, nullptr, 0, 0, 0, &next_room,
        geometry::planar::FloatLine(entry, entry), false, MazeSearchElement::Adjustment::LEFT, false);

    EXPECT_EQ(algo.ripup_resolver()->check_ripup(element, next_trace, false), 50000);
}

// ==========================================================================
// check_leaving_ripped_item
// ==========================================================================

TEST(MazeRipupResolverTest, LeavingARippedItemThroughANonDoorIsRejected) {
    auto board = make_routing_board();
    MazeSearchAlgo algo(board.get(), nullptr);
    CompleteFreeSpaceExpansionRoom room(0, 1, geometry::planar::IntBox(0, 0, 100, 100));
    MazeListElement element = make_list_element(nullptr, &room);

    EXPECT_FALSE(algo.ripup_resolver()->check_leaving_ripped_item(element));
}

TEST(MazeRipupResolverTest, LeavingARippedItemFromAFreeSpaceRoomIsRejected) {
    // Only a ripped obstacle item can be left through a small door.
    auto board = make_routing_board();
    MazeSearchAlgo algo(board.get(), nullptr);
    CompleteFreeSpaceExpansionRoom from_room(0, 1, geometry::planar::IntBox(0, 0, 100, 100));
    CompleteFreeSpaceExpansionRoom to_room(0, 2, geometry::planar::IntBox(100, 0, 200, 100));
    ExpansionDoor door(&from_room, &to_room, 1);
    MazeListElement element = make_list_element(&door, &to_room);

    EXPECT_FALSE(algo.ripup_resolver()->check_leaving_ripped_item(element));
}

TEST(MazeRipupResolverTest, LeavingARippedItemNeedsTheSearchTree) {
    // The overlap check of a small door looks into the compensated autoroute
    // search tree, which only an initialised maze search owns.
    auto board = make_routing_board();
    board::PolylineTrace* trace = insert_trace(board.get(),
        {geometry::planar::IntPoint(0, 0), geometry::planar::IntPoint(1000, 0)}, 50, {2});
    ASSERT_NE(trace, nullptr);

    MazeSearchAlgo algo(board.get(), nullptr);
    algo.set_control(make_control());
    ObstacleExpansionRoom from_room(trace, 0, 0, geometry::planar::IntBox(0, 0, 1000, 100));
    CompleteFreeSpaceExpansionRoom to_room(0, 2, geometry::planar::IntBox(0, 0, 1000, 1000));
    ExpansionDoor door(&from_room, &to_room, 1);
    MazeListElement element = make_list_element(&door, &to_room);

    ASSERT_EQ(algo.search_tree(), nullptr);
    EXPECT_FALSE(algo.ripup_resolver()->check_leaving_ripped_item(element));
}

} // namespace autoroute
} // namespace freerouting
