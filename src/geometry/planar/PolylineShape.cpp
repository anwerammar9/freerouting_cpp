#include "geometry/planar/PolylineShape.h"
#include "geometry/planar/FloatLine.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/Vector.h"

#include <limits>

namespace freerouting {
namespace geometry {
namespace planar {

std::vector<IntPoint> PolylineShape::bounded_corners() const
{
    std::vector<IntPoint> result;
    for (int i = 0; i < this->border_line_count(); ++i)
    {
        if (this->corner_is_bounded(i))
        {
            result.push_back(this->corner(i));
        }
    }
    return result;
}

FloatPoint PolylineShape::corner_approx(int p_no) const
{
    return this->corner(p_no).to_float();
}

std::vector<FloatPoint> PolylineShape::corner_approx_arr() const
{
    std::vector<FloatPoint> result;
    result.reserve(static_cast<size_t>(this->border_line_count()));
    for (int i = 0; i < this->border_line_count(); ++i)
    {
        result.push_back(this->corner_approx(i));
    }
    return result;
}

int PolylineShape::equals_corner(const IntPoint& p_point) const
{
    for (int i = 0; i < this->border_line_count(); ++i)
    {
        if (p_point == this->corner(i))
        {
            return i;
        }
    }
    return -1;
}

double PolylineShape::circumference() const
{
    if (!this->is_bounded())
    {
        return static_cast<double>(std::numeric_limits<int>::max());
    }
    int corner_count = this->border_line_count();
    double result = 0;
    FloatPoint prev_corner = this->corner_approx(corner_count - 1);
    for (int i = 0; i < corner_count; ++i)
    {
        FloatPoint curr_corner = this->corner_approx(i);
        result += curr_corner.distance(prev_corner);
        prev_corner = curr_corner;
    }
    return result;
}

FloatPoint PolylineShape::centre_of_gravity() const
{
    int corner_count = this->border_line_count();
    double x = 0;
    double y = 0;
    for (int i = 0; i < corner_count; ++i)
    {
        FloatPoint curr_point = this->corner_approx(i);
        x += curr_point.x;
        y += curr_point.y;
    }
    x /= corner_count;
    y /= corner_count;
    return FloatPoint(x, y);
}

bool PolylineShape::is_contained_in(const IntBox& p_box) const
{
    return p_box.contains(this->bounding_box());
}

int PolylineShape::index_of_left_most_corner(const FloatPoint& p_from_point) const
{
    FloatPoint left_most_corner = this->corner_approx(0);
    int corner_count = this->border_line_count();
    int result = 0;
    for (int i = 1; i < corner_count; ++i)
    {
        FloatPoint curr_corner = this->corner_approx(i);
        if (curr_corner.side_of(p_from_point, left_most_corner) == Side::ON_THE_LEFT)
        {
            left_most_corner = curr_corner;
            result = i;
        }
    }
    return result;
}

int PolylineShape::index_of_right_most_corner(const FloatPoint& p_from_point) const
{
    FloatPoint right_most_corner = this->corner_approx(0);
    int corner_count = this->border_line_count();
    int result = 0;
    for (int i = 1; i < corner_count; ++i)
    {
        FloatPoint curr_corner = this->corner_approx(i);
        if (curr_corner.side_of(p_from_point, right_most_corner) == Side::ON_THE_RIGHT)
        {
            right_most_corner = curr_corner;
            result = i;
        }
    }
    return result;
}

std::optional<FloatLine> PolylineShape::polar_line_segment(const FloatPoint& p_from_point) const
{
    if (this->is_empty())
    {
        return std::nullopt;
    }
    FloatPoint left_most_corner = this->corner_approx(0);
    FloatPoint right_most_corner = this->corner_approx(0);
    int corner_count = this->border_line_count();
    for (int i = 1; i < corner_count; ++i)
    {
        FloatPoint curr_corner = this->corner_approx(i);
        if (curr_corner.side_of(p_from_point, right_most_corner) == Side::ON_THE_RIGHT)
        {
            right_most_corner = curr_corner;
        }
        if (curr_corner.side_of(p_from_point, left_most_corner) == Side::ON_THE_LEFT)
        {
            left_most_corner = curr_corner;
        }
    }
    return FloatLine(left_most_corner, right_most_corner);
}

int PolylineShape::prev_no(int p_no) const
{
    if (p_no == 0)
    {
        return this->border_line_count() - 1;
    }
    return p_no - 1;
}

int PolylineShape::next_no(int p_no) const
{
    return (p_no + 1) % this->border_line_count();
}

bool PolylineShape::intersects(const Line& p_line) const
{
    Side side_of_first_corner = p_line.side_of(this->corner(0));
    if (side_of_first_corner == Side::COLLINEAR)
    {
        return true;
    }
    for (int i = 1; i < this->border_line_count(); ++i)
    {
        if (p_line.side_of(this->corner(i)) != side_of_first_corner)
        {
            return true;
        }
    }
    return false;
}

IntPoint PolylineShape::left_most_corner(const IntPoint& p_from_point) const
{
    if (this->is_empty())
    {
        return p_from_point;
    }
    IntPoint result = this->corner(0);
    int corner_count = this->border_line_count();
    for (int i = 1; i < corner_count; ++i)
    {
        IntPoint curr_corner = this->corner(i);
        if (curr_corner.side_of(p_from_point, result) == Side::ON_THE_LEFT)
        {
            result = curr_corner;
        }
    }
    return result;
}

IntPoint PolylineShape::right_most_corner(const IntPoint& p_from_point) const
{
    if (this->is_empty())
    {
        return p_from_point;
    }
    IntPoint result = this->corner(0);
    int corner_count = this->border_line_count();
    for (int i = 1; i < corner_count; ++i)
    {
        IntPoint curr_corner = this->corner(i);
        if (curr_corner.side_of(p_from_point, result) == Side::ON_THE_RIGHT)
        {
            result = curr_corner;
        }
    }
    return result;
}

} // namespace planar
} // namespace geometry
} // namespace freerouting