#pragma once

#include <string>
#include <memory>
#include <vector>
#include <optional>

#include "geometry/planar/Point.h"

namespace freerouting {
namespace geometry {
namespace planar {

class Vector
{
public:
    virtual ~Vector() = default;

    virtual std::unique_ptr<Point> add_to(const IntPoint& p_point) const = 0;
    virtual std::unique_ptr<Vector> add(const Vector& p_other) const = 0;
    virtual std::unique_ptr<Vector> negate() const = 0;
    virtual int64_t determinant(const Vector& p_other) const = 0;
    virtual double length_approx() const = 0;
    virtual Side side_of(const Vector& p_other) const = 0;
    virtual FloatPoint to_float() const = 0;
    virtual bool is_zero() const = 0;
    virtual double scalar_product(const Vector& p_other) const = 0;
};

class IntVector : public Vector
{
public:
    const int64_t x;
    const int64_t y;

    IntVector(int64_t p_x, int64_t p_y);
    IntVector(const IntVector& other) = default;
    IntVector& operator=(const IntVector&) = delete;
    IntVector& operator=(IntVector&&) = delete;

    std::unique_ptr<Point> add_to(const IntPoint& p_point) const override;
    std::unique_ptr<Vector> add(const Vector& p_other) const override;
    std::unique_ptr<Vector> negate() const override;
    int64_t determinant(const Vector& p_other) const override;
    double length_approx() const override;
    Side side_of(const Vector& p_other) const override;
    FloatPoint to_float() const override;
    bool is_zero() const override { return x == 0 && y == 0; }

    int64_t determinant(const IntVector& p_other) const;
    IntVector operator+(const IntVector& other) const;
    IntVector operator-() const;

    bool is_orthogonal() const;
    bool is_diagonal() const;
    IntVector turn_90_degree(int p_factor) const;
    IntVector mirror_at_y_axis() const;
    IntVector mirror_at_x_axis() const;
    double scalar_product(const IntVector& p_other) const;
    double scalar_product(const Vector& p_other) const override;

    std::string to_string() const;
};

class Line
{
public:
    const IntPoint a;
    const IntPoint b;

    Line(const IntPoint& p_a, const IntPoint& p_b);
    Line(const Line& other) = default;
    Line& operator=(const Line&) = delete;
    Line& operator=(Line&&) = delete;

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

    bool is_parallel(const Line& p_other) const;
    bool is_perpendicular(const Line& p_other) const;
    bool is_equal_or_opposite(const Line& p_other) const;
    Line opposite() const;
    std::unique_ptr<Point> intersection(const Line& p_other) const;
    double function_value_approx(double p_x) const;

    std::string to_string() const;
};

} // namespace planar
} // namespace geometry
} // namespace freerouting