#include "geometry/planar/Point.h"
#include "geometry/planar/Limits.h"
#include "geometry/planar/Vector.h"
#include "geometry/planar/Rational.h"

#include <cmath>
#include <memory>

namespace freerouting {
namespace geometry {
namespace planar {

// Point class methods, which are implemented here because they are not
// contained in Geometry.cpp.

// Point static factory methods
std::unique_ptr<Point> Point::get_instance(int64_t p_x, int64_t p_y)
{
    if (std::abs(p_x) > Limits::CRIT_INT || std::abs(p_y) > Limits::CRIT_INT) {
        return std::make_unique<RationalPoint>(IntPoint(p_x, p_y));
    }
    return std::make_unique<IntPoint>(p_x, p_y);
}

std::unique_ptr<Point> Point::get_instance(int64_t p_x, int64_t p_y, int64_t p_z)
{
    if (p_z < 0) {
        // the denominator z of a RationalPoint is expected to be positive
        p_x = -p_x;
        p_y = -p_y;
        p_z = -p_z;
    }

    if (p_z != 0 && p_x % p_z == 0 && p_y % p_z == 0) {
        // p_x and p_y can be divided by p_z
        p_x = p_x / p_z;
        p_y = p_y / p_z;
        p_z = 1;
    }

    if (p_z == 1) {
        return std::make_unique<IntPoint>(p_x, p_y);
    }

    return std::make_unique<RationalPoint>(datastructures::BigInt(p_x),
                                           datastructures::BigInt(p_y),
                                           datastructures::BigInt(p_z));
}

Side IntPoint::side_of(const IntPoint& p_1, const IntPoint& p_2) const
{
    // Java parity: Point.side_of(p_1, p_2) returns the negated side of
    // difference vectors, i.e. ON_THE_LEFT iff (this-p_1) x (p_2-p_1) < 0.
    IntVector v1(x - p_1.x, y - p_1.y);
    IntVector direction(p_2.x - p_1.x, p_2.y - p_1.y);
    int64_t det = v1.determinant(direction);
    if (det < 0)
    {
        return Side::ON_THE_LEFT;
    }
    if (det > 0)
    {
        return Side::ON_THE_RIGHT;
    }
    return Side::COLLINEAR;
}

IntPoint IntPoint::turn_90_degree(int p_factor, const IntPoint& p_pole) const
{
    int n = p_factor % 4;
    if (n < 0)
    {
        n += 4;
    }
    int64_t dx = x - p_pole.x;
    int64_t dy = y - p_pole.y;
    int64_t new_dx;
    int64_t new_dy;
    switch (n)
    {
        case 0:
            new_dx = dx;
            new_dy = dy;
            break;
        case 1:
            new_dx = -dy;
            new_dy = dx;
            break;
        case 2:
            new_dx = -dx;
            new_dy = -dy;
            break;
        case 3:
            new_dx = dy;
            new_dy = -dx;
            break;
        default:
            new_dx = 0;
            new_dy = 0;
            break;
    }
    return IntPoint(p_pole.x + new_dx, p_pole.y + new_dy);
}

IntPoint IntPoint::mirror_vertical(const IntPoint& p_pole) const
{
    return IntPoint(2 * p_pole.x - x, y);
}

IntPoint IntPoint::mirror_horizontal(const IntPoint& p_pole) const
{
    return IntPoint(x, 2 * p_pole.y - y);
}

FloatPoint FloatPoint::turn_90_degree(int p_factor) const
{
    int n = p_factor;
    while (n < 0) {
        n += 4;
    }
    while (n >= 4) {
        n -= 4;
    }

    double new_x, new_y;
    switch (n) {
        case 0: // 0 degree
            new_x = x;
            new_y = y;
            break;
        case 1: // 90 degree
            new_x = -y;
            new_y = x;
            break;
        case 2: // 180 degree
            new_x = -x;
            new_y = -y;
            break;
        case 3: // 270 degree
            new_x = y;
            new_y = -x;
            break;
        default:
            new_x = 0;
            new_y = 0;
            break;
    }
    return FloatPoint(new_x, new_y);
}

bool Point::equals(const Point* p_other) const
{
    if (!p_other) return false;
    if (this == p_other) return true;
    return to_float() == p_other->to_float();
}

bool IntPoint::equals(const Point* p_other) const
{
    if (!p_other) return false;
    return equals(*p_other);
}

bool FloatPoint::equals(const Point* p_other) const
{
    if (!p_other) return false;
    return to_float() == p_other->to_float();
}

double FloatPoint::size() const
{
    return std::hypot(x, y);
}

} // namespace planar
} // namespace geometry
} // namespace freerouting