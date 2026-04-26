#include "geometry/planar/Vector.h"
#include "geometry/planar/IntBox.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <sstream>

namespace freerouting {
namespace geometry {
namespace planar {

// ============== IntVector Implementation ==============

IntVector::IntVector(int64_t p_x, int64_t p_y)
    : x(p_x), y(p_y)
{
}

std::unique_ptr<Point> IntVector::add_to(const IntPoint& p_point) const
{
    return std::make_unique<IntPoint>(p_point.x + x, p_point.y + y);
}

std::unique_ptr<Vector> IntVector::add(const Vector& p_other) const
{
    const IntVector* other_int = dynamic_cast<const IntVector*>(&p_other);
    if (other_int)
    {
        return std::make_unique<IntVector>(x + other_int->x, y + other_int->y);
    }
    throw std::runtime_error("Vector::add: not implemented for non-IntVector");
}

std::unique_ptr<Vector> IntVector::negate() const
{
    return std::make_unique<IntVector>(-x, -y);
}

int64_t IntVector::determinant(const Vector& p_other) const
{
    const IntVector* other_int = dynamic_cast<const IntVector*>(&p_other);
    if (other_int)
    {
        return determinant(*other_int);
    }
    throw std::runtime_error("Vector::determinant: not implemented for non-IntVector");
}

int64_t IntVector::determinant(const IntVector& p_other) const
{
    return x * p_other.y - y * p_other.x;
}

double IntVector::length_approx() const
{
    return std::sqrt(static_cast<double>(x * x + y * y));
}

Side IntVector::side_of(const Vector& p_other) const
{
    const IntVector* other_int = dynamic_cast<const IntVector*>(&p_other);
    if (other_int)
    {
        int64_t det = determinant(*other_int);
        if (det > 0) return Side::ON_THE_LEFT;
        if (det < 0) return Side::ON_THE_RIGHT;
        return Side::COLLINEAR;
    }
    return Side::NO_SIDE;
}

FloatPoint IntVector::to_float() const
{
    return FloatPoint(static_cast<double>(x), static_cast<double>(y));
}

IntVector IntVector::operator+(const IntVector& other) const
{
    return IntVector(x + other.x, y + other.y);
}

IntVector IntVector::operator-() const
{
    return IntVector(-x, -y);
}

std::string IntVector::to_string() const
{
    std::ostringstream ss;
    ss << "IntVector(" << x << ", " << y << ")";
    return ss.str();
}

bool IntVector::is_orthogonal() const
{
    return x == 0 || y == 0;
}

bool IntVector::is_diagonal() const
{
    return std::abs(x) == std::abs(y);
}

IntVector IntVector::turn_90_degree(int p_factor) const
{
    int n = p_factor;
    while (n < 0) n += 4;
    while (n >= 4) n -= 4;

    switch (n)
    {
        case 0: return IntVector(x, y);
        case 1: return IntVector(-y, x);
        case 2: return IntVector(-x, -y);
        case 3: return IntVector(y, -x);
        default: return IntVector(0, 0);
    }
}

IntVector IntVector::mirror_at_y_axis() const
{
    return IntVector(-x, y);
}

IntVector IntVector::mirror_at_x_axis() const
{
    return IntVector(x, -y);
}

double IntVector::scalar_product(const IntVector& p_other) const
{
    return static_cast<double>(x) * static_cast<double>(p_other.x) + static_cast<double>(y) * static_cast<double>(p_other.y);
}

// ============== Line Implementation ==============

Line::Line(const IntPoint& p_a, const IntPoint& p_b)
    : a(p_a), b(p_b)
{
}

bool Line::is_vertical() const
{
    return a.x == b.x;
}

bool Line::is_horizontal() const
{
    return a.y == b.y;
}

bool Line::is_parallel_to(const Line& p_other) const
{
    return direction().determinant(p_other.direction()) == 0;
}

bool Line::is_perpendicular_to(const Line& p_other) const
{
    const IntVector d1 = direction();
    const IntVector d2 = p_other.direction();
    return d1.x * d2.x + d1.y * d2.y == 0;
}

bool Line::overlaps(const Line& p_other) const
{
    return side_of(p_other.a) == Side::COLLINEAR && side_of(p_other.b) == Side::COLLINEAR;
}

IntVector Line::direction() const
{
    return IntVector(b.x - a.x, b.y - a.y);
}

double Line::length_approx() const
{
    return direction().length_approx();
}

int64_t Line::distance_square(const IntPoint& p_point) const
{
    return side_of(p_point) == Side::COLLINEAR ? 0 : 1;
}

int64_t Line::distance(const IntPoint& p_point) const
{
    return static_cast<int64_t>(std::sqrt(static_cast<double>(distance_square(p_point))));
}

double Line::signed_distance(const FloatPoint& p_point) const
{
    const double dx = static_cast<double>(b.x - a.x);
    const double dy = static_cast<double>(b.y - a.y);
    const double det = dy * (p_point.x - static_cast<double>(a.x)) - dx * (p_point.y - static_cast<double>(a.y));
    const double len = std::sqrt(dx * dx + dy * dy);
    if (len == 0.0)
    {
        return 0.0;
    }
    return det / len;
}

FloatPoint Line::intersection_approx(const Line& p_other) const
{
    const double d1x = static_cast<double>(b.x - a.x);
    const double d1y = static_cast<double>(b.y - a.y);
    const double d2x = static_cast<double>(p_other.b.x - p_other.a.x);
    const double d2y = static_cast<double>(p_other.b.y - p_other.a.y);
    const double det1 = static_cast<double>(a.x) * static_cast<double>(b.y) - static_cast<double>(a.y) * static_cast<double>(b.x);
    const double det2 = static_cast<double>(p_other.a.x) * static_cast<double>(p_other.b.y)
                      - static_cast<double>(p_other.a.y) * static_cast<double>(p_other.b.x);
    const double det = d2x * d1y - d2y * d1x;
    if (det == 0.0)
    {
        return FloatPoint(static_cast<double>(CRIT_INT), static_cast<double>(CRIT_INT));
    }
    const double is_x = (d2x * det1 - d1x * det2) / det;
    const double is_y = (d2y * det1 - d1y * det2) / det;
    return FloatPoint(is_x, is_y);
}

Side Line::side_of(const IntPoint& p_point) const
{
    IntVector v1(p_point.x - a.x, p_point.y - a.y);
    IntVector v2 = direction();
    int64_t det = v1.determinant(v2);
    if (det > 0) return Side::ON_THE_LEFT;
    if (det < 0) return Side::ON_THE_RIGHT;
    return Side::COLLINEAR;
}

std::string Line::to_string() const
{
    std::ostringstream ss;
    ss << "Line[" << a.to_string() << " -> " << b.to_string() << "]";
    return ss.str();
}

bool Line::is_parallel(const Line& p_other) const
{
    return direction().side_of(p_other.direction()) == Side::COLLINEAR;
}

bool Line::is_perpendicular(const Line& p_other) const
{
    return direction().scalar_product(p_other.direction()) == 0;
}

bool Line::is_equal_or_opposite(const Line& p_other) const
{
    return side_of(p_other.a) == Side::COLLINEAR && side_of(p_other.b) == Side::COLLINEAR;
}

Line Line::opposite() const
{
    return Line(b, a);
}

std::unique_ptr<Point> Line::intersection(const Line& p_other) const
{
    // Simplified implementation for IntPoints
    IntVector delta_1 = direction();
    IntVector delta_2 = p_other.direction();

    if (delta_1.x == 0) // this line is vertical
    {
        if (delta_2.y == 0) // other line is horizontal
        {
            return std::make_unique<IntPoint>(a.x, p_other.a.y);
        }
    }
    else if (delta_1.y == 0) // this line is horizontal
    {
        if (delta_2.x == 0) // other line is vertical
        {
            return std::make_unique<IntPoint>(p_other.a.x, a.y);
        }
    }

    // Fall back to approximate intersection for other cases
    auto approx = intersection_approx(p_other);
    if (approx.x == CRIT_INT)
    {
        return nullptr; // Lines are parallel
    }
    return std::make_unique<FloatPoint>(approx.x, approx.y);
}

double Line::function_value_approx(double p_x) const
{
    FloatPoint p1 = a.to_float();
    FloatPoint p2 = b.to_float();
    double dx = p2.x - p1.x;
    if (dx == 0.0)
    {
        return 0.0;
    }
    double dy = p2.y - p1.y;
    double det = p1.x * p2.y - p2.x * p1.y;
    return (dy * p_x - det) / dx;
}

} // namespace planar
} // namespace geometry
} // namespace freerouting