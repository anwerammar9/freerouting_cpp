#pragma once

#include <string>
#include <memory>
#include <vector>
#include <optional>

#include "geometry/planar/Point.h"
#include "datastructures/Signum.h"
#include "datastructures/BigIntAux.h"

namespace freerouting {
namespace geometry {
namespace planar {

class Direction;
class IntDirection;
class RationalVector;

class Vector
{
public:
    virtual ~Vector() = default;

    /**
     * Creates a Vector (p_x, p_y) in the plane.
     * Port of Vector.get_instance(int, int). Only IntVector is implemented so
     * far; once RationalVector is ported, coordinates beyond Limits::CRIT_INT
     * will return a RationalVector instead.
     */
    static std::unique_ptr<Vector> get_instance(int64_t p_x, int64_t p_y);

    virtual std::unique_ptr<Point> add_to(const IntPoint& p_point) const = 0;
    virtual std::unique_ptr<Vector> add(const Vector& p_other) const = 0;
    virtual std::unique_ptr<Vector> negate() const = 0;
    virtual datastructures::BigInt determinant(const Vector& p_other) const = 0;
    virtual double length_approx() const = 0;
    virtual Side side_of(const Vector& p_other) const = 0;
    virtual FloatPoint to_float() const = 0;
    virtual bool is_zero() const = 0;
    virtual double scalar_product(const Vector& p_other) const = 0;

    double cos_angle(const Vector& p_other) const;
    double angle_approx(const Vector& p_other) const;
    double angle_approx() const;
};

class IntVector : public Vector
{
public:
    const int64_t x;
    const int64_t y;

    static const IntVector ZERO;

    IntVector(int64_t p_x, int64_t p_y);
    IntVector(const IntVector& other) = default;
    IntVector& operator=(const IntVector&) = delete;
    IntVector& operator=(IntVector&&) = delete;

    bool operator==(const IntVector& other) const;

    std::unique_ptr<Point> add_to(const IntPoint& p_point) const override;
    std::unique_ptr<Vector> add(const Vector& p_other) const override;
    std::unique_ptr<Vector> negate() const override;
    datastructures::BigInt determinant(const Vector& p_other) const override;
    double length_approx() const override;
    Side side_of(const Vector& p_other) const override;
    FloatPoint to_float() const override;
    bool is_zero() const override { return x == 0 && y == 0; }

    int64_t determinant(const IntVector& p_other) const;
    IntVector operator+(const IntVector& other) const;
    IntVector operator-() const;

    bool is_orthogonal() const;
    bool is_diagonal() const;
    bool is_multiple_of_45_degree() const { return is_orthogonal() || is_diagonal(); }
    IntVector turn_90_degree(int p_factor) const;
    IntVector mirror_at_y_axis() const;
    IntVector mirror_at_x_axis() const;
    datastructures::Signum projection(const IntVector& p_other) const;
    double scalar_product(const IntVector& p_other) const;
    double scalar_product(const Vector& p_other) const override;

    /**
     * Returns an approximation vector of this vector with the same direction
     * and length p_length. Port of IntVector.change_length_approx(double).
     */
    IntVector change_length_approx(double p_length) const;

    std::unique_ptr<Direction> to_normalized_direction() const;

    std::string to_string() const;
};

class Line
{
public:
    IntPoint a;
    IntPoint b;

    Line(const IntPoint& p_a, const IntPoint& p_b);
    Line(const Line& other) = default;
    Line& operator=(const Line&) = default;
    Line& operator=(Line&&) = default;

    static Line get_instance(const IntPoint& p_from, const IntDirection& p_direction);

    bool is_vertical() const;
    bool is_horizontal() const;
    bool is_parallel_to(const Line& p_other) const;
    bool is_perpendicular_to(const Line& p_other) const;
    bool overlaps(const Line& p_other) const;

    IntVector direction() const;
    double length_approx() const;
    int64_t distance(const IntPoint& p_point) const;
    int64_t distance_square(const IntPoint& p_point) const;
    double signed_distance(const FloatPoint& p_point) const;
    FloatPoint intersection_approx(const Line& p_other) const;

    Side side_of(const IntPoint& p_point) const;
    Side side_of(const FloatPoint& p_point, double p_tolerance) const;
    Side side_of(const FloatPoint& p_point) const;
    Side side_of(const Point& p_point) const;
    Side side_of_intersection(const Line& p_other_1, const Line& p_other_2) const;

    bool is_parallel(const Line& p_other) const;
    bool is_perpendicular(const Line& p_other) const;
    bool is_equal_or_opposite(const Line& p_other) const;
    Line opposite() const;
    std::unique_ptr<Point> intersection(const Line& p_other) const;
    double function_value_approx(double p_x) const;
    double function_in_y_value_approx(double p_y) const;

    Line translate(double p_dist) const;
    Line translate_by(const IntVector& p_offset) const;
    Line turn_90_degree(int p_factor, const IntPoint& p_pole) const;
    Line mirror_vertical(const IntPoint& p_pole) const;
    Line mirror_horizontal(const IntPoint& p_pole) const;
    bool fast_equals(const Line& p_other) const;
    bool equals(const Line& p_other) const;
    bool is_orthogonal() const;
    bool is_multiple_of_45_degree() const;
    int compare_by_direction(const Line& p_other) const;

    std::optional<IntDirection> perpendicular_direction(const IntPoint& p_from_point) const;

    std::string to_string() const;
};

} // namespace planar
} // namespace geometry
} // namespace freerouting