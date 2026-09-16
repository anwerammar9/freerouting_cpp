#include <memory>
#include <string>
#include <vector>

#include "board/Layer.h"
#include "board/Padstack.h"
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
#include "interactive/RatsNest.h"
#include "interactive/RatsNestItemInfo.h"
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

TEST(RatsNestTest, CountsMultipleNets)
{
    auto board = make_routing_board();
    board::Via* n1a = insert_via(board.get(), geometry::planar::IntPoint(200, 200), {1}, 50);
    board::Via* n1b = insert_via(board.get(), geometry::planar::IntPoint(6000, 5000), {1}, 50);
    board::Via* n2a = insert_via(board.get(), geometry::planar::IntPoint(100, 200), {2}, 50);
    board::Via* n2b = insert_via(board.get(), geometry::planar::IntPoint(3000, 200), {2}, 50);
    board::Via* n2c = insert_via(board.get(), geometry::planar::IntPoint(6000, 8000), {2}, 50);
    ASSERT_NE(n1a, nullptr);
    ASSERT_NE(n1b, nullptr);
    ASSERT_NE(n2a, nullptr);
    ASSERT_NE(n2b, nullptr);
    ASSERT_NE(n2c, nullptr);

    RatsNest ratsnest(board.get());
    // Net1 has 2 vias → 1 airline; net2 has 3 vias → 2 airlines.
    EXPECT_EQ(ratsnest.incomplete_count(), 3);
    EXPECT_EQ(ratsnest.incomplete_count(1), 1);
    EXPECT_EQ(ratsnest.incomplete_count(2), 2);
    EXPECT_EQ(ratsnest.max_connections, 3);
    EXPECT_EQ(ratsnest.length_violation_count(), 0);
    EXPECT_EQ(ratsnest.get_airlines().size(), 3u);
}

TEST(RatsNestTest, RecalculateUpdatesCount)
{
    auto board = make_routing_board();
    board::Via* n1a = insert_via(board.get(), geometry::planar::IntPoint(200, 200), {1}, 50);
    board::Via* n1b = insert_via(board.get(), geometry::planar::IntPoint(6000, 5000), {1}, 50);
    ASSERT_NE(n1a, nullptr);
    ASSERT_NE(n1b, nullptr);

    RatsNest ratsnest(board.get());
    EXPECT_EQ(ratsnest.incomplete_count(1), 1);

    // Route net1 by adding a trace connecting the vias.
    board::PolylineTrace* trace = insert_trace(
        board.get(),
        {geometry::planar::IntPoint(200, 200),
         geometry::planar::IntPoint(6000, 5000)},
        {1});
    ASSERT_NE(trace, nullptr);

    // Recalculate net1; now the net is completely routed.
    ratsnest.recalculate(1, board.get());
    EXPECT_EQ(ratsnest.incomplete_count(1), 0);
    EXPECT_EQ(ratsnest.incomplete_count(), 0);
}

TEST(RatsNestTest, RecalculateWithItemList)
{
    auto board = make_routing_board();
    board::Via* n1a = insert_via(board.get(), geometry::planar::IntPoint(200, 200), {1}, 50);
    board::Via* n1b = insert_via(board.get(), geometry::planar::IntPoint(6000, 5000), {1}, 50);
    ASSERT_NE(n1a, nullptr);
    ASSERT_NE(n1b, nullptr);

    RatsNest ratsnest(board.get());
    EXPECT_EQ(ratsnest.incomplete_count(1), 1);

    // Recalculate with an empty item list → 0 airlines.
    std::vector<board::Item*> empty;
    ratsnest.recalculate(1, empty, board.get());
    EXPECT_EQ(ratsnest.incomplete_count(1), 0);
}

TEST(RatsNestTest, HideShowAndFilter)
{
    auto board = make_routing_board();
    RatsNest ratsnest(board.get());

    ratsnest.hide();
    EXPECT_TRUE(ratsnest.is_hidden());
    ratsnest.show();
    EXPECT_FALSE(ratsnest.is_hidden());

    ratsnest.set_filter(1, true);
    EXPECT_TRUE(ratsnest.is_filtered(1));
    EXPECT_FALSE(ratsnest.is_filtered(2));
    EXPECT_FALSE(ratsnest.is_filtered(5));
}

TEST(RatsNestTest, AirLineItemInfo)
{
    auto board = make_routing_board();
    board::Via* via1 = insert_via(board.get(), geometry::planar::IntPoint(200, 200), {1}, 50);
    board::PolylineTrace* tail_trace = insert_trace(
        board.get(),
        {geometry::planar::IntPoint(5000, 5000),
         geometry::planar::IntPoint(6000, 4000)},
        {1});
    board::Via* via2 = insert_via(board.get(), geometry::planar::IntPoint(8000, 8000), {1}, 50);
    ASSERT_NE(via1, nullptr);
    ASSERT_NE(tail_trace, nullptr);
    ASSERT_NE(via2, nullptr);

    RatsNest ratsnest(board.get());
    auto airlines = ratsnest.get_airlines();
    ASSERT_EQ(airlines.size(), 1u);
    const auto& airline = airlines.front();

    // Item info for a via.
    RatsNestItemInfo via_info = airline->get_item_info(board.get(), via1);
    EXPECT_EQ(via_info.type, RatsNestItemType::VIA);
    EXPECT_EQ(via_info.text, "Via");

    // Item info for a trace.
    RatsNestItemInfo trace_info = airline->get_item_info(board.get(), tail_trace);
    EXPECT_EQ(trace_info.type, RatsNestItemType::TRACE);
    EXPECT_EQ(trace_info.text, "Trace");

    // The airline is described by the net name.
    std::string desc = airline->to_string(board.get());
    EXPECT_NE(desc.find("net1"), std::string::npos);
}

} // namespace
} // namespace freerouting::interactive