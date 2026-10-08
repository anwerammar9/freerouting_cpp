#pragma once

#include <vector>

#include "designforms/specctra/parser/Shape.h"

namespace freerouting::designforms::specctra::parser {

/** A path with a width. Port of Java parser.Path. */
class Path : public Shape
{
public:
    Path(std::shared_ptr<Layer> p_layer, double p_width, std::vector<double> p_coordinate_arr)
        : Shape(std::move(p_layer)), width(p_width), coordinate_arr(std::move(p_coordinate_arr)) {}

    double width;
    std::vector<double> coordinate_arr;
};

/** A (path layer width x1 y1 ...) through corner points. Port of Java parser.PolygonPath. */
class PolygonPath : public Path
{
public:
    using Path::Path;

    std::unique_ptr<geometry::planar::Shape> transform_to_board(const CoordinateTransform& p_transform) const override;
    std::unique_ptr<geometry::planar::Shape> transform_to_board_rel(const CoordinateTransform& p_transform) const override;
    std::unique_ptr<Rectangle> bounding_box() const override;
};

/**
 * A (polyline_path layer width x1 y1 x2 y2 ...) made of lines. Port of Java
 * parser.PolylinePath; its board transformations are not implemented in Java
 * either (they warn and return null).
 */
class PolylinePath : public Path
{
public:
    using Path::Path;

    std::unique_ptr<geometry::planar::Shape> transform_to_board(const CoordinateTransform& p_transform) const override;
    std::unique_ptr<geometry::planar::Shape> transform_to_board_rel(const CoordinateTransform& p_transform) const override;
    std::unique_ptr<Rectangle> bounding_box() const override;
};

} // namespace freerouting::designforms::specctra::parser
