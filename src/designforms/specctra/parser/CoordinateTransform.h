#pragma once

#include <vector>

#include "geometry/planar/Point.h"

namespace freerouting::designforms::specctra::parser {

/**
 * Transformation between DSN coordinates and board coordinates: board =
 * (dsn - base) * scale_factor. Port of the numeric part of Java
 * io.CoordinateTransform (the board-to-DSN shape conversions follow with the
 * DSN writer).
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
