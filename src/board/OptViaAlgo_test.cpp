#include <limits>
#include <memory>
#include <vector>

#include "board/Layer.h"
#include "board/OptViaAlgo.h"
#include "board/Padstack.h"
#include "board/RoutingBoard.h"
#include "board/Via.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/Point.h"
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

TEST(OptViaAlgoTest, UnconnectedViaIsNotMoved)
{
    auto board = make_routing_board();
    Padstack padstack("v1");
    padstack.set_from_layer(0);
    padstack.set_to_layer(1);
    Via via(&padstack, geometry::planar::IntPoint(50, 50), {1}, 1, 1, 0,
            FixedState::NOT_FIXED, true, board.get());
    // No contacts and not on the board: nothing to optimize.
    EXPECT_FALSE(OptViaAlgo::opt_via_location(board.get(), &via, nullptr, 100, 5));
}

TEST(OptViaAlgoTest, ShoveFixedViaIsNotMoved)
{
    auto board = make_routing_board();
    Padstack padstack("v1");
    padstack.set_from_layer(0);
    padstack.set_to_layer(1);
    Via via(&padstack, geometry::planar::IntPoint(50, 50), {1}, 1, 1, 0,
            FixedState::SHOVE_FIXED, true, board.get());
    EXPECT_FALSE(OptViaAlgo::opt_via_location(board.get(), &via, nullptr, 100, 5));
}

TEST(OptViaAlgoTest, ZeroRecursionDepthReturnsFalse)
{
    auto board = make_routing_board();
    Padstack padstack("v1");
    padstack.set_from_layer(0);
    padstack.set_to_layer(1);
    Via via(&padstack, geometry::planar::IntPoint(50, 50), {1}, 1, 1, 0,
            FixedState::NOT_FIXED, true, board.get());
    EXPECT_FALSE(OptViaAlgo::opt_via_location(board.get(), &via, nullptr, 100, 0));
}

TEST(RoutingBoardTest, CheckTraceSegmentConflictFreeOnEmptyBoard)
{
    auto board = make_routing_board();
    const double ok_length = board->check_trace_segment(
        geometry::planar::IntPoint(0, 0), geometry::planar::IntPoint(1000, 0), 0, {1},
        50, 1, false);
    EXPECT_DOUBLE_EQ(ok_length, static_cast<double>(std::numeric_limits<int>::max()));
}

TEST(RoutingBoardTest, CheckTraceSegmentDegenerateReturnsZero)
{
    auto board = make_routing_board();
    const double ok_length = board->check_trace_segment(
        geometry::planar::IntPoint(10, 10), geometry::planar::IntPoint(10, 10), 0, {1},
        50, 1, false);
    EXPECT_DOUBLE_EQ(ok_length, 0.0);
}

TEST(BasicBoardTest, PickItemsEmptyOnEmptyBoard)
{
    auto board = make_routing_board();
    std::set<Item*> picked = board->pick_items(geometry::planar::IntPoint(500, 500), 0, nullptr);
    EXPECT_TRUE(picked.empty());
}

} // namespace
} // namespace freerouting::board
