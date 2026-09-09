#include <memory>
#include <vector>

#include "board/BasicBoard.h"
#include "board/CalcFromSide.h"
#include "board/CalcShapeAndFromSide.h"
#include "board/FixedState.h"
#include "board/Layer.h"
#include "board/PolylineTrace.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/IntOctagon.h"
#include "geometry/planar/Point.h"
#include "geometry/planar/Polyline.h"
#include "geometry/planar/Vector.h"
#include "gtest/gtest.h"
#include "rules/BoardRules.h"

using namespace freerouting::board;
namespace planar = freerouting::geometry::planar;

namespace
{

constexpr FixedState NOT_FIXED = FixedState::NOT_FIXED;
constexpr int HALF_WIDTH = 300;

std::shared_ptr<BasicBoard> make_board()
{
    std::vector<std::shared_ptr<Layer>> layers;
    layers.push_back(std::make_shared<Layer>("Layer1", true));
    layers.push_back(std::make_shared<Layer>("Layer2", true));
    LayerStructure layer_structure(layers);
    auto board_rules = std::make_unique<::freerouting::rules::BoardRules>(layer_structure, 10);
    return std::make_shared<BasicBoard>(layer_structure, std::move(board_rules));
}

/**
 * Builds an orthogonal staircase trace with 6 corners (5 segments):
 *     (0,0) -> (1000,0) -> (1000,1000) -> (2000,1000) -> (2000,2000) -> (3000,2000)
 * The last segment lies on the line y == 2000, which clips the cap of segment
 * index 2 (the vertical segment ending at (2000,2000)).
 */
std::shared_ptr<PolylineTrace> make_staircase_trace()
{
    const std::vector<planar::IntPoint> corners = {planar::IntPoint(0, 0),
                                                    planar::IntPoint(1000, 0),
                                                    planar::IntPoint(1000, 1000),
                                                    planar::IntPoint(2000, 1000),
                                                    planar::IntPoint(2000, 2000),
                                                    planar::IntPoint(3000, 2000)};
    std::vector<planar::Line> lines;
    for (size_t i = 0; i + 1 < corners.size(); ++i)
    {
        lines.push_back(planar::Line(corners[i], corners[i + 1]));
    }
    auto polyline = std::make_shared<planar::Polyline>(lines);
    auto trace = std::make_shared<PolylineTrace>(polyline, 0, HALF_WIDTH, std::vector<int>{1}, 0, 1,
                                                 0, NOT_FIXED);
    return trace;
}

bool boxes_equal(const planar::IntBox& p_a, const planar::IntBox& p_b)
{
    return p_a.ll.x == p_b.ll.x && p_a.ll.y == p_b.ll.y && p_a.ur.x == p_b.ur.x
           && p_a.ur.y == p_b.ur.y;
}

bool boxes_contained(const planar::IntBox& p_inner, const planar::IntBox& p_outer)
{
    return p_inner.ll.x >= p_outer.ll.x && p_inner.ll.y >= p_outer.ll.y
           && p_inner.ur.x <= p_outer.ur.x && p_inner.ur.y <= p_outer.ur.y;
}

} // namespace

TEST(CalcShapeAndFromSideTest, NonOrthogonalMiddleIndexUsesFallback)
{
    auto board = make_board();
    auto trace = make_staircase_trace();
    trace->set_board(board.get());

    // Segment index 1 neither cuts at the end (p_index != 5 - 3) nor at the
    // start, so the CalcFromSide fallback is used.
    CalcShapeAndFromSide calc(*trace, 1, false, false);

    const planar::IntBox shape_box = calc.shape().bounding_box();
    const planar::IntBox reference_box = trace->get_tile_shape(1).bounding_box();
    EXPECT_TRUE(boxes_equal(shape_box, reference_box)) << "shape box must equal octagon box";

    ASSERT_NE(calc.from_side(), nullptr);
    EXPECT_GE(calc.from_side()->no, 0);
}

TEST(CalcShapeAndFromSideTest, NonOrthogonalForcedEndCutReducesShape)
{
    auto board = make_board();
    auto trace = make_staircase_trace();
    trace->set_board(board.get());

    // line_count - 3 == 2 forces the dog ear cut at the end of the trace. The
    // last segment line (y == 2000) through the joint at (2000,2000) clips the
    // vertical octagon of segment index 2, so the result must be strictly
    // smaller than the octagon but still contained in it.
    CalcShapeAndFromSide calc(*trace, 2, false, false);

    const planar::IntBox shape_box = calc.shape().bounding_box();
    const planar::IntBox reference_box = trace->get_tile_shape(2).bounding_box();
    EXPECT_TRUE(boxes_contained(shape_box, reference_box))
        << "cut shape must be contained in the octagon";
    EXPECT_FALSE(boxes_equal(shape_box, reference_box))
        << "cut shape must actually differ from the octagon";

    ASSERT_NE(calc.from_side(), nullptr);
    EXPECT_GE(calc.from_side()->no, 0);
}

TEST(CalcShapeAndFromSideTest, OrthogonalReturnsBoundingBox)
{
    auto board = make_board();
    auto trace = make_staircase_trace();
    trace->set_board(board.get());

    const planar::IntOctagon octagon = trace->get_tile_shape(1);
    CalcShapeAndFromSide calc(*trace, 1, true, false);

    EXPECT_TRUE(calc.shape().is_IntBox());
    EXPECT_TRUE(boxes_equal(calc.shape().bounding_box(), octagon.bounding_box()));

    ASSERT_NE(calc.from_side(), nullptr);
    EXPECT_GE(calc.from_side()->no, 0);
}

TEST(CalcShapeAndFromSideTest, ShoveCheckSuppressesFallback)
{
    auto board = make_board();
    auto trace = make_staircase_trace();
    trace->set_board(board.get());

    // Segment index 1 has no cutlines, and p_in_shove_check suppresses the
    // fallback.
    CalcShapeAndFromSide calc(*trace, 1, false, true);

    EXPECT_EQ(calc.from_side(), nullptr);
    EXPECT_FALSE(calc.shape().is_empty());
}

TEST(CalcShapeAndFromSideTest, ShoveCheckKeepsCutFromSide)
{
    auto board = make_board();
    auto trace = make_staircase_trace();
    trace->set_board(board.get());

    // Even in shove check mode the cut branch yields a fromside.
    CalcShapeAndFromSide calc(*trace, 2, false, true);

    ASSERT_NE(calc.from_side(), nullptr);
    EXPECT_GE(calc.from_side()->no, 0);
}