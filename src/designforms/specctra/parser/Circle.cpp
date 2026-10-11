#include "designforms/specctra/parser/Circle.h"

#include <cmath>

#include "datastructures/JavaFormat.h"
#include "designforms/specctra/parser/CoordinateTransform.h"
#include "designforms/specctra/parser/Rectangle.h"
#include "geometry/planar/Shape.h"

namespace freerouting::designforms::specctra::parser {

namespace fp = geometry::planar;

namespace {
int64_t java_round(double p_value) { return static_cast<int64_t>(std::floor(p_value + 0.5)); }
} // namespace

std::unique_ptr<fp::Shape> Circle::transform_to_board(const CoordinateTransform& p_transform) const
{
    const fp::IntPoint center = p_transform.dsn_to_board(coor[1], coor[2]).round();
    const int64_t radius = java_round(p_transform.dsn_to_board(coor[0]) / 2);
    return std::make_unique<fp::Circle>(center, radius);
}

std::unique_ptr<fp::Shape> Circle::transform_to_board_rel(const CoordinateTransform& p_transform) const
{
    const int64_t radius = java_round(p_transform.dsn_to_board(coor[0]) / 2);
    const int64_t x = java_round(p_transform.dsn_to_board(coor[1]));
    const int64_t y = java_round(p_transform.dsn_to_board(coor[2]));
    return std::make_unique<fp::Circle>(fp::IntPoint(x, y), radius);
}

std::unique_ptr<Rectangle> Circle::bounding_box() const
{
    // Java parity: the diameter coor[0] is used like a radius here.
    return std::make_unique<Rectangle>(
        layer, std::array<double, 4>{ coor[1] - coor[0], coor[2] - coor[0], coor[1] + coor[0], coor[2] + coor[0] });
}

void Circle::write_scope(
    datastructures::IndentFileWriter& p_file, const datastructures::IdentifierType& p_identifier) const
{
    p_file.new_line();
    p_file << "(circle ";
    p_identifier.write(this->layer->name, p_file.stream());
    for (double coordinate : coor)
    {
        p_file << " " << datastructures::java_double_to_string(coordinate);
    }
    p_file << ")";
}

void Circle::write_scope_int(
    datastructures::IndentFileWriter& p_file, const datastructures::IdentifierType& p_identifier) const
{
    p_file.new_line();
    p_file << "(circle ";
    p_identifier.write(this->layer->name, p_file.stream());
    for (double coordinate : coor)
    {
        const int current_coordinate = static_cast<int>(java_round(coordinate));
        p_file << " " << current_coordinate;
    }
    p_file << ")";
}

} // namespace freerouting::designforms::specctra::parser
