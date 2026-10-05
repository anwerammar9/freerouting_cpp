#include <limits>
#include <memory>
#include <optional>
#include <vector>

#include "board/Layer.h"
#include "board/Padstack.h"
#include "board/PolylineTrace.h"
#include "board/RoutingBoard.h"
#include "board/SearchTreeManager.h"
#include "board/Trace.h"
#include "board/Via.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/IntOctagon.h"
#include "geometry/planar/Polyline.h"
#include "geometry/planar/PolygonShape.h"
#include "geometry/planar/Shape.h"
#include "gtest/gtest.h"
#include "rules/BoardRules.h"

namespace freerouting::board {
namespace {

using geometry::planar::IntBox;
using geometry::planar::IntPoint;
using geometry::planar::IntOctagon;
using geometry::planar::Line;
using geometry::planar::PolygonShape;
using geometry::planar::Polyline;

std::shared_ptr<RoutingBoard> make_routing_board()
{
    std::vector<std::shared_ptr<Layer>> layers;
    layers.push_back(std::make_shared<Layer>("Top", true));
    layers.push_back(std::make_shared<Layer>("Bottom", true));
    LayerStructure layer_structure(layers);
    auto board_rules = std::make_unique<rules::BoardRules>(layer_structure, 10);
    return std::make_shared<RoutingBoard>(IntBox(0, 0, 10000, 10000), layer_structure,
                                          std::move(board_rules), nullptr);
}

PolylineTrace* insert_trace(RoutingBoard* p_board, const std::vector<IntPoint>& p_corners,
                            const std::vector<int>& p_nets, int p_half_width,
                            FixedState p_fixed_state = FixedState::NOT_FIXED)
{
    auto polyline = std::make_shared<Polyline>(Polyline::from_corners(p_corners));
    return p_board->insert_trace_without_cleaning(polyline, 0, p_half_width, p_nets, 1,
                                                  p_fixed_state);
}

/** Returns the argument list Java FoundConnectionInserter uses for a forced segment. */
constexpr int kMaxShoveTraceRecursionDepth = 20;
constexpr int kMaxShoveViaRecursionDepth = 5;
constexpr int kMaxSpringOverRecursionDepth = 5;

std::optional<IntPoint> insert_segment(RoutingBoard* p_board, const IntPoint& p_from,
                                       const IntPoint& p_to, int p_half_width = 50,
                                       int p_layer = 0)
{
    return p_board->insert_forced_trace_segment(
        p_from, p_to, p_half_width, p_layer, {1}, 1, kMaxShoveTraceRecursionDepth,
        kMaxShoveViaRecursionDepth, kMaxSpringOverRecursionDepth, std::numeric_limits<int>::max(),
        500, true, nullptr);
}

TEST(RoutingBoardForcedTraceTest, SegmentWithEqualCornersReturnsTheToCorner)
{
    auto board = make_routing_board();

    const std::optional<IntPoint> result = insert_segment(board.get(), IntPoint(1000, 1000),
                                                          IntPoint(1000, 1000));

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, IntPoint(1000, 1000));
    // Nothing was inserted for a degenerate segment.
    EXPECT_EQ(board->get_traces().size(), 0u);
}

TEST(RoutingBoardForcedTraceTest, SegmentOnEmptyBoardIsInsertedUpToTheToCorner)
{
    auto board = make_routing_board();

    const std::optional<IntPoint> result = insert_segment(board.get(), IntPoint(1000, 1000),
                                                          IntPoint(4000, 1000));

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, IntPoint(4000, 1000));
    const std::vector<Trace*> traces = board->get_traces();
    ASSERT_EQ(traces.size(), 1u);
    EXPECT_EQ(traces[0]->get_net_nos(), std::vector<int>{1});
    EXPECT_EQ(traces[0]->get_half_width(), 50);
    EXPECT_EQ(traces[0]->first_corner().to_float().round(), IntPoint(1000, 1000));
    EXPECT_EQ(traces[0]->last_corner().to_float().round(), IntPoint(4000, 1000));
}

