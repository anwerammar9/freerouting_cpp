#pragma once

#include <array>

#include "designforms/specctra/parser/Shape.h"

namespace freerouting::designforms::specctra::parser {

/** A (circle layer diameter [x y]) shape: coor = {diameter, x, y}. Port of Java parser.Circle. */
class Circle : public Shape
{
public:
    Circle(std::shared_ptr<Layer> p_layer, const std::array<double, 3>& p_coor) : Shape(std::move(p_layer)), coor(p_coor) {}

    std::unique_ptr<geometry::planar::Shape> transform_to_board(const CoordinateTransform& p_transform) const override;
    std::unique_ptr<geometry::planar::Shape> transform_to_board_rel(const CoordinateTransform& p_transform) const override;
    std::unique_ptr<Rectangle> bounding_box() const override;
    void write_scope(datastructures::IndentFileWriter& p_file,
        const datastructures::IdentifierType& p_identifier) const override;
    void write_scope_int(datastructures::IndentFileWriter& p_file,
        const datastructures::IdentifierType& p_identifier) const override;

    std::array<double, 3> coor;
};

} // namespace freerouting::designforms::specctra::parser
