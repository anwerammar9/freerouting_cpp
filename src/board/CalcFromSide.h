#pragma once

#include <optional>

#include "geometry/planar/Point.h"

namespace freerouting::geometry::planar
{
class LineSegment;
class Polyline;
class TileShape;
}

namespace freerouting::board
{

/**
 * Calculates the number of the edge line of a TileShape where an entering
 * polyline / line segment crosses the border of the shape. Used in the shove
 * algorithms to determine the shove direction. Mirrors Java CalcFromSide.java.
 */
class CalcFromSide
{
public:
    static const CalcFromSide NOT_CALCULATED;

    /**
     * Creates an instance from already calculated values. Mirrors Java
     * new ShapeEntrySide(int no, FloatPoint borderIntersection), where
     * p_border_intersection may be null.
     */
    static std::optional<CalcFromSide> values(
        int p_no, const std::optional<geometry::planar::FloatPoint>& p_border_intersection);

    /** Number of the shape edge line, or -1 if no side could be determined. */
    int no;

    /**
     * Approximate point where the border of the shape is crossed, empty if
     * it is unknown. Java ShapeEntrySide.borderIntersection is a nullable
     * FloatPoint and ShapeTraceEntries::resort branches on null, so the
     * optional models that exactly (the (0,0) point is a legal intersection).
     */
    std::optional<geometry::planar::FloatPoint> border_intersection;

    CalcFromSide(int p_no, const geometry::planar::FloatPoint& p_border_intersection);

    /** Values already calculated, without a border intersection. */
    explicit CalcFromSide(int p_no);

    /**
     * Calculates the number of the edge line of p_shape where p_polyline enters.
     * p_no is expected between 1 and p_polyline.line_count - 2 inclusive.
     */
    CalcFromSide(const geometry::planar::Polyline& p_polyline, int p_no,
                 const geometry::planar::TileShape& p_shape);

    /**
     * Calculates the nearest border side of p_shape to p_from_point.
     */
    CalcFromSide(const geometry::planar::IntPoint& p_from_point,
                 const geometry::planar::TileShape& p_shape);

    /**
     * Calculates the side of p_shape at the start of p_line_segment.
     * If p_shove_to_the_left, the from_side_no is decremented by 2,
     * else it is increased by 2.
     */
    CalcFromSide(const geometry::planar::LineSegment& p_line_segment,
                 const geometry::planar::TileShape& p_shape, bool p_shove_to_the_left);
};

} // namespace freerouting::board