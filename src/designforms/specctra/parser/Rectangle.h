#pragma once

#include <array>

#include "designforms/specctra/parser/Shape.h"

namespace freerouting::designforms::specctra::parser {

/** A (rect layer x1 y1 x2 y2) shape. Port of Java parser.Rectangle. */
class Rectangle : public Shape
{
public:
    Rectangle(std::shared_ptr<Layer> p_layer, const std::array<double, 4>& p_coor) : Shape(std::move(p_layer)), coor(p_coor) {}

    std::unique_ptr<geometry::planar::Shape> transform_to_board(const CoordinateTransform& p_transform) const override;
    std::unique_ptr<geometry::planar::Shape> transform_to_board_rel(const CoordinateTransform& p_transform) const override;
    std::unique_ptr<Rectangle> bounding_box() const override { return std::make_unique<Rectangle>(*this); }
    /** Java Rectangle.union. */
    Rectangle union_with(const Rectangle& p_other) const;

    std::array<double, 4> coor;
};

} // namespace freerouting::designforms::specctra::parser
