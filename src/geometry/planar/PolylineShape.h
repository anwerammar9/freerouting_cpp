#pragma once

#include <memory>
#include <optional>
#include <vector>

#include "geometry/planar/Shape.h"

namespace freerouting {
namespace geometry {
namespace planar {

class Line;
class IntBox;
class FloatLine;
class TileShape;

/**
 * Abstract base class for shapes whose borders consist of straight lines.
 * Port of the Java PolylineShape contract (geometry/planar/PolylineShape.java).
 * Concrete subclasses must expose their corners through corner(i) and
 * border_line_count(); the generic methods are implemented on top of that.
 */
class PolylineShape : public Shape
{
public:
    using Shape::intersects;

    ~PolylineShape() override = default;

    // ---- pure virtuals supplied by concrete classes -------------------------

    virtual bool is_empty() const = 0;
    virtual bool is_bounded() const = 0;
    virtual int dimension() const = 0;
    virtual bool corner_is_bounded(int p_no) const = 0;
    virtual int border_line_count() const = 0;
    virtual IntPoint corner(int p_no) const = 0;
    virtual Line border_line(int p_no) const = 0;

    /**
     * Returns a division of this shape into convex pieces.
     * Port of the Java Area.split_to_convex contract. Concrete polyline
     * shapes (e.g. PolygonShape) cache their decomposition.
     */
    virtual const std::vector<std::unique_ptr<TileShape>>& split_to_convex() const = 0;

    /**
     * Returns the smallest surrounding octagon of this shape.
     */
    virtual IntOctagon bounding_octagon() const = 0;

    // ---- generic methods ------------------------------------------------

    /**
     * Returns all bounded corners of this shape.
     */
    std::vector<IntPoint> bounded_corners() const;

    /**
     * Returns an approximation of the p_no-th corner.
     */
    FloatPoint corner_approx(int p_no) const;

    /**
     * Returns an approximation of all corners of this shape.
     */
    std::vector<FloatPoint> corner_approx_arr() const;

    /**
     * If p_point is equal to a corner of this shape, the number of that corner
     * is returned; -1 otherwise.
     */
    int equals_corner(const IntPoint& p_point) const;

    /**
     * Returns the cumulative border line length of the shape.
     */
    double circumference() const override;

    /**
     * Returns the arithmetic middle of the corners of this shape.
     */
    FloatPoint centre_of_gravity() const override;

    /**
     * Checks if this shape is completely contained in p_box.
     */
    bool is_contained_in(const IntBox& p_box) const;

    /**
     * Returns the index of the corner of the shape, so that all other points of
     * the shape are to the right of the line from p_from_point to this corner.
     */
    int index_of_left_most_corner(const FloatPoint& p_from_point) const;

    /**
     * Returns the index of the corner of the shape, so that all other points of
     * the shape are to the left of the line from p_from_point to this corner.
     */
    int index_of_right_most_corner(const FloatPoint& p_from_point) const;

    /**
     * Returns a FloatLine result, so that result.a is an approximation of the
     * left most corner of this shape when viewed from p_from_point, and
     * result.b is an approximation of the right most corner.
     */
    std::optional<FloatLine> polar_line_segment(const FloatPoint& p_from_point) const;

    /**
     * Returns the previous border line or corner number of this shape.
     */
    int prev_no(int p_no) const;

    /**
     * Returns the next border line or corner number of this shape.
     */
    int next_no(int p_no) const;

    /**
     * Checks if this shape and p_line have a common point.
     */
    bool intersects(const Line& p_line) const;

    /**
     * Calculates the left most corner of this shape, when looked at from
     * p_from_point.
     */
    IntPoint left_most_corner(const IntPoint& p_from_point) const;

    /**
     * Calculates the right most corner of this shape, when looked at from
     * p_from_point.
     */
    IntPoint right_most_corner(const IntPoint& p_from_point) const;
};

} // namespace planar
} // namespace geometry
} // namespace freerouting