#include <cmath>
#include <memory>
#include <set>
#include <vector>

#include "board/ConductionArea.h"
#include "board/Layer.h"
#include "board/Padstack.h"
#include "board/Pin.h"
#include "board/PolylineTrace.h"
#include "board/RoutingBoard.h"
#include "board/SearchTreeManager.h"
#include "board/Trace.h"
#include "board/Via.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/Point.h"
#include "geometry/planar/PolygonShape.h"
#include "geometry/planar/Polyline.h"
#include "gtest/gtest.h"
#include "interactive/NetIncompletes.h"
#include "interactive/RatsNest.h"
#include "rules/BoardRules.h"
#include "rules/Net.h"
#include "rules/NetClass.h"

namespace freerouting::interactive {
namespace {

std::shared_ptr<board::RoutingBoard> make_routing_board()
{
    std::vector<std::shared_ptr<board::Layer>> layers;
    layers.push_back(std::make_shared<board::Layer>("Top", true));
    layers.push_back(std::make_shared<board::Layer>("Bottom", true));
    board::LayerStructure layer_structure(layers);
    auto board_rules = std::make_unique<rules::BoardRules>(layer_structure, 10);
    board_rules->get_nets().add("net1", 1, false);
    board_rules->get_nets().add("net2", 1, false);
    return std::make_shared<board::RoutingBoard>(
        geometry::planar::IntBox(0, 0, 10000, 10000),
        layer_structure, std::move(board_rules), nullptr);
}

board::PolylineTrace* insert_trace(board::RoutingBoard* p_board,
                                   const std::vector<geometry::planar::IntPoint>& p_corners,
                                   const std::vector<int>& p_nets)
{
    auto polyline = std::make_shared<geometry::planar::Polyline>(
        geometry::planar::Polyline::from_corners(p_corners));
    return p_board->insert_trace_without_cleaning(polyline, 0, 50, p_nets, 1,
                                                  board::FixedState::NOT_FIXED);
}

board::Via* insert_via(board::RoutingBoard* p_board,
                       const geometry::planar::IntPoint& p_center,
                       const std::vector<int>& p_nets, int p_pad_half_width)
{
    auto* padstack = new board::Padstack("v1");
    padstack->set_from_layer(0);
    padstack->set_to_layer(1);
    padstack->add_shape(0, new geometry::planar::PolygonShape(
        std::vector<geometry::planar::IntPoint>{
            {static_cast<int64_t>(-p_pad_half_width), static_cast<int64_t>(-p_pad_half_width)},
            {static_cast<int64_t>(p_pad_half_width), static_cast<int64_t>(-p_pad_half_width)},
            {static_cast<int64_t>(p_pad_half_width), static_cast<int64_t>(p_pad_half_width)},
            {static_cast<int64_t>(-p_pad_half_width), static_cast<int64_t>(p_pad_half_width)}}));
    auto* via = new board::Via(padstack, p_center, p_nets, 1,
                               p_board->generate_item_id(), 0,
                               board::FixedState::NOT_FIXED, true, p_board);
    via->set_bounding_box(geometry::planar::IntBox(
        p_center.x - p_pad_half_width, p_center.y - p_pad_half_width,
        p_center.x + p_pad_half_width, p_center.y + p_pad_half_width));
    p_board->get_search_tree_manager()->insert(via);
    p_board->add_item(via);
    return via;
}

/**
 * Inserts a single-layer SMD pin. Unconnected pins are not "tails" in Java
 * (Item.isTail() defaults to false and is only overridden by Via/Trace), so
 * they survive the NetIncompletes filter and appear in the ratsnest.
 */
board::Pin* insert_smd_pin(board::RoutingBoard* p_board,
                           const geometry::planar::IntPoint& p_center,
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
            {static_cast<int64_t>(-p_pad_half_width), static_cast<int64_t>(-p_pad_half_width)},
            {static_cast<int64_t>(p_pad_half_width), static_cast<int64_t>(-p_pad_half_width)},
            {static_cast<int64_t>(p_pad_half_width), static_cast<int64_t>(p_pad_half_width)},
            {static_cast<int64_t>(-p_pad_half_width), static_cast<int64_t>(p_pad_half_width)}}));
    pin->set_padstack(padstack);
    pin->set_bounding_box(geometry::planar::IntBox(
        p_center.x - p_pad_half_width, p_center.y - p_pad_half_width,
        p_center.x + p_pad_half_width, p_center.y + p_pad_half_width));
    p_board->get_search_tree_manager()->insert(pin);
    p_board->add_item(pin);
    return pin;
}

