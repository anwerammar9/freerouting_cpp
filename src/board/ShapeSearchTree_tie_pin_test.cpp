// Tests for the tie-pin shape reduction port of Java
// ShapeSearchTree.reduceTraceShapeAtTiePin(Pin, PolylineTrace) and
// ShapeSearchTree.changeItemShape(Item, int, TileShape).

#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include "board/Layer.h"
#include "board/Padstack.h"
#include "board/Pin.h"
#include "board/PolylineTrace.h"
#include "board/RoutingBoard.h"
#include "board/SearchTreeManager.h"
#include "board/ShapeSearchTree.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/Point.h"
#include "geometry/planar/PolygonShape.h"
#include "geometry/planar/Polyline.h"
#include "geometry/planar/Simplex.h"
#include "geometry/planar/TileShape.h"
#include "rules/BoardRules.h"

namespace freerouting::board {
namespace {

namespace fp = freerouting::geometry::planar;

std::shared_ptr<RoutingBoard> make_routing_board()
{
    std::vector<std::shared_ptr<Layer>> layers;
    layers.push_back(std::make_shared<Layer>("Top", true));
    layers.push_back(std::make_shared<Layer>("Bottom", true));
    LayerStructure layer_structure(layers);
    auto board_rules = std::make_unique<rules::BoardRules>(layer_structure, 10);
    return std::make_shared<RoutingBoard>(fp::IntBox(0, 0, 10000, 10000), layer_structure,
        std::move(board_rules), nullptr);
}

PolylineTrace* insert_trace(RoutingBoard* p_board, const std::vector<fp::IntPoint>& p_corners,
    const std::vector<int>& p_nets, int p_layer = 0)
{
    auto polyline = std::make_shared<fp::Polyline>(fp::Polyline::from_corners(p_corners));
    return p_board->insert_trace_without_cleaning(polyline, p_layer, 50, p_nets, 1,
        FixedState::NOT_FIXED);
}

Pin* insert_smd_pin(RoutingBoard* p_board, const fp::IntPoint& p_center,
    const std::vector<int>& p_nets, int p_pad_half_width)
{
    auto* pin = new Pin(7, 1, p_nets, 1, p_board->generate_item_id(), FixedState::NOT_FIXED,
        p_board);
    pin->set_center(p_center);
    auto* padstack = new Padstack("p1");
    padstack->set_from_layer(0);
    padstack->set_to_layer(0);
    padstack->add_shape(0, new fp::PolygonShape(
        std::vector<fp::IntPoint>{
            fp::IntPoint(-p_pad_half_width, -p_pad_half_width),
            fp::IntPoint(p_pad_half_width, -p_pad_half_width),
            fp::IntPoint(p_pad_half_width, p_pad_half_width),
            fp::IntPoint(-p_pad_half_width, p_pad_half_width)}));
    pin->set_padstack(padstack);
    pin->set_bounding_box(fp::IntBox(p_center.x - p_pad_half_width, p_center.y - p_pad_half_width,
        p_center.x + p_pad_half_width, p_center.y + p_pad_half_width));
    p_board->get_search_tree_manager()->insert(pin);
    p_board->add_item(pin);
    return pin;
}

/** True, if the tree contains an object overlapping with p_box on layer 0. */
bool tree_contains_box(ShapeSearchTree& p_tree, const fp::IntBox& p_box)
{
    const std::set<SearchTreeObject*, TreeStorableLess> objects = p_tree.overlapping_objects(p_box, 0);
    return !objects.empty();
}

} // namespace

// ==========================================================================
// change_item_shape
// ==========================================================================

TEST(ShapeSearchTreeTiePinTest, ChangeItemShapeReplacesTheShapeAndKeepsTheTreeConsistent)
{
    auto board = make_routing_board();
    PolylineTrace* trace = insert_trace(board.get(),
        { fp::IntPoint(2000, 2000), fp::IntPoint(6000, 2000) }, { 2 });
    ASSERT_NE(trace, nullptr);
    ShapeSearchTree tree(board.get(), 0);
    tree.insert(trace);

    const fp::TileShape* old_shape = trace->get_tree_shape(tree, 0);
    ASSERT_NE(old_shape, nullptr);
    EXPECT_TRUE(tree_contains_box(tree, fp::IntBox(4800, 1800, 5200, 2200)));

    auto new_shape = std::make_shared<fp::IntBox>(2000, 1950, 3000, 2050);
    tree.change_item_shape(trace, 0, new_shape);

    // The precalculated shape is replaced...
    EXPECT_EQ(trace->get_tree_shape(tree, 0), new_shape.get());
    // ... the tree keeps exactly one leaf for the trace ...
    EXPECT_EQ(static_cast<int>(tree.to_array().size()), tree.size());
    EXPECT_EQ(tree.size(), 1);
    EXPECT_TRUE(tree.validate_entries(trace));
    // ... and the leaf now covers only the new shape.
    EXPECT_FALSE(tree_contains_box(tree, fp::IntBox(4800, 1800, 5200, 2200)));
    EXPECT_TRUE(tree_contains_box(tree, fp::IntBox(2200, 1900, 2600, 2100)));
}

TEST(ShapeSearchTreeTiePinTest, ChangeItemShapeWithOutOfRangeIndexKeepsTheItemUntouched)
{
    auto board = make_routing_board();
    PolylineTrace* trace = insert_trace(board.get(),
        { fp::IntPoint(2000, 2000), fp::IntPoint(6000, 2000) }, { 2 });
    ASSERT_NE(trace, nullptr);
    ShapeSearchTree tree(board.get(), 0);
    tree.insert(trace);

    const fp::TileShape* old_shape = trace->get_tree_shape(tree, 0);
    auto new_shape = std::make_shared<fp::IntBox>(2000, 1950, 3000, 2050);
    tree.change_item_shape(trace, 5, new_shape);

    EXPECT_EQ(trace->get_tree_shape(tree, 0), old_shape);
    EXPECT_EQ(tree.size(), 1);
    EXPECT_TRUE(tree.validate_entries(trace));
    EXPECT_TRUE(tree_contains_box(tree, fp::IntBox(4800, 1800, 5200, 2200)));
}

// ==========================================================================
// reduce_trace_shape_at_tie_pin
// ==========================================================================

TEST(ShapeSearchTreeTiePinTest, ReduceCutsThePinOutOfTheFirstTraceShape)
{
    auto board = make_routing_board();
    // A tie pin of the nets 1 and 2.
    Pin* pin = insert_smd_pin(board.get(), fp::IntPoint(2000, 2000), { 1, 2 }, 300);
    // A foreign (net 2) trace starting at the pin center.
    PolylineTrace* trace = insert_trace(board.get(),
        { fp::IntPoint(2000, 2000), fp::IntPoint(6000, 2000) }, { 2 });
    ASSERT_NE(pin, nullptr);
    ASSERT_NE(trace, nullptr);
    ShapeSearchTree tree(board.get(), 0);
    tree.insert(pin);
    tree.insert(trace);

    const fp::TileShape* old_shape = trace->get_tree_shape(tree, 0);
    const fp::TileShape* pin_shape = pin->get_tree_shape_on_layer(&tree, 0);
    ASSERT_NE(old_shape, nullptr);
    ASSERT_NE(pin_shape, nullptr);
    ASSERT_GE(fp::TileShape::intersection_of(*old_shape, *pin_shape)->dimension(), 2);

    tree.reduce_trace_shape_at_tie_pin(pin, trace);

    const fp::TileShape* new_shape = trace->get_tree_shape(tree, 0);
    ASSERT_NE(new_shape, nullptr);
    EXPECT_NE(new_shape, old_shape);
    // The pin interior is cut out of the trace shape ...
    EXPECT_FALSE(new_shape->contains(fp::IntPoint(2000, 2000)));
    EXPECT_LT(fp::TileShape::intersection_of(*new_shape, *pin_shape)->dimension(), 2);
    // ... while the piece containing the far corner of the trace survives.
    EXPECT_TRUE(new_shape->contains(fp::FloatPoint(6000.0, 2000.0)));
    EXPECT_EQ(tree.size(), 2);
    EXPECT_TRUE(tree.validate_entries(trace));
}

TEST(ShapeSearchTreeTiePinTest, ReduceCutsThePinOutOfTheLastTraceShape)
{
    auto board = make_routing_board();
    Pin* pin = insert_smd_pin(board.get(), fp::IntPoint(2000, 2000), { 1, 2 }, 300);
    // Three corners: only the last corner touches the pin, so the second tile
    // shape (index corner_count - 2 = 1) is the one to reduce.
    PolylineTrace* trace = insert_trace(board.get(),
        { fp::IntPoint(6000, 2000), fp::IntPoint(4000, 5000), fp::IntPoint(2000, 2000) }, { 2 });
    ASSERT_NE(pin, nullptr);
    ASSERT_NE(trace, nullptr);
    ASSERT_EQ(trace->corner_count(), 3);
    ShapeSearchTree tree(board.get(), 0);
    tree.insert(pin);
    tree.insert(trace);

    const fp::TileShape* first_shape_before = trace->get_tree_shape(tree, 0);
    const fp::TileShape* last_shape_before = trace->get_tree_shape(tree, 1);
    const fp::TileShape* pin_shape = pin->get_tree_shape_on_layer(&tree, 0);
    ASSERT_NE(last_shape_before, nullptr);
    ASSERT_NE(pin_shape, nullptr);
    ASSERT_GE(fp::TileShape::intersection_of(*last_shape_before, *pin_shape)->dimension(), 2);

    tree.reduce_trace_shape_at_tie_pin(pin, trace);

    const fp::TileShape* new_last_shape = trace->get_tree_shape(tree, 1);
    EXPECT_NE(new_last_shape, last_shape_before);
    EXPECT_FALSE(new_last_shape->contains(fp::IntPoint(2000, 2000)));
    EXPECT_LT(fp::TileShape::intersection_of(*new_last_shape, *pin_shape)->dimension(), 2);
    // The reduced piece contains the middle corner (the compare corner of the
    // Java code) ...
    EXPECT_TRUE(new_last_shape->contains(fp::FloatPoint(4000.0, 5000.0)));
    // ... and the first shape of the trace is untouched.
    EXPECT_EQ(trace->get_tree_shape(tree, 0), first_shape_before);
    EXPECT_TRUE(tree.validate_entries(trace));
}

TEST(ShapeSearchTreeTiePinTest, ReduceKeepsTheTraceUntouchedWhenNoCornerIsAtThePin)
{
    auto board = make_routing_board();
    Pin* pin = insert_smd_pin(board.get(), fp::IntPoint(2000, 2000), { 1, 2 }, 300);
    PolylineTrace* trace = insert_trace(board.get(),
        { fp::IntPoint(4000, 4000), fp::IntPoint(6000, 4000) }, { 2 });
    ASSERT_NE(pin, nullptr);
    ASSERT_NE(trace, nullptr);
    ShapeSearchTree tree(board.get(), 0);
    tree.insert(pin);
    tree.insert(trace);

    const fp::TileShape* old_shape = trace->get_tree_shape(tree, 0);
    tree.reduce_trace_shape_at_tie_pin(pin, trace);

    EXPECT_EQ(trace->get_tree_shape(tree, 0), old_shape);
    EXPECT_TRUE(tree.validate_entries(trace));
}

TEST(ShapeSearchTreeTiePinTest, ReduceKeepsTheTraceUntouchedWhenThePinHasNoShapeOnTheLayer)
{
    auto board = make_routing_board();
    Pin* pin = insert_smd_pin(board.get(), fp::IntPoint(2000, 2000), { 1, 2 }, 300);
    // The pin exists on layer 0 only, the trace runs on layer 1. Java would
    // throw a NullPointerException here; the port skips the reduction.
    PolylineTrace* trace = insert_trace(board.get(),
        { fp::IntPoint(2000, 2000), fp::IntPoint(6000, 2000) }, { 2 }, 1);
    ASSERT_NE(pin, nullptr);
    ASSERT_NE(trace, nullptr);
    ShapeSearchTree tree(board.get(), 0);
    tree.insert(pin);
    tree.insert(trace);
    ASSERT_EQ(pin->get_tree_shape_on_layer(&tree, 1), nullptr);

    const fp::TileShape* old_shape = trace->get_tree_shape(tree, 0);
    tree.reduce_trace_shape_at_tie_pin(pin, trace);

    EXPECT_EQ(trace->get_tree_shape(tree, 0), old_shape);
    EXPECT_TRUE(tree.validate_entries(trace));
}

TEST(ShapeSearchTreeTiePinTest, ReduceRejectsNullArguments)
{
    auto board = make_routing_board();
    Pin* pin = insert_smd_pin(board.get(), fp::IntPoint(2000, 2000), { 1, 2 }, 300);
    PolylineTrace* trace = insert_trace(board.get(),
        { fp::IntPoint(2000, 2000), fp::IntPoint(6000, 2000) }, { 2 });
    ASSERT_NE(pin, nullptr);
    ASSERT_NE(trace, nullptr);
    ShapeSearchTree tree(board.get(), 0);
    tree.insert(trace);

    const fp::TileShape* old_shape = trace->get_tree_shape(tree, 0);
    tree.reduce_trace_shape_at_tie_pin(nullptr, trace);
    tree.reduce_trace_shape_at_tie_pin(pin, nullptr);

    EXPECT_EQ(trace->get_tree_shape(tree, 0), old_shape);
    EXPECT_EQ(tree.size(), 1);
}

} // namespace freerouting::board
