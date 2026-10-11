#pragma once

#include "geometry/planar/Direction.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/Point.h"
#include "geometry/planar/Vector.h"

#include <memory>
#include <optional>
#include <vector>

namespace freerouting {
namespace geometry {
namespace planar {

class Polyline;
class PolylineShape;
class Simplex;
class TileShape;

class LineSegment
{
public:
    LineSegment(const Line& p_start_line, const Line& p_middle_line, const Line& p_end_line);
    LineSegment(const Polyline& p_polyline, int p_no);
    LineSegment(const PolylineShape& p_shape, int p_no);

    /**
     * Creates the p_no-th border line segment of p_shape for p_no between 0
     * and p_shape.border_line_count() - 1. Port of the Java
     * LineSegment(PolylineShape, int) constructor, used e.g. by
     * ShapeSearchTree.restrain_shape() to cut rooms along obstacle edge
     * segments.
     */
    LineSegment(const TileShape& p_shape, int p_no);

    static LineSegment from_endpoints(const IntPoint& p_start, const IntPoint& p_end);

    /** The start point rounded to an IntPoint (exact if the intersection is integral). */
    IntPoint start_point() const;
    IntPoint end_point() const;
    /**
     * The exact start / end point (IntPoint or RationalPoint), Java LineSegment.startPoint() /
     * endPoint(). Use these wherever Java compares or classifies the end points.
     */
    const Point& start_point_exact() const;
    const Point& end_point_exact() const;
    FloatPoint start_point_approx() const;
    FloatPoint end_point_approx() const;

    const Line& get_line() const { return middle_; }
    const Line& get_start_closing_line() const { return start_; }
    const Line& get_end_closing_line() const { return end_; }

    LineSegment opposite() const;
    Polyline to_polyline() const;
    std::unique_ptr<Simplex> to_simplex() const;

    bool contains(const IntPoint& p_point) const;

    IntBox bounding_box() const;
    IntOctagon bounding_octagon() const;

    LineSegment change_length_approx(double p_new_length) const;

    std::vector<Line> intersection(const LineSegment& p_other) const;
    bool intersects(const LineSegment& p_other) const;
    bool overlaps(const LineSegment& p_other) const;

    std::vector<IntPoint> stair_approximation(double p_width, bool p_to_the_right) const;
    std::vector<IntPoint> stair_approximation_45(double p_width, bool p_to_the_right) const;
    std::vector<int> border_intersections(const TileShape& p_shape) const;

    LineSegment sort_endpoints_in_x_y() const;

private:
    Line start_;
    Line middle_;
    Line end_;
    mutable std::optional<IntPoint> precalculated_start_point_;
    mutable std::optional<IntPoint> precalculated_end_point_;
    mutable std::shared_ptr<Point> exact_start_point_;
    mutable std::shared_ptr<Point> exact_end_point_;

    static IntDirection normalized_direction(const IntPoint& p_from, const IntPoint& p_to);
};

} // namespace planar
} // namespace geometry
} // namespace freerouting
