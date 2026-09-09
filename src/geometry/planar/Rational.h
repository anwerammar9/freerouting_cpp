#pragma once

#include "geometry/planar/Vector.h"
#include "geometry/planar/Point.h"
#include "datastructures/Signum.h"
#include "datastructures/BigIntAux.h"

#include <memory>
#include <stdexcept>
#include <string>

namespace freerouting {
namespace geometry {
namespace planar {

class Direction;
class RationalPoint;

/**
 * Analog RationalPoint, but implementing the functionality of a Vector instead
 * of the functionality of a Point. The 3 coordinates x, y, z are infinite
 * precision integers representing the vector (x/z, y/z). The denominator z is
 * normalized to be positive (the constructor negates all coordinates if z < 0).
 */
class RationalVector : public Vector
{
public:
    const datastructures::BigInt x;
    const datastructures::BigInt y;
    const datastructures::BigInt z;

    RationalVector(const datastructures::BigInt& p_x, const datastructures::BigInt& p_y,
                   const datastructures::BigInt& p_z);
    explicit RationalVector(const IntVector& p_vector);

    bool is_zero() const override;
    std::unique_ptr<Vector> negate() const override;
    std::unique_ptr<Vector> add(const Vector& p_other) const override;
    std::unique_ptr<Vector> add(const IntVector& p_other) const;
    std::unique_ptr<Vector> add(const RationalVector& p_other) const;
    Side side_of(const Vector& p_other) const override;
    datastructures::BigInt determinant(const Vector& p_other) const override;
    datastructures::BigInt determinant(const IntVector& p_other) const;
    datastructures::BigInt determinant(const RationalVector& p_other) const;
    double scalar_product(const Vector& p_other) const override;
    double scalar_product(const IntVector& p_other) const;
    double scalar_product(const RationalVector& p_other) const;
    datastructures::Signum projection(const Vector& p_other) const;
    datastructures::Signum projection(const IntVector& p_other) const;
    datastructures::Signum projection(const RationalVector& p_other) const;
    FloatPoint to_float() const override;
    double length_approx() const override;

    std::unique_ptr<Vector> change_length_approx(double p_length) const;
    std::unique_ptr<Vector> turn_90_degree(int p_factor) const;
    std::unique_ptr<Vector> mirror_at_y_axis() const;
    std::unique_ptr<Vector> mirror_at_x_axis() const;
    std::unique_ptr<Direction> to_normalized_direction() const;

    std::unique_ptr<Point> add_to(const IntPoint& p_point) const override;
    std::unique_ptr<Point> add_to(const RationalPoint& p_point) const;

    bool is_orthogonal() const;
    bool is_diagonal() const;
    bool is_multiple_of_45_degree() const { return is_orthogonal() || is_diagonal(); }
};

/**
 * Implementation of points in the projective plane represented by 3 coordinates
 * x, y, z, which are infinite precision integers. The affine point represented
 * by the projective point (x, y, z) is (x/z, y/z). The projective plane with
 * integer coordinates contains in addition the so-called line at infinity
 * (all projective points with z = 0).
 */
class RationalPoint : public Point
{
public:
    const datastructures::BigInt x;
    const datastructures::BigInt y;
    const datastructures::BigInt z;

    /**
     * Creates a RationalPoint from 3 BigIntegers p_x, p_y and p_z representing
     * the point (p_x / p_z, p_y / p_z). Throws std::invalid_argument if the
     * denominator p_z is < 0.
     */
    RationalPoint(const datastructures::BigInt& p_x, const datastructures::BigInt& p_y,
                  const datastructures::BigInt& p_z);
    explicit RationalPoint(const IntPoint& p_point);

    FloatPoint to_float() const override;
    bool is_infinite() const override;
    IntBox surrounding_box() const override;
    IntOctagon surrounding_octagon() const override;
    bool is_contained_in(const IntBox& p_box) const override;
    bool equals(const Point* p_other) const override;

    std::unique_ptr<Point> translate_by(const Vector& p_vector) const override;
    std::unique_ptr<Point> translate_by(const IntVector& p_vector) const;
    std::unique_ptr<Point> translate_by(const RationalVector& p_vector) const;

    std::unique_ptr<Vector> difference_by(const Point& p_other) const override;
    std::unique_ptr<Vector> difference_by(const IntPoint& p_other) const;
    std::unique_ptr<Vector> difference_by(const RationalPoint& p_other) const;

    Side side_of(const Line& p_line) const override;
    std::unique_ptr<Point> perpendicular_projection(const Line& p_line) const override;

    int compare_x(const Point& p_other) const override;
    int compare_y(const Point& p_other) const override;
    int compare_x(const IntPoint& p_other) const;
    int compare_y(const IntPoint& p_other) const;
    int compare_x(const RationalPoint& p_other) const;
    int compare_y(const RationalPoint& p_other) const;

    std::string to_string() const override;
};

} // namespace planar
} // namespace geometry
} // namespace freerouting