/**
 * Creates a board with a net class assigned to net 1 whose minimum and
 * maximum trace lengths are configured before the board is finalised.
 * This avoids mutating through the const BoardRules accessor.
 */
struct BoardWithNetClass
{
    std::shared_ptr<board::RoutingBoard> board;
    rules::NetClass* net_class;
};

BoardWithNetClass make_board_with_length_limits(double p_min_length, double p_max_length)
{
    std::vector<std::shared_ptr<board::Layer>> layers;
    layers.push_back(std::make_shared<board::Layer>("Top", true));
    layers.push_back(std::make_shared<board::Layer>("Bottom", true));
    board::LayerStructure layer_structure(layers);
    auto board_rules = std::make_unique<rules::BoardRules>(layer_structure, 10);
    board_rules->get_nets().add("net1", 1, false);
    board_rules->get_nets().add("net2", 1, false);
    auto* net_class = board_rules->get_net_classes().append(
        "test_class", layer_structure, board_rules->get_clearance_matrix(), false);
    net_class->set_minimum_trace_length(p_min_length);
    net_class->set_maximum_trace_length(p_max_length);
    board_rules->get_nets().get(1)->set_net_class(net_class);
    auto board = std::make_shared<board::RoutingBoard>(
        geometry::planar::IntBox(0, 0, 10000, 10000),
        layer_structure, std::move(board_rules), nullptr);
    return {board, net_class};
}

TEST(NetIncompletesTest, ConnectedNetHasNoIncompletes)
{
    auto board = make_routing_board();
    board::PolylineTrace* trace = insert_trace(
        board.get(),
        {geometry::planar::IntPoint(1000, 1000),
         geometry::planar::IntPoint(4000, 4000)},
        {1});
    ASSERT_NE(trace, nullptr);
    board::Via* via1 = insert_via(board.get(), geometry::planar::IntPoint(1000, 1000), {1}, 50);
    board::Via* via2 = insert_via(board.get(), geometry::planar::IntPoint(4000, 4000), {1}, 50);
    ASSERT_NE(via1, nullptr);
    ASSERT_NE(via2, nullptr);

    std::vector<board::Item*> net_items = {via1, via2, trace};
    NetIncompletes ni(1, net_items, board.get());

    EXPECT_EQ(ni.count(), 0);
    EXPECT_TRUE(ni.empty());
    EXPECT_EQ(ni.get_connected_group_count(), 1);
    EXPECT_DOUBLE_EQ(ni.get_length_violation(), 0.0);
    EXPECT_NE(ni.get_net(), nullptr);
    EXPECT_EQ(ni.get_net()->get_net_no(), 1);
}

