#include <cstdio>
#include <memory>
#include <vector>

#include "board/BasicBoard.h"
#include "board/Layer.h"
#include "board/PolylineTrace.h"
#include "board/RoutingBoard.h"
#include "board/Trace.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/Point.h"
#include "geometry/planar/Polyline.h"
#include "gtest/gtest.h"
#include "rules/BoardRules.h"

namespace freerouting::board {
namespace {

std::shared_ptr<RoutingBoard> make_routing_board()
{
    std::vector<std::shared_ptr<Layer>> layers;
    layers.push_back(std::make_shared<Layer>("Top", true));
    layers.push_back(std::make_shared<Layer>("Bottom", true));
    LayerStructure layer_structure(layers);
    auto board_rules = std::make_unique<rules::BoardRules>(layer_structure, 10);
    return std::make_shared<RoutingBoard>(geometry::planar::IntBox(0, 0, 10000, 10000),
                                          layer_structure, std::move(board_rules), nullptr);
}

PolylineTrace* insert_trace(RoutingBoard* p_board,
                            const std::vector<geometry::planar::IntPoint>& p_corners,
                            const std::vector<int>& p_nets)
{
    auto polyline = std::make_shared<geometry::planar::Polyline>(
        geometry::planar::Polyline::from_corners(p_corners));
    return p_board->insert_trace_without_cleaning(polyline, 0, 50, p_nets, 1,
                                                  FixedState::NOT_FIXED);
}

TEST(TraceSplitTest, SplitPointOffTraceReturnsEmpty)
{
    auto board = make_routing_board();
    PolylineTrace* trace = insert_trace(board.get(),
                                        {geometry::planar::IntPoint(0, 0),
                                         geometry::planar::IntPoint(200, 0)},
                                        {1});
    ASSERT_NE(trace, nullptr);
    std::vector<Trace*> pieces =
        trace->split(geometry::planar::IntPoint(100, 100));
    EXPECT_TRUE(pieces.empty());
}

TEST(TraceSplitTest, SplitPointOnTraceYieldsTwoPieces)
{
    auto board = make_routing_board();
    PolylineTrace* trace = insert_trace(board.get(),
                                        {geometry::planar::IntPoint(0, 0),
                                         geometry::planar::IntPoint(200, 0)},
                                        {1});
    ASSERT_NE(trace, nullptr);
    std::vector<Trace*> pieces =
        trace->split(geometry::planar::IntPoint(100, 0));
    ASSERT_EQ(pieces.size(), 2u);
    for (Trace* piece : pieces)
    {
        EXPECT_NE(dynamic_cast<PolylineTrace*>(piece), nullptr);
    }
}

TEST(TraceSplitTest, SplitTracesAtCrossing)
{
    auto board = make_routing_board();
    PolylineTrace* horizontal = insert_trace(board.get(),
                                             {geometry::planar::IntPoint(0, 0),
                                              geometry::planar::IntPoint(200, 0)},
                                             {1});
    PolylineTrace* vertical = insert_trace(board.get(),
                                           {geometry::planar::IntPoint(100, -100),
                                            geometry::planar::IntPoint(100, 100)},
                                           {1});
    ASSERT_NE(horizontal, nullptr);
    ASSERT_NE(vertical, nullptr);
    EXPECT_TRUE(
        board->split_traces(geometry::planar::IntPoint(100, 0), 0, 1));
}

TEST(TraceSplitTest, SplitTracesElsewhereReturnsFalse)
{
    auto board = make_routing_board();
    PolylineTrace* trace = insert_trace(board.get(),
                                        {geometry::planar::IntPoint(0, 0),
                                         geometry::planar::IntPoint(200, 0)},
                                        {1});
    ASSERT_NE(trace, nullptr);
    EXPECT_FALSE(
        board->split_traces(geometry::planar::IntPoint(5000, 5000), 0, 1));
}

TEST(TraceSplitTest, RemoveIfCycleReturnsFalseForPlainTrace)
{
    auto board = make_routing_board();
    PolylineTrace* trace = insert_trace(board.get(),
                                        {geometry::planar::IntPoint(0, 0),
                                         geometry::planar::IntPoint(200, 0)},
                                        {1});
    ASSERT_NE(trace, nullptr);
    EXPECT_FALSE(board->remove_if_cycle(trace));
}

TEST(TraceCombineTest, CollinearTracesCombineEndToEnd)
{
    auto board = make_routing_board();
    PolylineTrace* first = insert_trace(board.get(),
                                        {geometry::planar::IntPoint(0, 0),
                                         geometry::planar::IntPoint(100, 0)},
                                        {1});
    PolylineTrace* second = insert_trace(board.get(),
                                         {geometry::planar::IntPoint(100, 0),
                                          geometry::planar::IntPoint(200, 0)},
                                         {1});
    ASSERT_NE(first, nullptr);
    ASSERT_NE(second, nullptr);
    EXPECT_TRUE(first->combine());
    const geometry::planar::IntPoint expected_first(0, 0);
    const geometry::planar::IntPoint expected_last(200, 0);
    EXPECT_TRUE(first->first_corner().equals(&expected_first));
    EXPECT_TRUE(first->last_corner().equals(&expected_last));
    EXPECT_FALSE(second->is_on_board());
}

TEST(TraceCombineTest, DifferentNetsDoNotCombine)
{
    auto board = make_routing_board();
    PolylineTrace* first = insert_trace(board.get(),
                                        {geometry::planar::IntPoint(0, 0),
                                         geometry::planar::IntPoint(100, 0)},
                                        {1});
    PolylineTrace* second = insert_trace(board.get(),
                                         {geometry::planar::IntPoint(100, 0),
                                          geometry::planar::IntPoint(200, 0)},
                                         {2});
    ASSERT_NE(first, nullptr);
    ASSERT_NE(second, nullptr);
    EXPECT_FALSE(first->combine());
    EXPECT_TRUE(second->is_on_board());
}

TEST(TraceNormalizeTest, SimpleTraceReturnsFalse)
{
    auto board = make_routing_board();
    PolylineTrace* trace = insert_trace(board.get(),
                                        {geometry::planar::IntPoint(0, 0),
                                         geometry::planar::IntPoint(200, 0)},
                                        {1});
    ASSERT_NE(trace, nullptr);
    EXPECT_FALSE(trace->normalize(nullptr));
    EXPECT_FALSE(board->normalize_traces(99));
}

TEST(TraceNormalizeTest, CrossingTracesNormalize)
{
    auto board = make_routing_board();
    PolylineTrace* angled = insert_trace(board.get(),
                                         {geometry::planar::IntPoint(0, 0),
                                          geometry::planar::IntPoint(200, 0),
                                          geometry::planar::IntPoint(200, 200)},
                                         {1});
    PolylineTrace* vertical = insert_trace(board.get(),
                                           {geometry::planar::IntPoint(100, -100),
                                            geometry::planar::IntPoint(100, 300)},
                                           {1});
    ASSERT_NE(angled, nullptr);
    ASSERT_NE(vertical, nullptr);
    EXPECT_TRUE(board->normalize_traces(1));
}

TEST(BasicBoardTest, InsertedTracesGetUniqueIds)
{
    auto board = make_routing_board();
    PolylineTrace* first = insert_trace(board.get(),
                                        {geometry::planar::IntPoint(0, 0),
                                         geometry::planar::IntPoint(200, 0)},
                                        {1});
    PolylineTrace* second = insert_trace(board.get(),
                                         {geometry::planar::IntPoint(0, 100),
                                          geometry::planar::IntPoint(200, 100)},
                                         {1});
    ASSERT_NE(first, nullptr);
    ASSERT_NE(second, nullptr);
    EXPECT_GT(first->get_id(), 0);
    EXPECT_GT(second->get_id(), 0);
    EXPECT_NE(first->get_id(), second->get_id());
}

} // namespace
} // namespace freerouting::board
