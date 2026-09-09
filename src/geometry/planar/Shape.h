#pragma once

#include <string>
#include <memory>
#include <optional>
#include <vector>

#include "geometry/planar/Point.h"
#include "geometry/planar/Vector.h"

namespace freerouting {
namespace geometry {
namespace planar {

class IntBox;
class IntOctagon;
class Simplex;
class TileShape;
class Polyline;
class Circle;
class ShapeBoundingDirections;

/**
 * Abstract connected 2-dimensional shape. Port of the Java Shape interface
 * (geometry/planar/Shape.java). The geometric methods of the Java interface
 * are declared here as virtuals with conservative default implementations;
 * concrete shapes (Circle, PolygonShape, ...) override them with exact math.
 */
class Shape
{
public:
    virtual ~Shape() = default;
    virtual IntBox bounding_box() const = 0;
    virtual bool intersects(const Shape& other) const = 0;
    virtual bool contains(const IntPoint& point) const = 0;
    virtual std::string to_string() const = 0;

    // ---- Java Shape interface surface (Shape.java) ------------------------

    /**
     * Returns the length of the border of this shape.
     */
    virtual double circumference() const;

    /**
     * Returns the content of the area of this shape.
     */
    virtual double area() const;

    /**
     * Returns the gravity point of this shape.
     */
    virtual FloatPoint centre_of_gravity() const;

    /**
     * Returns true, if p_point is not contained in the inside or the boundary
     * of the shape.
     */
    virtual bool is_outside(const IntPoint& p_point) const;
    virtual bool is_outside(const FloatPoint& p_point) const;

    /**
     * Returns true, if p_point is contained in this shape, but not on the border.
     */
    virtual bool contains_inside(const IntPoint& p_point) const;
    virtual bool contains_inside(const FloatPoint& p_point) const;

    /**
     * Returns true, if p_point lies exact on the boundary of the shape.
     */
    virtual bool contains_on_border(const IntPoint& p_point) const;
    virtual bool contains_on_border(const FloatPoint& p_point) const;

    virtual bool contains(const FloatPoint& p_point) const;

    /**
     * Returns the distance between p_point and its nearest point on the shape.
     * 0, if p_point is contained in this shape.
     */
    virtual double distance(const FloatPoint& p_point) const;

    /**
     * Returns the distance between p_point and its nearest point on the border
     * of the shape.
     */
    virtual double border_distance(const FloatPoint& p_point) const;

    /**
     * Returns a bounding tile shape of this shape.
     */
    virtual std::unique_ptr<TileShape> bounding_tile() const;

    /**
     * Returns the bounding regular tile shape with the fixed directions p_dirs.
     */
    virtual IntOctagon bounding_shape(const ShapeBoundingDirections& p_dirs) const;

    /**
     * Returns the smallest distance from the centre of gravity to the border
     * of the shape.
     */
    virtual double smallest_radius() const;

    /**
     * Returns the offset shape of this shape by offsetting the boundary by
     * p_distance to the outside.
     */
    virtual std::unique_ptr<Shape> enlarge(double p_offset) const;

    /**
     * Checks, if this shape and p_other have a nonempty intersection.
     */
    virtual bool intersects(const IntBox& p_box) const;
    virtual bool intersects(const IntOctagon& p_oct) const;
    virtual bool intersects(const Simplex& p_simplex) const;
    virtual bool intersects(const Circle& p_circle) const;

    /**
     * Cuts out the parts of p_polyline in the interior of this shape. Not yet
     * implemented in Java either (warns and returns null); returns empty in C++.
     */
    virtual std::vector<std::unique_ptr<Polyline>> cutout(const Polyline& p_polyline) const;
};

class Area : public Shape
{
public:
    virtual ~Area() = default;

    virtual IntBox bounding_box() const override = 0;
    virtual IntOctagon bounding_octagon() const = 0;
    virtual bool contains(const IntPoint& point) const override = 0;
    virtual bool contains(const FloatPoint& p_point) const = 0;
    virtual bool is_empty() const = 0;
    virtual bool is_bounded() const = 0;
    virtual int dimension() const = 0;
    virtual bool is_contained_in(const IntBox& p_box) const = 0;
    virtual std::optional<FloatPoint> nearest_point_approx(const FloatPoint& p_from_point) const = 0;
    virtual std::vector<FloatPoint> corner_approx_arr() const = 0;
    virtual const std::vector<std::unique_ptr<TileShape>>& split_to_convex() const = 0;
    virtual std::unique_ptr<Area> clone() const = 0;

    virtual std::unique_ptr<Area> turn_90_degree(int p_factor, const IntPoint& p_pole) const = 0;
    virtual std::unique_ptr<Area> rotate_approx(double p_angle, const FloatPoint& p_pole) const = 0;
    virtual std::unique_ptr<Area> translate_by(const Vector& p_vector) const = 0;
    virtual std::unique_ptr<Area> mirror_horizontal(const IntPoint& p_pole) const = 0;
    virtual std::unique_ptr<Area> mirror_vertical(const IntPoint& p_pole) const = 0;
};

class PolygonArea : public Area
{
public:
    std::vector<IntPoint> corners;

