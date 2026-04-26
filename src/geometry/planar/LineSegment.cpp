#include "geometry/planar/LineSegment.h"

#include <algorithm>

namespace freerouting {
namespace geometry {
namespace planar {

LineSegment::LineSegment(const IntPoint& p_start, const IntPoint& p_end)
    : start_(p_start)
    , end_(p_end)
{
}

Line LineSegment::supporting_line() const
{
    return Line(start_, end_);
}

IntBox LineSegment::bounding_box() const
{
    return IntBox(
        std::min(start_.x, end_.x),
        std::min(start_.y, end_.y),
        std::max(start_.x, end_.x),
        std::max(start_.y, end_.y)
    );
}

int LineSegment::orientation(const IntPoint& p, const IntPoint& q, const IntPoint& r)
{
    const int64_t value = (q.y - p.y) * (r.x - q.x) - (q.x - p.x) * (r.y - q.y);
    if (value == 0)
    {
        return 0;
    }
    return (value > 0) ? 1 : 2;
}

bool LineSegment::on_segment(const IntPoint& p, const IntPoint& q, const IntPoint& r)
{
    return q.x <= std::max(p.x, r.x) &&
           q.x >= std::min(p.x, r.x) &&
           q.y <= std::max(p.y, r.y) &&
           q.y >= std::min(p.y, r.y);
}

bool LineSegment::contains(const IntPoint& p_point) const
{
    if (orientation(start_, p_point, end_) != 0)
    {
        return false;
    }
    return on_segment(start_, p_point, end_);
}

bool LineSegment::intersects(const LineSegment& p_other) const
{
    const IntPoint& p1 = start_;
    const IntPoint& q1 = end_;
    const IntPoint& p2 = p_other.start_;
    const IntPoint& q2 = p_other.end_;

    const int o1 = orientation(p1, q1, p2);
    const int o2 = orientation(p1, q1, q2);
    const int o3 = orientation(p2, q2, p1);
    const int o4 = orientation(p2, q2, q1);

    if (o1 != o2 && o3 != o4)
    {
        return true;
    }

    if (o1 == 0 && on_segment(p1, p2, q1)) return true;
    if (o2 == 0 && on_segment(p1, q2, q1)) return true;
    if (o3 == 0 && on_segment(p2, p1, q2)) return true;
    if (o4 == 0 && on_segment(p2, q1, q2)) return true;

    return false;
}

} // namespace planar
} // namespace geometry
} // namespace freerouting