TEST(NetIncompletesTest, TwoIsolatedPinsProduceOneAirLine)
{
    auto board = make_routing_board();
    board::Pin* pin1 = insert_smd_pin(board.get(), geometry::planar::IntPoint(200, 200), {1}, 50);
    board::Pin* pin2 = insert_smd_pin(board.get(), geometry::planar::IntPoint(6000, 5000), {1}, 50);
    ASSERT_NE(pin1, nullptr);
    ASSERT_NE(pin2, nullptr);

    std::vector<board::Item*> net_items = {pin1, pin2};
    NetIncompletes ni(1, net_items, board.get());

    EXPECT_EQ(ni.count(), 1);
    EXPECT_EQ(ni.get_connected_group_count(), 2);
    ASSERT_EQ(ni.incompletes().size(), 1u);
    const auto& airline = ni.incompletes().front();
    EXPECT_EQ(airline->net, board->get_rules().get_nets().get(1));
    // The airline should connect the two distinct items (order may vary).
    std::set<board::Item*> airline_items = {airline->from_item, airline->to_item};
    EXPECT_EQ(airline_items.size(), 2u);
    EXPECT_TRUE(airline_items.count(pin1) > 0);
    EXPECT_TRUE(airline_items.count(pin2) > 0);
    // The corners should match the two via centers (one corner per center).
    geometry::planar::FloatPoint center1(200, 200);
    geometry::planar::FloatPoint center2(6000, 5000);
    // A corner lies on a center iff its distance to one of them is zero.
    EXPECT_NEAR(airline->from_corner.distance_square(center1)
                * airline->from_corner.distance_square(center2), 0, 1e-6);
    EXPECT_NEAR(airline->to_corner.distance_square(center1)
                * airline->to_corner.distance_square(center2), 0, 1e-6);
    // The airline connects the two different centers.
    EXPECT_NEAR(airline->from_corner.distance_square(airline->to_corner), 56680000, 1e-6);
}

TEST(NetIncompletesTest, DanglingTraceIsFiltered)
{
    auto board = make_routing_board();
    board::Via* via1 = insert_via(board.get(), geometry::planar::IntPoint(200, 200), {1}, 50);
    board::PolylineTrace* tail_trace = insert_trace(
        board.get(),
        {geometry::planar::IntPoint(5000, 5000),
         geometry::planar::IntPoint(6000, 4000)},
        {1});
    ASSERT_NE(via1, nullptr);
    ASSERT_NE(tail_trace, nullptr);
    EXPECT_TRUE(tail_trace->is_tail());

    std::vector<board::Item*> net_items = {via1, tail_trace};
    NetIncompletes ni(1, net_items, board.get());

    // Only via1 remains after filtering → single item → 0 incompletes.
    EXPECT_EQ(ni.count(), 0);
    EXPECT_TRUE(ni.empty());
}

TEST(NetIncompletesTest, LengthViolationNegativeWhenTooShort)
{
    auto [board, net_class] = make_board_with_length_limits(100000.0, 0.0);
    board::Pin* pin1 = insert_smd_pin(board.get(), geometry::planar::IntPoint(1000, 1000), {1}, 50);
    board::Pin* pin2 = insert_smd_pin(board.get(), geometry::planar::IntPoint(8000, 8000), {1}, 50);
    board::PolylineTrace* trace = insert_trace(
        board.get(),
        {geometry::planar::IntPoint(1000, 1000),
         geometry::planar::IntPoint(8000, 8000)},
        {1});
    ASSERT_NE(pin1, nullptr);
    ASSERT_NE(pin2, nullptr);
    ASSERT_NE(trace, nullptr);

    std::vector<board::Item*> net_items = {pin1, pin2, trace};
    NetIncompletes ni(1, net_items, board.get());

    // Connected set → 0 incompletes. Trace length≈ 9899 < min 100000.
    EXPECT_EQ(ni.count(), 0);
    EXPECT_LT(ni.get_length_violation(), 0);
    // The violation was already computed in the constructor; without a change
    // calc_length_violation() reports false.
    EXPECT_FALSE(ni.calc_length_violation());
    EXPECT_LT(ni.get_length_violation(), 0);
}

