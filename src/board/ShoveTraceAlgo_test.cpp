#include <algorithm>
#include <limits>
#include <memory>
#include <optional>
#include <set>
#include <vector>

#include "board/CalcFromSide.h"
#include "board/ShapeTraceEntries.h"
#include "board/Layer.h"
#include "board/Padstack.h"
#include "board/Pin.h"
#include "board/PolylineTrace.h"
#include "board/RoutingBoard.h"
#include "board/SearchTreeManager.h"
#include "board/ShoveTraceAlgo.h"
#include "board/Via.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/IntOctagon.h"
#include "geometry/planar/LineSegment.h"
#include "geometry/planar/Polyline.h"
#include "geometry/planar/PolygonShape.h"
#include "geometry/planar/Shape.h"
#include "geometry/planar/TileShape.h"
#include "geometry/planar/Vector.h"
#include "gtest/gtest.h"
#include "rules/BoardRules.h"

namespace freerouting::board {
namespace {

using geometry::planar::FloatPoint;
using geometry::planar::IntBox;
using geometry::planar::IntPoint;
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

Pin* insert_smd_pin(RoutingBoard* p_board, const IntPoint& p_center,
                    const std::vector<int>& p_nets, int p_pad_half_width)
{
    auto* pin = new Pin(7, 1, p_nets, 1, p_board->generate_item_id(), FixedState::NOT_FIXED,
                        p_board);
    pin->set_center(p_center);
    auto* padstack = new Padstack("p1");
    padstack->set_from_layer(0);
    padstack->set_to_layer(0);
    padstack->add_shape(0, new PolygonShape(std::vector<IntPoint>{
                                               {static_cast<int64_t>(-p_pad_half_width),
                                                static_cast<int64_t>(-p_pad_half_width)},
                                               {static_cast<int64_t>(p_pad_half_width),
                                                static_cast<int64_t>(-p_pad_half_width)},
                                               {static_cast<int64_t>(p_pad_half_width),
                                                static_cast<int64_t>(p_pad_half_width)},
                                               {static_cast<int64_t>(-p_pad_half_width),
                                                static_cast<int64_t>(p_pad_half_width)}}));
    pin->set_padstack(padstack);
    pin->set_bounding_box(IntBox(p_center.x - p_pad_half_width, p_center.y - p_pad_half_width,
                                 p_center.x + p_pad_half_width, p_center.y + p_pad_half_width));
    p_board->get_search_tree_manager()->insert(pin);
    p_board->add_item(pin);
    return pin;
}

/** Returns the obstacle trace of the spring over tests: net 2, shove fixed. */
PolylineTrace* insert_shove_fixed_obstacle(RoutingBoard* p_board)
{
    return insert_trace(p_board, {IntPoint(5000, 2000), IntPoint(5000, 8000)}, {2}, 50,
                        FixedState::SHOVE_FIXED);
}

/**
 * Returns a 3-line polyline of net 1, which crosses the obstacle trace above.
 * The first line overlaps the obstacle, the second line crosses the offset
 * shape of the obstacle, and the third line is far away.
 */
Polyline crossing_polyline()
{
    return Polyline(std::vector<Line>{Line(IntPoint(4000, 3000), IntPoint(6000, 3000)),
                                      Line(IntPoint(6000, 3000), IntPoint(4000, 6000)),
                                      Line(IntPoint(4000, 6000), IntPoint(3000, 7000))});
}

/** Returns true, if p_point lies strictly inside p_box. */
bool is_strictly_inside(const IntBox& p_box, const IntPoint& p_point)
{
    return p_point.x > p_box.ll.x && p_point.x < p_box.ur.x && p_point.y > p_box.ll.y &&
           p_point.y < p_box.ur.y;
}

TEST(ShoveTraceAlgoSpringOverTest, NoObstacleReturnsTheInputPolylineUnchanged)
{
    auto board = make_routing_board();
    ASSERT_NE(insert_shove_fixed_obstacle(board.get()), nullptr);
    ShoveTraceAlgo algo(board.get());
    const Polyline polyline(std::vector<Line>{Line(IntPoint(1000, 9000), IntPoint(3000, 9000)),
                                              Line(IntPoint(3000, 9000), IntPoint(3000, 9500)),
                                              Line(IntPoint(3000, 9500), IntPoint(9000, 9500))});

    const ShoveTraceAlgo::SpringOverResult result =
        algo.spring_over(polyline, 50, 0, {1}, 1, true, 20, nullptr);

    EXPECT_FALSE(result.changed);
    ASSERT_TRUE(result.polyline.has_value());
    EXPECT_EQ(result.polyline->arr.size(), polyline.arr.size());
    EXPECT_EQ(result.polyline->arr.front().a.x, polyline.arr.front().a.x);
    EXPECT_EQ(result.polyline->arr.back().b.y, polyline.arr.back().b.y);
}

TEST(ShoveTraceAlgoSpringOverTest, ObstacleIsWrappedInCounterClockSense)
{
    auto board = make_routing_board();
    ASSERT_NE(insert_shove_fixed_obstacle(board.get()), nullptr);
    ShoveTraceAlgo algo(board.get());
    const Polyline polyline = crossing_polyline();

    const ShoveTraceAlgo::SpringOverResult result =
        algo.spring_over(polyline, 50, 0, {1}, 1, true, 20, nullptr);

    ASSERT_TRUE(result.changed);
    ASSERT_TRUE(result.polyline.has_value());
    // The wrapped polyline needs more lines than the input polyline and is
    // longer than the direct connection.
    EXPECT_GT(result.polyline->arr.size(), polyline.arr.size());
    EXPECT_GT(result.polyline->length_approx(), polyline.length_approx());
    // No corner of the result lies inside the obstacle trace anymore.
    const IntBox obstacle_box(4950, 1950, 5050, 8050);
    for (int i = 0; i < result.polyline->corner_count(); i++)
    {
        EXPECT_FALSE(is_strictly_inside(obstacle_box, result.polyline->corner(i)))
            << "corner " << i << " of the spring over result is inside the obstacle";
    }
}

TEST(ShoveTraceAlgoSpringOverTest, ZeroRecursionDepthFailsForTheObstacle)
{
    auto board = make_routing_board();
    ASSERT_NE(insert_shove_fixed_obstacle(board.get()), nullptr);
    ShoveTraceAlgo algo(board.get());
    const Polyline polyline = crossing_polyline();

    const ShoveTraceAlgo::SpringOverResult result =
        algo.spring_over(polyline, 50, 0, {1}, 1, true, 0, nullptr);

    EXPECT_FALSE(result.changed);
    EXPECT_FALSE(result.polyline.has_value());
}

TEST(ShoveTraceAlgoSpringOverTest, SpringOverObstaclesReturnsTheShorterWrap)
{
    auto board = make_routing_board();
    ASSERT_NE(insert_shove_fixed_obstacle(board.get()), nullptr);
    ShoveTraceAlgo algo(board.get());
    const Polyline polyline = crossing_polyline();
    const std::set<Pin*> contact_pins;

    const std::optional<Polyline> result =
        algo.spring_over_obstacles(polyline, 50, 0, {1}, 1, &contact_pins);

    ASSERT_TRUE(result.has_value());
    EXPECT_GE(result->arr.size(), polyline.arr.size());
    EXPECT_GT(result->length_approx(), polyline.length_approx());
    // The result must not be the identity of the input polyline.
    EXPECT_NE(result->arr.size(), polyline.arr.size());
}

TEST(ShoveTraceAlgoSpringOverTest, SpringOverObstaclesWithoutObstacleReturnsTheInputPolyline)
{
    auto board = make_routing_board();
    ASSERT_NE(insert_shove_fixed_obstacle(board.get()), nullptr);
    ShoveTraceAlgo algo(board.get());
    const Polyline polyline(std::vector<Line>{Line(IntPoint(1000, 9000), IntPoint(3000, 9000)),
                                              Line(IntPoint(3000, 9000), IntPoint(3000, 9500)),
                                              Line(IntPoint(3000, 9500), IntPoint(9000, 9500))});
    const std::set<Pin*> contact_pins;

    const std::optional<Polyline> result =
        algo.spring_over_obstacles(polyline, 50, 0, {1}, 1, &contact_pins);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->arr.size(), polyline.arr.size());
    EXPECT_EQ(result->length_approx(), polyline.length_approx());
}

TEST(ShoveTraceAlgoSpringOverTest, TiePinContactsAreReturnedAsIgnoreItems)
{
    auto board = make_routing_board();
    Pin* pin = insert_smd_pin(board.get(), IntPoint(5000, 5000), {1}, 100);
    ASSERT_NE(pin, nullptr);
    PolylineTrace* trace = insert_trace(board.get(), {IntPoint(5100, 5000), IntPoint(7000, 5000)},
                                        {1}, 50);
    ASSERT_NE(trace, nullptr);
    ShoveTraceAlgo algo(board.get());
    const geometry::planar::IntOctagon trace_shape = trace->get_tile_shape(0);
    ASSERT_FALSE(trace_shape.is_empty());

    const std::vector<Item*> ignore_items =
        algo.get_ignore_items_at_tie_pins(trace_shape, 0, {1});

    ASSERT_EQ(ignore_items.size(), 1u);
    EXPECT_EQ(ignore_items[0], static_cast<Item*>(trace));
}

TEST(ShoveTraceAlgoSpringOverTest, OtherNetPinsAreNoTiePins)
{
    auto board = make_routing_board();
    Pin* pin = insert_smd_pin(board.get(), IntPoint(5000, 5000), {2}, 100);
    ASSERT_NE(pin, nullptr);
    PolylineTrace* trace = insert_trace(board.get(), {IntPoint(5100, 5000), IntPoint(7000, 5000)},
                                        {1}, 50);
    ASSERT_NE(trace, nullptr);
    ShoveTraceAlgo algo(board.get());
    const geometry::planar::IntOctagon trace_shape = trace->get_tile_shape(0);
    ASSERT_FALSE(trace_shape.is_empty());

    const std::vector<Item*> ignore_items =
        algo.get_ignore_items_at_tie_pins(trace_shape, 0, {1});

    EXPECT_TRUE(ignore_items.empty());
    (void)pin;
}

/** Returns the traces of p_board. */
std::vector<PolylineTrace*> board_traces(RoutingBoard* p_board)
{
    std::vector<PolylineTrace*> result;
    for (Item* item : p_board->get_autoroute_items())
    {
        if (auto* trace = dynamic_cast<PolylineTrace*>(item))
        {
            result.push_back(trace);
        }
    }
    return result;
}

/** Returns true, if p_board contains p_item in its item list. */
bool board_contains(RoutingBoard* p_board, Item* p_item)
{
    const std::vector<Item*> items = p_board->get_autoroute_items();
    return std::find(items.begin(), items.end(), p_item) != items.end();
}

/**
 * Returns a line segment whose middle line runs from p_from to p_to. Java
 * LineSegment(Polyline, int) needs the neighbouring lines as well, so the
 * polyline is extended diagonally at both ends.
 */
geometry::planar::LineSegment horizontal_line_segment(const IntPoint& p_from, const IntPoint& p_to)
{
    const IntPoint before(p_from.x - 500, p_from.y + 500);
    const IntPoint after(p_to.x + 500, p_to.y + 500);
    const Polyline polyline(
        std::vector<Line>{Line(before, p_from), Line(p_from, p_to), Line(p_to, after)});
    return geometry::planar::LineSegment(polyline, 1);
}

TEST(ShoveTraceAlgoCheckTest, StaticCheckWithoutObstacleReturnsMaxInt)
{
    auto board = make_routing_board();
    ASSERT_NE(insert_shove_fixed_obstacle(board.get()), nullptr);
    const geometry::planar::LineSegment line_segment =
        horizontal_line_segment(IntPoint(1000, 9000), IntPoint(9000, 9000));

    const double result = ShoveTraceAlgo::check(board.get(), line_segment, true, 0, {1}, 50, 1, 10, 0);

    EXPECT_EQ(result, static_cast<double>(std::numeric_limits<int>::max()));
}

TEST(ShoveTraceAlgoCheckTest, StaticCheckOutsideTheBoardReturnsZero)
{
    auto board = make_routing_board();
    const geometry::planar::LineSegment line_segment =
        horizontal_line_segment(IntPoint(20000, 20000), IntPoint(30000, 20000));

    const double result = ShoveTraceAlgo::check(board.get(), line_segment, true, 0, {1}, 50, 1, 10, 0);

    EXPECT_EQ(result, 0.0);
}

TEST(ShoveTraceAlgoCheckTest, StaticCheckWithShoveFixedObstacleReturnsZero)
{
    auto board = make_routing_board();
    ASSERT_NE(insert_shove_fixed_obstacle(board.get()), nullptr);
    const geometry::planar::LineSegment line_segment =
        horizontal_line_segment(IntPoint(3000, 5000), IntPoint(7000, 5000));

    const double result = ShoveTraceAlgo::check(board.get(), line_segment, true, 0, {1}, 50, 1, 10, 0);

    EXPECT_EQ(result, 0.0);
}

TEST(ShoveTraceAlgoCheckTest, CheckWithoutObstacleReturnsTrue)
{
    auto board = make_routing_board();
    ASSERT_NE(insert_shove_fixed_obstacle(board.get()), nullptr);
    ShoveTraceAlgo algo(board.get());
    const geometry::planar::IntBox trace_shape(1000, 8500, 9000, 9500);
    CalcFromSide from_side(0, geometry::planar::FloatPoint(1000.0, 8500.0));

    EXPECT_TRUE(algo.check(trace_shape, &from_side, nullptr, 0, {1}, 1, 10, 0, 0, nullptr));
}

TEST(ShoveTraceAlgoCheckTest, CheckOfAnEmptyShapeReturnsTrue)
{
    auto board = make_routing_board();
    ShoveTraceAlgo algo(board.get());
    CalcFromSide from_side(0, geometry::planar::FloatPoint(0.0, 0.0));

    // Java TraceShover.check returns true for an empty shape.
    EXPECT_TRUE(algo.check(geometry::planar::IntBox::EMPTY, &from_side, nullptr, 0, {1}, 1, 10, 0, 0,
                           nullptr));
}

TEST(ShoveTraceAlgoCheckTest, CheckOutsideTheBoardFails)
{
    auto board = make_routing_board();
    ShoveTraceAlgo algo(board.get());
    const geometry::planar::IntBox trace_shape(20000, 20000, 30000, 30000);
    CalcFromSide from_side(0, geometry::planar::FloatPoint(20000.0, 20000.0));

    EXPECT_FALSE(algo.check(trace_shape, &from_side, nullptr, 0, {1}, 1, 10, 0, 0, nullptr));
}

TEST(ShoveTraceAlgoCheckTest, CheckWithShoveFixedObstacleReportsTheFailingObstacle)
{
    auto board = make_routing_board();
    PolylineTrace* obstacle = insert_shove_fixed_obstacle(board.get());
    ASSERT_NE(obstacle, nullptr);
    ShoveTraceAlgo algo(board.get());
    const geometry::planar::IntBox trace_shape(3000, 4000, 7000, 6000);
    CalcFromSide from_side(0, geometry::planar::FloatPoint(3000.0, 4000.0));

    EXPECT_FALSE(algo.check(trace_shape, &from_side, nullptr, 0, {1}, 1, 10, 0, 0, nullptr));
    EXPECT_EQ(board->get_shove_failing_obstacle(), static_cast<Item*>(obstacle));
}

TEST(ShoveTraceAlgoCheckTest, CheckWithoutRecursionDepthFailsOnAnObstacle)
{
    auto board = make_routing_board();
    ASSERT_NE(insert_trace(board.get(), {IntPoint(4000, 2000), IntPoint(4000, 8000)}, {2}, 50),
              nullptr);
    ShoveTraceAlgo algo(board.get());
    const geometry::planar::IntBox trace_shape(3000, 4000, 7000, 6000);
    CalcFromSide from_side(0, geometry::planar::FloatPoint(3000.0, 4000.0));

    EXPECT_FALSE(algo.check(trace_shape, &from_side, nullptr, 0, {1}, 1, 0, 0, 0, nullptr));
}

TEST(ShoveTraceAlgoInsertTest, InsertWithoutObstacleDoesNotChangeTheBoard)
{
    auto board = make_routing_board();
    ASSERT_NE(insert_shove_fixed_obstacle(board.get()), nullptr);
    const size_t item_count_before = board->get_autoroute_items().size();
    ShoveTraceAlgo algo(board.get());
    const geometry::planar::IntBox trace_shape(1000, 8500, 9000, 9500);
    CalcFromSide from_side(0, geometry::planar::FloatPoint(1000.0, 8500.0));

    EXPECT_TRUE(algo.insert(trace_shape, &from_side, 0, {1}, 1, {}, 10, 0, 0));
    EXPECT_EQ(board->get_autoroute_items().size(), item_count_before);
}

TEST(ShoveTraceAlgoInsertTest, InsertOfAnEmptyShapeDoesNotChangeTheBoard)
{
    auto board = make_routing_board();
    const size_t item_count_before = board->get_autoroute_items().size();
    ShoveTraceAlgo algo(board.get());
    CalcFromSide from_side(0, geometry::planar::FloatPoint(0.0, 0.0));

    EXPECT_TRUE(algo.insert(geometry::planar::IntBox::EMPTY, &from_side, 0, {1}, 1, {}, 10, 0, 0));
    EXPECT_EQ(board->get_autoroute_items().size(), item_count_before);
}

TEST(ShoveTraceAlgoInsertTest, InsertOutsideTheBoardFails)
{
    auto board = make_routing_board();
    ShoveTraceAlgo algo(board.get());
    const geometry::planar::IntBox trace_shape(20000, 20000, 30000, 30000);
    CalcFromSide from_side(0, geometry::planar::FloatPoint(20000.0, 20000.0));

    EXPECT_FALSE(algo.insert(trace_shape, &from_side, 0, {1}, 1, {}, 10, 0, 0));
}

/**
 * The obstacle trace crosses the new trace shape, so its cut out pieces end at
 * the cut points (4000, 6061) and (4000, 3939), which are the corners of the
 * offset shape of half width 50, clearance 10 and c_offset_add 1.
 */
TEST(ShapeTraceEntriesCutoutTest, CutoutReplacesTheCrossingTraceWithTwoPieces)
{
    auto board = make_routing_board();
    PolylineTrace* obstacle =
        insert_trace(board.get(), {IntPoint(4000, 2000), IntPoint(4000, 8000)}, {2}, 50);
    ASSERT_NE(obstacle, nullptr);
    const geometry::planar::IntBox trace_shape(3000, 4000, 7000, 6000);
    ShapeTraceEntries entries(trace_shape, 0, {1}, 1, nullptr, board.get());

    ASSERT_TRUE(entries.store_items({static_cast<Item*>(obstacle)}, false, true));
    EXPECT_EQ(entries.substitute_trace_count(), 1);
    EXPECT_EQ(entries.stack_depth(), 1);

    entries.cutout_traces({static_cast<Item*>(obstacle)});

    // The crossing trace is replaced by two pieces, which are on the board.
    EXPECT_FALSE(board_contains(board.get(), obstacle));
    EXPECT_FALSE(obstacle->is_on_board());
    const std::vector<PolylineTrace*> net2_traces = board_traces(board.get());
    ASSERT_EQ(net2_traces.size(), 2u);
    for (const PolylineTrace* trace : net2_traces)
    {
        EXPECT_TRUE(trace->shares_net_no({2}));
        EXPECT_TRUE(trace->is_on_board());
        // No corner of a cut out piece is inside the new trace shape.
        for (int i = 0; i < trace->polyline().corner_count(); i++)
        {
            EXPECT_FALSE(is_strictly_inside(trace_shape, trace->polyline().corner(i)));
        }
    }
}

TEST(ShapeTraceEntriesCutoutTest, CutoutOfATraceOutsideTheShapeKeepsTheTrace)
{
    auto board = make_routing_board();
    PolylineTrace* obstacle =
        insert_trace(board.get(), {IntPoint(4000, 2000), IntPoint(4000, 8000)}, {2}, 50);
    ASSERT_NE(obstacle, nullptr);
    const geometry::planar::IntBox trace_shape(1000, 1000, 2000, 2000);
    ShapeTraceEntries entries(trace_shape, 0, {1}, 1, nullptr, board.get());

    entries.cutout_traces({static_cast<Item*>(obstacle)});

    EXPECT_TRUE(board_contains(board.get(), obstacle));
    EXPECT_TRUE(obstacle->is_on_board());
    EXPECT_EQ(board_traces(board.get()).size(), 1u);
}

/**
 * The substitute trace piece wraps around the new trace shape along the border
 * of its offset shape, so its first corner is the entry point (4000, 6077) and
 * its last corner is (4000, 3923). The offset is the trace half width 50 plus
 * the clearance 10 plus the clearance safety margin 16 plus 1.
 * Verified against the Java baseline.
 */
TEST(ShapeTraceEntriesPieceTest, SubstitutePieceWrapsAroundTheShape)
{
    auto board = make_routing_board();
    PolylineTrace* obstacle =
        insert_trace(board.get(), {IntPoint(4000, 2000), IntPoint(4000, 8000)}, {2}, 50);
    ASSERT_NE(obstacle, nullptr);
    const geometry::planar::IntBox trace_shape(3000, 4000, 7000, 6000);
    ShapeTraceEntries entries(trace_shape, 0, {1}, 1, nullptr, board.get());

    ASSERT_TRUE(entries.store_items({static_cast<Item*>(obstacle)}, false, true));
    EXPECT_EQ(entries.substitute_trace_count(), 1);
    std::unique_ptr<PolylineTrace> piece(entries.next_substitute_trace_piece());

    ASSERT_NE(piece, nullptr);
    // trace line, 3 border lines, trace line
    EXPECT_EQ(piece->polyline().arr.size(), 5u);
    EXPECT_EQ(piece->get_net_nos(), std::vector<int>({2}));
    const std::vector<FloatPoint> expected_corners = {FloatPoint(4000.0, 6077.0),
                                                     FloatPoint(2923.0, 6077.0),
                                                     FloatPoint(2923.0, 3923.0),
                                                     FloatPoint(4000.0, 3923.0)};
    ASSERT_EQ(piece->polyline().corner_count(),
              static_cast<int>(expected_corners.size()));
    for (int i = 0; i < static_cast<int>(expected_corners.size()); ++i)
    {
        EXPECT_EQ(piece->polyline().corner(i).to_float().x, expected_corners[i].x);
        EXPECT_EQ(piece->polyline().corner(i).to_float().y, expected_corners[i].y);
    }
    // The piece is not on the board, the caller inserts it after the recursion.
    EXPECT_FALSE(piece->is_on_board());
    EXPECT_EQ(entries.next_substitute_trace_piece(), nullptr);
}

/**
 * A crossing foreign trace is cut out of the new trace shape and the cut pieces
 * are joined around the shape, so that the net of the obstacle is preserved.
 * Verified against the Java baseline: TraceShover.insert() returns true and one
 * net 2 trace with the corners listed below remains on the board, because
 * ShapeTraceEntries.calculate_stack_levels() ends at level 1.
 */
TEST(ShoveTraceAlgoInsertTest, InsertJoinsACrossingForeignTraceAroundTheShape)
{
    auto board = make_routing_board();
    PolylineTrace* obstacle =
        insert_trace(board.get(), {IntPoint(4000, 2000), IntPoint(4000, 8000)}, {2}, 50);
    ASSERT_NE(obstacle, nullptr);
    const geometry::planar::IntBox trace_shape(3000, 4000, 7000, 6000);
    CalcFromSide from_side(0, geometry::planar::FloatPoint(3000.0, 4000.0));
    ShoveTraceAlgo algo(board.get());

    EXPECT_TRUE(algo.insert(trace_shape, &from_side, 0, {1}, 1, {}, 10, 0, 0));

    // The original obstacle trace is cut out and replaced by a joined trace.
    EXPECT_FALSE(board_contains(board.get(), obstacle));
    EXPECT_EQ(board->get_shove_failing_obstacle(), nullptr);
    const std::vector<PolylineTrace*> net2_traces = board_traces(board.get());
    ASSERT_EQ(net2_traces.size(), 1u);
    const PolylineTrace* joined = net2_traces[0];
    EXPECT_TRUE(joined->shares_net_no({2}));
    EXPECT_TRUE(joined->is_on_board());
    const std::vector<IntPoint> expected_corners = {IntPoint(4000, 2000), IntPoint(4000, 3923),
                                                   IntPoint(7077, 3923), IntPoint(7077, 6077),
                                                   IntPoint(4000, 6077), IntPoint(4000, 8000)};
    ASSERT_EQ(joined->polyline().corner_count(), expected_corners.size());
    for (size_t i = 0; i < expected_corners.size(); ++i)
    {
        EXPECT_EQ(joined->polyline().corner(static_cast<int>(i)), expected_corners[i]);
    }
}

/**
 * A via of a foreign net inside the new trace shape is collected in
 * ShapeTraceEntries.shove_via_list. Without a via recursion depth
 * MoveDrillItemAlgo::shove_vias gives up without moving it, so the insert fails
 * with the via as the failing obstacle. This mirrors Java TraceShover.insert().
 */
TEST(ShoveTraceAlgoInsertTest, InsertFailsWhenAViaCannotBeShoved)
{
    auto board = make_routing_board();
    auto* padstack = new Padstack("v1");
    padstack->set_from_layer(0);
    padstack->set_to_layer(1);
    padstack->add_shape(0, new PolygonShape(std::vector<IntPoint>{
                                              {-100, -100},
                                              {100, -100},
                                              {100, 100},
                                              {-100, 100}}));
    auto* via = new Via(padstack, IntPoint(4000, 5000), {2}, 1, board->generate_item_id(), 0,
                        FixedState::NOT_FIXED, true, board.get());
    via->set_bounding_box(IntBox(3900, 4900, 4100, 5100));
    board->get_search_tree_manager()->insert(via);
    board->add_item(via);
    const geometry::planar::IntBox trace_shape(3000, 4000, 7000, 6000);
    CalcFromSide from_side(0, geometry::planar::FloatPoint(3000.0, 4000.0));
    ShoveTraceAlgo algo(board.get());

    EXPECT_FALSE(algo.insert(trace_shape, &from_side, 0, {1}, 1, {}, 10, 0, 0));
    EXPECT_EQ(board->get_shove_failing_obstacle(), static_cast<Item*>(via));
    EXPECT_EQ(via->get_center().x, 4000);
    EXPECT_EQ(via->get_center().y, 5000);
}

TEST(ShoveTraceAlgoInsertTest, InsertWithAShoveFixedObstacleFails)
{
    auto board = make_routing_board();
    ASSERT_NE(insert_shove_fixed_obstacle(board.get()), nullptr);
    const geometry::planar::IntBox trace_shape(3000, 4000, 7000, 6000);
    CalcFromSide from_side(0, geometry::planar::FloatPoint(3000.0, 4000.0));
    ShoveTraceAlgo algo(board.get());

    EXPECT_FALSE(algo.insert(trace_shape, &from_side, 0, {1}, 1, {}, 10, 0, 0));
}

} // namespace
} // namespace board
