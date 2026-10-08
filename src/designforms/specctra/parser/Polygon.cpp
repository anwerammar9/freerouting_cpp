#include "designforms/specctra/parser/Polygon.h"

#include <algorithm>
#include <climits>
#include <cmath>

#include "designforms/specctra/parser/CoordinateTransform.h"
#include "designforms/specctra/parser/Rectangle.h"
#include "geometry/planar/PolygonShape.h"
#include "geometry/planar/Simplex.h"

namespace freerouting::designforms::specctra::parser {

namespace fp = geometry::planar;

namespace {
int64_t java_round(double p_value) { return static_cast<int64_t>(std::floor(p_value + 0.5)); }
} // namespace

std::unique_ptr<fp::Shape> Polygon::transform_to_board(const CoordinateTransform& p_transform) const
{
    std::vector<fp::IntPoint> corners;
    for (size_t i = 0; i + 1 < coor.size(); i += 2)
    {
        corners.push_back(p_transform.dsn_to_board(coor[i], coor[i + 1]).round());
    }
    return std::make_unique<fp::PolygonShape>(corners);
}

std::unique_ptr<fp::Shape> Polygon::transform_to_board_rel(const CoordinateTransform& p_transform) const
{
    if (coor.size() < 2)
    {
        return as_board_shape(std::make_unique<fp::Simplex>(fp::Simplex::EMPTY));
    }
    std::vector<fp::IntPoint> corners;
    for (size_t i = 0; i + 1 < coor.size(); i += 2)
    {
        corners.emplace_back(java_round(p_transform.dsn_to_board(coor[i])), java_round(p_transform.dsn_to_board(coor[i + 1])));
    }
    return std::make_unique<fp::PolygonShape>(corners);
}

std::unique_ptr<Rectangle> Polygon::bounding_box() const
{
    std::array<double, 4> bounds{ static_cast<double>(INT_MAX), static_cast<double>(INT_MAX),
        static_cast<double>(INT_MIN), static_cast<double>(INT_MIN) };
    for (size_t i = 0; i < coor.size(); ++i)
    {
        if (i % 2 == 0)
        {
            bounds[0] = std::min(bounds[0], coor[i]);
            bounds[2] = std::max(bounds[2], coor[i]);
        }
        else
        {
            bounds[1] = std::min(bounds[1], coor[i]);
            bounds[3] = std::max(bounds[3], coor[i]);
        }
    }
    return std::make_unique<Rectangle>(layer, bounds);
}

} // namespace freerouting::designforms::specctra::parser