TEST(NetIncompletesTest, LengthViolationPositiveWhenTooLong)
{
    auto [board, net_class] = make_board_with_length_limits(0.0, 5000.0);
    board::Pin* pin1 = insert_smd_pin(board.get(), geometry::planar::IntPoint(1000, 1000), {1}, 50);
    board::Pin* pin2 = insert_smd_pin(board.get(), geometry::planar::IntPoint(8000, 8000), {1}, 50);
    board::PolylineTrace* trace = insert_trace(
        board.get(),
        {geometry::planar::IntPoint(1000, 1000),
         geometry::planar::IntPoint(8000, 8000)},
        {1});
    ASSERT_NE(pin1, nullptr);
    ASSERT_NE(pin2, nullptr);
    ASSERT_NE(trace, nullptr);

    std::vector<board::Item*> net_items = {pin1, pin2, trace};
    NetIncompletes ni(1, net_items, board.get());

    // Connected set → 0 incompletes. Trace length ≈ 9899 > max 5000.
    EXPECT_EQ(ni.count(), 0);
    EXPECT_GT(ni.get_length_violation(), 0);
    // No change → false.
    EXPECT_FALSE(ni.calc_length_violation());

    // Adding more trace length to the net changes the violation → true.
    board::PolylineTrace* extra_trace = insert_trace(
        board.get(),
        {geometry::planar::IntPoint(9000, 9000),
         geometry::planar::IntPoint(9500, 9500)},
        {1});
    ASSERT_NE(extra_trace, nullptr);
    EXPECT_TRUE(ni.calc_length_violation());
    EXPECT_GT(ni.get_length_violation(), 0);
}

TEST(NetIncompletesTest, NoViolationWhenLengthsOk)
{
    auto [board, net_class] = make_board_with_length_limits(5000.0, 20000.0);
    board::Pin* pin1 = insert_smd_pin(board.get(), geometry::planar::IntPoint(1000, 1000), {1}, 50);
    board::Pin* pin2 = insert_smd_pin(board.get(), geometry::planar::IntPoint(8000, 8000), {1}, 50);
    board::PolylineTrace* trace = insert_trace(
        board.get(),
        {geometry::planar::IntPoint(1000, 1000),
         geometry::planar::IntPoint(8000, 8000)},
        {1});
    ASSERT_NE(pin1, nullptr);
    ASSERT_NE(pin2, nullptr);
    ASSERT_NE(trace, nullptr);

    std::vector<board::Item*> net_items = {pin1, pin2, trace};
    NetIncompletes ni(1, net_items, board.get());

    EXPECT_EQ(ni.count(), 0);
    EXPECT_EQ(ni.get_connected_group_count(), 1);
    EXPECT_DOUBLE_EQ(ni.get_length_violation(), 0.0);
    EXPECT_FALSE(ni.calc_length_violation());
}

TEST(NetIncompletesTest, ThreeIsolatedPinsProduceTwoAirlines)
{
    auto board = make_routing_board();
    board::Pin* pin1 = insert_smd_pin(board.get(), geometry::planar::IntPoint(200, 200), {1}, 50);
    board::Pin* pin2 = insert_smd_pin(board.get(), geometry::planar::IntPoint(6000, 200), {1}, 50);
    board::Pin* pin3 = insert_smd_pin(board.get(), geometry::planar::IntPoint(3000, 8000), {1}, 50);
    ASSERT_NE(pin1, nullptr);
    ASSERT_NE(pin2, nullptr);
    ASSERT_NE(pin3, nullptr);

    std::vector<board::Item*> net_items = {pin1, pin2, pin3};
    NetIncompletes ni(1, net_items, board.get());

    // 3 isolated groups → 2 airlines (MST).
    EXPECT_EQ(ni.count(), 2);
    EXPECT_EQ(ni.get_connected_group_count(), 3);
    EXPECT_TRUE(ni.incompletes().size() == 2);
}

TEST(NetIncompletesTest, SingleItemHasNoAirLines)
{
    auto board = make_routing_board();
    board::Pin* pin1 = insert_smd_pin(board.get(), geometry::planar::IntPoint(200, 200), {1}, 50);
    ASSERT_NE(pin1, nullptr);

    std::vector<board::Item*> net_items = {pin1};
    NetIncompletes ni(1, net_items, board.get());

    EXPECT_EQ(ni.count(), 0);
    EXPECT_EQ(ni.get_connected_group_count(), 1);
}

} // namespace
} // namespace freerouting::interactive