#pragma once

#include <memory>
#include <string>

#include "geometry/planar/Vector.h"
#include "datastructures/Signum.h"

namespace freerouting {
namespace geometry {
namespace planar {

class IntDirection;

class Direction
{
public:
    virtual ~Direction() = default;

    virtual std::unique_ptr<Vector> get_vector() const = 0;
    virtual bool is_orthogonal() const = 0;
    virtual bool is_diagonal() const = 0;
    virtual bool is_multiple_of_45_degree() const;
    virtual std::unique_ptr<Direction> turn_45_degree(int p_factor) const = 0;
    virtual std::unique_ptr<Direction> opposite() const = 0;

    bool equals(const Direction* p_other) const;
    Side side_of(const Direction& p_other) const;
    freerouting::datastructures::Signum projection(const Direction& p_other) const;
    std::unique_ptr<Direction> middle_approx(const Direction& p_other) const;

    int compare_from(const Direction& p_1, const Direction& p_2) const;
    double angle_approx() const;

    virtual int compareTo(const IntDirection& p_other) const = 0;
    virtual std::string to_string() const = 0;

    static std::unique_ptr<Direction> get_instance(const Vector& p_vector);
    static std::unique_ptr<Direction> get_instance(const Point& p_from, const Point& p_to);
    static std::unique_ptr<Direction> get_instance_approx(double p_angle);
};

class IntDirection : public Direction
{
public:
    const int64_t x;
    const int64_t y;

    IntDirection(int64_t p_x, int64_t p_y);
    IntDirection(const IntDirection& other) = default;

    std::unique_ptr<Vector> get_vector() const override;
    bool is_orthogonal() const override;
    bool is_diagonal() const override;
    std::unique_ptr<Direction> turn_45_degree(int p_factor) const override;
    std::unique_ptr<Direction> opposite() const override;

    int compareTo(const IntDirection& p_other) const override;
    std::string to_string() const override;

    double determinant(const IntDirection& p_other) const;
    bool equals(const IntDirection& p_other) const;
    IntDirection turn_45_degree_int(int p_factor) const;

    static const IntDirection NULL_DIR;
    static const IntDirection RIGHT;
    static const IntDirection RIGHT45;
    static const IntDirection UP;
    static const IntDirection UP45;
    static const IntDirection LEFT;
    static const IntDirection LEFT45;
    static const IntDirection DOWN;
    static const IntDirection DOWN45;
};

/**
 * Implements the abstract class Direction as a tuple of infinite precision
 * integers. Port of Java BigIntDirection.
 */
class BigIntDirection : public Direction
{
public:
    const datastructures::BigInt x;
    const datastructures::BigInt y;

    BigIntDirection(const datastructures::BigInt& p_x, const datastructures::BigInt& p_y);
    explicit BigIntDirection(const IntDirection& p_dir);

    std::unique_ptr<Vector> get_vector() const override;
    bool is_orthogonal() const override;
    bool is_diagonal() const override;
    std::unique_ptr<Direction> turn_45_degree(int p_factor) const override;
    std::unique_ptr<Direction> opposite() const override;

    int compareTo(const IntDirection& p_other) const override;
    int compareTo(const BigIntDirection& p_other) const;
    std::string to_string() const override;
};

} // namespace planar
} // namespace geometry
} // namespace freerouting