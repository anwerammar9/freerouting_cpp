#include "designforms/specctra/parser/Circle.h"

#include <cmath>

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

} // namespace freerouting::designforms::specctra::parser
