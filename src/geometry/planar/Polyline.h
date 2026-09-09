#pragma once

#include <vector>
#include <memory>
#include <optional>
#include "geometry/planar/Point.h"
#include "geometry/planar/Vector.h"

namespace freerouting {
namespace geometry {
namespace planar {

class PolygonShape;
class IntBox;
class IntOctagon;
class TileShape;
class LineSegment;

class Polyline {
public:
    std::vector<Line> arr;

    Polyline() = default;
    explicit Polyline(const std::vector<Line>& p_line_arr);
    explicit Polyline(const std::vector<IntPoint>& p_points);
    Polyline(const IntPoint& p_from_corner, const IntPoint& p_to_corner);

    /**
     * Copies the line array; derived caches are recomputed lazily.
     * Mirrors Java Polyline value semantics.
     */
    Polyline(const Polyline& p_other);
    Polyline& operator=(const Polyline& p_other);

    /**
     * Creates a polyline through p_points with perpendicular closing lines at
     * both ends. Mirrors Java new Polyline(Point[]).
     */
    static Polyline from_corners(const std::vector<IntPoint>& p_points);

    int corner_count() const;
    bool is_empty() const;
    bool is_point() const;
    bool is_orthogonal() const;
    bool is_multiple_of_45_degree() const;

    IntPoint first_corner() const;
    IntPoint last_corner() const;
    
    std::vector<IntPoint> corner_arr() const;
    std::vector<FloatPoint> corner_approx_arr() const;
    FloatPoint corner_approx(int p_no) const;
    IntPoint corner(int p_no) const;

    Polyline reverse() const;
    
    double length_approx(int p_from_corner, int p_to_corner) const;
    double length_approx() const;

    LineSegment projection_line(int p_no) const;

    /**
     * Calculates the segment of this polyline closest to p_from_point together
     * with the line of the projection of p_from_point onto that segment and the
     * perpendicular line through p_from_point. Returns std::nullopt if no such
     * segment exists. Port of Polyline.projection_line(Point).
     */
    std::optional<LineSegment> projection_line(const IntPoint& p_from_point) const;

    /**
     * Shortens this polyline to p_new_line_count lines, so that the length of
     * the last line segment is p_last_segment_length.
     * Port of Polyline.shorten(int, double).
     */
    Polyline shorten(int p_new_line_count, double p_last_segment_length) const;

    std::unique_ptr<Polyline> translate_by(const IntVector& p_vector) const;

    Polyline turn_90_degree(int p_factor, const IntPoint& p_pole) const;
    Polyline rotate_approx(double p_angle, const FloatPoint& p_pole) const;
    Polyline mirror_vertical(const IntPoint& p_pole) const;
    Polyline mirror_horizontal(const IntPoint& p_pole) const;

    IntBox bounding_box(int p_from_corner_no, int p_to_corner_no) const;
    IntBox bounding_box() const;
    IntOctagon bounding_octagon(int p_from_corner_no, int p_to_corner_no) const;

    double distance(const FloatPoint& p_from_point) const;
    FloatPoint nearest_point_approx(const FloatPoint& p_from_point) const;
    bool contains(const IntPoint& p_point) const;

    std::vector<std::unique_ptr<Polyline>> split(int p_line_no, const Line& p_end_line) const;
    Polyline skip_lines(int p_from_no, int p_to_no) const;

    std::unique_ptr<Polyline> combine(const Polyline& p_other) const;

    std::vector<std::unique_ptr<TileShape>> offset_shapes(int p_half_width) const;
    std::vector<std::unique_ptr<TileShape>> offset_shapes(int p_half_width, int p_from_no, int p_to_no) const;
    std::unique_ptr<TileShape> offset_shape(int p_half_width, int p_no) const;
    IntBox offset_box(int p_half_width, int p_no) const;

private:
    mutable std::vector<FloatPoint> precalculated_float_corners;
    mutable std::vector<IntPoint> precalculated_corners;
    mutable std::unique_ptr<IntBox> precalculated_bounding_box;

    static std::vector<Line> remove_consecutive_parallel_lines(const std::vector<Line>& p_line_arr);
    static std::vector<Line> remove_overlaps(const std::vector<Line>& p_line_arr);
};

} // namespace planar
} // namespace geometry
} // namespace freerouting