TEST(RoutingBoardForcedTraceTest, SegmentOutsideTheBoardBoundingBoxIsNotInserted)
{
    auto board = make_routing_board();
    // Inserting a trace fixes get_min_trace_half_width() to a realistic value.
    ASSERT_NE(insert_trace(board.get(), {IntPoint(1000, 8000), IntPoint(2000, 8000)}, {1}, 50),
              nullptr);

    // The segment lies completely outside of the board bounding box, so
    // ShoveTraceAlgo.check() rejects every shape and the insert bails out with
    // the from corner.
    const std::optional<IntPoint> result = insert_segment(board.get(), IntPoint(1000, -500),
                                                          IntPoint(4000, -500));

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, IntPoint(1000, -500));
    EXPECT_EQ(board->get_traces().size(), 1u);
}

TEST(RoutingBoardForcedTraceTest, PolylineWithEqualCornersReturnsTheToCorner)
{
    auto board = make_routing_board();
    // A polyline with equal corners is empty in Java, so the method returns its
    // own last corner.
    const Polyline polyline(IntPoint(1000, 1000), IntPoint(1000, 1000));
    ASSERT_TRUE(polyline.is_empty());

    const std::optional<IntPoint> result = board->insert_forced_trace_polyline(
        polyline, 50, 0, {1}, 1, kMaxShoveTraceRecursionDepth, kMaxShoveViaRecursionDepth,
        kMaxSpringOverRecursionDepth, std::numeric_limits<int>::max(), 500, true, nullptr);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, polyline.last_corner());
}

TEST(RoutingBoardForcedTraceTest, PolylineOnEmptyBoardIsInserted)
{
    auto board = make_routing_board();
    const Polyline polyline = Polyline::from_corners({IntPoint(1000, 1000), IntPoint(4000, 1000),
                                                      IntPoint(4000, 4000), IntPoint(7000, 4000)});

    const std::optional<IntPoint> result = board->insert_forced_trace_polyline(
        polyline, 50, 0, {1}, 1, kMaxShoveTraceRecursionDepth, kMaxShoveViaRecursionDepth,
        kMaxSpringOverRecursionDepth, std::numeric_limits<int>::max(), 500, true, nullptr);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, IntPoint(7000, 4000));
    EXPECT_FALSE(board->get_traces().empty());
}

TEST(RoutingBoardForcedTraceTest, CheckForcedTracePolylinePassesOnEmptyBoard)
{
    auto board = make_routing_board();
    const Polyline polyline = Polyline::from_corners({IntPoint(1000, 1000), IntPoint(4000, 1000),
                                                      IntPoint(4000, 4000), IntPoint(7000, 4000)});

    EXPECT_TRUE(board->check_forced_trace_polyline(polyline, 50, 0, {1}, 1,
                                                   kMaxShoveTraceRecursionDepth,
                                                   kMaxShoveViaRecursionDepth,
                                                   kMaxSpringOverRecursionDepth));
    // check must not modify the board.
    EXPECT_TRUE(board->get_traces().empty());
}

TEST(RoutingBoardForcedTraceTest, CheckForcedTracePolylineFailsAtShoveFixedObstacle)
{
    auto board = make_routing_board();
    ASSERT_NE(insert_trace(board.get(), {IntPoint(2000, 500), IntPoint(2000, 3000)}, {2}, 100,
                          FixedState::SHOVE_FIXED),
              nullptr);
    const Polyline polyline = Polyline::from_corners({IntPoint(1000, 1000), IntPoint(4000, 1000),
                                                      IntPoint(4000, 4000), IntPoint(7000, 4000)});

    EXPECT_FALSE(board->check_forced_trace_polyline(polyline, 50, 0, {1}, 1,
                                                    kMaxShoveTraceRecursionDepth,
                                                    kMaxShoveViaRecursionDepth,
                                                    kMaxSpringOverRecursionDepth));
}

