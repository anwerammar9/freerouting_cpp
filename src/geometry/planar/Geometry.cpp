#include "geometry/planar/Point.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/Vector.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>
#include <sstream>

namespace freerouting {
namespace geometry {
namespace planar {

// ============== IntPoint Implementation ==============

IntPoint::IntPoint(int64_t p_x, int64_t p_y)
    : x(p_x), y(p_y)
{
}

bool IntPoint::equals(const Point& other) const
{
    const IntPoint* other_int = dynamic_cast<const IntPoint*>(&other);
    if (other_int == nullptr)
        return false;
    return x == other_int->x && y == other_int->y;
}

IntBox IntPoint::surrounding_box() const
{
    return IntBox(*this, *this);
}

IntOctagon IntPoint::surrounding_octagon() const
{
    int64_t tmp_1 = x - y;
    int64_t tmp_2 = x + y;
    return IntOctagon(x, y, x, y, tmp_1, tmp_1, tmp_2, tmp_2);
}

bool IntPoint::is_contained_in(const IntBox& p_box) const
{
    return x >= p_box.ll.x && y >= p_box.ll.y && x <= p_box.ur.x && y <= p_box.ur.y;
}

FloatPoint IntPoint::to_float() const
{
    return FloatPoint(static_cast<double>(x), static_cast<double>(y));
}

std::string IntPoint::to_string() const
{
    std::ostringstream ss;
    ss << "(" << x << "," << y << ")";
    return ss.str();
}

int64_t IntPoint::determinant(const IntPoint& p_other) const
{
    return x * p_other.y - y * p_other.x;
}

double IntPoint::distance_square(const IntPoint& p_to_point) const
{
    double dx = static_cast<double>(p_to_point.x - x);
    double dy = static_cast<double>(p_to_point.y - y);
    return dx * dx + dy * dy;
}

double IntPoint::distance(const IntPoint& p_to_point) const
{
    return std::sqrt(distance_square(p_to_point));
}

IntPoint IntPoint::orthogonal_projection(const IntPoint& p_other) const
{
    const int64_t horizontal_distance = std::llabs(x - p_other.x);
    const int64_t vertical_distance = std::llabs(y - p_other.y);
    if (horizontal_distance <= vertical_distance)
    {
        return IntPoint(p_other.x, y);
    }
    return IntPoint(x, p_other.y);
}

IntPoint IntPoint::fortyfive_degree_projection(const IntPoint& p_other) const
{
    const int64_t dx = x - p_other.x;
    const int64_t dy = y - p_other.y;
    const double diagonal_1 = (static_cast<double>(dy) - static_cast<double>(dx)) / 2.0;
    const double diagonal_2 = (static_cast<double>(dy) + static_cast<double>(dx)) / 2.0;
    const std::array<double, 4> distances = {
        std::abs(static_cast<double>(dx)),
        std::abs(static_cast<double>(dy)),
        std::abs(diagonal_1),
        std::abs(diagonal_2),
    };

    int best_idx = 0;
    for (int i = 1; i < 4; ++i)
    {
        if (distances[i] < distances[best_idx])
        {
            best_idx = i;
        }
    }

    if (best_idx == 0)
    {
        return IntPoint(p_other.x, y);
    }
    if (best_idx == 1)
    {
        return IntPoint(x, p_other.y);
    }
    if (best_idx == 2)
    {
        const int64_t diag = static_cast<int64_t>(diagonal_2);
        return IntPoint(p_other.x + diag, p_other.y + diag);
    }

    const int64_t diag = static_cast<int64_t>(diagonal_1);
    return IntPoint(p_other.x - diag, p_other.y + diag);
}

IntPoint IntPoint::fortyfive_degree_corner(const IntPoint& p_to_point, bool p_left_turn) const
{
    const int64_t dx = p_to_point.x - x;
    const int64_t dy = p_to_point.y - y;

    if (dy > 0 && dy < dx)
    {
        return p_left_turn ? IntPoint(p_to_point.x - dy, y) : IntPoint(x + dy, p_to_point.y);
    }
    if (dx > 0 && dy > dx)
    {
        return p_left_turn ? IntPoint(p_to_point.x, y + dx) : IntPoint(x, p_to_point.y - dx);
    }
    if (dx < 0 && dy > -dx)
    {
        return p_left_turn ? IntPoint(x, p_to_point.y + dx) : IntPoint(p_to_point.x, y - dx);
    }
    if (dy > 0 && dy < -dx)
    {
        return p_left_turn ? IntPoint(x - dy, p_to_point.y) : IntPoint(p_to_point.x + dy, y);
    }
    if (dy < 0 && dy > dx)
    {
        return p_left_turn ? IntPoint(p_to_point.x - dy, y) : IntPoint(x + dy, p_to_point.y);
    }
    if (dx < 0 && dy < dx)
    {
        return p_left_turn ? IntPoint(p_to_point.x, y + dx) : IntPoint(x, p_to_point.y - dx);
    }
    if (dx > 0 && dy < -dx)
    {
        return p_left_turn ? IntPoint(x, p_to_point.y + dx) : IntPoint(p_to_point.x, y - dx);
    }
    if (dy < 0 && dy > -dx)
    {
        return p_left_turn ? IntPoint(x - dy, p_to_point.y) : IntPoint(p_to_point.x + dy, y);
    }

    // Already a multiple of 45 degree; Java returns null in this case.
    return *this;
}

IntPoint IntPoint::ninety_degree_corner(const IntPoint& p_to_point, bool p_left_turn) const
{
    const int64_t dx = p_to_point.x - x;
    const int64_t dy = p_to_point.y - y;

    if ((dx > 0 && dy > 0) || (dx < 0 && dy < 0))
    {
        return p_left_turn ? IntPoint(p_to_point.x, y) : IntPoint(x, p_to_point.y);
    }
    if ((dx < 0 && dy > 0) || (dx > 0 && dy < 0))
    {
        return p_left_turn ? IntPoint(x, p_to_point.y) : IntPoint(p_to_point.x, y);
    }

    // Already orthogonal; Java returns null in this case.
    return *this;
}

std::unique_ptr<Point> IntPoint::translate_by(const Vector& p_vector) const
{
    const IntVector* v = dynamic_cast<const IntVector*>(&p_vector);
    if (v)
    {
        return std::make_unique<IntPoint>(x + v->x, y + v->y);
    }
    throw std::runtime_error("IntPoint::translate_by: not implemented for non-IntVector");
}

std::unique_ptr<Vector> IntPoint::difference_by(const Point& p_other) const
{
    const IntPoint* other_int = dynamic_cast<const IntPoint*>(&p_other);
    if (other_int)
    {
        return std::make_unique<IntVector>(x - other_int->x, y - other_int->y);
    }
    throw std::runtime_error("IntPoint::difference_by: not implemented for non-IntPoint");
}

Side IntPoint::side_of(const Line& p_line) const
{
    auto v1 = difference_by(p_line.a);
    auto v2 = p_line.b.difference_by(p_line.a);
    return v1->side_of(*v2);
}

std::unique_ptr<Point> IntPoint::perpendicular_projection(const Line& p_line) const
{
    // Simplified - returns closest point on line
    IntVector v = p_line.direction();
    int64_t vxvx = v.x * v.x;
    int64_t vyvy = v.y * v.y;
    int64_t denominator = vxvx + vyvy;
    
    if (denominator == 0)
        return std::make_unique<IntPoint>(p_line.a.x, p_line.a.y);
    
    int64_t det = determinant(p_line.b);
    int64_t proj_x = (vxvx * x + v.x * v.y * y + det * v.y) / denominator;
    int64_t proj_y = (v.x * v.y * x + vyvy * y - det * v.x) / denominator;
    
    return std::make_unique<IntPoint>(proj_x, proj_y);
}

int IntPoint::compare_x(const Point& p_other) const
{
    const IntPoint* other_int = dynamic_cast<const IntPoint*>(&p_other);
    if (other_int)
        return compare_x(*other_int);
    return 0;
}

int IntPoint::compare_y(const Point& p_other) const
{
    const IntPoint* other_int = dynamic_cast<const IntPoint*>(&p_other);
    if (other_int)
        return compare_y(*other_int);
    return 0;
}

int IntPoint::compare_x(const IntPoint& p_other) const
{
    if (x > p_other.x) return 1;
    if (x < p_other.x) return -1;
    return 0;
}

int IntPoint::compare_y(const IntPoint& p_other) const
{
    if (y > p_other.y) return 1;
    if (y < p_other.y) return -1;
    return 0;
}

double IntPoint::signed_area(const IntPoint& p_1, const IntPoint& p_2) const
{
    IntVector d21(p_2.x - p_1.x, p_2.y - p_1.y);
    IntVector d01(x - p_1.x, y - p_1.y);
    return static_cast<double>(d21.determinant(d01));
}

// ============== FloatPoint Implementation ==============

FloatPoint::FloatPoint(double p_x, double p_y)
    : x(p_x), y(p_y)
{
}

IntBox FloatPoint::surrounding_box() const
{
    return IntBox(static_cast<int64_t>(std::floor(x)), static_cast<int64_t>(std::floor(y)),
                  static_cast<int64_t>(std::ceil(x)), static_cast<int64_t>(std::ceil(y)));
}

IntOctagon FloatPoint::surrounding_octagon() const
{
    return surrounding_box().bounding_octagon();
}

bool FloatPoint::is_contained_in(const IntBox& p_box) const
{
    return x >= p_box.ll.x && y >= p_box.ll.y && x <= p_box.ur.x && y <= p_box.ur.y;
}

std::unique_ptr<Point> FloatPoint::translate_by(const Vector& p_vector) const
{
    const IntVector* v = dynamic_cast<const IntVector*>(&p_vector);
    if (v)
    {
        return std::make_unique<FloatPoint>(x + static_cast<double>(v->x), y + static_cast<double>(v->y));
    }
    throw std::runtime_error("FloatPoint::translate_by: not implemented for non-IntVector");
}

std::unique_ptr<Vector> FloatPoint::difference_by(const Point& p_other) const
{
    const FloatPoint* other_fp = dynamic_cast<const FloatPoint*>(&p_other);
    if (other_fp)
    {
        return std::make_unique<IntVector>(
            static_cast<int64_t>(other_fp->x - x),
            static_cast<int64_t>(other_fp->y - y));
    }
    throw std::runtime_error("FloatPoint::difference_by: not implemented");
}

Side FloatPoint::side_of(const Line& p_line) const
{
    double v1x = x - p_line.a.x;
    double v1y = y - p_line.a.y;
    double v2x = static_cast<double>(p_line.b.x - p_line.a.x);
    double v2y = static_cast<double>(p_line.b.y - p_line.a.y);
    double det = v1x * v2y - v1y * v2x;
    if (det > 0) return Side::ON_THE_LEFT;
    if (det < 0) return Side::ON_THE_RIGHT;
    return Side::COLLINEAR;
}

std::unique_ptr<Point> FloatPoint::perpendicular_projection(const Line& p_line) const
{
    IntVector v = p_line.direction();
    double vx = static_cast<double>(v.x);
    double vy = static_cast<double>(v.y);
    double denominator = vx * vx + vy * vy;
    
    if (denominator < 1e-10)
        return std::make_unique<FloatPoint>(p_line.a.x, p_line.a.y);
    
    double proj_x = (vx * vx * x + vx * vy * y + (p_line.b.y - p_line.a.y) * vy) / denominator;
    double proj_y = (vx * vy * x + vy * vy * y - (p_line.b.y - p_line.a.y) * vx) / denominator;
    
    return std::make_unique<FloatPoint>(proj_x, proj_y);
}

int FloatPoint::compare_x(const Point& p_other) const
{
    const FloatPoint* other_fp = dynamic_cast<const FloatPoint*>(&p_other);
    if (other_fp)
    {
        if (x > other_fp->x) return 1;
        if (x < other_fp->x) return -1;
        return 0;
    }
    return 0;
}

int FloatPoint::compare_y(const Point& p_other) const
{
    const FloatPoint* other_fp = dynamic_cast<const FloatPoint*>(&p_other);
    if (other_fp)
    {
        if (y > other_fp->y) return 1;
        if (y < other_fp->y) return -1;
        return 0;
    }
    return 0;
}

std::string FloatPoint::to_string() const
{
    std::ostringstream ss;
    ss << "(" << x << "," << y << ")";
    return ss.str();
}

double FloatPoint::distance_square(const FloatPoint& p_other) const
{
    double dx = p_other.x - x;
    double dy = p_other.y - y;
    return dx * dx + dy * dy;
}

double FloatPoint::distance(const FloatPoint& p_other) const
{
    return std::sqrt(distance_square(p_other));
}

FloatPoint FloatPoint::middle_point(const FloatPoint& p_other) const
{
    return FloatPoint((x + p_other.x) / 2.0, (y + p_other.y) / 2.0);
}

double FloatPoint::scalar_product(const FloatPoint& p_from, const FloatPoint& p_to) const
{
    double dx1 = p_from.x - x;
    double dy1 = p_from.y - y;
    double dx2 = p_to.x - x;
    double dy2 = p_to.y - y;
    return dx1 * dx2 + dy1 * dy2;
}

bool FloatPoint::is_contained_in_box(const FloatPoint& p_a, const FloatPoint& p_b, double p_tolerance) const
{
    double min_x = std::min(p_a.x, p_b.x) - p_tolerance;
    double max_x = std::max(p_a.x, p_b.x) + p_tolerance;
    double min_y = std::min(p_a.y, p_b.y) - p_tolerance;
    double max_y = std::max(p_a.y, p_b.y) + p_tolerance;
    return x >= min_x && x <= max_x && y >= min_y && y <= max_y;
}

FloatPoint FloatPoint::turn_90_degree(int p_factor, const FloatPoint& p_from_point) const
{
    double dx = x - p_from_point.x;
    double dy = y - p_from_point.y;
    double new_x = p_from_point.x - p_factor * dy;
    double new_y = p_from_point.y + p_factor * dx;
    return FloatPoint(new_x, new_y);
}

} // namespace planar
} // namespace geometry
} // namespace freerouting