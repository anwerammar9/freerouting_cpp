#include "designforms/specctra/parser/Rectangle.h"

#include <algorithm>
#include <cmath>

#include "designforms/specctra/parser/CoordinateTransform.h"
#include "geometry/planar/IntBox.h"

namespace freerouting::designforms::specctra::parser {

namespace fp = geometry::planar;

namespace {
int64_t java_round(double p_value) { return static_cast<int64_t>(std::floor(p_value + 0.5)); }
} // namespace

Rectangle Rectangle::union_with(const Rectangle& p_other) const
{
    return Rectangle(layer, { std::min(coor[0], p_other.coor[0]), std::min(coor[1], p_other.coor[1]),
                                std::max(coor[2], p_other.coor[2]), std::max(coor[3], p_other.coor[3]) });
}

std::unique_ptr<fp::Shape> Rectangle::transform_to_board_rel(const CoordinateTransform& p_transform) const
{
    std::array<int64_t, 4> box_coor{};
    for (int i = 0; i < 4; ++i)
    {
        box_coor[i] = java_round(p_transform.dsn_to_board(coor[i]));
    }
    if (box_coor[1] <= box_coor[3])
    {
        return as_board_shape(std::make_unique<fp::IntBox>(box_coor[0], box_coor[1], box_coor[2], box_coor[3]));
    }
    return as_board_shape(std::make_unique<fp::IntBox>(box_coor[0], box_coor[3], box_coor[2], box_coor[1]));
}

std::unique_ptr<fp::Shape> Rectangle::transform_to_board(const CoordinateTransform& p_transform) const
{
    const fp::FloatPoint lower_left =
        p_transform.dsn_to_board(std::min(coor[0], coor[2]), std::min(coor[1], coor[3]));
    const fp::FloatPoint upper_right =
        p_transform.dsn_to_board(std::max(coor[0], coor[2]), std::max(coor[1], coor[3]));
    return as_board_shape(std::make_unique<fp::IntBox>(lower_left.round(), upper_right.round()));
}

} // namespace freerouting::designforms::specctra::parser
