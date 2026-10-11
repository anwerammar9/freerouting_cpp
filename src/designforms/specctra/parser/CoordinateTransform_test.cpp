// Tests for the board<->DSN CoordinateTransform (Java io.CoordinateTransform).
// Golden strings/values come from the Java baseline (JDK 25): scale 10, base
// (5, -7).

#include "designforms/specctra/parser/CoordinateTransform.h"

#include <gtest/gtest.h>

#include <array>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "datastructures/IdentifierType.h"
#include "datastructures/IndentFileWriter.h"
#include "designforms/specctra/parser/Layer.h"
#include "designforms/specctra/parser/Rectangle.h"
#include "designforms/specctra/parser/Shape.h"
#include "geometry/planar/ConvexTileShape.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/Point.h"
#include "geometry/planar/PolygonShape.h"
#include "geometry/planar/Shape.h"
#include "geometry/planar/Vector.h"

namespace freerouting::designforms::specctra::parser {
namespace {

namespace fp = freerouting::geometry::planar;

const CoordinateTransform& transform()
{
    static const CoordinateTransform instance(10.0, 5.0, -7.0);
    return instance;
}

std::shared_ptr<Layer> front_layer()
{
    return std::make_shared<Layer>("F.Cu", 0, true);
}

std::string write_shape(const Shape& p_shape)
{
    std::ostringstream stream;
    datastructures::IndentFileWriter file(stream);
    const datastructures::IdentifierType identifier({ "(", ")", " ", ";", "-", "_" }, "\"");
    p_shape.write_scope(file, identifier);
    return stream.str();
}

TEST(CoordinateTransformTest, ScalesScalarsAndPoints)
{
    EXPECT_DOUBLE_EQ(2.0, transform().board_to_dsn(20.0));
    const std::vector<double> point = transform().board_to_dsn(fp::FloatPoint(20.0, 30.0));
    ASSERT_EQ(2u, point.size());
    EXPECT_DOUBLE_EQ(7.0, point[0]);
    EXPECT_DOUBLE_EQ(-4.0, point[1]);

    const std::vector<double> rel_point = transform().board_to_dsn_rel(fp::FloatPoint(20.0, 30.0));
    ASSERT_EQ(2u, rel_point.size());
    EXPECT_DOUBLE_EQ(2.0, rel_point[0]);
    EXPECT_DOUBLE_EQ(3.0, rel_point[1]);

    const fp::FloatPoint back = transform().dsn_to_board(7.0, -4.0);
    EXPECT_DOUBLE_EQ(20.0, back.x);
    EXPECT_DOUBLE_EQ(30.0, back.y);
}

TEST(CoordinateTransformTest, TransformsPointArrays)
{
    const std::vector<fp::FloatPoint> points{ fp::FloatPoint(20.0, 30.0), fp::FloatPoint(100.0, 0.0),
        fp::FloatPoint(100.0, 50.0) };
    const std::vector<double> dsn = transform().board_to_dsn(points);
    ASSERT_EQ(6u, dsn.size());
    EXPECT_DOUBLE_EQ(7.0, dsn[0]);
    EXPECT_DOUBLE_EQ(-4.0, dsn[1]);
    EXPECT_DOUBLE_EQ(15.0, dsn[2]);
    EXPECT_DOUBLE_EQ(-7.0, dsn[3]);
    EXPECT_DOUBLE_EQ(15.0, dsn[4]);
    EXPECT_DOUBLE_EQ(-2.0, dsn[5]);

    const std::vector<double> rel = transform().board_to_dsn_rel(points);
    ASSERT_EQ(6u, rel.size());
    EXPECT_DOUBLE_EQ(2.0, rel[0]);
    EXPECT_DOUBLE_EQ(3.0, rel[1]);
    EXPECT_DOUBLE_EQ(10.0, rel[2]);
    EXPECT_DOUBLE_EQ(0.0, rel[3]);
    EXPECT_DOUBLE_EQ(10.0, rel[4]);
    EXPECT_DOUBLE_EQ(5.0, rel[5]);
}

TEST(CoordinateTransformTest, TransformsIntBox)
{
    const fp::IntBox box(20, 30, 40, 50);
    const std::vector<double> dsn = transform().board_to_dsn(box);
    ASSERT_EQ(4u, dsn.size());
    EXPECT_DOUBLE_EQ(7.0, dsn[0]);
    EXPECT_DOUBLE_EQ(-4.0, dsn[1]);
    EXPECT_DOUBLE_EQ(9.0, dsn[2]);
    EXPECT_DOUBLE_EQ(-2.0, dsn[3]);

    const std::vector<double> rel = transform().board_to_dsn_rel(box);
    ASSERT_EQ(4u, rel.size());
    EXPECT_DOUBLE_EQ(2.0, rel[0]);
    EXPECT_DOUBLE_EQ(3.0, rel[1]);
    EXPECT_DOUBLE_EQ(4.0, rel[2]);
    EXPECT_DOUBLE_EQ(5.0, rel[3]);
}

TEST(CoordinateTransformTest, TransformsVectorsAndLines)
{
    const fp::IntVector vector(20, 30);
    const std::vector<double> dsn = transform().board_to_dsn(vector);
    ASSERT_EQ(2u, dsn.size());
    EXPECT_DOUBLE_EQ(2.0, dsn[0]);
    EXPECT_DOUBLE_EQ(3.0, dsn[1]);

    const std::vector<fp::Line> lines{ fp::Line(fp::IntPoint(20, 30), fp::IntPoint(40, 50)) };
    const std::vector<double> line_dsn = transform().board_to_dsn(lines);
    ASSERT_EQ(4u, line_dsn.size());
    EXPECT_DOUBLE_EQ(7.0, line_dsn[0]);
    EXPECT_DOUBLE_EQ(-4.0, line_dsn[1]);
    EXPECT_DOUBLE_EQ(9.0, line_dsn[2]);
    EXPECT_DOUBLE_EQ(-2.0, line_dsn[3]);
}

TEST(CoordinateTransformTest, TransformsIntBoxShape)
{
    const fp::IntBox box(20, 30, 40, 50);
    const std::unique_ptr<Shape> rectangle = transform().board_to_dsn(box, front_layer());
    ASSERT_NE(nullptr, rectangle);
    EXPECT_EQ("\n(rect F.Cu 7.0 -4.0 9.0 -2.0)", write_shape(*rectangle));

    const std::unique_ptr<Shape> rel_rectangle = transform().board_to_dsn_rel(box, front_layer());
    ASSERT_NE(nullptr, rel_rectangle);
    EXPECT_EQ("\n(rect F.Cu 2.0 3.0 4.0 5.0)", write_shape(*rel_rectangle));
}

TEST(CoordinateTransformTest, TransformsWrappedIntBoxShape)
{
    const fp::ConvexTileShape wrapped(std::make_unique<fp::IntBox>(20, 30, 40, 50));
    const std::unique_ptr<Shape> rectangle = transform().board_to_dsn(wrapped, front_layer());
    ASSERT_NE(nullptr, rectangle);
    EXPECT_EQ("\n(rect F.Cu 7.0 -4.0 9.0 -2.0)", write_shape(*rectangle));
}

TEST(CoordinateTransformTest, TransformsCircleShape)
{
    const fp::Circle circle(fp::IntPoint(100, 200), 50);
    const std::unique_ptr<Shape> dsn = transform().board_to_dsn(circle, front_layer());
    ASSERT_NE(nullptr, dsn);
    EXPECT_EQ("\n(circle F.Cu 10.0 15.0 13.0)", write_shape(*dsn));

    const std::unique_ptr<Shape> rel = transform().board_to_dsn_rel(circle, front_layer());
    ASSERT_NE(nullptr, rel);
    EXPECT_EQ("\n(circle F.Cu 10.0 10.0 20.0)", write_shape(*rel));
}

TEST(CoordinateTransformTest, TransformsPolygonShape)
{
    const fp::PolygonShape polygon(
        std::vector<fp::IntPoint>{ fp::IntPoint(0, 0), fp::IntPoint(100, 0), fp::IntPoint(100, 50) });
    const std::unique_ptr<Shape> dsn = transform().board_to_dsn(polygon, front_layer());
    ASSERT_NE(nullptr, dsn);
    EXPECT_EQ("\n(polygon F.Cu 0\n  5.0 -7.0\n  15.0 -7.0\n  15.0 -2.0\n)", write_shape(*dsn));

    const std::unique_ptr<Shape> rel = transform().board_to_dsn_rel(polygon, front_layer());
    ASSERT_NE(nullptr, rel);
    EXPECT_EQ("\n(polygon F.Cu 0\n  0.0 0.0\n  10.0 0.0\n  10.0 5.0\n)", write_shape(*rel));
}

} // namespace
} // namespace freerouting::designforms::specctra::parser
