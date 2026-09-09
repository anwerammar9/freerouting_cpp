#include "geometry/planar/Vector.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/Direction.h"
#include "geometry/planar/Limits.h"
#include "geometry/planar/Rational.h"
#include "datastructures/Signum.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>
#include <sstream>

namespace freerouting {
namespace geometry {
namespace planar {

std::unique_ptr<Vector> Vector::get_instance(int64_t p_x, int64_t p_y)
{
    // Java parity (Vector.get_instance): coordinates beyond Limits::CRIT_INT
    // are represented by a RationalVector (values between CRIT_INT and the
    // 64-bit range would fit into an IntVector, but Java's range check is
    // preserved here for exact behavioral parity).
    if (std::abs(p_x) > Limits::CRIT_INT || std::abs(p_y) > Limits::CRIT_INT)
    {
        return std::make_unique<RationalVector>(
            datastructures::BigInt(p_x), datastructures::BigInt(p_y), datastructures::BigInt(1));
    }
    return std::make_unique<IntVector>(p_x, p_y);
}

namespace {
// Java parity: Integer.MAX_VALUE is the parallel-lines sentinel of
// Line.intersection_approx.
constexpr int64_t INTEGER_MAX_VALUE = 2147483647;
}

// ============== IntVector Implementation ==============

IntVector IntVector::change_length_approx(double p_length) const
{
    const FloatPoint new_point = to_float().change_size(p_length);
    const IntPoint rounded = new_point.round();
    return IntVector(rounded.x, rounded.y);
}

const IntVector IntVector::ZERO(0, 0);

IntVector::IntVector(int64_t p_x, int64_t p_y)
    : x(p_x), y(p_y)
{
}

bool IntVector::operator==(const IntVector& other) const
{
    return x == other.x && y == other.y;
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
    const RationalVector* other_rat = dynamic_cast<const RationalVector*>(&p_other);
    if (other_rat)
    {
        return other_rat->add(*this);
    }
    throw std::runtime_error("Vector::add: not implemented for this vector type");
}

std::unique_ptr<Vector> IntVector::negate() const
{
    return std::make_unique<IntVector>(-x, -y);
}

datastructures::BigInt IntVector::determinant(const Vector& p_other) const
{
    const IntVector* other_int = dynamic_cast<const IntVector*>(&p_other);
    if (other_int)
    {
        return datastructures::BigInt(determinant(*other_int));
    }
    const RationalVector* other_rat = dynamic_cast<const RationalVector*>(&p_other);
    if (other_rat)
    {
        // det(this, p_other) = -det(p_other, this)
        return -other_rat->determinant(*this);
    }
    throw std::runtime_error("Vector::determinant: not implemented for this vector type");
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
        // Java parity: IntVector.side_of(Vector) returns the negated side of the
        // other direction, i.e. ON_THE_LEFT iff det(this, p_other) < 0.
        int64_t det = determinant(*other_int);
        if (det < 0) return Side::ON_THE_LEFT;
        if (det > 0) return Side::ON_THE_RIGHT;
        return Side::COLLINEAR;
    }
    const RationalVector* other_rat = dynamic_cast<const RationalVector*>(&p_other);
    if (other_rat)
    {
        // side(this, other) = -side(other, this)
        Side tmp = other_rat->side_of(*this);
        if (tmp == Side::ON_THE_LEFT) return Side::ON_THE_RIGHT;
        if (tmp == Side::ON_THE_RIGHT) return Side::ON_THE_LEFT;
        return tmp;
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

datastructures::Signum IntVector::projection(const IntVector& p_other) const
{
    double tmp = static_cast<double>(x) * p_other.x + static_cast<double>(y) * p_other.y;
    return datastructures::Signum::of(tmp);
}

double IntVector::scalar_product(const Vector& p_other) const
{
    const IntVector* other_int = dynamic_cast<const IntVector*>(&p_other);
    if (other_int != nullptr)
    {
        return scalar_product(*other_int);
    }
    const RationalVector* other_rat = dynamic_cast<const RationalVector*>(&p_other);
    if (other_rat != nullptr)
    {
        return other_rat->scalar_product(*this);
    }
    throw std::runtime_error("IntVector::scalar_product: not implemented for this vector type");
}

double Vector::cos_angle(const Vector& p_other) const
{
    double denominator = length_approx() * p_other.length_approx();
    if (denominator == 0.0)
    {
        return 0.0;
    }
    double result = scalar_product(p_other);
    result /= denominator;
    return result;
}

double Vector::angle_approx(const Vector& p_other) const
{
    double cos_value = cos_angle(p_other);
    if (cos_value > 1.0)
    {
        cos_value = 1.0;
    }
    if (cos_value < -1.0)
    {
        cos_value = -1.0;
    }
    double result = std::acos(cos_value);
    if (side_of(p_other) == Side::ON_THE_LEFT)
    {
        result = -result;
    }
    return result;
}

double Vector::angle_approx() const
{
    IntVector other(1, 0);
    return other.angle_approx(*this);
}

std::unique_ptr<Direction> IntVector::to_normalized_direction() const
{
    int64_t dx = x;
    int64_t dy = y;
    int64_t g = std::gcd(std::abs(dx), std::abs(dy));
    if (g > 1)
    {
        dx /= g;
        dy /= g;
    }
    return std::make_unique<IntDirection>(dx, dy);
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
        return FloatPoint(static_cast<double>(INTEGER_MAX_VALUE), static_cast<double>(INTEGER_MAX_VALUE));
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

Line Line::get_instance(const IntPoint& p_from, const IntDirection& p_direction)
{
    return Line(p_from, IntPoint(p_from.x + p_direction.x, p_from.y + p_direction.y));
}

Side Line::side_of(const FloatPoint& p_point, double p_tolerance) const
{
    const double dx = static_cast<double>(b.x - a.x);
    const double dy = static_cast<double>(b.y - a.y);
    const double det = dy * (p_point.x - static_cast<double>(a.x))
                     - dx * (p_point.y - static_cast<double>(a.y));
    if (det - p_tolerance > 0.0)
    {
        return Side::ON_THE_LEFT;
    }
    if (det + p_tolerance < 0.0)
    {
        return Side::ON_THE_RIGHT;
    }
    return Side::COLLINEAR;
}

Side Line::side_of(const FloatPoint& p_point) const
{
    return side_of(p_point, 0.0);
}

Side Line::side_of(const Point& p_point) const
{
    if (const IntPoint* ip = dynamic_cast<const IntPoint*>(&p_point))
    {
        return side_of(*ip);
    }
    if (const FloatPoint* fp = dynamic_cast<const FloatPoint*>(&p_point))
    {
        return side_of(*fp);
    }
    if (const RationalPoint* rp = dynamic_cast<const RationalPoint*>(&p_point))
    {
        return rp->side_of(*this);
    }
    return side_of(p_point.to_float());
}

Side Line::side_of_intersection(const Line& p_other_1, const Line& p_other_2) const
{
    FloatPoint intersection_approx = p_other_1.intersection_approx(p_other_2);
    Side result = side_of(intersection_approx, 1.0);
    if (result == Side::COLLINEAR)
    {
        // Previous calculation was with FloatPoints and a tolerance
        // for performance reasons. Make an exact check for
        // collinearity now with class Point instead of FloatPoint.
        std::unique_ptr<Point> intersection = p_other_1.intersection(p_other_2);
        if (intersection)
        {
            const IntPoint* int_point = dynamic_cast<const IntPoint*>(intersection.get());
            const FloatPoint* float_point = dynamic_cast<const FloatPoint*>(intersection.get());
            if (int_point != nullptr)
            {
                result = side_of(*int_point);
            }
            else if (float_point != nullptr)
            {
                result = side_of(*float_point);
            }
        }
        else
        {
            result = Side::NO_SIDE;
        }
    }
    return result;
}

Line Line::translate(double p_dist) const
{
    if (p_dist == 0.0)
    {
        return *this;
    }
    const IntVector v = direction();
    const double vxvx = static_cast<double>(v.x) * static_cast<double>(v.x);
    const double vyvy = static_cast<double>(v.y) * static_cast<double>(v.y);
    const double length = std::sqrt(vxvx + vyvy);
    int64_t new_a_x = a.x;
    int64_t new_a_y = a.y;
    if (vxvx <= vyvy)
    {
        // translate along the x axis
        int64_t rel_x = static_cast<int64_t>(std::floor((p_dist * length) / static_cast<double>(v.y) + 0.5));
        new_a_x = a.x - rel_x;
    }
    else
    {
        // translate along the y axis
        int64_t rel_y = static_cast<int64_t>(std::floor((p_dist * length) / static_cast<double>(v.x) + 0.5));
        new_a_y = a.y + rel_y;
    }
    return get_instance(IntPoint(new_a_x, new_a_y), IntDirection(v.x, v.y));
}

Line Line::translate_by(const IntVector& p_offset) const
{
    if (p_offset.x == 0 && p_offset.y == 0)
    {
        return *this;
    }
    return Line(IntPoint(a.x + p_offset.x, a.y + p_offset.y),
                IntPoint(b.x + p_offset.x, b.y + p_offset.y));
}

Line Line::turn_90_degree(int p_factor, const IntPoint& p_pole) const
{
    return Line(a.turn_90_degree(p_factor, p_pole), b.turn_90_degree(p_factor, p_pole));
}

Line Line::mirror_vertical(const IntPoint& p_pole) const
{
    return Line(b.mirror_vertical(p_pole), a.mirror_vertical(p_pole));
}

Line Line::mirror_horizontal(const IntPoint& p_pole) const
{
    return Line(b.mirror_horizontal(p_pole), a.mirror_horizontal(p_pole));
}

bool Line::fast_equals(const Line& p_other) const
{
    const double dx1 = static_cast<double>(p_other.a.x - a.x);
    const double dy1 = static_cast<double>(p_other.a.y - a.y);
    const double dx2 = static_cast<double>(b.x - a.x);
    const double dy2 = static_cast<double>(b.y - a.y);
    const double det = dx1 * dy2 - dx2 * dy1;
    if (det != 0.0)
    {
        return false;
    }
    const IntVector d1 = direction();
    const IntVector d2 = p_other.direction();
    return d1.x == d2.x && d1.y == d2.y;
}

bool Line::equals(const Line& p_other) const
{
    return fast_equals(p_other);
}

bool Line::is_orthogonal() const
{
    return direction().is_orthogonal();
}

bool Line::is_multiple_of_45_degree() const
{
    return direction().is_orthogonal() || direction().is_diagonal();
}

int Line::compare_by_direction(const Line& p_other) const
{
    const int64_t dx1 = b.x - a.x;
    const int64_t dy1 = b.y - a.y;
    const int64_t dx2 = p_other.b.x - p_other.a.x;
    const int64_t dy2 = p_other.b.y - p_other.a.y;
    if (dy1 > 0)
    {
        if (dy2 < 0)
        {
            return -1;
        }
        if (dy2 == 0)
        {
            if (dx2 > 0)
            {
                return 1;
            }
            return -1;
        }
    }
    else if (dy1 < 0)
    {
        if (dy2 >= 0)
        {
            return 1;
        }
    }
    else // dy1 == 0
    {
        if (dx1 > 0)
        {
            if (dy2 != 0 || dx2 < 0)
            {
                return -1;
            }
            return 0;
        }
        // dx1 < 0 (null direction lines sort before everything positive)
        if (dy2 > 0 || (dy2 == 0 && dx2 > 0))
        {
            return 1;
        }
        if (dy2 < 0)
        {
            return -1;
        }
        return 0;
    }

    // now this direction and p_other are located in the same
    // open horizontal half plane
    const double determinant = static_cast<double>(dx2) * static_cast<double>(dy1)
                             - static_cast<double>(dy2) * static_cast<double>(dx1);
    return freerouting::datastructures::Signum::as_int(determinant);
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
    IntVector delta_1 = direction();
    IntVector delta_2 = p_other.direction();

    // Separate handling for orthogonal and 45 degree lines for performance
    if (delta_1.x == 0) // this line is vertical
    {
        if (delta_2.y == 0) // other line is horizontal
        {
            return std::make_unique<IntPoint>(a.x, p_other.a.y);
        }
        if (delta_2.x == delta_2.y) // other line is right diagonal
        {
            return std::make_unique<IntPoint>(a.x, p_other.a.y + a.x - p_other.a.x);
        }
        if (delta_2.x == -delta_2.y) // other line is left diagonal
        {
            return std::make_unique<IntPoint>(a.x, p_other.a.y + p_other.a.x - a.x);
        }
    }
    else if (delta_1.y == 0) // this line is horizontal
    {
        if (delta_2.x == 0) // other line is vertical
        {
            return std::make_unique<IntPoint>(p_other.a.x, a.y);
        }
        if (delta_2.x == delta_2.y) // other line is right diagonal
        {
            return std::make_unique<IntPoint>(p_other.a.x + a.y - p_other.a.y, a.y);
        }
        if (delta_2.x == -delta_2.y) // other line is left diagonal
        {
            return std::make_unique<IntPoint>(p_other.a.x + p_other.a.y - a.y, a.y);
        }
    }
    else if (delta_1.x == delta_1.y) // this line is right diagonal
    {
        if (delta_2.x == 0) // other line is vertical
        {
            return std::make_unique<IntPoint>(p_other.a.x, a.y + p_other.a.x - a.x);
        }
        if (delta_2.y == 0) // other line is horizontal
        {
            return std::make_unique<IntPoint>(a.x + p_other.a.y - a.y, p_other.a.y);
        }
    }
    else if (delta_1.x == -delta_1.y) // this line is left diagonal
    {
        if (delta_2.x == 0) // other line is vertical
        {
            return std::make_unique<IntPoint>(p_other.a.x, a.y + a.x - p_other.a.x);
        }
        if (delta_2.y == 0) // other line is horizontal
        {
            return std::make_unique<IntPoint>(a.x + a.y - p_other.a.y, p_other.a.y);
        }
    }

    // General case. Computed with int64; the coordinates are expected to
    // stay inside Limits::CRIT_INT so the products do not overflow.
    int64_t det_1 = a.x * b.y - a.y * b.x;
    int64_t det_2 = p_other.a.x * p_other.b.y - p_other.a.y * p_other.b.x;
    int64_t det = delta_2.x * delta_1.y - delta_2.y * delta_1.x;
    int64_t is_x = det_1 * delta_2.x - det_2 * delta_1.x;
    int64_t is_y = det_1 * delta_2.y - det_2 * delta_1.y;
    if (det != 0)
    {
        if (det < 0)
        {
            det = -det;
            is_x = -is_x;
            is_y = -is_y;
        }
        if (is_x % det == 0 && is_y % det == 0)
        {
            is_x /= det;
            is_y /= det;
            if (std::abs(is_x) <= Limits::CRIT_INT && std::abs(is_y) <= Limits::CRIT_INT)
            {
                return std::make_unique<IntPoint>(is_x, is_y);
            }
        }
    }
    // Not an exact integer intersection (Java would return a RationalPoint
    // which is deferred). Fall back to an approximate result.
    FloatPoint approx = intersection_approx(p_other);
    if (approx.x == static_cast<double>(INTEGER_MAX_VALUE))
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

double Line::function_in_y_value_approx(double p_y) const
{
    FloatPoint p1 = a.to_float();
    FloatPoint p2 = b.to_float();
    double dy = p2.y - p1.y;
    if (dy == 0.0)
    {
        return 0.0;
    }
    double dx = p2.x - p1.x;
    double det = p1.x * p2.y - p2.x * p1.y;
    return (dx * p_y + det) / dy;
}

std::optional<IntDirection> Line::perpendicular_direction(const IntPoint& p_from_point) const
{
    const Side line_side = side_of(p_from_point);
    if (line_side == Side::COLLINEAR)
    {
        return std::nullopt;
    }

    IntVector d = direction();
    int64_t g = std::gcd(std::llabs(d.x), std::llabs(d.y));
    if (g == 0)
    {
        return std::nullopt;
    }
    IntDirection line_dir(d.x / g, d.y / g);
    const IntDirection dir1 = line_dir.turn_45_degree_int(2);
    const IntDirection dir2 = line_dir.turn_45_degree_int(6);

    const IntVector v1(dir1.x, dir1.y);
    const IntPoint check_point_1(p_from_point.x + v1.x, p_from_point.y + v1.y);
    if (side_of(check_point_1) != line_side)
    {
        return dir1;
    }

    const IntVector v2(dir2.x, dir2.y);
    const IntPoint check_point_2(p_from_point.x + v2.x, p_from_point.y + v2.y);
    if (side_of(check_point_2) != line_side)
    {
        return dir2;
    }

    const FloatPoint nearest_line_point = p_from_point.to_float().projection_approx(*this);
    const FloatPoint fp1(static_cast<double>(check_point_1.x), static_cast<double>(check_point_1.y));
    const FloatPoint fp2(static_cast<double>(check_point_2.x), static_cast<double>(check_point_2.y));
    if (nearest_line_point.distance_square(fp1) <= nearest_line_point.distance_square(fp2))
    {
        return dir1;
    }
    return dir2;
}

} // namespace planar
} // namespace geometry
} // namespace freerouting