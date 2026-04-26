#include "geometry/planar/FloatLine.h"
#include <algorithm>
#include <cmath>

namespace freerouting::geometry::planar {

FloatLine::FloatLine(const FloatPoint& p_a, const FloatPoint& p_b)
    : a_(p_a), b_(p_b)
{
}

FloatLine FloatLine::opposite() const
{
    return FloatLine(b_, a_);
}

double FloatLine::segment_distance(const FloatPoint& p_point) const
{
    FloatPoint projection = perpendicular_projection(p_point);
    if (projection.is_contained_in_box(a_, b_, 0.01))
    {
        double dx = p_point.x - projection.x;
        double dy = p_point.y - projection.y;
        return std::sqrt(dx * dx + dy * dy);
    }
    double dist_to_a = std::sqrt(p_point.distance_square(a_));
    double dist_to_b = std::sqrt(p_point.distance_square(b_));
    return std::min(dist_to_a, dist_to_b);
}

FloatPoint FloatLine::perpendicular_projection(const FloatPoint& p_point) const
{
    double dx = b_.x - a_.x;
    double dy = b_.y - a_.y;
    if (dx == 0 && dy == 0)
    {
        return a_;
    }

    double dxdx = dx * dx;
    double dydy = dy * dy;
    double dxdy = dx * dy;
    double denominator = dxdx + dydy;
    double det = a_.x * b_.y - b_.x * a_.y;

    double x = (p_point.x * dxdx + p_point.y * dxdy + det * dy) / denominator;
    double y = (p_point.x * dxdy + p_point.y * dydy - det * dx) / denominator;

    return FloatPoint(x, y);
}

FloatPoint FloatLine::nearest_segment_point(const FloatPoint& p_from_point) const
{
    FloatPoint projection = perpendicular_projection(p_from_point);
    if (projection.is_contained_in_box(a_, b_, 0.01))
    {
        return projection;
    }

    if (p_from_point.distance_square(a_) <= p_from_point.distance_square(b_))
    {
        return a_;
    }
    return b_;
}

FloatLine FloatLine::translate(double p_dist) const
{
    double dx = b_.x - a_.x;
    double dy = b_.y - a_.y;
    double dxdx = dx * dx;
    double dydy = dy * dy;
    double length = std::sqrt(dxdx + dydy);

    double new_x, new_y;
    if (dxdx <= dydy)
    {
        double rel_x = (p_dist * length) / dy;
        new_x = a_.x - rel_x;
        new_y = a_.y;
    }
    else
    {
        double rel_y = (p_dist * length) / dx;
        new_x = a_.x;
        new_y = a_.y + rel_y;
    }

    return FloatLine(FloatPoint(new_x, new_y), FloatPoint(new_x + dx, new_y + dy));
}

double FloatLine::signed_distance(const FloatPoint& p_point) const
{
    double dx = b_.x - a_.x;
    double dy = b_.y - a_.y;
    double det = dy * (p_point.x - a_.x) - dx * (p_point.y - a_.y);
    double length = std::sqrt(dx * dx + dy * dy);
    return det / length;
}

} // namespace freerouting::geometry::planar