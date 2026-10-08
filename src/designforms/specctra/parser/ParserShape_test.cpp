// Tests for the DSN shape readers (Java parser.Shape, Rectangle, Circle,
// Polygon, PolygonPath) and the resolution scope.

#include "designforms/specctra/parser/Shape.h"

#include <gtest/gtest.h>

#include "designforms/specctra/parser/Circle.h"
#include "designforms/specctra/parser/CoordinateTransform.h"
#include "designforms/specctra/parser/Keyword.h"
#include "designforms/specctra/parser/Path.h"
#include "designforms/specctra/parser/Polygon.h"
#include "designforms/specctra/parser/ReadScopeParameter.h"
#include "designforms/specctra/parser/Rectangle.h"
#include "designforms/specctra/parser/ScopeKeyword.h"
#include "designforms/specctra/parser/SpecctraDsnStreamReader.h"
#include "geometry/planar/ConvexTileShape.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/IntOctagon.h"
#include "geometry/planar/PolygonShape.h"
#include "geometry/planar/PolylineArea.h"

namespace freerouting::designforms::specctra::parser {
namespace {

namespace fp = geometry::planar;

LayerStructure two_layers()
{
    return LayerStructure({ std::make_shared<Layer>("F.Cu", 0, true), std::make_shared<Layer>("B.Cu", 1, true) });
}

TEST(ParserShapeTest, RectangleIsReadAndScaledToAnIntBox)
{
    const LayerStructure layers = two_layers();
    SpecctraDsnStreamReader scanner("(rect B.Cu 10 20 -5.5 4)", true);
    std::unique_ptr<Shape> shape = Shape::read_scope(scanner, &layers);
    auto* rect = dynamic_cast<Rectangle*>(shape.get());
    ASSERT_NE(nullptr, rect);
    EXPECT_EQ("B.Cu", rect->layer->name);
    const CoordinateTransform transform(10, 0, 0);
    const std::unique_ptr<fp::Shape> board = rect->transform_to_board(transform);
    const auto* tile = dynamic_cast<const fp::ConvexTileShape*>(board.get());
    ASSERT_NE(nullptr, tile);
    ASSERT_NE(nullptr, dynamic_cast<const fp::IntBox*>(&tile->tile()));
    EXPECT_EQ(fp::IntPoint(-55, 40), board->bounding_box().ll);
    EXPECT_EQ(fp::IntPoint(100, 200), board->bounding_box().ur);
}

TEST(ParserShapeTest, RectangleOnUnknownLayerFallsBackToSignal)
{
    const LayerStructure layers = two_layers();
    SpecctraDsnStreamReader scanner("(rect Inner7 0 0 1 1)", true);
    std::unique_ptr<Shape> shape = Shape::read_scope(scanner, &layers);
    ASSERT_NE(nullptr, shape);
    EXPECT_EQ(Layer::signal(), shape->layer);
}

TEST(ParserShapeTest, LayerNamesContainingTopOrBottomMapToTheOuterLayers)
{
    const LayerStructure layers = two_layers();
    EXPECT_EQ(0, layers.get_no("Top Layer"));
    EXPECT_EQ(1, layers.get_no("Bottom Layer"));
    EXPECT_EQ(-1, layers.get_no("Mid"));
}

TEST(ParserShapeTest, CircleDiameterBecomesTheRadius)
{
    SpecctraDsnStreamReader scanner("(circle signal 100 5 6)", true);
    std::unique_ptr<Shape> shape = Shape::read_scope(scanner, nullptr);
    auto* circle = dynamic_cast<Circle*>(shape.get());
    ASSERT_NE(nullptr, circle);
    const std::unique_ptr<fp::Shape> board = circle->transform_to_board_rel(CoordinateTransform(10, 0, 0));
    const auto* board_circle = dynamic_cast<const fp::Circle*>(board.get());
    ASSERT_NE(nullptr, board_circle);
    EXPECT_EQ(500, board_circle->radius);
    EXPECT_EQ(fp::IntPoint(50, 60), board_circle->center);
}

TEST(ParserShapeTest, PolygonSkipsTheApertureWidth)
{
    const LayerStructure layers = two_layers();
    SpecctraDsnStreamReader scanner("(polygon F.Cu 0 0 0 100 0 100 100)", true);
    std::unique_ptr<Shape> shape = Shape::read_scope(scanner, &layers);
    auto* polygon = dynamic_cast<Polygon*>(shape.get());
    ASSERT_NE(nullptr, polygon);
    ASSERT_EQ(6u, polygon->coor.size());
    const std::unique_ptr<fp::Shape> board = polygon->transform_to_board(CoordinateTransform(1, 0, 0));
    ASSERT_NE(nullptr, dynamic_cast<const fp::PolygonShape*>(board.get()));
    EXPECT_EQ(fp::IntPoint(100, 100), board->bounding_box().ur);
}

TEST(ParserShapeTest, TwoPointPathBecomesAnEnlargedOctagon)
{
    const LayerStructure layers = two_layers();
    SpecctraDsnStreamReader scanner("(path F.Cu 20 0 0 100 0)", true);
    std::unique_ptr<Shape> shape = Shape::read_scope(scanner, &layers);
    auto* path = dynamic_cast<PolygonPath*>(shape.get());
    ASSERT_NE(nullptr, path);
    EXPECT_EQ(20, path->width);
    const std::unique_ptr<fp::Shape> board = path->transform_to_board(CoordinateTransform(1, 0, 0));
    const auto* tile = dynamic_cast<const fp::ConvexTileShape*>(board.get());
    ASSERT_NE(nullptr, tile);
    ASSERT_NE(nullptr, dynamic_cast<const fp::IntOctagon*>(&tile->tile()));
    EXPECT_EQ(fp::IntPoint(-10, -10), board->bounding_box().ll);
    EXPECT_EQ(fp::IntPoint(110, 10), board->bounding_box().ur);
}

TEST(ParserShapeTest, SinglePointPathIsSkipped)
{
    const LayerStructure layers = two_layers();
    SpecctraDsnStreamReader scanner("(path F.Cu 20 5 5)", true);
    EXPECT_EQ(nullptr, Shape::read_scope(scanner, &layers));
}

TEST(ParserShapeTest, AreaWithWindowBecomesAPolylineArea)
{
    const LayerStructure layers = two_layers();
    SpecctraDsnStreamReader scanner(
        "keepout1 (rect F.Cu 0 0 100 100) (window (rect F.Cu 40 40 60 60)) (clearance_class wide))", true);
    std::optional<Shape::ReadAreaScopeResult> area = Shape::read_area_scope(scanner, &layers, false);
    ASSERT_TRUE(area.has_value());
    EXPECT_EQ("keepout1", area->area_name);
    EXPECT_EQ("wide", area->clearance_class_name);
    ASSERT_EQ(2u, area->shape_list.size());
    const std::unique_ptr<fp::Shape> board = Shape::transform_area_to_board(area->shape_list, CoordinateTransform(1, 0, 0));
    ASSERT_NE(nullptr, dynamic_cast<const fp::PolylineArea*>(board.get()));
    EXPECT_FALSE(board->contains(fp::IntPoint(50, 50)));
    EXPECT_TRUE(board->contains(fp::IntPoint(10, 10)));
}

TEST(ParserShapeTest, UnknownShapeKeywordIsSkipped)
{
    SpecctraDsnStreamReader scanner("(qarc signal 1 2 3) next", true);
    EXPECT_EQ(nullptr, Shape::read_scope(scanner, nullptr));
    EXPECT_EQ("next", scanner.next_token().string_value);
}

TEST(ParserResolutionTest, ReadsUnitAndResolution)
{
    SpecctraDsnStreamReader scanner("um 10)", true);
    ReadScopeParameter par(scanner);
    ASSERT_TRUE(Keyword::RESOLUTION_SCOPE->read_scope(par));
    EXPECT_EQ(board::Unit::UM, par.unit);
    EXPECT_EQ(10, par.resolution);
}

TEST(ParserResolutionTest, UnknownUnitFails)
{
    SpecctraDsnStreamReader scanner("furlong 10)", true);
    ReadScopeParameter par(scanner);
    EXPECT_FALSE(Keyword::RESOLUTION_SCOPE->read_scope(par));
}

} // namespace
} // namespace freerouting::designforms::specctra::parser