    explicit PolygonArea(const std::vector<IntPoint>& p_corners);
    PolygonArea() = default;

    IntBox bounding_box() const override;
    IntOctagon bounding_octagon() const override;
    bool contains(const IntPoint& point) const override;
    bool contains(const FloatPoint& p_point) const override;
    bool is_empty() const override;
    bool is_bounded() const override;
    int dimension() const override;
    bool is_contained_in(const IntBox& p_box) const override;
    std::optional<FloatPoint> nearest_point_approx(const FloatPoint& p_from_point) const override;
    std::vector<FloatPoint> corner_approx_arr() const override;
    const std::vector<std::unique_ptr<TileShape>>& split_to_convex() const override;
    std::unique_ptr<Area> clone() const override;

    std::unique_ptr<Area> turn_90_degree(int p_factor, const IntPoint& p_pole) const override;
    std::unique_ptr<Area> rotate_approx(double p_angle, const FloatPoint& p_pole) const override;
    std::unique_ptr<Area> translate_by(const Vector& p_vector) const override;
    std::unique_ptr<Area> mirror_horizontal(const IntPoint& p_pole) const override;
    std::unique_ptr<Area> mirror_vertical(const IntPoint& p_pole) const override;

    bool intersects(const Shape& other) const override;
    std::string to_string() const override;

private:
    mutable std::shared_ptr<std::vector<std::unique_ptr<TileShape>>> precalculated_convex_pieces_;
};

class Circle : public Shape
{
public:
    const IntPoint center;
    const int64_t radius;

    Circle(const IntPoint& p_center, int64_t p_radius);

    IntBox bounding_box() const override;
    IntOctagon bounding_octagon() const;
    bool intersects(const Shape& other) const override;
    bool contains(const IntPoint& point) const override;
    std::string to_string() const override;

    bool is_empty() const;
    bool is_bounded() const;
    int dimension() const;
    double circumference() const override;
    double area() const override;
    FloatPoint centre_of_gravity() const override;
    bool is_outside(const IntPoint& p_point) const override;
    bool is_outside(const FloatPoint& p_point) const override;
    bool contains(const FloatPoint& p_point) const override;
    bool contains_inside(const IntPoint& p_point) const override;
    bool contains_inside(const FloatPoint& p_point) const override;
    bool contains_on_border(const IntPoint& p_point) const override;
    bool contains_on_border(const FloatPoint& p_point) const override;
    double distance(const FloatPoint& p_point) const override;
    double border_distance(const FloatPoint& p_point) const override;
    double smallest_radius() const override;
    double max_width() const;
    double min_width() const;
    std::unique_ptr<TileShape> bounding_tile() const override;

    /**
     * Creates a bounding tile shape around this circle, so that the length of
     * the line segments of the tile is at most p_max_segment_length.
     * Port of Circle.bounding_tile(int).
     */
    std::unique_ptr<TileShape> bounding_tile(int p_max_segment_length) const;

    /**
     * Splits this circle into convex tiles. The result is a single tile: the
     * bounding tile of this circle. Port of Circle.split_to_convex().
     */
    std::vector<std::unique_ptr<TileShape>> split_to_convex() const;

    bool is_contained_in(const IntBox& p_box) const;
    bool intersects(const Circle& p_other) const override;
    bool intersects(const IntBox& p_box) const override;
    bool intersects(const IntOctagon& p_oct) const override;
    bool intersects(const Simplex& p_simplex) const override;
    Circle turn_90_degree(int p_factor, const IntPoint& p_pole) const;
    Circle rotate_approx(double p_angle, const FloatPoint& p_pole) const;
    Circle mirror_vertical(const IntPoint& p_pole) const;
    Circle mirror_horizontal(const IntPoint& p_pole) const;
    Circle offset(double p_offset) const;
    Circle shrink(double p_offset) const;
    std::unique_ptr<Shape> enlarge(double p_offset) const override;
    Circle translate_by(const Vector& p_vector) const;
    FloatPoint nearest_point_approx(const FloatPoint& p_point) const;

    /**
     * Returns a surrounding regular tile shape with the fixed directions
     * of p_dirs. Port of Circle.bounding_shape(ShapeBoundingDirections).
     */
    IntOctagon bounding_shape(const ShapeBoundingDirections& p_dirs) const override;

    /**
     * Returns this circle as its own border.
     * Port of Circle.get_border().
     */
    Circle get_border() const;

    /**
     * A circle has no holes. Port of Circle.get_holes().
     */
    std::vector<std::unique_ptr<Shape>> get_holes() const;

    /**
     * A circle has no corners. Port of Circle.corner_approx_arr().
     */
    std::vector<FloatPoint> corner_approx_arr() const;

    /**
     * Cuts p_polyline with this circle. Not yet implemented in Java either
     * (logs a warning and returns null); returns an empty vector in C++.
     * Port of Circle.cutout(Polyline).
     */
    std::vector<std::unique_ptr<Polyline>> cutout(const Polyline& p_polyline) const override;
};

} // namespace planar
} // namespace geometry
} // namespace freerouting