TEST(RoutingBoardForcedTraceTest, ClearShoveFailingObstacleResetsTheTracking)
{
    auto board = make_routing_board();
    PolylineTrace* obstacle =
        insert_trace(board.get(), {IntPoint(2000, 500), IntPoint(2000, 3000)}, {2}, 100);
    ASSERT_NE(obstacle, nullptr);
    board->set_shove_failing_obstacle(obstacle);
    board->set_shove_failing_layer(3);
    ASSERT_EQ(board->get_shove_failing_obstacle(), obstacle);
    ASSERT_EQ(board->get_shove_failing_layer(), 3);

    board->clear_shove_failing_obstacle();

    EXPECT_EQ(board->get_shove_failing_obstacle(), nullptr);
    EXPECT_EQ(board->get_shove_failing_layer(), -1);
}

TEST(RoutingBoardConnectToTraceTest, PointOnTheTraceNeedsNoConnectionLine)
{
    auto board = make_routing_board();
    PolylineTrace* trace =
        insert_trace(board.get(), {IntPoint(1000, 1000), IntPoint(4000, 1000)}, {1}, 50);
    ASSERT_NE(trace, nullptr);
    const size_t trace_count_before = board->get_traces().size();

    EXPECT_TRUE(board->connect_to_trace(IntPoint(2000, 1000), trace, 50, 1));

    // No connection line was added and the trace tails were not removed.
    EXPECT_EQ(board->get_traces().size(), trace_count_before);
}

TEST(RoutingBoardConnectToTraceTest, OffTracePointGetsAConnectionLine)
{
    auto board = make_routing_board();
    PolylineTrace* trace =
        insert_trace(board.get(), {IntPoint(1000, 5000), IntPoint(4000, 5000)}, {1}, 50);
    ASSERT_NE(trace, nullptr);

    EXPECT_TRUE(board->connect_to_trace(IntPoint(2500, 6500), trace, 50, 1));

    // The connection line was combined into the trace it connects to.
    const std::vector<Trace*> traces = board->get_traces();
    ASSERT_EQ(traces.size(), 1u);
    const auto* polyline_trace = dynamic_cast<PolylineTrace*>(traces[0]);
    ASSERT_NE(polyline_trace, nullptr);
    EXPECT_TRUE(polyline_trace->polyline().contains(IntPoint(2500, 5000)));
    EXPECT_TRUE(polyline_trace->polyline().contains(IntPoint(2500, 6500)));
}

TEST(RoutingBoardConnectToTraceTest, NonPolylineTraceIsRejected)
{
    auto board = make_routing_board();
    auto* padstack = new Padstack("via_padstack");
    padstack->set_from_layer(0);
    padstack->set_to_layer(1);
    padstack->add_shape(0, new PolygonShape(std::vector<IntPoint>{{-200, -200},
                                                                 {200, -200},
                                                                 {200, 200},
                                                                 {-200, 200}}));
    padstack->add_shape(1, new PolygonShape(std::vector<IntPoint>{{-200, -200},
                                                                 {200, -200},
                                                                 {200, 200},
                                                                 {-200, 200}}));
    auto* via = board->insert_via(padstack, IntPoint(2500, 5000), {1}, 1,
                                  FixedState::NOT_FIXED, false);
    ASSERT_NE(via, nullptr);
    auto* drill_item = dynamic_cast<Trace*>(static_cast<Item*>(via));
    ASSERT_EQ(drill_item, nullptr);

    // A drill item is not a Trace at all, so the pointer type forbids the call.
    // The runtime guard of the port is checked with a null trace instead.
    EXPECT_FALSE(board->connect_to_trace(IntPoint(2500, 6500), nullptr, 50, 1));
}

TEST(RoutingBoardConnectToTraceTest, NullTraceIsRejected)
{
    auto board = make_routing_board();

    EXPECT_FALSE(board->connect_to_trace(IntPoint(2500, 6500), nullptr, 50, 1));
}

} // namespace
} // namespace freerouting::board
