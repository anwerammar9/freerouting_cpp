#include "geometry/planar/Line.h"
#include "geometry/planar/Vector.h"
#include "geometry/planar/IntVector.h"
#include "geometry/planar/FloatPoint.h"
#include <sstream>
#include <cmath>
#include <algorithm>

namespace freerouting {
namespace geometry {
namespace planar {

Line::Line(const IntPoint& p_a, const IntPoint& p_b)
    : a(p_a)
    , b(p_b)
    , cached_direction_(std::nullopt)
{
}

Line::Line(int p_a_x, int p_a_y, int p_b_x, int p_b_y)
    : a(IntPoint(p_a_x, p_a_y))
    , b(IntPoint(p_b_x, p_b_y))
    , cached_direction_(std::nullopt)
{
}

Line::Line(const IntPoint& p_a, const Direction& p_dir)
    : a(p_a)
    , b(p_a + p_dir.get_vector())
    , cached_direction_(p_dir)
{
}

Line Line::get_instance(const IntPoint& p_a, const Direction& p_dir) {
    return Line(p_a, p_dir);
}

bool Line::equals(const Line& p_other) const {
    if (this == &p_other) {
        return true;
    }
    if (side_of(p_other.a) != Side::COLLINEAR) {
        return false;
    }
    return direction().equals(p_other.direction());
}

bool Line::fast_equals(const Line& p_other) const {
    if (this == &p_other) {
        return true;
    }
    
    double dx1 = p_other.a.x - a.x;
    double dy1 = p_other.a.y - a.y;
    double dx2 = b.x - a.x;
    double dy2 = b.y - a.y;
    double det = dx1 * dy2 - dx2 * dy1;
    
    if (det != 0) {
        return false;
    }
    return direction().equals(p_other.direction());
}

Direction Line::direction() const {
    if (!cached_direction_.has_value()) {
        cached_direction_ = calculate_direction();
    }
    return *cached_direction_;
}

Direction Line::calculate_direction() const {
    IntVector delta(b.x - a.x, b.y - a.y);
    return Direction::get_instance(delta);
}

Side Line::side_of(const IntPoint& p_point) const {
    // For IntPoint, use the point's side_of method
    // This is a simplified implementation
    double det = (b.y - a.y) * (p_point.x - a.x) - (b.x - a.x) * (p_point.y - a.y);
    
    if (det > 0) {
        return Side::ON_THE_LEFT;
    } else if (det < 0) {
        return Side::ON_THE_RIGHT;
    } else {
        return Side::COLLINEAR;
    }
}

Side Line::side_of(const FloatPoint& p_point, double p_tolerance) const {
    double det = (b.y - a.y) * (p_point.x - a.x) - (b.x - a.x) * (p_point.y - a.y);
    
    if (det - p_tolerance > 0) {
        return Side::ON_THE_LEFT;
    } else if (det + p_tolerance < 0) {
        return Side::ON_THE_RIGHT;
    } else {
        return Side::COLLINEAR;
    }
}

Side Line::side_of(const FloatPoint& p_point) const {
    return side_of(p_point, 0.0);
}

Side Line::side_of_intersection(const Line& p_1, const Line& p_2) const {
    FloatPoint intersection_approx = p_1.intersection_approx(p_2);
    return side_of(intersection_approx);
}

bool Line::overlaps(const Line& p_other) const {
    return side_of(p_other.a) == Side::COLLINEAR && side_of(p_other.b) == Side::COLLINEAR;
}

Line Line::opposite() const {
    return Line(b, a);
}

std::shared_ptr<IntPoint> Line::intersection(const Line& p_other) const {
    // Check for special cases first
    auto special_result = handle_special_intersection(p_other);
    if (special_result) {
        return special_result;
    }
    
    // General case - solve linear equations
    IntVector delta_1(b.x - a.x, b.y - a.y);
    IntVector delta_2(p_other.b.x - p_other.a.x, p_other.b.y - p_other.a.y);
    
    double det = calculate_determinant(delta_1, delta_2);
    
    if (std::abs(det) < 1e-10) {
        // Lines are parallel
        return nullptr;
    }
    
    // Calculate intersection using parametric equations
    double t1 = ((p_other.a.x - a.x) * delta_2.y - (p_other.a.y - a.y) * delta_2.x) / det;
    
    int intersection_x = static_cast<int>(a.x + t1 * delta_1.x);
    int intersection_y = static_cast<int>(a.y + t1 * delta_1.y);
    
    return std::make_shared<IntPoint>(intersection_x, intersection_y);
}

FloatPoint Line::intersection_approx(const Line& p_other) const {
    IntVector delta_1(b.x - a.x, b.y - a.y);
    IntVector delta_2(p_other.b.x - p_other.a.x, p_other.b.y - p_other.a.y);
    
    double det = calculate_determinant(delta_1, delta_2);
    
    if (std::abs(det) < 1e-10) {
        // Lines are parallel, return a point at infinity
        return FloatPoint(std::numeric_limits<double>::infinity(), 
                        std::numeric_limits<double>::infinity());
    }
    
    // Calculate intersection using parametric equations
    double t1 = ((p_other.a.x - a.x) * delta_2.y - (p_other.a.y - a.y) * delta_2.x) / det;
    
    double intersection_x = a.x + t1 * delta_1.x;
    double intersection_y = a.y + t1 * delta_1.y;
    
    return FloatPoint(intersection_x, intersection_y);
}

bool Line::is_vertical() const {
    return a.x == b.x;
}

bool Line::is_horizontal() const {
    return a.y == b.y;
}

bool Line::is_parallel_to(const Line& p_other) const {
    IntVector delta_1(b.x - a.x, b.y - a.y);
    IntVector delta_2(p_other.b.x - p_other.a.x, p_other.b.y - p_other.a.y);
    
    double det = calculate_determinant(delta_1, delta_2);
    return std::abs(det) < 1e-10;
}

bool Line::is_perpendicular_to(const Line& p_other) const {
    IntVector delta_1(b.x - a.x, b.y - a.y);
    IntVector delta_2(p_other.b.x - p_other.a.x, p_other.b.y - p_other.a.y);
    
    // Check if dot product is zero
    double dot_product = delta_1.x * delta_2.x + delta_1.y * delta_2.y;
    return std::abs(dot_product) < 1e-10;
}

Line Line::shift_by(const IntVector& p_vector) const {
    return Line(a + p_vector, b + p_vector);
}

Line Line::shift_by(const Direction& p_dir) const {
    return shift_by(p_dir.get_vector());
}

Line Line::shift_by(const FloatPoint& p_vector) const {
    IntPoint new_a(static_cast<int>(a.x + p_vector.x), 
                  static_cast<int>(a.y + p_vector.y));
    IntPoint new_b(static_cast<int>(b.x + p_vector.x), 
                  static_cast<int>(b.y + p_vector.y));
    return Line(new_a, new_b);
}

Line Line::shift_by(int p_amount) const {
    Direction dir = direction();
    IntVector shift_vector = dir.get_vector() * p_amount;
    return shift_by(shift_vector);
}

double Line::determinant(const Line& p_other) const {
    IntVector delta_1(b.x - a.x, b.y - a.y);
    IntVector delta_2(p_other.b.x - p_other.a.x, p_other.b.y - p_other.a.y);
    return calculate_determinant(delta_1, delta_2);
}

std::string Line::to_string() const {
    std::ostringstream oss;
    oss << "Line[(" << a.x << "," << a.y << ")-(" << b.x << "," << b.y << ")]";
    return oss.str();
}

bool Line::operator<(const Line& p_other) const {
    // Simple comparison based on start point and direction
    if (a.x != p_other.a.x) {
        return a.x < p_other.a.x;
    }
    if (a.y != p_other.a.y) {
        return a.y < p_other.a.y;
    }
    if (b.x != p_other.b.x) {
        return b.x < p_other.b.x;
    }
    return b.y < p_other.b.y;
}

double Line::calculate_determinant(const IntVector& v1, const IntVector& v2) {
    return v1.x * v2.y - v1.y * v2.x;
}

std::shared_ptr<IntPoint> Line::handle_special_intersection(const Line& p_other) const {
    IntVector delta_1(b.x - a.x, b.y - a.y);
    IntVector delta_2(p_other.b.x - p_other.a.x, p_other.b.y - p_other.a.y);
    
    // Handle vertical line
    if (delta_1.x == 0) {
        if (delta_2.y == 0) {
            // Vertical x horizontal
            return std::make_shared<IntPoint>(a.x, p_other.a.y);
        }
        if (delta_2.x == delta_2.y) {
            // Vertical x right diagonal
            int this_x = a.x;
            return std::make_shared<IntPoint>(this_x, p_other.a.y + this_x - p_other.a.x);
        }
        if (delta_2.x == -delta_2.y) {
            // Vertical x left diagonal
            int this_x = a.x;
            return std::make_shared<IntPoint>(this_x, p_other.a.y + p_other.a.x - this_x);
        }
    }
    
    // Handle horizontal line
    if (delta_1.y == 0) {
        if (delta_2.x == 0) {
            // Horizontal x vertical
            return std::make_shared<IntPoint>(p_other.a.x, a.y);
        }
        if (delta_2.x == delta_2.y) {
            // Horizontal x right diagonal
            int this_y = a.y;
            return std::make_shared<IntPoint>(p_other.a.x + this_y - p_other.a.y, this_y);
        }
        if (delta_2.x == -delta_2.y) {
            // Horizontal x left diagonal
            int this_y = a.y;
            return std::make_shared<IntPoint>(p_other.a.x - this_y + p_other.a.y, this_y);
        }
    }
    
    // Handle right diagonal line
    if (delta_1.x == delta_1.y) {
        if (delta_2.x == 0) {
            // Right diagonal x vertical
            int other_x = p_other.a.x;
            return std::make_shared<IntPoint>(other_x, a.y + other_x - a.x);
        }
        if (delta_2.y == 0) {
            // Right diagonal x horizontal
            int other_y = p_other.a.y;
            return std::make_shared<IntPoint>(a.x + other_y - a.y, other_y);
        }
        if (delta_2.x == -delta_2.y) {
            // Right diagonal x left diagonal
            int sum1 = a.x + a.y;
            int sum2 = p_other.a.x + p_other.a.y;
            int avg_sum = (sum1 + sum2) / 2;
            return std::make_shared<IntPoint>(avg_sum - a.y, avg_sum - a.x);
        }
    }
    
    // Handle left diagonal line
    if (delta_1.x == -delta_1.y) {
        if (delta_2.x == 0) {
            // Left diagonal x vertical
            int other_x = p_other.a.x;
            return std::make_shared<IntPoint>(other_x, a.y - other_x + a.x);
        }
        if (delta_2.y == 0) {
            // Left diagonal x horizontal
            int other_y = p_other.a.y;
            return std::make_shared<IntPoint>(a.x - other_y + a.y, other_y);
        }
        if (delta_2.x == delta_2.y) {
            // Left diagonal x right diagonal
            int sum1 = a.x - a.y;
            int sum2 = p_other.a.x - p_other.a.y;
            int avg_sum = (sum1 + sum2) / 2;
            return std::make_shared<IntPoint>(avg_sum + a.y, avg_sum - a.x);
        }
    }
    
    return nullptr; // No special case found
}

} // namespace planar
} // namespace geometry
} // namespace freerouting
