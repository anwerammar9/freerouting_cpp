#pragma once

#include <vector>

#include "designforms/specctra/parser/Shape.h"

namespace freerouting::designforms::specctra::parser {

/** A closed (polygon layer aperture x1 y1 ...) shape. Port of Java parser.Polygon. */
class Polygon : public Shape
{
public:
    Polygon(std::shared_ptr<Layer> p_layer, std::vector<double> p_coor) : Shape(std::move(p_layer)), coor(std::move(p_coor)) {}

    std::unique_ptr<geometry::planar::Shape> transform_to_board(const CoordinateTransform& p_transform) const override;
    std::unique_ptr<geometry::planar::Shape> transform_to_board_rel(const CoordinateTransform& p_transform) const override;
    std::unique_ptr<Rectangle> bounding_box() const override;
    void write_scope(datastructures::IndentFileWriter& p_file,
        const datastructures::IdentifierType& p_identifier) const override;
    void write_scope_int(datastructures::IndentFileWriter& p_file,
        const datastructures::IdentifierType& p_identifier) const override;

    std::vector<double> coor;
};

} // namespace freerouting::designforms::specctra::parser
