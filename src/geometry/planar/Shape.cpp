#include "geometry/planar/Shape.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/Vector.h"

#include <cmath>
#include <algorithm>
#include <sstream>

namespace freerouting {
namespace geometry {
namespace planar {

// ============== Circle Implementation ==============

Circle::Circle(const IntPoint& p_center, int64_t p_radius)
    : center(p_center), radius(p_radius)
{
}

IntBox Circle::bounding_box() const
{
    return IntBox(center.x - radius, center.y - radius,
                  center.x + radius, center.y + radius);
}

bool Circle::intersects(const Shape& other) const
{
    const Circle* other_circle = dynamic_cast<const Circle*>(&other);
    if (other_circle)
    {
        int64_t dx = center.x - other_circle->center.x;
        int64_t dy = center.y - other_circle->center.y;
        int64_t dist_sq = dx*dx + dy*dy;
        int64_t rad_sum = radius + other_circle->radius;
        return dist_sq <= rad_sum * rad_sum;
    }
    return false;
}

bool Circle::contains(const IntPoint& point) const
{
    int64_t dx = point.x - center.x;
    int64_t dy = point.y - center.y;
    return dx*dx + dy*dy <= radius * radius;
}

std::string Circle::to_string() const
{
    std::ostringstream ss;
    ss << "Circle[" << center.to_string() << ", r=" << radius << "]";
    return ss.str();
}

// ============== PolygonArea Implementation ==============

PolygonArea::PolygonArea(const std::vector<IntPoint>& p_corners)
    : corners(p_corners)
{
}

IntBox PolygonArea::bounding_box() const
{
    if (corners.empty())
        return IntBox::EMPTY;

    int64_t min_x = corners[0].x;
    int64_t max_x = corners[0].x;
    int64_t min_y = corners[0].y;
    int64_t max_y = corners[0].y;

    for (const auto& pt : corners)
    {
        min_x = std::min(min_x, pt.x);
        max_x = std::max(max_x, pt.x);
        min_y = std::min(min_y, pt.y);
        max_y = std::max(max_y, pt.y);
    }

    return IntBox(min_x, min_y, max_x, max_y);
}

IntOctagon PolygonArea::bounding_octagon() const
{
    IntBox box = bounding_box();
    return IntOctagon(box.ur.x, box.ur.y, box.ll.x, box.ll.y,
                     box.ll.x - box.ur.y, box.ur.x - box.ll.y,
                     box.ur.x + box.ur.y, box.ll.x + box.ll.y);
}

bool PolygonArea::is_empty() const
{
    return corners.empty();
}

std::unique_ptr<Area> PolygonArea::clone() const
{
    return std::make_unique<PolygonArea>(corners);
}

bool PolygonArea::contains(const IntPoint& point) const
{
    if (corners.empty())
        return false;

    int n = static_cast<int>(corners.size());
    bool inside = false;

    for (int i = 0, j = n - 1; i < n; j = i++)
    {
        if (((corners[i].y > point.y) != (corners[j].y > point.y)) &&
            (point.x < (corners[j].x - corners[i].x) * (point.y - corners[i].y) /
                           (corners[j].y - corners[i].y) + corners[i].x))
        {
            inside = !inside;
        }
    }
    return inside;
}

std::string PolygonArea::to_string() const
{
    std::ostringstream ss;
    ss << "PolygonArea[";
    for (size_t i = 0; i < corners.size(); ++i)
    {
        if (i > 0) ss << ", ";
        ss << corners[i].to_string();
    }
    ss << "]";
    return ss.str();
}

bool PolygonArea::intersects(const Shape& other) const
{
    if (is_empty())
        return false;

    IntBox this_box = bounding_box();
    IntBox other_box = other.bounding_box();

    if (!this_box.intersects(other_box))
        return false;

    for (const auto& corner : corners)
    {
        if (other.contains(corner))
            return true;
    }
    return false;
}

} // namespace planar
} // namespace geometry
} // namespace freerouting