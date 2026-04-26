#include "geometry/planar/Point.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/Vector.h"
#include "geometry/planar/Direction.h"
#include "logger/FRLogger.h"

#include <cmath>
#include <memory>

namespace freerouting {
namespace geometry {
namespace planar {

// Point static factory methods
std::unique_ptr<Point> Point::get_instance(int64_t p_x, int64_t p_y)
{
    if (std::abs(p_x) > CRIT_INT || std::abs(p_y) > CRIT_INT) {
        // Would need to create RationalPoint, but for now return IntPoint
        // TODO: Implement RationalPoint class
        return std::make_unique<IntPoint>(p_x, p_y);
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
    
    if (p_x % p_z == 0 && p_y % p_z == 0) {
        // p_x and p_y can be divided by p_z
        p_x = p_x / p_z;
        p_y = p_y / p_z;
        p_z = 1;
    }
    
    if (p_z == 1) {
        if (std::abs(p_x) <= CRIT_INT && std::abs(p_y) <= CRIT_INT) {
            // the Point fits into an IntPoint
            return std::make_unique<IntPoint>(p_x, p_y);
        }
    }
    
    // TODO: Implement RationalPoint class
    // For now, return IntPoint as approximation
    return std::make_unique<IntPoint>(p_x / p_z, p_y / p_z);
}

// IntPoint implementation
IntPoint::IntPoint(int64_t p_x, int64_t p_y) : x(p_x), y(p_y)
{
    if (std::abs(p_x) > CRIT_INT) {
        logger::FRLogger::instance().debug("IntPoint: p_x is out of range");
    }
    if (std::abs(p_y) > CRIT_INT) {
        logger::FRLogger::instance().debug("IntPoint: p_y is out of range");
    }
}

bool IntPoint::equals(const Point& other) const
{
    const IntPoint* other_int = dynamic_cast<const IntPoint*>(&other);
    if (other_int) {
        return x == other_int->x && y == other_int->y;
    }
    return false;
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

std::unique_ptr<Point> IntPoint::translate_by(const Vector& p_vector) const
{
    // TODO: Implement proper vector addition
    // For now, return this point as placeholder
    return std::make_unique<IntPoint>(*this);
}

std::unique_ptr<Vector> IntPoint::difference_by(const Point& p_other) const
{
    // TODO: Implement proper point difference
    // For now, return zero vector as placeholder
    return std::make_unique<IntVector>(0, 0);
}

Side IntPoint::side_of(const Line& p_line) const
{
    // TODO: Implement proper line side calculation
    // For now, return COLLINEAR as placeholder
    return Side::COLLINEAR;
}

FloatPoint IntPoint::to_float() const
{
    return FloatPoint(static_cast<double>(x), static_cast<double>(y));
}

std::unique_ptr<Point> IntPoint::perpendicular_projection(const Line& p_line) const
{
    // TODO: Implement proper perpendicular projection
    // For now, return this point as placeholder
    return std::make_unique<IntPoint>(*this);
}

int IntPoint::compare_x(const Point& p_other) const
{
    const IntPoint* other_int = dynamic_cast<const IntPoint*>(&p_other);
    if (other_int) {
        return compare_x(*other_int);
    }
    // TODO: Implement comparison with other point types
    return 0;
}

int IntPoint::compare_y(const Point& p_other) const
{
    const IntPoint* other_int = dynamic_cast<const IntPoint*>(&p_other);
    if (other_int) {
        return compare_y(*other_int);
    }
    // TODO: Implement comparison with other point types
    return 0;
}

int IntPoint::compare_x(const IntPoint& p_other) const
{
    if (this->x > p_other.x) {
        return 1;
    } else if (this->x == p_other.x) {
        return 0;
    } else {
        return -1;
    }
}

int IntPoint::compare_y(const IntPoint& p_other) const
{
    if (this->y > p_other.y) {
        return 1;
    } else if (this->y == p_other.y) {
        return 0;
    } else {
        return -1;
    }
}

std::string IntPoint::to_string() const
{
    return "(" + std::to_string(x) + "," + std::to_string(y) + ")";
}

int64_t IntPoint::determinant(const IntPoint& p_other) const
{
    return x * p_other.y - y * p_other.x;
}

double IntPoint::distance_square(const IntPoint& p_to_point) const
{
    double dx = static_cast<double>(p_to_point.x - this->x);
    double dy = static_cast<double>(p_to_point.y - this->y);
    return dx * dx + dy * dy;
}

double IntPoint::distance(const IntPoint& p_to_point) const
{
    return std::sqrt(distance_square(p_to_point));
}

IntPoint IntPoint::orthogonal_projection(const IntPoint& p_other) const
{
    int horizontal_distance = std::abs(this->x - p_other.x);
    int vertical_distance = std::abs(this->y - p_other.y);
    if (horizontal_distance <= vertical_distance) {
        // projection onto the vertical line through p_other
        return IntPoint(p_other.x, this->y);
    } else {
        // projection onto the horizontal line through p_other
        return IntPoint(this->x, p_other.y);
    }
}

IntPoint IntPoint::fortyfive_degree_projection(const IntPoint& p_other) const
{
    int dx = this->x - p_other.x;
    int dy = this->y - p_other.y;
    
    double dist_arr[4];
    dist_arr[0] = std::abs(dx);
    dist_arr[1] = std::abs(dy);
    double diagonal_1 = (static_cast<double>(dy) - static_cast<double>(dx)) / 2;
    double diagonal_2 = (static_cast<double>(dy) + static_cast<double>(dx)) / 2;
    dist_arr[2] = std::abs(diagonal_1);
    dist_arr[3] = std::abs(diagonal_2);
    
    double min_dist = dist_arr[0];
    for (int i = 1; i < 4; i++) {
        if (dist_arr[i] < min_dist) {
            min_dist = dist_arr[i];
        }
    }
    
    if (min_dist == dist_arr[0]) {
        // projection onto the vertical line through p_other
        return IntPoint(p_other.x, this->y);
    } else if (min_dist == dist_arr[1]) {
        // projection onto the horizontal line through p_other
        return IntPoint(this->x, p_other.y);
    } else if (min_dist == dist_arr[2]) {
        // projection onto the right diagonal line through p_other
        int diagonal_value = static_cast<int>(diagonal_2);
        return IntPoint(p_other.x + diagonal_value, p_other.y + diagonal_value);
    } else {
        // projection onto the left diagonal line through p_other
        int diagonal_value = static_cast<int>(diagonal_1);
        return IntPoint(p_other.x - diagonal_value, p_other.y + diagonal_value);
    }
}

IntPoint IntPoint::fortyfive_degree_corner(const IntPoint& p_to_point, bool p_left_turn) const
{
    int dx = p_to_point.x - this->x;
    int dy = p_to_point.y - this->y;
    // handle the 8 sections between the 45 degree lines

    if (dy > 0 && dy < dx) {
        if (p_left_turn) {
            return IntPoint(p_to_point.x - dy, this->y);
        } else {
            return IntPoint(this->x + dy, p_to_point.y);
        }
    } else if (dx > 0 && dy > dx) {
        if (p_left_turn) {
            return IntPoint(p_to_point.x, this->y + dx);
        } else {
            return IntPoint(this->x, p_to_point.y - dx);
        }
    } else if (dx < 0 && dy > -dx) {
        if (p_left_turn) {
            return IntPoint(this->x, p_to_point.y + dx);
        } else {
            return IntPoint(p_to_point.x, this->y - dx);
        }
    } else if (dy > 0 && dy < -dx) {
        if (p_left_turn) {
            return IntPoint(this->x - dy, p_to_point.y);
        } else {
            return IntPoint(p_to_point.x + dy, this->y);
        }
    } else if (dy < 0 && dy > dx) {
        if (p_left_turn) {
            return IntPoint(p_to_point.x - dy, this->y);
        } else {
            return IntPoint(this->x + dy, p_to_point.y);
        }
    } else if (dx < 0 && dy < dx) {
        if (p_left_turn) {
            return IntPoint(p_to_point.x, this->y + dx);
        } else {
            return IntPoint(this->x, p_to_point.y - dx);
        }
    } else if (dx > 0 && dy < -dx) {
        if (p_left_turn) {
            return IntPoint(this->x, p_to_point.y + dx);
        } else {
            return IntPoint(p_to_point.x, this->y - dx);
        }
    } else if (dy < 0 && dy > -dx) {
        if (p_left_turn) {
            return IntPoint(this->x - dy, p_to_point.y);
        } else {
            return IntPoint(p_to_point.x + dy, this->y);
        }
    } else {
        // the line from this point to p_to_point is already a multiple of 45 degree
        // Return a default point - in Java this would return null
        return *this;
    }
}

IntPoint IntPoint::ninety_degree_corner(const IntPoint& p_to_point, bool p_left_turn) const
{
    int dx = p_to_point.x - this->x;
    int dy = p_to_point.y - this->y;
    // handle the 4 quadrants
    if ((dx > 0 && dy > 0) || (dx < 0 && dy < 0)) {
        if (p_left_turn) {
            return IntPoint(p_to_point.x, this->y);
        } else {
            return IntPoint(this->x, p_to_point.y);
        }
    } else if ((dx < 0 && dy > 0) || (dx > 0 && dy < 0)) {
        if (p_left_turn) {
            return IntPoint(this->x, p_to_point.y);
        } else {
            return IntPoint(p_to_point.x, this->y);
        }
    } else {
        // the line from this point to p_to_point is already orthogonal
        // Return a default point - in Java this would return null
        return *this;
    }
}

double IntPoint::signed_area(const IntPoint& p_1, const IntPoint& p_2) const
{
    // TODO: Implement proper vector operations
    // For now, return 0 as placeholder
    return 0.0;
}

// FloatPoint implementation
FloatPoint::FloatPoint(double p_x, double p_y) : x(p_x), y(p_y)
{
}

IntBox FloatPoint::surrounding_box() const
{
    IntPoint lower_left(static_cast<int64_t>(std::floor(x)), static_cast<int64_t>(std::floor(y)));
    IntPoint upper_right(static_cast<int64_t>(std::ceil(x)), static_cast<int64_t>(std::ceil(y)));
    return IntBox(lower_left, upper_right);
}

IntOctagon FloatPoint::surrounding_octagon() const
{
    // TODO: Implement proper octagon calculation
    // For now, return a simple octagon
    int64_t lx = static_cast<int64_t>(std::floor(x));
    int64_t ly = static_cast<int64_t>(std::floor(y));
    int64_t rx = static_cast<int64_t>(std::ceil(x));
    int64_t uy = static_cast<int64_t>(std::ceil(y));
    int64_t tmp_1 = lx - ly;
    int64_t tmp_2 = lx + ly;
    return IntOctagon(lx, ly, rx, uy, tmp_1, tmp_1, tmp_2, tmp_2);
}

bool FloatPoint::is_contained_in(const IntBox& p_box) const
{
    return x >= p_box.ll.x && y >= p_box.ll.y && x <= p_box.ur.x && y <= p_box.ur.y;
}

std::unique_ptr<Point> FloatPoint::translate_by(const Vector& p_vector) const
{
    // TODO: Implement proper vector translation
    // For now, return this point as placeholder
    return std::make_unique<FloatPoint>(*this);
}

std::unique_ptr<Vector> FloatPoint::difference_by(const Point& p_other) const
{
    // TODO: Implement proper point difference
    // For now, return zero vector as placeholder
    return std::make_unique<IntVector>(0, 0);
}

Side FloatPoint::side_of(const Line& p_line) const
{
    // TODO: Implement proper line side calculation
    // For now, return COLLINEAR as placeholder
    return Side::COLLINEAR;
}

std::unique_ptr<Point> FloatPoint::perpendicular_projection(const Line& p_line) const
{
    // TODO: Implement proper perpendicular projection
    // For now, return this point as placeholder
    return std::make_unique<FloatPoint>(*this);
}

int FloatPoint::compare_x(const Point& p_other) const
{
    const FloatPoint* other_float = dynamic_cast<const FloatPoint*>(&p_other);
    if (other_float) {
        if (this->x > other_float->x) {
            return 1;
        } else if (this->x == other_float->x) {
            return 0;
        } else {
            return -1;
        }
    }
    // TODO: Implement comparison with other point types
    return 0;
}

int FloatPoint::compare_y(const Point& p_other) const
{
    const FloatPoint* other_float = dynamic_cast<const FloatPoint*>(&p_other);
    if (other_float) {
        if (this->y > other_float->y) {
            return 1;
        } else if (this->y == other_float->y) {
            return 0;
        } else {
            return -1;
        }
    }
    // TODO: Implement comparison with other point types
    return 0;
}

std::string FloatPoint::to_string() const
{
    return "(" + std::to_string(x) + "," + std::to_string(y) + ")";
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
    if (x == p_other.x && y == p_other.y) {
        return *this;
    }
    double middle_x = 0.5 * (x + p_other.x);
    double middle_y = 0.5 * (y + p_other.y);
    return FloatPoint(middle_x, middle_y);
}

double FloatPoint::scalar_product(const FloatPoint& p_from, const FloatPoint& p_to) const
{
    double dx_1 = p_from.x - x;
    double dx_2 = p_to.x - x;
    double dy_1 = p_from.y - y;
    double dy_2 = p_to.y - y;
    return dx_1 * dx_2 + dy_1 * dy_2;
}

bool FloatPoint::is_contained_in_box(const FloatPoint& p_a, const FloatPoint& p_b, double p_tolerance) const
{
    double min_x, max_x;
    if (p_a.x < p_b.x) {
        min_x = p_a.x;
        max_x = p_b.x;
    } else {
        min_x = p_b.x;
        max_x = p_a.x;
    }
    if (x < min_x - p_tolerance || x > max_x + p_tolerance) {
        return false;
    }
    
    double min_y, max_y;
    if (p_a.y < p_b.y) {
        min_y = p_a.y;
        max_y = p_b.y;
    } else {
        min_y = p_b.y;
        max_y = p_a.y;
    }
    return y >= min_y - p_tolerance && y <= max_y + p_tolerance;
}

FloatPoint FloatPoint::turn_90_degree(int p_factor, const FloatPoint& p_from_point) const
{
    FloatPoint v = FloatPoint(x - p_from_point.x, y - p_from_point.y);
    v = v.turn_90_degree(p_factor);
    return FloatPoint(p_from_point.x + v.x, p_from_point.y + v.y);
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

} // namespace planar
} // namespace geometry
} // namespace freerouting
