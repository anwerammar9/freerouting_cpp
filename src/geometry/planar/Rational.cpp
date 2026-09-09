#include "geometry/planar/Rational.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/IntOctagon.h"
#include "geometry/planar/Limits.h"
#include "geometry/planar/Direction.h"
#include "logger/FRLogger.h"

#include <cmath>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace freerouting {
namespace geometry {
namespace planar {

namespace {

Side side_of_determinant(const datastructures::BigInt& p_det)
{
    // Java parity: IntVector.side_of(IntVector) / RationalVector.side_of(...)
    // return Side.of(other.x * y - other.y * x), i.e. ON_THE_LEFT iff
    // x_this * y_other - y_this * x_other < 0.
    if (p_det < 0) return Side::ON_THE_LEFT;
    if (p_det > 0) return Side::ON_THE_RIGHT;
    return Side::COLLINEAR;
}

} // namespace

// ============== RationalVector Implementation ==============

RationalVector::RationalVector(const datastructures::BigInt& p_x,
                               const datastructures::BigInt& p_y,
                               const datastructures::BigInt& p_z)
    : x(p_z >= 0 ? p_x : -p_x),
      y(p_z >= 0 ? p_y : -p_y),
      z(p_z >= 0 ? p_z : -p_z)
{
}

RationalVector::RationalVector(const IntVector& p_vector)
    : x(p_vector.x), y(p_vector.y), z(1)
{
}

bool RationalVector::is_zero() const
{
    return x.sign() == 0 && y.sign() == 0;
}

std::unique_ptr<Vector> RationalVector::negate() const
{
    return std::make_unique<RationalVector>(-x, -y, z);
}

std::unique_ptr<Vector> RationalVector::add(const Vector& p_other) const
{
    if (const RationalVector* other_rat = dynamic_cast<const RationalVector*>(&p_other))
    {
        return add(*other_rat);
    }
    if (const IntVector* other_int = dynamic_cast<const IntVector*>(&p_other))
    {
        return add(*other_int);
    }
    throw std::runtime_error("RationalVector::add: not implemented for this vector type");
}

std::unique_ptr<Vector> RationalVector::add(const IntVector& p_other) const
{
    return add(RationalVector(p_other));
}

std::unique_ptr<Vector> RationalVector::add(const RationalVector& p_other) const
{
    std::array<datastructures::BigInt, 3> v1 = {x, y, z};
    std::array<datastructures::BigInt, 3> v2 = {p_other.x, p_other.y, p_other.z};
    std::array<datastructures::BigInt, 3> result =
        datastructures::BigIntAux::add_rational_coordinates(v1, v2);
    return std::make_unique<RationalVector>(result[0], result[1], result[2]);
}

Side RationalVector::side_of(const Vector& p_other) const
{
    if (const RationalVector* other_rat = dynamic_cast<const RationalVector*>(&p_other))
    {
        datastructures::BigInt det = x * other_rat->y - y * other_rat->x;
        return side_of_determinant(det);
    }
    if (const IntVector* other_int = dynamic_cast<const IntVector*>(&p_other))
    {
        datastructures::BigInt det =
            x * datastructures::BigInt(other_int->y) - y * datastructures::BigInt(other_int->x);
        return side_of_determinant(det);
    }
    return Side::NO_SIDE;
}

datastructures::BigInt RationalVector::determinant(const Vector& p_other) const
{
    if (const RationalVector* other_rat = dynamic_cast<const RationalVector*>(&p_other))
    {
        return determinant(*other_rat);
    }
    if (const IntVector* other_int = dynamic_cast<const IntVector*>(&p_other))
    {
        return determinant(*other_int);
    }
    throw std::runtime_error("RationalVector::determinant: not implemented for this vector type");
}

datastructures::BigInt RationalVector::determinant(const IntVector& p_other) const
{
    return x * datastructures::BigInt(p_other.y) - y * datastructures::BigInt(p_other.x);
}

datastructures::BigInt RationalVector::determinant(const RationalVector& p_other) const
{
    return x * p_other.y - y * p_other.x;
}

double RationalVector::scalar_product(const Vector& p_other) const
{
    if (const RationalVector* other_rat = dynamic_cast<const RationalVector*>(&p_other))
    {
        return scalar_product(*other_rat);
    }
    if (const IntVector* other_int = dynamic_cast<const IntVector*>(&p_other))
    {
        return scalar_product(*other_int);
    }
    throw std::runtime_error("RationalVector::scalar_product: not implemented for this vector type");
}

double RationalVector::scalar_product(const IntVector& p_other) const
{
    FloatPoint v1 = to_float();
    return v1.x * static_cast<double>(p_other.x) + v1.y * static_cast<double>(p_other.y);
}

double RationalVector::scalar_product(const RationalVector& p_other) const
{
    FloatPoint v1 = to_float();
    FloatPoint v2 = p_other.to_float();
    return v1.x * v2.x + v1.y * v2.y;
}

datastructures::Signum RationalVector::projection(const Vector& p_other) const
{
    if (const RationalVector* other_rat = dynamic_cast<const RationalVector*>(&p_other))
    {
        return projection(*other_rat);
    }
    if (const IntVector* other_int = dynamic_cast<const IntVector*>(&p_other))
    {
        return projection(*other_int);
    }
    return datastructures::Signum::ZERO;
}

datastructures::Signum RationalVector::projection(const IntVector& p_other) const
{
    datastructures::BigInt tmp =
        x * datastructures::BigInt(p_other.x) + y * datastructures::BigInt(p_other.y);
    return datastructures::Signum::of(tmp.sign());
}

datastructures::Signum RationalVector::projection(const RationalVector& p_other) const
{
    datastructures::BigInt tmp = x * p_other.x + y * p_other.y;
    return datastructures::Signum::of(tmp.sign());
}

FloatPoint RationalVector::to_float() const
{
    double xd = x.convert_to<double>();
    double yd = y.convert_to<double>();
    double zd = z.convert_to<double>();
    return FloatPoint(xd / zd, yd / zd);
}

double RationalVector::length_approx() const
{
    FloatPoint fp = to_float();
    return std::sqrt(fp.x * fp.x + fp.y * fp.y);
}

std::unique_ptr<Vector> RationalVector::change_length_approx(double p_length) const
{
    (void)p_length;
    FR_LOG_WARN("RationalVector: change_length_approx not yet implemented");
    return std::make_unique<RationalVector>(*this);
}

std::unique_ptr<Vector> RationalVector::turn_90_degree(int p_factor) const
{
    int n = p_factor % 4;
    if (n < 0)
    {
        n += 4;
    }
    datastructures::BigInt new_x;
    datastructures::BigInt new_y;
    switch (n)
    {
        case 1:
            new_x = -y;
            new_y = x;
            break;
        case 2:
            new_x = -x;
            new_y = -y;
            break;
        case 3:
            new_x = y;
            new_y = -x;
            break;
        default:
            return std::make_unique<RationalVector>(*this);
    }
    return std::make_unique<RationalVector>(new_x, new_y, z);
}

std::unique_ptr<Vector> RationalVector::mirror_at_y_axis() const
{
    return std::make_unique<RationalVector>(-x, y, z);
}

std::unique_ptr<Vector> RationalVector::mirror_at_x_axis() const
{
    return std::make_unique<RationalVector>(x, -y, z);
}

std::unique_ptr<Direction> RationalVector::to_normalized_direction() const
{
    datastructures::BigInt dx = x;
    datastructures::BigInt dy = y;
    datastructures::BigInt gcd = boost::multiprecision::gcd(dx, dy);
    dx /= gcd;
    dy /= gcd;
    datastructures::BigInt crit = datastructures::BigInt(Limits::CRIT_INT);
    if (dx >= -crit && dx <= crit && dy >= -crit && dy <= crit)
    {
        return std::make_unique<IntDirection>(dx.convert_to<int64_t>(), dy.convert_to<int64_t>());
    }
    return std::make_unique<BigIntDirection>(dx, dy);
}

std::unique_ptr<Point> RationalVector::add_to(const IntPoint& p_point) const
{
    datastructures::BigInt new_x = z * datastructures::BigInt(p_point.x) + x;
    datastructures::BigInt new_y = z * datastructures::BigInt(p_point.y) + y;
    return std::make_unique<RationalPoint>(new_x, new_y, z);
}

std::unique_ptr<Point> RationalVector::add_to(const RationalPoint& p_point) const
{
    std::array<datastructures::BigInt, 3> v1 = {x, y, z};
    std::array<datastructures::BigInt, 3> v2 = {p_point.x, p_point.y, p_point.z};
    std::array<datastructures::BigInt, 3> result =
        datastructures::BigIntAux::add_rational_coordinates(v1, v2);
    return std::make_unique<RationalPoint>(result[0], result[1], result[2]);
}

bool RationalVector::is_orthogonal() const
{
    return x.sign() == 0 || y.sign() == 0;
}

bool RationalVector::is_diagonal() const
{
    return boost::multiprecision::abs(x) == boost::multiprecision::abs(y);
}

// ============== RationalPoint Implementation ==============

RationalPoint::RationalPoint(const datastructures::BigInt& p_x,
                             const datastructures::BigInt& p_y,
                             const datastructures::BigInt& p_z)
    : x(p_x), y(p_y), z(p_z)
{
    if (p_z.sign() < 0)
    {
        throw std::invalid_argument("RationalPoint: p_z is expected to be >= 0");
    }
}

RationalPoint::RationalPoint(const IntPoint& p_point)
    : x(p_point.x), y(p_point.y), z(1)
{
}

FloatPoint RationalPoint::to_float() const
{
    double xd = x.convert_to<double>();
    double yd = y.convert_to<double>();
    double zd = z.convert_to<double>();
    if (zd == 0)
    {
        xd = static_cast<double>(std::numeric_limits<float>::max());
        yd = static_cast<double>(std::numeric_limits<float>::max());
    }
    else
    {
        xd /= zd;
        yd /= zd;
    }
    return FloatPoint(xd, yd);
}

bool RationalPoint::is_infinite() const
{
    return z.sign() == 0;
}

IntBox RationalPoint::surrounding_box() const
{
    FloatPoint fp = to_float();
    int64_t llx = static_cast<int64_t>(std::floor(fp.x));
    int64_t lly = static_cast<int64_t>(std::floor(fp.y));
    int64_t urx = static_cast<int64_t>(std::ceil(fp.x));
    int64_t ury = static_cast<int64_t>(std::ceil(fp.y));
    return IntBox(llx, lly, urx, ury);
}

IntOctagon RationalPoint::surrounding_octagon() const
{
    FloatPoint fp = to_float();
    int64_t lx = static_cast<int64_t>(std::floor(fp.x));
    int64_t ly = static_cast<int64_t>(std::floor(fp.y));
    int64_t rx = static_cast<int64_t>(std::ceil(fp.x));
    int64_t uy = static_cast<int64_t>(std::ceil(fp.y));

    double tmp = fp.x - fp.y;
    int64_t ulx = static_cast<int64_t>(std::floor(tmp));
    int64_t lrx = static_cast<int64_t>(std::ceil(tmp));

    tmp = fp.x + fp.y;
    int64_t llx = static_cast<int64_t>(std::floor(tmp));
    int64_t urx = static_cast<int64_t>(std::ceil(tmp));
    return IntOctagon(lx, ly, rx, uy, ulx, lrx, llx, urx);
}

bool RationalPoint::is_contained_in(const IntBox& p_box) const
{
    datastructures::BigInt tmp = datastructures::BigInt(p_box.ll.x) * z;
    if (x < tmp)
    {
        return false;
    }
    tmp = datastructures::BigInt(p_box.ll.y) * z;
    if (y < tmp)
    {
        return false;
    }
    tmp = datastructures::BigInt(p_box.ur.x) * z;
    if (x > tmp)
    {
        return false;
    }
    tmp = datastructures::BigInt(p_box.ur.y) * z;
    return y <= tmp;
}

bool RationalPoint::equals(const Point* p_other) const
{
    const RationalPoint* other = dynamic_cast<const RationalPoint*>(p_other);
    if (other == nullptr)
    {
        return false;
    }
    datastructures::BigInt det = datastructures::BigIntAux::determinant(x, other->x, z, other->z);
    if (det.sign() != 0)
    {
        return false;
    }
    det = datastructures::BigIntAux::determinant(y, other->y, z, other->z);
    return det.sign() == 0;
}

std::unique_ptr<Point> RationalPoint::translate_by(const Vector& p_vector) const
{
    if (p_vector.is_zero())
    {
        return std::make_unique<RationalPoint>(*this);
    }
    if (const RationalVector* vector_rat = dynamic_cast<const RationalVector*>(&p_vector))
    {
        return translate_by(*vector_rat);
    }
    if (const IntVector* vector_int = dynamic_cast<const IntVector*>(&p_vector))
    {
        return translate_by(*vector_int);
    }
    throw std::runtime_error("RationalPoint::translate_by: not implemented for this vector type");
}

std::unique_ptr<Point> RationalPoint::translate_by(const IntVector& p_vector) const
{
    return translate_by(RationalVector(p_vector));
}

std::unique_ptr<Point> RationalPoint::translate_by(const RationalVector& p_vector) const
{
    std::array<datastructures::BigInt, 3> v1 = {x, y, z};
    std::array<datastructures::BigInt, 3> v2 = {p_vector.x, p_vector.y, p_vector.z};
    std::array<datastructures::BigInt, 3> result =
        datastructures::BigIntAux::add_rational_coordinates(v1, v2);
    return std::make_unique<RationalPoint>(result[0], result[1], result[2]);
}

std::unique_ptr<Vector> RationalPoint::difference_by(const Point& p_other) const
{
    if (const RationalPoint* other_rat = dynamic_cast<const RationalPoint*>(&p_other))
    {
        return difference_by(*other_rat);
    }
    if (const IntPoint* other_int = dynamic_cast<const IntPoint*>(&p_other))
    {
        return difference_by(*other_int);
    }
    throw std::runtime_error("RationalPoint::difference_by: not implemented for this point type");
}

std::unique_ptr<Vector> RationalPoint::difference_by(const IntPoint& p_other) const
{
    RationalPoint other(p_other);
    return difference_by(other);
}

std::unique_ptr<Vector> RationalPoint::difference_by(const RationalPoint& p_other) const
{
    std::array<datastructures::BigInt, 3> v1 = {x, y, z};
    std::array<datastructures::BigInt, 3> v2 = {-p_other.x, -p_other.y, p_other.z};
    std::array<datastructures::BigInt, 3> result =
        datastructures::BigIntAux::add_rational_coordinates(v1, v2);
    return std::make_unique<RationalVector>(result[0], result[1], result[2]);
}

Side RationalPoint::side_of(const Line& p_line) const
{
    std::unique_ptr<Vector> v1 = difference_by(p_line.a);
    std::unique_ptr<Vector> v2 = p_line.b.difference_by(p_line.a);
    return v1->side_of(*v2);
}

std::unique_ptr<Point> RationalPoint::perpendicular_projection(const Line& p_line) const
{
    // Port of RationalPoint.perpendicular_projection(Line). It is at the moment
    // only implemented for lines consisting of IntPoints.
    std::unique_ptr<Vector> line_direction = p_line.b.difference_by(p_line.a);
    const IntVector* v = dynamic_cast<const IntVector*>(line_direction.get());
    if (v == nullptr)
    {
        throw std::runtime_error("RationalPoint::perpendicular_projection: line direction is not an IntVector");
    }
    datastructures::BigInt vxvx = datastructures::BigInt(v->x) * v->x;
    datastructures::BigInt vyvy = datastructures::BigInt(v->y) * v->y;
    datastructures::BigInt vxvy = datastructures::BigInt(v->x) * v->y;
    datastructures::BigInt denominator = vxvx + vyvy;
    datastructures::BigInt det =
        datastructures::BigInt(p_line.a.determinant(p_line.b));

    datastructures::BigInt tmp1 = vxvx * x;
    datastructures::BigInt tmp2 = vxvy * y;
    tmp1 += tmp2;
    tmp2 = det * v->y;
    tmp2 *= z;
    datastructures::BigInt proj_x = tmp1 + tmp2;

    tmp1 = vxvy * x;
    tmp2 = vyvy * y;
    tmp1 += tmp2;
    tmp2 = det * v->x;
    tmp2 *= z;
    datastructures::BigInt proj_y = tmp1 + tmp2;

    int signum = denominator.sign();
    if (signum != 0)
    {
        if (signum < 0)
        {
            denominator = -denominator;
            proj_x = -proj_x;
            proj_y = -proj_y;
        }
        if (proj_x % denominator == 0 && proj_y % denominator == 0)
        {
            proj_x /= denominator;
            proj_y /= denominator;
            datastructures::BigInt crit = datastructures::BigInt(Limits::CRIT_INT);
            if (proj_x >= -crit && proj_x <= crit && proj_y >= -crit && proj_y <= crit)
            {
                return std::make_unique<IntPoint>(proj_x.convert_to<int64_t>(),
                                                  proj_y.convert_to<int64_t>());
            }
            denominator = 1;
        }
    }
    return std::make_unique<RationalPoint>(proj_x, proj_y, denominator);
}

int RationalPoint::compare_x(const Point& p_other) const
{
    if (const RationalPoint* other_rat = dynamic_cast<const RationalPoint*>(&p_other))
    {
        return compare_x(*other_rat);
    }
    if (const IntPoint* other_int = dynamic_cast<const IntPoint*>(&p_other))
    {
        return compare_x(*other_int);
    }
    return 0;
}

int RationalPoint::compare_y(const Point& p_other) const
{
    if (const RationalPoint* other_rat = dynamic_cast<const RationalPoint*>(&p_other))
    {
        return compare_y(*other_rat);
    }
    if (const IntPoint* other_int = dynamic_cast<const IntPoint*>(&p_other))
    {
        return compare_y(*other_int);
    }
    return 0;
}

int RationalPoint::compare_x(const IntPoint& p_other) const
{
    datastructures::BigInt tmp1 = z * datastructures::BigInt(p_other.x);
    return x.compare(tmp1);
}

int RationalPoint::compare_y(const IntPoint& p_other) const
{
    datastructures::BigInt tmp1 = z * datastructures::BigInt(p_other.y);
    return y.compare(tmp1);
}

int RationalPoint::compare_x(const RationalPoint& p_other) const
{
    datastructures::BigInt tmp1 = x * p_other.z;
    datastructures::BigInt tmp2 = p_other.x * z;
    return tmp1.compare(tmp2);
}

int RationalPoint::compare_y(const RationalPoint& p_other) const
{
    datastructures::BigInt tmp1 = y * p_other.z;
    datastructures::BigInt tmp2 = p_other.y * z;
    return tmp1.compare(tmp2);
}

std::string RationalPoint::to_string() const
{
    std::ostringstream ss;
    ss << "RationalPoint[" << x << ", " << y << ", " << z << "]";
    return ss.str();
}

} // namespace planar
} // namespace geometry
} // namespace freerouting