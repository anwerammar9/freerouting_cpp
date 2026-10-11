#pragma once

#include <memory>
#include <vector>

#include "geometry/planar/Point.h"

namespace freerouting::geometry::planar {
class IntBox;
class Line;
class Shape;
class Vector;
} // namespace freerouting::geometry::planar

namespace freerouting::designforms::specctra::parser {

class Layer;
class Shape;

/**
 * Transformation between DSN coordinates and board coordinates: board =
 * (dsn - base) * scale_factor. Port of Java io.CoordinateTransform, including
 * the board-to-DSN shape conversions used by the DSN and SES writers.
 */
class CoordinateTransform
{
public:
    CoordinateTransform(double p_scale_factor, double p_base_x, double p_base_y)
        : scale_factor_(p_scale_factor), base_x_(p_base_x), base_y_(p_base_y) {}

    double board_to_dsn(double p_value) const { return p_value / scale_factor_; }
    /** Java boardToDsn(FloatPoint): {x, y} in DSN coordinates. */
    std::vector<double> board_to_dsn(const geometry::planar::FloatPoint& p_point) const
    {
        return { board_to_dsn(p_point.x) + base_x_, board_to_dsn(p_point.y) + base_y_ };
    }
    /** Java boardToDsnRel(FloatPoint): without the base offset. */
    std::vector<double> board_to_dsn_rel(const geometry::planar::FloatPoint& p_point) const
    {
        return { board_to_dsn(p_point.x), board_to_dsn(p_point.y) };
    }

    /** Java boardToDsn(FloatPoint[]). */
    std::vector<double> board_to_dsn(const std::vector<geometry::planar::FloatPoint>& p_points) const;
    /** Java boardToDsnRel(FloatPoint[]). */
    std::vector<double> board_to_dsn_rel(const std::vector<geometry::planar::FloatPoint>& p_points) const;
    /** Java boardToDsn(IntBox). */
    std::vector<double> board_to_dsn(const geometry::planar::IntBox& p_box) const;
    /** Java boardToDsnRel(IntBox). */
    std::vector<double> board_to_dsn_rel(const geometry::planar::IntBox& p_box) const;
    /** Java boardToDsn(Vector). */
    std::vector<double> board_to_dsn(const geometry::planar::Vector& p_vector) const;
    /** Java boardToDsn(Line[]). */
    std::vector<double> board_to_dsn(const std::vector<geometry::planar::Line>& p_lines) const;

    /**
     * Java boardToDsn(Shape, Layer): IntBox -> Rectangle, PolylineShape (and
     * the C++ ConvexTileShape wrapping an octagon/simplex) -> Polygon,
     * Circle -> Circle, otherwise a warning and nullptr.
     */
    std::unique_ptr<Shape> board_to_dsn(
        const geometry::planar::Shape& p_board_shape, std::shared_ptr<Layer> p_layer) const;
    /** Java boardToDsn(IntBox) branch as a shape (C++ IntBox is not a Shape). */
    std::unique_ptr<Shape> board_to_dsn(
        const geometry::planar::IntBox& p_box, std::shared_ptr<Layer> p_layer) const;
    /** Java boardToDsnRel(Shape, Layer). */
    std::unique_ptr<Shape> board_to_dsn_rel(
        const geometry::planar::Shape& p_board_shape, std::shared_ptr<Layer> p_layer) const;
    /** Java boardToDsnRel(IntBox) branch as a shape. */
    std::unique_ptr<Shape> board_to_dsn_rel(
        const geometry::planar::IntBox& p_box, std::shared_ptr<Layer> p_layer) const;

    double dsn_to_board(double p_value) const { return p_value * scale_factor_; }
    /** Java dsnToBoard(double[] tuple). */
    geometry::planar::FloatPoint dsn_to_board(double p_x, double p_y) const
    {
        return geometry::planar::FloatPoint(dsn_to_board(p_x - base_x_), dsn_to_board(p_y - base_y_));
    }
    /** Java dsnToBoardRel(double[] tuple). */
    geometry::planar::FloatPoint dsn_to_board_rel(double p_x, double p_y) const
    {
        return geometry::planar::FloatPoint(dsn_to_board(p_x), dsn_to_board(p_y));
    }

    double scale_factor() const { return scale_factor_; }

private:
    double scale_factor_;
    double base_x_;
    double base_y_;
};

} // namespace freerouting::designforms::specctra::parser
