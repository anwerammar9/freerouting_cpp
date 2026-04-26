#pragma once

#include "geometry/planar/Point.h"
#include <cmath>
#include <memory>
#include <string>

namespace freerouting::geometry::planar {

class FloatLine
{
public:
    FloatLine(const FloatPoint& p_a, const FloatPoint& p_b);

    FloatLine opposite() const;

    double segment_distance(const FloatPoint& p_point) const;
    FloatPoint perpendicular_projection(const FloatPoint& p_point) const;
    FloatPoint nearest_segment_point(const FloatPoint& p_from_point) const;
    FloatLine translate(double p_dist) const;
    double signed_distance(const FloatPoint& p_point) const;

private:
    FloatPoint a_;
    FloatPoint b_;
};

} // namespace freerouting::geometry::planar