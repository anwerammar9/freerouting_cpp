#include "geometry/planar/FloatLine.h"
#include "geometry/planar/Limits.h"
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

FloatLine FloatLine::adjust_direction(const FloatLine& p_other) const
{
    if (b_.side_of(a_, p_other.a_) == p_other.b_.side_of(a_, p_other.a_))
    {
        return *this;
    }
    return opposite();
}

std::optional<FloatPoint> FloatLine::intersection(const FloatLine& p_other) const
{
    double d1x = b_.x - a_.x;
    double d1y = b_.y - a_.y;
    double d2x = p_other.b_.x - p_other.a_.x;
    double d2y = p_other.b_.y - p_other.a_.y;
    double det_1 = a_.x * b_.y - a_.y * b_.x;
    double det_2 = p_other.a_.x * p_other.b_.y - p_other.a_.y * p_other.b_.x;
    double det = d2x * d1y - d2y * d1x;
    if (det == 0)
    {
        return std::nullopt;
    }
    double is_x = (d2x * det_1 - d1x * det_2) / det;
    double is_y = (d2y * det_1 - d1y * det_2) / det;
    return FloatPoint(is_x, is_y);
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

std::optional<FloatLine> FloatLine::segment_projection(const FloatLine& p_line_segment) const
{
    if (a_.scalar_product(b_, p_line_segment.a_) < 0)
    {
        return std::nullopt;
    }
    if (b_.scalar_product(a_, p_line_segment.b_) < 0)
    {
        return std::nullopt;
    }
    FloatPoint projected_a(0, 0);
    if (a_.scalar_product(b_, p_line_segment.a_) < 0)
    {
        projected_a = a_;
    }
    else
    {
        projected_a = perpendicular_projection(p_line_segment.a_);
        if (std::abs(projected_a.x) >= Limits::CRIT_INT || std::abs(projected_a.y) >= Limits::CRIT_INT)
        {
            return std::nullopt;
        }
    }
    FloatPoint projected_b(0, 0);
    if (b_.scalar_product(a_, p_line_segment.b_) < 0)
    {
        projected_b = b_;
    }
    else
    {
        projected_b = perpendicular_projection(p_line_segment.b_);
    }
    if (std::abs(projected_b.x) >= Limits::CRIT_INT || std::abs(projected_b.y) >= Limits::CRIT_INT)
    {
        return std::nullopt;
    }
    return FloatLine(projected_a, projected_b);
}

std::optional<FloatLine> FloatLine::segment_projection_2(const FloatLine& p_line_segment) const
{
    if (p_line_segment.a_.scalar_product(p_line_segment.b_, b_) <= 0)
    {
        return std::nullopt;
    }
    if (p_line_segment.b_.scalar_product(p_line_segment.a_, a_) <= 0)
    {
        return std::nullopt;
    }
    FloatPoint projected_a(0, 0);
    if (p_line_segment.a_.scalar_product(p_line_segment.b_, a_) < 0)
    {
        FloatLine curr_perpendicular_line(p_line_segment.a_,
            p_line_segment.b_.turn_90_degree(1, p_line_segment.a_));
        std::optional<FloatPoint> intersection = curr_perpendicular_line.intersection(*this);
        if (!intersection
            || std::abs(intersection->x) >= Limits::CRIT_INT
            || std::abs(intersection->y) >= Limits::CRIT_INT)
        {
            return std::nullopt;
        }
        projected_a = *intersection;
    }
    else
    {
        projected_a = a_;
    }

    FloatPoint projected_b(0, 0);
    if (p_line_segment.b_.scalar_product(p_line_segment.a_, b_) < 0)
    {
        FloatLine curr_perpendicular_line(p_line_segment.b_,
            p_line_segment.a_.turn_90_degree(1, p_line_segment.b_));
        std::optional<FloatPoint> intersection = curr_perpendicular_line.intersection(*this);
        if (!intersection
            || std::abs(intersection->x) >= Limits::CRIT_INT
            || std::abs(intersection->y) >= Limits::CRIT_INT)
        {
            return std::nullopt;
        }
        projected_b = *intersection;
    }
    else
    {
        projected_b = b_;
    }
    return FloatLine(projected_a, projected_b);
}

FloatLine FloatLine::shrink_segment(double p_offset) const
{
    double dx = b_.x - a_.x;
    double dy = b_.y - a_.y;
    if (dx == 0 && dy == 0)
    {
        return *this;
    }
    double length = std::sqrt(dx * dx + dy * dy);
    double offset = std::min(p_offset, length / 2);
    FloatPoint new_a(a_.x + (dx * offset) / length, a_.y + (dy * offset) / length);
    double new_length = length - offset;
    FloatPoint new_b(a_.x + (dx * new_length) / length, a_.y + (dy * new_length) / length);
    return FloatLine(new_a, new_b);
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

std::vector<FloatLine> FloatLine::divide_segment_into_sections(int p_count) const
{
    std::vector<FloatLine> result;
    if (p_count == 0)
    {
        return result;
    }
    if (p_count == 1)
    {
        result.push_back(*this);
        return result;
    }
    double line_length = b_.distance(a_);
    result.reserve(p_count);
    double section_length = line_length / p_count;
    double dx = b_.x - a_.x;
    double dy = b_.y - a_.y;
    FloatPoint curr_a = a_;
    for (int i = 0; i < p_count; i++)
    {
        FloatPoint curr_b(0, 0);
        if (i == p_count - 1)
        {
            curr_b = b_;
        }
        else
        {
            double curr_b_dist = (i + 1) * section_length;
            double curr_b_x = a_.x + (dx * curr_b_dist) / line_length;
            double curr_b_y = a_.y + (dy * curr_b_dist) / line_length;
            curr_b = FloatPoint(curr_b_x, curr_b_y);
        }
        result.push_back(FloatLine(curr_a, curr_b));
        curr_a = curr_b;
    }
    return result;
}

} // namespace freerouting::geometry::planar