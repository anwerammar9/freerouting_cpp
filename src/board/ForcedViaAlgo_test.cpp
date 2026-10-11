#include <memory>
#include <string>
#include <vector>

#include "board/ForcedViaAlgo.h"
#include "board/Layer.h"
#include "board/Padstack.h"
#include "board/PolylineTrace.h"
#include "board/RoutingBoard.h"
#include "board/Trace.h"
#include "board/Via.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/Point.h"
#include "geometry/planar/Polyline.h"
#include "geometry/planar/PolygonShape.h"
#include "gtest/gtest.h"
#include "rules/BoardRules.h"
#include "rules/ViaInfo.h"

namespace freerouting::board {
namespace {

using geometry::planar::IntBox;
using geometry::planar::IntPoint;
using geometry::planar::PolygonShape;
using geometry::planar::Polyline;

constexpr int kViaCenter = 5000;
/** Pad shape of the test padstacks: a 200 x 200 box around the origin. */
constexpr int kPadHalfWidth = 100;
constexpr int kForeignTraceHalfWidth = 50;
constexpr int kMaxRecursionDepth = 3;
constexpr int kMaxViaRecursionDepth = 2;

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

/** Returns a square polygon shape around the origin, as the pad stacks use. */
std::unique_ptr<PolygonShape> make_square_shape(int p_half_width)
{
    return std::make_unique<PolygonShape>(std::vector<IntPoint>{{-p_half_width, -p_half_width},
                                                                {p_half_width, -p_half_width},
                                                                {p_half_width, p_half_width},
                                                                {-p_half_width, p_half_width}});
}

std::shared_ptr<Padstack> make_via_padstack(const std::string& p_name, bool p_pad_on_both_layers)
{
    auto padstack = std::make_shared<Padstack>(p_name);
    padstack->set_from_layer(0);
    padstack->set_to_layer(1);
    padstack->add_shape(0, make_square_shape(kPadHalfWidth).release());
    if (p_pad_on_both_layers)
    {
        padstack->add_shape(1, make_square_shape(kPadHalfWidth).release());
    }
    return padstack;
}

std::shared_ptr<rules::ViaInfo> make_via_info(const std::shared_ptr<Padstack>& p_padstack,
                                              RoutingBoard* p_board)
{
    return std::make_shared<rules::ViaInfo>(p_padstack->get_name(), p_padstack.get(), 1, true,
                                            &p_board->mutable_rules());
}

/**
 * Inserts a horizontal trace of the foreign net 2 at the input y coordinate.
 * A shove-fixed trace is used so that it really blocks a via instead of being
 * shoved aside, which is the observable difference of the hole clearance checks.
 */
PolylineTrace* insert_foreign_trace(RoutingBoard* p_board, int p_y,
                                    FixedState p_fixed_state = FixedState::SHOVE_FIXED)
{
    auto polyline = std::make_shared<Polyline>(
        Polyline::from_corners(std::vector<IntPoint>{IntPoint(0, p_y), IntPoint(2000, p_y),
                                                     IntPoint(10000, p_y)}));
    return p_board->insert_trace_without_cleaning(polyline, 1, kForeignTraceHalfWidth, {2}, 1,
                                                  p_fixed_state);
}

bool check_via(const std::shared_ptr<rules::ViaInfo>& p_via_info, RoutingBoard* p_board)
{
    return ForcedViaAlgo::check(p_via_info, IntPoint(kViaCenter, kViaCenter), {1},
                                kMaxRecursionDepth, kMaxViaRecursionDepth, p_board);
}

// ---------------------------------------------------------------------------
// Padstack.getDrillRadius (Java core.Padstack.getDrillRadius)
// ---------------------------------------------------------------------------

TEST(PadstackDrillRadiusTest, PadstackWithoutShapeHasNoDrillRadius)
{
    Padstack padstack("empty");
    EXPECT_DOUBLE_EQ(padstack.get_drill_radius(), 0.0);
}

/** Port of Java Padstack.getSmallestRadius() applied through getDrillRadius(). */
TEST(PadstackDrillRadiusTest, FallbackIs45PercentOfTheSmallestRadius)
{
    Padstack padstack("no_drill_in_name");
    padstack.add_shape(0, make_square_shape(100).release());
    EXPECT_DOUBLE_EQ(padstack.get_drill_radius(), 45.0);
}

TEST(PadstackDrillRadiusTest, SmallestShapeWinsOverTheLargestOne)
{
    Padstack padstack("no_drill_in_name");
    padstack.add_shape(0, make_square_shape(500).release());
    padstack.add_shape(1, make_square_shape(40).release());
    // min(width, height) / 2 = 40, times 0.45
    EXPECT_DOUBLE_EQ(padstack.get_drill_radius(), 18.0);
}

/**
 * A padstack name of the form <outer>_<drill>:<drill>_<inner> carries the
 * drill diameter, so the radius is scaled down to the smallest real shape.
 */
TEST(PadstackDrillRadiusTest, DrillDiameterIsParsedFromTheName)
{
    Padstack padstack("Via[0-1]_800:400_um");
    padstack.add_shape(0, make_square_shape(100).release());
    // Smallest radius is 100, drill dia 400 of outer dia 800 -> 50.
    EXPECT_DOUBLE_EQ(padstack.get_drill_radius(), 50.0);
}

TEST(PadstackDrillRadiusTest, NameWithoutUnderscoreAfterTheColonUsesTheRest)
{
    Padstack padstack("Via[0-1]_800:400");
    padstack.add_shape(0, make_square_shape(100).release());
    // drill dia 400 of outer dia 800 -> 50.
    EXPECT_DOUBLE_EQ(padstack.get_drill_radius(), 50.0);
}

TEST(PadstackDrillRadiusTest, MalformedNameFallsBackToTheSmallestRadius)
{
    Padstack padstack("Via[0-1]_x:y_z");
    padstack.add_shape(0, make_square_shape(100).release());
    // The numbers cannot be parsed, so the fallback is used.
    EXPECT_DOUBLE_EQ(padstack.get_drill_radius(), 45.0);
}

TEST(PadstackDrillRadiusTest, ZeroOuterDiameterFallsBackToTheSmallestRadius)
{
    Padstack padstack("Via[0-1]_0:400_um");
    padstack.add_shape(0, make_square_shape(100).release());
    EXPECT_DOUBLE_EQ(padstack.get_drill_radius(), 45.0);
}

TEST(PadstackDrillRadiusTest, CacheIsDroppedWhenTheShapeIsAdded)
{
    Padstack padstack("no_drill_in_name");
    EXPECT_DOUBLE_EQ(padstack.get_drill_radius(), 0.0);
    padstack.add_shape(0, make_square_shape(100).release());
    EXPECT_DOUBLE_EQ(padstack.get_drill_radius(), 45.0);
}

// ---------------------------------------------------------------------------
// BoardRules hole clearance (Java BoardRules.getHoleClearance/setHoleClearance)
// ---------------------------------------------------------------------------

TEST(BoardRulesHoleClearanceTest, DefaultIsZeroAndNegativeValuesAreClamped)
{
    std::vector<std::shared_ptr<Layer>> layers;
    layers.push_back(std::make_shared<Layer>("Top", true));
    LayerStructure layer_structure(layers);
    rules::BoardRules board_rules(layer_structure, 10);
    EXPECT_EQ(board_rules.get_hole_clearance(), 0);
    board_rules.set_hole_clearance(-25);
    EXPECT_EQ(board_rules.get_hole_clearance(), 0);
    board_rules.set_hole_clearance(400);
    EXPECT_EQ(board_rules.get_hole_clearance(), 400);
}

TEST(BoardRulesHoleClearanceTest, ClearanceClassNoneIsZero)
{
    EXPECT_EQ(rules::BoardRules::clearance_class_none(), 0);
}

// ---------------------------------------------------------------------------
// ForcedViaAlgo.check with the hole clearance shape
// ---------------------------------------------------------------------------

/** Without the hole clearance rule a copper-less via layer is simply skipped. */
TEST(ForcedViaAlgoHoleClearanceTest, CopperlessLayerIsSkippedWithoutHoleClearance)
{
    auto board = make_routing_board();
    auto padstack = make_via_padstack("via", false);
    auto via_info = make_via_info(padstack, board.get());
    ASSERT_NE(insert_foreign_trace(board.get(), kViaCenter), nullptr);

    EXPECT_TRUE(check_via(via_info, board.get()));
}

/**
 * Port of Java ForcedViaInserter.check() lines 147-154 and 183-208: with the
 * hole clearance rule active, a copper-less via layer is checked against the
 * hole shape with the null clearance class.
 */
TEST(ForcedViaAlgoHoleClearanceTest, CopperlessLayerIsCheckedAgainstTheHoleShape)
{
    auto board = make_routing_board();
    board->mutable_rules().set_hole_clearance(1000);
    auto padstack = make_via_padstack("via", false);
    auto via_info = make_via_info(padstack, board.get());
    ASSERT_NE(insert_foreign_trace(board.get(), kViaCenter), nullptr);

    EXPECT_FALSE(check_via(via_info, board.get()));
    EXPECT_EQ(board->get_shove_failing_layer(), 1);
}

/** The same board as above passes when the foreign trace is outside the hole. */
TEST(ForcedViaAlgoHoleClearanceTest, CopperlessLayerPassesOutsideTheHoleShape)
{
    auto board = make_routing_board();
    board->mutable_rules().set_hole_clearance(1000);
    auto padstack = make_via_padstack("via", false);
    auto via_info = make_via_info(padstack, board.get());
    // Drill radius 45 plus hole clearance 1000 plus the constant 10 -> 1055.
    ASSERT_NE(insert_foreign_trace(board.get(), kViaCenter + 4000), nullptr);

    EXPECT_TRUE(check_via(via_info, board.get()));
}

/**
 * Port of Java ForcedViaInserter.check() lines 183-208: on a layer that has a
 * pad shape, the drill hole must additionally keep hole clearance from
 * other-net copper. The foreign trace here satisfies the copper clearance of
 * the pad but not the hole clearance.
 */
TEST(ForcedViaAlgoHoleClearanceTest, HoleShapeIsCheckedOnAPadLayer)
{
    auto board = make_routing_board();
    board->mutable_rules().set_hole_clearance(1000);
    auto padstack = make_via_padstack("via", true);
    auto via_info = make_via_info(padstack, board.get());
    // The trace is 400 units above the via center, well outside the 100 unit
    // pad plus the copper clearance, but inside the 1055 unit hole shape.
    ASSERT_NE(insert_foreign_trace(board.get(), kViaCenter + 400), nullptr);

    EXPECT_FALSE(check_via(via_info, board.get()));
    EXPECT_EQ(board->get_shove_failing_layer(), 1);
}

TEST(ForcedViaAlgoHoleClearanceTest, PadLayerPassesWithoutTheHoleClearanceRule)
{
    auto board = make_routing_board();
    auto padstack = make_via_padstack("via", true);
    auto via_info = make_via_info(padstack, board.get());
    ASSERT_NE(insert_foreign_trace(board.get(), kViaCenter + 400), nullptr);

    EXPECT_TRUE(check_via(via_info, board.get()));
}

TEST(ForcedViaAlgoHoleClearanceTest, HoleShapeIsNotBuiltWithoutAKnownDrillRadius)
{
    auto board = make_routing_board();
    board->mutable_rules().set_hole_clearance(1000);
    // A padstack without any shape has a drill radius of 0, so no hole shape
    // exists and the layer is skipped again.
    auto padstack = std::make_shared<Padstack>("shapeless");
    padstack->set_from_layer(0);
    padstack->set_to_layer(1);
    auto via_info = make_via_info(padstack, board.get());
    ASSERT_NE(insert_foreign_trace(board.get(), kViaCenter), nullptr);

    EXPECT_TRUE(check_via(via_info, board.get()));
}

// ---------------------------------------------------------------------------
// ForcedViaAlgo.insert with the hole clearance shape
// ---------------------------------------------------------------------------

TEST(ForcedViaAlgoHoleClearanceTest, InsertOnACopperlessLayerInsertsTheVia)
{
    auto board = make_routing_board();
    board->mutable_rules().set_hole_clearance(1000);
    auto padstack = make_via_padstack("via", false);
    auto via_info = make_via_info(padstack, board.get());

    EXPECT_TRUE(ForcedViaAlgo::insert(via_info, IntPoint(kViaCenter, kViaCenter), {1}, 1, {}, 3, 2,
                                     board.get()));
    EXPECT_EQ(board->get_vias().size(), 1u);
}

TEST(ForcedViaAlgoHoleClearanceTest, InsertFailsWhenTheHoleShapeIsBlocked)
{
    auto board = make_routing_board();
    board->mutable_rules().set_hole_clearance(1000);
    auto padstack = make_via_padstack("via", false);
    auto via_info = make_via_info(padstack, board.get());
    ASSERT_NE(insert_foreign_trace(board.get(), kViaCenter), nullptr);

    EXPECT_FALSE(ForcedViaAlgo::insert(via_info, IntPoint(kViaCenter, kViaCenter), {1}, 1, {}, 3, 2,
                                       board.get()));
    EXPECT_EQ(board->get_shove_failing_layer(), 1);
    EXPECT_TRUE(board->get_vias().empty());
}

/**
 * The hole shape is a circle around the via location, so its bounding tile is
 * that circle and not a translation of the pad stack shape. Pinned on the
 * observable effect of its radius: even a hole clearance of one unit adds the
 * drill radius of 45 plus the constant 10 to the blocked area.
 */
TEST(ForcedViaAlgoHoleClearanceTest, HoleShapeRadiusCoversTheDrillRadius)
{
    auto near_board = make_routing_board();
    near_board->mutable_rules().set_hole_clearance(1);
    auto near_padstack = make_via_padstack("via", false);
    auto near_via_info = make_via_info(near_padstack, near_board.get());
    ASSERT_NE(insert_foreign_trace(near_board.get(), kViaCenter + 45), nullptr);
    EXPECT_FALSE(check_via(near_via_info, near_board.get()));

    auto far_board = make_routing_board();
    far_board->mutable_rules().set_hole_clearance(1);
    auto far_padstack = make_via_padstack("via", false);
    auto far_via_info = make_via_info(far_padstack, far_board.get());
    ASSERT_NE(insert_foreign_trace(far_board.get(), kViaCenter + 500), nullptr);
    EXPECT_TRUE(check_via(far_via_info, far_board.get()));
}

} // namespace
} // namespace freerouting::board
