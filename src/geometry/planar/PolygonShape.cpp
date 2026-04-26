#include "geometry/planar/PolygonShape.h"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace freerouting::geometry::planar {

PolygonShape::PolygonShape(const std::vector<IntPoint>& p_corners)
    : corners(p_corners)
{
}

IntBox PolygonShape::bounding_box() const
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

IntOctagon PolygonShape::bounding_octagon() const
{
    return bounding_box().bounding_octagon();
}

bool PolygonShape::intersects(const Shape& other) const
{
    IntBox this_box = bounding_box();
    IntBox other_box = other.bounding_box();
    return this_box.intersects(other_box);
}

bool PolygonShape::contains(const IntPoint& point) const
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

bool PolygonShape::corner_is_bounded(int) const
{
    return true;
}

IntPoint PolygonShape::corner(int p_no) const
{
    if (p_no >= 0 && p_no < border_line_count())
    {
        return corners[p_no];
    }
    return IntPoint(0, 0);
}

std::string PolygonShape::to_string() const
{
    std::ostringstream ss;
    ss << "PolygonShape[";
    for (size_t i = 0; i < corners.size(); ++i)
    {
        if (i > 0) ss << ", ";
        ss << corners[i].to_string();
    }
    ss << "]";
    return ss.str();
}

} // namespace freerouting::geometry::planar