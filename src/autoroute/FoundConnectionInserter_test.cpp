#include <memory>
#include <optional>
#include <vector>

#include "autoroute/AutorouteControl.h"
#include "autoroute/FoundConnectionInserter.h"
#include "autoroute/FoundConnectionLocator.h"
#include "board/Layer.h"
#include "board/Padstack.h"
#include "board/Pin.h"
#include "board/PolylineTrace.h"
#include "board/RoutingBoard.h"
#include "board/Trace.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/Polyline.h"
#include "geometry/planar/PolygonShape.h"
#include <iostream>

#include "gtest/gtest.h"
#include "rules/BoardRules.h"

namespace freerouting::autoroute {
namespace {

using board::FixedState;
using board::PolylineTrace;
using board::RoutingBoard;
using board::Trace;
using geometry::planar::IntBox;
using geometry::planar::IntPoint;
using geometry::planar::Polyline;

std::shared_ptr<RoutingBoard> make_routing_board()
{
    std::vector<std::shared_ptr<board::Layer>> layers;
    layers.push_back(std::make_shared<board::Layer>("Top", true));
    layers.push_back(std::make_shared<board::Layer>("Bottom", true));
    board::LayerStructure layer_structure(layers);
    auto board_rules = std::make_unique<rules::BoardRules>(layer_structure, 10);
    return std::make_shared<RoutingBoard>(IntBox(0, 0, 10000, 10000), layer_structure,
                                          std::move(board_rules), nullptr);
}

/** A control with the per-layer arrays sized as the Java code expects. */
/** Inserts an smd pin with a square pad of the given half width. */
board::Pin* insert_smd_pin(RoutingBoard* p_board, const IntPoint& p_center,
                           const std::vector<int>& p_nets, int p_pad_half_width)
{
    auto* pin = new board::Pin(7, 1, p_nets, 1, p_board->generate_item_id(), FixedState::NOT_FIXED,
                               p_board);
    pin->set_center(p_center);
    auto* padstack = new board::Padstack("pin_padstack");
    padstack->set_from_layer(0);
    padstack->set_to_layer(0);
    padstack->add_shape(0, new geometry::planar::PolygonShape(
                               std::vector<IntPoint>{{-p_pad_half_width, -p_pad_half_width},
                                                     {p_pad_half_width, -p_pad_half_width},
                                                     {p_pad_half_width, p_pad_half_width},
                                                     {-p_pad_half_width, p_pad_half_width}}));
    pin->set_padstack(padstack);
    pin->set_bounding_box(IntBox(p_center.x - p_pad_half_width, p_center.y - p_pad_half_width,
                                 p_center.x + p_pad_half_width, p_center.y + p_pad_half_width));
    p_board->get_search_tree_manager()->insert(pin);
    p_board->add_item(pin);
    return pin;
}

AutorouteControl make_ctrl(int p_net_no)
{
    AutorouteControl ctrl;
    ctrl.layer_count = 2;
    ctrl.layer_active.assign(2, true);
    ctrl.trace_half_width.assign(2, 50);
    ctrl.compensated_trace_half_width.assign(2, 50);
    ctrl.trace_costs.assign(2, ExpansionCostFactor(1.0, 1.0));
    ctrl.bend_costs.assign(2, 0.0);
    ctrl.net_no = p_net_no;
    ctrl.trace_clearance_class_no = 1;
    ctrl.with_neckdown = false;
    ctrl.is_fanout = false;
    ctrl.fanout_start_pin_name = "1";
    return ctrl;
}

/**
 * Minimal concrete locator for tests: the result items are filled in
 * directly, so the backtrack machinery is not needed.
 */
class TestLocator : public FoundConnectionLocator
{
public:
    std::vector<geometry::planar::FloatPoint> calculate_next_trace_corners() override
    {
        return {};
    }
};

/** Fills p_locator with a single straight trace result item. */
void configure_straight_locator(TestLocator& p_locator, const std::vector<IntPoint>& p_corners,
                                board::Item* p_start_item, board::Item* p_target_item, int p_layer)
{
    p_locator.start_item = p_start_item;
    p_locator.start_layer = p_layer;
    p_locator.target_item = p_target_item;
    p_locator.target_layer = p_layer;
    FoundConnectionLocator::ResultItem item;
    item.corners = p_corners;
    item.layer = p_layer;
    p_locator.connection_items.push_back(item);
}

TEST(FoundConnectionInserterTest, SingleCornerConnectionSetsBothCorners)
{
    auto board = make_routing_board();
    const AutorouteControl ctrl = make_ctrl(1);
    TestLocator locator;
    configure_straight_locator(locator, {IntPoint(1000, 1000)}, nullptr, nullptr, 0);

    const std::unique_ptr<FoundConnectionInserter> inserter =
        FoundConnectionInserter::get_instance(&locator, board.get(), ctrl);

    ASSERT_NE(inserter, nullptr);
    ASSERT_TRUE(inserter->first_corner().has_value());
    ASSERT_TRUE(inserter->last_corner().has_value());
    EXPECT_EQ(*inserter->first_corner(), IntPoint(1000, 1000));
    EXPECT_EQ(*inserter->last_corner(), IntPoint(1000, 1000));
}

TEST(FoundConnectionInserterTest, StraightConnectionIsInsertedAndNormalized)
{
    auto board = make_routing_board();
    // The start pin anchors the new trace, so that it is not treated as a
    // contact free stub by FoundConnectionInserter.insert_trace.
    ASSERT_NE(insert_smd_pin(board.get(), IntPoint(1000, 1000), {1}, 150), nullptr);
    const AutorouteControl ctrl = make_ctrl(1);
    TestLocator locator;
    configure_straight_locator(locator, {IntPoint(1000, 1000), IntPoint(4000, 1000)}, nullptr,
                               nullptr, 0);

    const std::unique_ptr<FoundConnectionInserter> inserter =
        FoundConnectionInserter::get_instance(&locator, board.get(), ctrl);

    ASSERT_NE(inserter, nullptr);
    ASSERT_TRUE(inserter->first_corner().has_value());
    EXPECT_EQ(*inserter->first_corner(), IntPoint(1000, 1000));
    EXPECT_EQ(*inserter->last_corner(), IntPoint(4000, 1000));

    // The new connection ends up on the board with the routed net.
    bool found = false;
    for (const Trace* trace : board->get_traces())
    {
        if (trace->shares_net_no(std::vector<int>{1}))
        {
            const auto* polyline_trace = dynamic_cast<const PolylineTrace*>(trace);
            if (polyline_trace != nullptr &&
                polyline_trace->polyline().contains(IntPoint(2500, 1000)))
            {
                found = true;
            }
        }
    }
    EXPECT_TRUE(found);
}

/**
 * A connection with 3 corners is inserted in several steps. Each step shoves
 * the traces inserted by the previous steps, which is the case that needs
 * ShapeTraceEntries::resort() to drop own net entries from the entry list.
 */
TEST(FoundConnectionInserterTest, MultiSegmentConnectionIsInserted)
{
    auto board = make_routing_board();
    ASSERT_NE(insert_smd_pin(board.get(), IntPoint(1000, 1000), {1}, 150), nullptr);
    ASSERT_NE(insert_smd_pin(board.get(), IntPoint(4000, 2000), {1}, 150), nullptr);
    const AutorouteControl ctrl = make_ctrl(1);
    TestLocator locator;
    configure_straight_locator(locator,
                               {IntPoint(1000, 1000), IntPoint(2000, 1000), IntPoint(3000, 2000),
                                IntPoint(4000, 2000)},
                               nullptr, nullptr, 0);

    const std::unique_ptr<FoundConnectionInserter> inserter =
        FoundConnectionInserter::get_instance(&locator, board.get(), ctrl);

    ASSERT_NE(inserter, nullptr);
    ASSERT_TRUE(inserter->first_corner().has_value());
    EXPECT_EQ(*inserter->first_corner(), IntPoint(1000, 1000));
    EXPECT_EQ(*inserter->last_corner(), IntPoint(4000, 2000));

    bool found = false;
    for (const Trace* trace : board->get_traces())
    {
        if (trace->shares_net_no(std::vector<int>{1}))
        {
            const auto* polyline_trace = dynamic_cast<const PolylineTrace*>(trace);
            if (polyline_trace != nullptr && polyline_trace->polyline().contains(IntPoint(2500, 1500)))
            {
                found = true;
            }
        }
    }
    EXPECT_TRUE(found);
}

TEST(FoundConnectionInserterTest, EmptyConnectionReturnsNullptr)
{
    auto board = make_routing_board();
    const AutorouteControl ctrl = make_ctrl(1);
    TestLocator locator;
    locator.start_item = nullptr;
    locator.target_item = nullptr;

    EXPECT_EQ(FoundConnectionInserter::get_instance(&locator, board.get(), ctrl), nullptr);
}

TEST(FoundConnectionInserterTest, NullLocatorReturnsNullptr)
{
    auto board = make_routing_board();
    const AutorouteControl ctrl = make_ctrl(1);

    EXPECT_EQ(FoundConnectionInserter::get_instance(nullptr, board.get(), ctrl), nullptr);
}

TEST(FoundConnectionInserterTest, NullBoardReturnsNullptr)
{
    const AutorouteControl ctrl = make_ctrl(1);
    TestLocator locator;
    configure_straight_locator(locator, {IntPoint(1000, 1000), IntPoint(4000, 1000)}, nullptr, nullptr, 0);

    EXPECT_EQ(FoundConnectionInserter::get_instance(&locator, nullptr, ctrl), nullptr);
}

} // namespace
} // namespace freerouting::autoroute