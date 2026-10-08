#include <memory>
#include <vector>

#include "board/BoardOutline.h"
#include "board/Layer.h"
#include "board/PolylineTrace.h"
#include "board/RoutingBoard.h"
#include "board/Trace.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/Point.h"
#include "geometry/planar/PolygonShape.h"
#include "geometry/planar/Polyline.h"
#include "board/SearchTreeManager.h"
#include "board/ShapeSearchTree.h"
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

TEST(RoutingBoardStubsTest, GetOutlineReturnsNullWithoutOutline)
{
    auto board = make_routing_board();
    EXPECT_EQ(board->get_outline(), nullptr);
}

TEST(RoutingBoardStubsTest, GetOutlineReturnsInsertedOutline)
{
    auto board = make_routing_board();
    auto square = std::make_shared<geometry::planar::PolygonShape>(std::vector<geometry::planar::IntPoint>{
        { 0, 0 }, { 10000, 0 }, { 10000, 10000 }, { 0, 10000 } });
    BoardOutline* outline_ptr = board->insert_outline({ square }, 1);
    EXPECT_EQ(board->get_outline(), outline_ptr);
}

TEST(RoutingBoardStubsTest, OutlineTreeShapesAreThinShapesAlongTheLinesOnEveryLayer)
{
    auto board = make_routing_board();
    auto square = std::make_shared<geometry::planar::PolygonShape>(std::vector<geometry::planar::IntPoint>{
        { 0, 0 }, { 10000, 0 }, { 10000, 10000 }, { 0, 10000 } });
    BoardOutline* outline = board->insert_outline({ square }, 1);
    EXPECT_EQ(4, outline->line_count());
    const ShapeSearchTree* tree = board->get_search_tree_manager()->get_default_tree();
    ASSERT_EQ(4 * board->get_layer_count(), outline->tree_shape_count(*tree));
    // The first line of the square lies on y = 0; its tree shape is that line
    // widened by the half width 100 of the outline.
    const geometry::planar::IntBox box = outline->get_tree_shape(*tree, 0)->bounding_box();
    EXPECT_LE(box.ur.y - box.ll.y, 2 * BoardOutline::kHalfWidth + 2);
    EXPECT_TRUE(outline->is_obstacle(outline) == false);
    EXPECT_TRUE(outline->contains(geometry::planar::IntPoint(5000, 5000)));
    EXPECT_FALSE(outline->contains(geometry::planar::IntPoint(20000, 5000)));
}

TEST(RoutingBoardStubsTest, ContainsTraceTailsDetectsUncontactedTrace)
{
    auto board = make_routing_board();
    insert_trace(board.get(), {geometry::planar::IntPoint(0, 0),
                               geometry::planar::IntPoint(200, 0)}, {1});
    std::vector<Item*> board_items = board->get_autoroute_items();
    EXPECT_TRUE(board->contains_trace_tails(board_items, {2}));
}

TEST(RoutingBoardStubsTest, ContainsTraceTailsIgnoresExceptedNet)
{
    auto board = make_routing_board();
    insert_trace(board.get(), {geometry::planar::IntPoint(0, 0),
                               geometry::planar::IntPoint(200, 0)}, {1});
    std::vector<Item*> board_items = board->get_autoroute_items();
    EXPECT_FALSE(board->contains_trace_tails(board_items, {1}));
}

TEST(RoutingBoardStubsTest, GetTraceTailFindsTailAtItsOwnEndPoint)
{
    auto board = make_routing_board();
    PolylineTrace* trace = insert_trace(board.get(),
                                        {geometry::planar::IntPoint(0, 0),
                                         geometry::planar::IntPoint(200, 0)},
                                        {1});
    ASSERT_NE(trace, nullptr);
    Trace* tail = board->get_trace_tail(geometry::planar::IntPoint(0, 0), 0, {1});
    ASSERT_NE(tail, nullptr);
    EXPECT_EQ(tail, static_cast<Trace*>(trace));
}

TEST(RoutingBoardStubsTest, GetTraceTailReturnsNullAtOtherNetOrMiddlePoint)
{
    auto board = make_routing_board();
    PolylineTrace* trace = insert_trace(board.get(),
                                        {geometry::planar::IntPoint(0, 0),
                                         geometry::planar::IntPoint(200, 0)},
                                        {1});
    ASSERT_NE(trace, nullptr);
    EXPECT_EQ(board->get_trace_tail(geometry::planar::IntPoint(0, 0), 0, {99}), nullptr);
    EXPECT_EQ(board->get_trace_tail(geometry::planar::IntPoint(100, 0), 0, {1}), nullptr);
    (void)trace;
}

TEST(RoutingBoardStubsTest, CombineTracesCombinesCollinearPair)
{
    auto board = make_routing_board();
    insert_trace(board.get(), {geometry::planar::IntPoint(0, 0),
                               geometry::planar::IntPoint(100, 0)}, {1});
    insert_trace(board.get(), {geometry::planar::IntPoint(100, 0),
                               geometry::planar::IntPoint(200, 0)}, {1});
    board->combine_traces(1);
    std::vector<Trace*> traces = board->get_traces();
    EXPECT_EQ(traces.size(), 1u);
    EXPECT_EQ(traces[0]->corner_count(), 2);
}

TEST(RoutingBoardStubsTest, CombineTracesSkippedForOtherNet)
{
    auto board = make_routing_board();
    insert_trace(board.get(), {geometry::planar::IntPoint(0, 0),
                               geometry::planar::IntPoint(100, 0)}, {1});
    insert_trace(board.get(), {geometry::planar::IntPoint(100, 0),
                               geometry::planar::IntPoint(200, 0)}, {2});
    board->combine_traces(1);
    EXPECT_EQ(board->get_traces().size(), 2u);
}

} // namespace
} // namespace freerouting::board