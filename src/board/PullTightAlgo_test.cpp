#include <memory>
#include <vector>

#include "board/ChangedArea.h"
#include "board/Layer.h"
#include "board/PullTightAlgo.h"
#include "board/PullTightAlgo45.h"
#include "board/PullTightAlgo90.h"
#include "board/PullTightAlgoAnyAngle.h"
#include "board/RoutingBoard.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/IntOctagon.h"
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
    // Margin around the origin: check_trace_shape (like Java) rejects shapes
    // sticking out of the board bounding box, including trace half width.
    return std::make_shared<RoutingBoard>(geometry::planar::IntBox(-1000, -1000, 11000, 11000),
                                          layer_structure, std::move(board_rules), nullptr);
}

TEST(PullTightAlgoTest, GetInstanceSelects90Degree)
{
    auto board = make_routing_board();
    const_cast<rules::BoardRules&>(board->get_rules())
        .set_trace_angle_restriction(rules::AngleRestriction::RIGHT);
    auto algo = PullTightAlgo::get_instance(board.get(), {}, nullptr, 100, nullptr, -1,
                                            nullptr, -1);
    ASSERT_NE(algo, nullptr);
    EXPECT_NE(dynamic_cast<PullTightAlgo90*>(algo.get()), nullptr);
}

TEST(PullTightAlgoTest, GetInstanceSelects45DegreeByDefault)
{
    auto board = make_routing_board();
    // Default restriction is FORTY_FIVE.
    auto algo = PullTightAlgo::get_instance(board.get(), {}, nullptr, 100, nullptr, -1,
                                            nullptr, -1);
    ASSERT_NE(algo, nullptr);
    EXPECT_NE(dynamic_cast<PullTightAlgo45*>(algo.get()), nullptr);
}

TEST(PullTightAlgoTest, GetInstanceSelectsAnyAngle)
{
    auto board = make_routing_board();
    const_cast<rules::BoardRules&>(board->get_rules())
        .set_trace_angle_restriction(rules::AngleRestriction::NONE);
    auto algo = PullTightAlgo::get_instance(board.get(), {}, nullptr, 100, nullptr, -1,
                                            nullptr, -1);
    ASSERT_NE(algo, nullptr);
    EXPECT_NE(dynamic_cast<PullTightAlgoAnyAngle*>(algo.get()), nullptr);
}

TEST(PullTightAlgo90Test, StraightTraceIsUnchanged)
{
    auto board = make_routing_board();
    const_cast<rules::BoardRules&>(board->get_rules())
        .set_trace_angle_restriction(rules::AngleRestriction::RIGHT);
    auto algo = PullTightAlgo::get_instance(board.get(), {}, nullptr, 100, nullptr, -1,
                                            nullptr, -1);
    const geometry::planar::Polyline straight =
        geometry::planar::Polyline::from_corners({geometry::planar::IntPoint(0, 0),
                                                  geometry::planar::IntPoint(1000, 0)});
    const geometry::planar::Polyline result = algo->pull_tight(
        straight, 0, 50, {1}, 1, {});
    EXPECT_EQ(result.corner_count(), straight.corner_count());
    EXPECT_EQ(result.first_corner(), straight.first_corner());
    EXPECT_EQ(result.last_corner(), straight.last_corner());
}

TEST(PullTightAlgo90Test, RightAngleJogIsTightened)
{
    auto board = make_routing_board();
    const_cast<rules::BoardRules&>(board->get_rules())
        .set_trace_angle_restriction(rules::AngleRestriction::RIGHT);
    auto algo = PullTightAlgo::get_instance(board.get(), {}, nullptr, 100, nullptr, -1,
                                            nullptr, -1);
    // Right-angle jog: the second corner can be skipped when clearance allows.
    const geometry::planar::Polyline dogleg =
        geometry::planar::Polyline::from_corners({geometry::planar::IntPoint(0, 0),
                                                  geometry::planar::IntPoint(100, 0),
                                                  geometry::planar::IntPoint(100, 100),
                                                  geometry::planar::IntPoint(200, 100)});
    const geometry::planar::Polyline result = algo->pull_tight(
        dogleg, 0, 50, {1}, 1, {});
    EXPECT_LT(result.corner_count(), dogleg.corner_count());
    EXPECT_EQ(result.first_corner(), dogleg.first_corner());
    EXPECT_EQ(result.last_corner(), dogleg.last_corner());
}

TEST(PullTightAlgoAnyAngleTest, CollinearCornerIsRemoved)
{
    auto board = make_routing_board();
    const_cast<rules::BoardRules&>(board->get_rules())
        .set_trace_angle_restriction(rules::AngleRestriction::NONE);
    auto algo = PullTightAlgo::get_instance(board.get(), {}, nullptr, 100, nullptr, -1,
                                            nullptr, -1);
    ASSERT_NE(dynamic_cast<PullTightAlgoAnyAngle*>(algo.get()), nullptr);
    // Middle corner (100, 0) is collinear and redundant.
    const geometry::planar::Polyline poly =
        geometry::planar::Polyline::from_corners({geometry::planar::IntPoint(0, 0),
                                                  geometry::planar::IntPoint(100, 0),
                                                  geometry::planar::IntPoint(200, 0),
                                                  geometry::planar::IntPoint(200, 100)});
    const geometry::planar::Polyline result = algo->pull_tight(
        poly, 0, 50, {1}, 1, {});
    EXPECT_LT(result.corner_count(), poly.corner_count());
    EXPECT_EQ(result.first_corner(), poly.first_corner());
    EXPECT_EQ(result.last_corner(), poly.last_corner());
}

TEST(PolylineTest, FromCornersAddsClosingLines)
{
    const geometry::planar::Polyline poly =
        geometry::planar::Polyline::from_corners({geometry::planar::IntPoint(0, 0),
                                                  geometry::planar::IntPoint(100, 0),
                                                  geometry::planar::IntPoint(100, 100)});
    // 2 segments + 2 closing lines, 3 corners.
    EXPECT_EQ(poly.arr.size(), 4u);
    EXPECT_EQ(poly.corner_count(), 3);
    EXPECT_EQ(poly.corner(0), geometry::planar::IntPoint(0, 0));
    EXPECT_EQ(poly.corner(1), geometry::planar::IntPoint(100, 0));
    EXPECT_EQ(poly.corner(2), geometry::planar::IntPoint(100, 100));
}

TEST(ChangedAreaTest, GetAndSetEmptyRoundtrip)
{
    ChangedArea area(2);
    EXPECT_TRUE(area.get_area(0).is_empty());
    area.join(10, 20, 0);
    area.join(30, 40, 0);
    const geometry::planar::IntOctagon marked = area.get_area(0);
    EXPECT_FALSE(marked.is_empty());
    EXPECT_TRUE(marked.contains(geometry::planar::IntPoint(20, 30)));
    area.set_empty(0);
    EXPECT_TRUE(area.get_area(0).is_empty());
    // Out-of-range layers yield empty shapes without crashing.
    EXPECT_TRUE(area.get_area(-1).is_empty());
    EXPECT_TRUE(area.get_area(99).is_empty());
}

} // namespace
} // namespace freerouting::board
