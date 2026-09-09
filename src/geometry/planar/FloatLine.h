#pragma once

#include "geometry/planar/Point.h"
#include <cmath>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace freerouting::geometry::planar {

class FloatLine
{
public:
    FloatLine(const FloatPoint& p_a, const FloatPoint& p_b);

    const FloatPoint& a() const { return a_; }
    const FloatPoint& b() const { return b_; }

    FloatLine opposite() const;
    FloatLine adjust_direction(const FloatLine& p_other) const;

    std::optional<FloatPoint> intersection(const FloatLine& p_other) const;
    double segment_distance(const FloatPoint& p_point) const;
    FloatPoint perpendicular_projection(const FloatPoint& p_point) const;
    std::optional<FloatLine> segment_projection(const FloatLine& p_line_segment) const;
    std::optional<FloatLine> segment_projection_2(const FloatLine& p_line_segment) const;
    FloatLine shrink_segment(double p_offset) const;
    FloatPoint nearest_segment_point(const FloatPoint& p_from_point) const;
    FloatLine translate(double p_dist) const;
    double signed_distance(const FloatPoint& p_point) const;
    std::vector<FloatLine> divide_segment_into_sections(int p_count) const;

private:
    FloatPoint a_;
    FloatPoint b_;
};

} // namespace freerouting::geometry::planar