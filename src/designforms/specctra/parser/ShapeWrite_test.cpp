// Tests for the DSN shape write halves (Java parser.Shape/Rectangle/Circle/
// Polygon/Path writeScope + writeScopeInt + writeHoleScope). The golden strings
// are the exact output of the Java baseline (JDK 25) for the same shapes.

#include "designforms/specctra/parser/Shape.h"

#include <gtest/gtest.h>

#include <array>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "datastructures/IdentifierType.h"
#include "datastructures/IndentFileWriter.h"
#include "designforms/specctra/parser/Circle.h"
#include "designforms/specctra/parser/Layer.h"
#include "designforms/specctra/parser/Path.h"
#include "designforms/specctra/parser/Polygon.h"
#include "designforms/specctra/parser/Rectangle.h"
#include "designforms/specctra/parser/SpecctraDsnStreamReader.h"

namespace freerouting::designforms::specctra::parser {
namespace {

namespace ds = freerouting::datastructures;

std::shared_ptr<Layer> front_layer()
{
    return std::make_shared<Layer>("F.Cu", 0, true);
}

LayerStructure front_layer_structure()
{
    return LayerStructure({ front_layer() });
}

std::string write_shape(const Shape& p_shape, bool p_integer)
{
    std::ostringstream stream;
    ds::IndentFileWriter file(stream);
    const ds::IdentifierType identifier({ "(", ")", " ", ";", "-", "_" }, "\"");
    if (p_integer)
    {
        p_shape.write_scope_int(file, identifier);
    }
    else
    {
        p_shape.write_scope(file, identifier);
    }
    return stream.str();
}

std::string write_hole(const Shape& p_shape)
{
    std::ostringstream stream;
    ds::IndentFileWriter file(stream);
    const ds::IdentifierType identifier({ "(", ")", " ", ";", "-", "_" }, "\"");
    p_shape.write_hole_scope(file, identifier);
    return stream.str();
}

TEST(ShapeWriteTest, RectangleMatchesJava)
{
    const Rectangle rectangle(front_layer(), std::array<double, 4>{ 0, -12.5, 100.0, 200.25 });
    EXPECT_EQ("\n(rect F.Cu 0.0 -12.5 100.0 200.25)", write_shape(rectangle, false));
    EXPECT_EQ("\n(rect F.Cu 0 -12 100 200)", write_shape(rectangle, true));
}

TEST(ShapeWriteTest, CircleMatchesJava)
{
    const Circle circle(front_layer(), std::array<double, 3>{ 100.0, 5.5, 6.25 });
    EXPECT_EQ("\n(circle F.Cu 100.0 5.5 6.25)", write_shape(circle, false));
    EXPECT_EQ("\n(circle F.Cu 100 6 6)", write_shape(circle, true));
}

TEST(ShapeWriteTest, PolygonMatchesJava)
{
    const Polygon polygon(front_layer(), std::vector<double>{ 0, 0, 100, 0, 100.5, 100, 0, 100 });
    EXPECT_EQ("\n(polygon F.Cu 0\n  0.0 0.0\n  100.0 0.0\n  100.5 100.0\n  0.0 100.0\n)",
        write_shape(polygon, false));
    EXPECT_EQ("\n(polygon F.Cu 0\n  0 0\n  100 0\n  101 100\n  0 100\n)", write_shape(polygon, true));
}

TEST(ShapeWriteTest, PolygonPathMatchesJava)
{
    const PolygonPath path(front_layer(), 20.0, std::vector<double>{ 0, 0, 100, 0, 100, 100 });
    EXPECT_EQ("\n(path F.Cu 20.0\n  0.0 0.0\n  100.0 0.0\n  100.0 100.0\n)", write_shape(path, false));
    // Java parity: the width stays unrounded in the integer variant.
    EXPECT_EQ("\n(path F.Cu 20.0\n  0 0\n  100 0\n  100 100\n)", write_shape(path, true));
}

TEST(ShapeWriteTest, PolylinePathMatchesJava)
{
    const PolylinePath path(front_layer(), 20.0, std::vector<double>{ 0, 0, 100, 0, 100, 0, 100, 100 });
    EXPECT_EQ("\n(polyline_path F.Cu 20.0\n  0.0 0.0 100.0 0.0 \n  100.0 0.0 100.0 100.0 \n)",
        write_shape(path, false));
    EXPECT_EQ("\n(polyline_path F.Cu 20.0\n  0 0 100 0 \n  100 0 100 100 \n)", write_shape(path, true));
}

TEST(ShapeWriteTest, HoleScopeMatchesJava)
{
    const Rectangle rectangle(front_layer(), std::array<double, 4>{ 0, -12.5, 100.0, 200.25 });
    EXPECT_EQ("\n(window\n  (rect F.Cu 0.0 -12.5 100.0 200.25)\n)", write_hole(rectangle));
}

TEST(ShapeWriteTest, RectangleRoundTrip)
{
    const Rectangle original(front_layer(), std::array<double, 4>{ 0, -12.5, 100.0, 200.25 });
    const LayerStructure layers = front_layer_structure();
    SpecctraDsnStreamReader scanner(write_shape(original, false), true);
    const std::unique_ptr<Shape> reread = Shape::read_scope(scanner, &layers);
    const auto* rectangle = dynamic_cast<const Rectangle*>(reread.get());
    ASSERT_NE(nullptr, rectangle);
    EXPECT_EQ(original.coor, rectangle->coor);
}

TEST(ShapeWriteTest, CircleRoundTrip)
{
    const Circle original(front_layer(), std::array<double, 3>{ 100.0, 5.5, 6.25 });
    const LayerStructure layers = front_layer_structure();
    SpecctraDsnStreamReader scanner(write_shape(original, false), true);
    const std::unique_ptr<Shape> reread = Shape::read_scope(scanner, &layers);
    const auto* circle = dynamic_cast<const Circle*>(reread.get());
    ASSERT_NE(nullptr, circle);
    EXPECT_EQ(original.coor, circle->coor);
}

TEST(ShapeWriteTest, PolygonRoundTrip)
{
    const Polygon original(front_layer(), std::vector<double>{ 0, 0, 100, 0, 100.5, 100, 0, 100 });
    const LayerStructure layers = front_layer_structure();
    SpecctraDsnStreamReader scanner(write_shape(original, false), true);
    const std::unique_ptr<Shape> reread = Shape::read_scope(scanner, &layers);
    const auto* polygon = dynamic_cast<const Polygon*>(reread.get());
    ASSERT_NE(nullptr, polygon);
    EXPECT_EQ(original.coor, polygon->coor);
}

TEST(ShapeWriteTest, PolygonPathRoundTrip)
{
    const PolygonPath original(front_layer(), 20.0, std::vector<double>{ 0, 0, 100, 0, 100, 100 });
    const LayerStructure layers = front_layer_structure();
    SpecctraDsnStreamReader scanner(write_shape(original, false), true);
    const std::unique_ptr<Shape> reread = Shape::read_scope(scanner, &layers);
    const auto* path = dynamic_cast<const PolygonPath*>(reread.get());
    ASSERT_NE(nullptr, path);
    EXPECT_EQ(original.width, path->width);
    EXPECT_EQ(original.coordinate_arr, path->coordinate_arr);
}

TEST(ShapeWriteTest, PolylinePathRoundTrip)
{
    const PolylinePath original(front_layer(), 20.0, std::vector<double>{ 0, 0, 100, 0, 100, 0, 100, 100 });
    const LayerStructure layers = front_layer_structure();
    SpecctraDsnStreamReader scanner(write_shape(original, false), true);
    const std::unique_ptr<Shape> reread = Shape::read_scope(scanner, &layers);
    const auto* path = dynamic_cast<const PolylinePath*>(reread.get());
    ASSERT_NE(nullptr, path);
    EXPECT_EQ(original.width, path->width);
    EXPECT_EQ(original.coordinate_arr, path->coordinate_arr);
}

} // namespace
} // namespace freerouting::designforms::specctra::parser
