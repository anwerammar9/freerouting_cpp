#include "geometry/planar/IntBox.h"
#include "geometry/planar/Vector.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <sstream>

namespace freerouting {
namespace geometry {
namespace planar {

// ============== IntOctagon Implementation ==============

IntOctagon::IntOctagon()
    : right(0), top(0), left(0), bottom(0),
      right_bottom(0), left_top(0), right_top(0), left_bottom(0)
{
}

IntOctagon::IntOctagon(int64_t p_right, int64_t p_top, int64_t p_left, int64_t p_bottom,
                       int64_t p_right_bottom, int64_t p_left_top, int64_t p_right_top, int64_t p_left_bottom)
    : right(p_right), top(p_top), left(p_left), bottom(p_bottom),
      right_bottom(p_right_bottom), left_top(p_left_top), right_top(p_right_top), left_bottom(p_left_bottom)
{
}

bool IntOctagon::contains(const IntPoint& p_point) const
{
    return p_point.x >= left && p_point.x <= right &&
           p_point.y >= bottom && p_point.y <= top &&
           p_point.x - p_point.y >= left_bottom && p_point.x - p_point.y <= right_top &&
           p_point.x + p_point.y >= right_bottom && p_point.x + p_point.y <= left_top;
}

IntBox IntOctagon::bounding_box() const
{
    return IntBox(left, bottom, right, top);
}

std::string IntOctagon::to_string() const
{
    std::ostringstream ss;
    ss << "IntOctagon[r=" << right << ", t=" << top << ", l=" << left << ", b=" << bottom << "]";
    return ss.str();
}

// ============== IntBox Implementation ==============

const IntBox IntBox::EMPTY(0, 0, -1, -1);
const IntBox IntBox::INFINITE(-2147483647LL, -2147483647LL, 2147483647LL, 2147483647LL);

IntBox::IntBox()
    : ll(0, 0), ur(-1, -1)
{
}

IntBox::IntBox(int64_t p_ll_x, int64_t p_ll_y, int64_t p_ur_x, int64_t p_ur_y)
    : ll(p_ll_x, p_ll_y), ur(p_ur_x, p_ur_y)
{
}

IntBox::IntBox(const IntPoint& p_ll, const IntPoint& p_ur)
    : ll(p_ll), ur(p_ur)
{
}

bool IntBox::contains(const IntPoint& p_point) const
{
    return p_point.x >= ll.x && p_point.y >= ll.y &&
           p_point.x <= ur.x && p_point.y <= ur.y;
}

bool IntBox::contains(const IntBox& p_box) const
{
    return contains(p_box.ll) && contains(p_box.ur);
}

bool IntBox::intersects(const IntBox& p_box) const
{
    return !(ur.x < p_box.ll.x || ll.x > p_box.ur.x ||
             ur.y < p_box.ll.y || ll.y > p_box.ur.y);
}

bool IntBox::intersects_approx(const IntBox& p_box, double p_precision) const
{
    double min_dx = std::abs(ur.x - p_box.ll.x);
    double min_dy = std::abs(ur.y - p_box.ll.y);
    return min_dx < p_precision || min_dy < p_precision;
}

IntPoint IntBox::center() const
{
    return IntPoint((ll.x + ur.x) / 2, (ll.y + ur.y) / 2);
}

int64_t IntBox::width() const
{
    return ur.x - ll.x;
}

int64_t IntBox::height() const
{
    return ur.y - ll.y;
}

int64_t IntBox::area() const
{
    return width() * height();
}

IntBox IntBox::intersection(const IntBox& p_box) const
{
    int64_t new_ll_x = std::max(ll.x, p_box.ll.x);
    int64_t new_ll_y = std::max(ll.y, p_box.ll.y);
    int64_t new_ur_x = std::min(ur.x, p_box.ur.x);
    int64_t new_ur_y = std::min(ur.y, p_box.ur.y);
    return IntBox(new_ll_x, new_ll_y, new_ur_x, new_ur_y);
}

IntBox IntBox::union_with(const IntBox& p_box) const
{
    int64_t new_ll_x = std::min(ll.x, p_box.ll.x);
    int64_t new_ll_y = std::min(ll.y, p_box.ll.y);
    int64_t new_ur_x = std::max(ur.x, p_box.ur.x);
    int64_t new_ur_y = std::max(ur.y, p_box.ur.y);
    return IntBox(new_ll_x, new_ll_y, new_ur_x, new_ur_y);
}

bool IntBox::operator==(const IntBox& other) const
{
    return ll == other.ll && ur == other.ur;
}

std::string IntBox::to_string() const
{
    std::ostringstream ss;
    ss << "IntBox[" << ll.to_string() << ", " << ur.to_string() << "]";
    return ss.str();
}

IntOctagon IntBox::bounding_octagon() const
{
    return IntOctagon(ur.x, ur.y, ll.x, ll.y,
                      ll.x - ur.y, ur.x - ll.y,
                      ll.x + ll.y, ur.x + ur.y);
}

} // namespace planar
} // namespace geometry
} // namespace freerouting