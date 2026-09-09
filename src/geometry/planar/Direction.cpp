#include "geometry/planar/Direction.h"
#include "geometry/planar/Vector.h"
#include "geometry/planar/Rational.h"
#include "datastructures/Signum.h"
#include "logger/FRLogger.h"
#include <cmath>
#include <algorithm>
#include <sstream>

namespace freerouting {
namespace geometry {
namespace planar {

bool Direction::is_multiple_of_45_degree() const
{
    return is_orthogonal() || is_diagonal();
}

bool Direction::equals(const Direction* p_other) const
{
    if (this == p_other)
    {
        return true;
    }
    if (p_other == nullptr)
    {
        return false;
    }
    const IntDirection* this_int = dynamic_cast<const IntDirection*>(this);
    const IntDirection* other_int = dynamic_cast<const IntDirection*>(p_other);
    if (this_int != nullptr && other_int != nullptr)
    {
        return this_int->x == other_int->x && this_int->y == other_int->y;
    }
    return false;
}

Side Direction::side_of(const Direction& p_other) const
{
    std::unique_ptr<Vector> v1 = this->get_vector();
    std::unique_ptr<Vector> v2 = p_other.get_vector();
    return v1->side_of(*v2);
}

freerouting::datastructures::Signum Direction::projection(const Direction& p_other) const
{
    std::unique_ptr<Vector> v1 = get_vector();
    std::unique_ptr<Vector> v2 = p_other.get_vector();
    return freerouting::datastructures::Signum::of(v1->scalar_product(*v2));
}

std::unique_ptr<Direction> Direction::middle_approx(const Direction& p_other) const
{
    FloatPoint v1 = get_vector()->to_float();
    FloatPoint v2 = p_other.get_vector()->to_float();
    double length1 = v1.size();
    double length2 = v2.size();
    if (length1 == 0.0 || length2 == 0.0)
    {
        return std::make_unique<IntDirection>(1, 0);
    }
    double x = v1.x / length1 + v2.x / length2;
    double y = v1.y / length1 + v2.y / length2;
    constexpr double scale_factor = 1000.0;
    IntVector vm(static_cast<int64_t>(std::lround(x * scale_factor)),
                  static_cast<int64_t>(std::lround(y * scale_factor)));
    return Direction::get_instance(vm);
}

int Direction::compare_from(const Direction& p_1, const Direction& p_2) const
{
    int result;
    const IntDirection* this_int = dynamic_cast<const IntDirection*>(this);
    const IntDirection* p1_int = dynamic_cast<const IntDirection*>(&p_1);
    const IntDirection* p2_int = dynamic_cast<const IntDirection*>(&p_2);
    const BigIntDirection* this_big = dynamic_cast<const BigIntDirection*>(this);
    const BigIntDirection* p1_big = dynamic_cast<const BigIntDirection*>(&p_1);
    const BigIntDirection* p2_big = dynamic_cast<const BigIntDirection*>(&p_2);
    if (this_int && p1_int && p2_int)
    {
        if (p1_int->compareTo(*this_int) >= 0)
        {
            if (p2_int->compareTo(*this_int) >= 0)
            {
                result = p1_int->compareTo(*p2_int);
            }
            else
            {
                result = -1;
            }
        }
        else
        {
            if (p2_int->compareTo(*this_int) >= 0)
            {
                result = 1;
            }
            else
            {
                result = p1_int->compareTo(*p2_int);
            }
        }
        return result;
    }

    if (this_big || p1_big || p2_big)
    {
        // Java parity: a direction is compared by converting it to its
        // BigIntDirection representation, which is exact for both
        // IntDirection and BigIntDirection.
        const BigIntDirection* this_ref = this_big ? this_big : nullptr;
        const BigIntDirection* p1_ref = p1_big ? p1_big : nullptr;
        const BigIntDirection* p2_ref = p2_big ? p2_big : nullptr;
        std::unique_ptr<BigIntDirection> this_owned;
        std::unique_ptr<BigIntDirection> p1_owned;
        std::unique_ptr<BigIntDirection> p2_owned;
        if (!this_ref && this_int) { this_owned = std::make_unique<BigIntDirection>(*this_int); this_ref = this_owned.get(); }
        if (!p1_ref && p1_int) { p1_owned = std::make_unique<BigIntDirection>(*p1_int); p1_ref = p1_owned.get(); }
        if (!p2_ref && p2_int) { p2_owned = std::make_unique<BigIntDirection>(*p2_int); p2_ref = p2_owned.get(); }
        if (p1_ref && p2_ref && this_ref)
        {
            if (p1_ref->compareTo(*this_ref) >= 0)
            {
                if (p2_ref->compareTo(*this_ref) >= 0)
                {
                    result = p1_ref->compareTo(*p2_ref);
                }
                else
                {
                    result = -1;
                }
            }
            else
            {
                if (p2_ref->compareTo(*this_ref) >= 0)
                {
                    result = 1;
                }
                else
                {
                    result = p1_ref->compareTo(*p2_ref);
                }
            }
            return result;
        }
    }

    const double a_this = angle_approx();
    double a1 = p_1.angle_approx() - a_this;
    double a2 = p_2.angle_approx() - a_this;
    if (a1 < 0) a1 += 2 * M_PI;
    if (a2 < 0) a2 += 2 * M_PI;
    if (a1 > a2) return 1;
    if (a1 < a2) return -1;
    return 0;
}

double Direction::angle_approx() const
{
    IntVector other(1, 0);
    return get_vector()->angle_approx(other);
}

std::unique_ptr<Direction> Direction::get_instance(const Vector& p_vector)
{
    if (const IntVector* iv = dynamic_cast<const IntVector*>(&p_vector))
    {
        return iv->to_normalized_direction();
    }
    if (const RationalVector* rv = dynamic_cast<const RationalVector*>(&p_vector))
    {
        return rv->to_normalized_direction();
    }
    FloatPoint fp = p_vector.to_float();
    constexpr double scale_factor = 10000.0;
    int64_t x = static_cast<int64_t>(std::lround(fp.x * scale_factor));
    int64_t y = static_cast<int64_t>(std::lround(fp.y * scale_factor));
    return IntVector(x, y).to_normalized_direction();
}

std::unique_ptr<Direction> Direction::get_instance(const Point& p_from, const Point& p_to)
{
    if (p_from.equals(&p_to))
    {
        return nullptr;
    }
    std::unique_ptr<Vector> diff = p_to.difference_by(p_from);
    return get_instance(*diff);
}

std::unique_ptr<Direction> Direction::get_instance_approx(double p_angle)
{
    constexpr double scale_factor = 10000.0;
    int64_t x = static_cast<int64_t>(std::lround(std::cos(p_angle) * scale_factor));
    int64_t y = static_cast<int64_t>(std::lround(std::sin(p_angle) * scale_factor));
    IntVector v(x, y);
    return get_instance(v);
}

const IntDirection IntDirection::NULL_DIR(0, 0);
const IntDirection IntDirection::RIGHT(1, 0);
const IntDirection IntDirection::RIGHT45(1, 1);
const IntDirection IntDirection::UP(0, 1);
const IntDirection IntDirection::UP45(-1, 1);
const IntDirection IntDirection::LEFT(-1, 0);
const IntDirection IntDirection::LEFT45(-1, -1);
const IntDirection IntDirection::DOWN(0, -1);
const IntDirection IntDirection::DOWN45(1, -1);

IntDirection::IntDirection(int64_t p_x, int64_t p_y) : x(p_x), y(p_y) {}

std::unique_ptr<Vector> IntDirection::get_vector() const
{
    return std::make_unique<IntVector>(x, y);
}

bool IntDirection::is_orthogonal() const
{
    return x == 0 || y == 0;
}

bool IntDirection::is_diagonal() const
{
    return std::abs(x) == std::abs(y);
}

std::unique_ptr<Direction> IntDirection::turn_45_degree(int p_factor) const
{
    return std::make_unique<IntDirection>(turn_45_degree_int(p_factor));
}

IntDirection IntDirection::turn_45_degree_int(int p_factor) const
{
    int n = p_factor % 8;
    if (n < 0)
    {
        n += 8;
    }
    int64_t new_x;
    int64_t new_y;
    switch (n)
    {
        case 0: new_x = x; new_y = y; break;                                  // 0 degree
        case 1: new_x = x - y; new_y = x + y; break;                          // 45 degree
        case 2: new_x = -y; new_y = x; break;                                 // 90 degree
        case 3: new_x = -x - y; new_y = x - y; break;                         // 135 degree
        case 4: new_x = -x; new_y = -y; break;                                // 180 degree
        case 5: new_x = y - x; new_y = -x - y; break;                         // 225 degree
        case 6: new_x = y; new_y = -x; break;                                 // 270 degree
        case 7: new_x = x + y; new_y = y - x; break;                          // 315 degree
        default: new_x = 0; new_y = 0; break;
    }
    return IntDirection(new_x, new_y);
}

std::unique_ptr<Direction> IntDirection::opposite() const
{
    return std::make_unique<IntDirection>(-x, -y);
}

double IntDirection::determinant(const IntDirection& p_other) const
{
    return static_cast<double>(x) * static_cast<double>(p_other.y)
         - static_cast<double>(y) * static_cast<double>(p_other.x);
}

bool IntDirection::equals(const IntDirection& p_other) const
{
    return x == p_other.x && y == p_other.y;
}

int IntDirection::compareTo(const IntDirection& p_other) const
{
    if (y > 0)
    {
        if (p_other.y < 0)
        {
            return -1;
        }
        if (p_other.y == 0)
        {
            if (p_other.x > 0)
            {
                return 1;
            }
            return -1;
        }
    }
    else if (y < 0)
    {
        if (p_other.y >= 0)
        {
            return 1;
        }
    }
    else // y == 0
    {
        if (x > 0)
        {
            if (p_other.y != 0 || p_other.x < 0)
            {
                return -1;
            }
            return 0;
        }
        // x < 0 (or x == 0 for the null direction)
        if (p_other.y > 0 || (p_other.y == 0 && p_other.x > 0))
        {
            return 1;
        }
        if (p_other.y < 0)
        {
            return -1;
        }
        return 0;
    }

    // now this direction and p_other are located in the same
    // open horizontal half plane
    double determinant_value = static_cast<double>(p_other.x) * static_cast<double>(y)
                             - static_cast<double>(p_other.y) * static_cast<double>(x);
    return freerouting::datastructures::Signum::as_int(determinant_value);
}

std::string IntDirection::to_string() const
{
    return "IntDirection[" + std::to_string(x) + ", " + std::to_string(y) + "]";
}

// ============== BigIntDirection Implementation ==============

BigIntDirection::BigIntDirection(const datastructures::BigInt& p_x, const datastructures::BigInt& p_y)
    : x(p_x), y(p_y)
{
}

BigIntDirection::BigIntDirection(const IntDirection& p_dir)
    : x(p_dir.x), y(p_dir.y)
{
}

bool BigIntDirection::is_orthogonal() const
{
    return x.sign() == 0 || y.sign() == 0;
}

bool BigIntDirection::is_diagonal() const
{
    return boost::multiprecision::abs(x) == boost::multiprecision::abs(y);
}

std::unique_ptr<Vector> BigIntDirection::get_vector() const
{
    return std::make_unique<RationalVector>(x, y, datastructures::BigInt(1));
}

std::unique_ptr<Direction> BigIntDirection::turn_45_degree(int p_factor) const
{
    (void)p_factor;
    FR_LOG_WARN("BigIntDirection: turn_45_degree not yet implemented");
    return std::make_unique<BigIntDirection>(*this);
}

std::unique_ptr<Direction> BigIntDirection::opposite() const
{
    return std::make_unique<BigIntDirection>(-x, -y);
}

int BigIntDirection::compareTo(const IntDirection& p_other) const
{
    return compareTo(BigIntDirection(p_other));
}

int BigIntDirection::compareTo(const BigIntDirection& p_other) const
{
    const int x1 = x.sign();
    const int y1 = y.sign();
    const int x2 = p_other.x.sign();
    const int y2 = p_other.y.sign();
    if (y1 > 0)
    {
        if (y2 < 0)
        {
            return -1;
        }
        if (y2 == 0)
        {
            return (x2 > 0) ? 1 : -1;
        }
    }
    else if (y1 < 0)
    {
        if (y2 >= 0)
        {
            return 1;
        }
    }
    else // y1 == 0
    {
        if (x1 > 0)
        {
            if (y2 != 0 || x2 < 0)
            {
                return -1;
            }
            return 0;
        }
        // x1 < 0 (or x1 == 0 for the null direction)
        if (y2 > 0 || (y2 == 0 && x2 > 0))
        {
            return 1;
        }
        if (y2 < 0)
        {
            return -1;
        }
        return 0;
    }

    // now this direction and p_other are located in the same
    // open horizontal half plane
    datastructures::BigInt tmp_1 = y * p_other.x;
    datastructures::BigInt tmp_2 = x * p_other.y;
    datastructures::BigInt determinant = tmp_1 - tmp_2;
    return determinant.sign();
}

std::string BigIntDirection::to_string() const
{
    std::ostringstream ss;
    ss << "BigIntDirection[" << x << ", " << y << "]";
    return ss.str();
}

} // namespace planar
} // namespace geometry
} // namespace freerouting