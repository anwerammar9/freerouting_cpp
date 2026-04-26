#pragma once

#include <vector>
#include <memory>
#include "geometry/planar/Point.h"
#include "geometry/planar/Vector.h"

namespace freerouting {
namespace geometry {
namespace planar {

class PolygonShape;
class IntBox;

class Polyline {
public:
    std::vector<Line> arr;

    Polyline() = default;
    explicit Polyline(const std::vector<Line>& p_line_arr);
    explicit Polyline(const std::vector<IntPoint>& p_points);
    Polyline(const IntPoint& p_from_corner, const IntPoint& p_to_corner);

    int corner_count() const;
    bool is_empty() const;
    bool is_point() const;
    bool is_orthogonal() const;
    bool is_multiple_of_45_degree() const;

    IntPoint first_corner() const;
    IntPoint last_corner() const;
    
    std::vector<IntPoint> corner_arr() const;
    FloatPoint corner_approx(int p_no) const;
    IntPoint corner(int p_no) const;

    Polyline reverse() const;
    
    double length_approx(int p_from_corner, int p_to_corner) const;
    double length_approx() const;

    std::unique_ptr<Polyline> translate_by(const IntVector& p_vector) const;
    
    IntBox bounding_box(int p_from_corner_no, int p_to_corner_no) const;
    IntBox bounding_box() const;

    double distance(const FloatPoint& p_from_point) const;
    
    std::unique_ptr<Polyline> combine(const Polyline& p_other) const;

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