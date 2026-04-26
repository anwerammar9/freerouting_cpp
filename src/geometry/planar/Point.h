#pragma once

#include <array>
#include <cmath>
#include <compare>
#include <optional>
#include <string>
#include <memory>

namespace freerouting {
namespace geometry {
namespace planar {

class IntBox;
class IntOctagon;
class Vector;
class FloatPoint;
class Line;

enum class Side
{
    ON_THE_LEFT,
    ON_THE_RIGHT,
    COLLINEAR,
    NO_SIDE
};

constexpr int64_t CRIT_INT = 2147483647LL;

class Point
{
public:
    virtual ~Point() = default;

    virtual std::unique_ptr<Point> translate_by(const Vector& p_vector) const = 0;
    virtual std::unique_ptr<Vector> difference_by(const Point& p_other) const = 0;
    virtual FloatPoint to_float() const = 0;
    virtual bool is_infinite() const = 0;
    virtual IntBox surrounding_box() const = 0;
    virtual IntOctagon surrounding_octagon() const = 0;
    virtual bool is_contained_in(const IntBox& p_box) const = 0;
    virtual Side side_of(const Line& p_line) const = 0;
    virtual std::unique_ptr<Point> perpendicular_projection(const Line& p_line) const = 0;
    virtual int compare_x(const Point& p_other) const = 0;
    virtual int compare_y(const Point& p_other) const = 0;
    virtual std::string to_string() const = 0;

    static std::unique_ptr<Point> get_instance(int64_t p_x, int64_t p_y);
    static std::unique_ptr<Point> get_instance(int64_t p_x, int64_t p_y, int64_t p_z);
};

class IntPoint : public Point
{
public:
    const int64_t x;
    const int64_t y;

    IntPoint(int64_t p_x, int64_t p_y);
    IntPoint(const IntPoint& other) = default;
    IntPoint& operator=(const IntPoint&) = delete;
    IntPoint& operator=(IntPoint&&) = delete;

    bool operator==(const IntPoint& other) const { return x == other.x && y == other.y; }

    bool equals(const Point& other) const;

    bool is_infinite() const override { return false; }
    IntBox surrounding_box() const override;
    IntOctagon surrounding_octagon() const override;
    bool is_contained_in(const IntBox& p_box) const override;
    std::unique_ptr<Point> translate_by(const Vector& p_vector) const override;
    std::unique_ptr<Vector> difference_by(const Point& p_other) const override;
    Side side_of(const Line& p_line) const override;
    FloatPoint to_float() const override;
    std::unique_ptr<Point> perpendicular_projection(const Line& p_line) const override;
    int compare_x(const Point& p_other) const override;
    int compare_y(const Point& p_other) const override;
    int compare_x(const IntPoint& p_other) const;
    int compare_y(const IntPoint& p_other) const;
    std::string to_string() const override;

    int64_t determinant(const IntPoint& p_other) const;
    double distance_square(const IntPoint& p_to_point) const;
    double distance(const IntPoint& p_to_point) const;
    IntPoint orthogonal_projection(const IntPoint& p_other) const;
    IntPoint fortyfive_degree_projection(const IntPoint& p_other) const;
    IntPoint fortyfive_degree_corner(const IntPoint& p_to_point, bool p_left_turn) const;
    IntPoint ninety_degree_corner(const IntPoint& p_to_point, bool p_left_turn) const;
    double signed_area(const IntPoint& p_1, const IntPoint& p_2) const;
};

class FloatPoint : public Point
{
public:
    const double x;
    const double y;

    FloatPoint(double p_x, double p_y);
    FloatPoint(const FloatPoint& other) = default;
    FloatPoint& operator=(const FloatPoint&) = delete;
    FloatPoint& operator=(FloatPoint&&) = delete;

    bool operator==(const FloatPoint& other) const { return x == other.x && y == other.y; }

    bool is_infinite() const override { return false; }
    IntBox surrounding_box() const override;
    IntOctagon surrounding_octagon() const override;
    bool is_contained_in(const IntBox& p_box) const override;
    std::unique_ptr<Point> translate_by(const Vector& p_vector) const override;
    std::unique_ptr<Vector> difference_by(const Point& p_other) const override;
    Side side_of(const Line& p_line) const override;
    FloatPoint to_float() const override { return *this; }
    std::unique_ptr<Point> perpendicular_projection(const Line& p_line) const override;
    int compare_x(const Point& p_other) const override;
    int compare_y(const Point& p_other) const override;
    std::string to_string() const override;

    double distance_square(const FloatPoint& p_other) const;
    double distance(const FloatPoint& p_other) const;
    FloatPoint middle_point(const FloatPoint& p_other) const;
    double scalar_product(const FloatPoint& p_from, const FloatPoint& p_to) const;
    bool is_contained_in_box(const FloatPoint& p_a, const FloatPoint& p_b, double p_tolerance) const;
    FloatPoint turn_90_degree(int p_factor) const;
    FloatPoint turn_90_degree(int p_factor, const FloatPoint& p_from_point) const;
};

} // namespace planar
} // namespace geometry
} // namespace freerouting