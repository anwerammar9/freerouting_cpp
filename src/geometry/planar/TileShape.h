#pragma once

#include <memory>
#include <optional>
#include <vector>

#include "geometry/planar/Point.h"
#include "geometry/planar/Vector.h"

namespace freerouting {
namespace geometry {
namespace planar {

class Direction;
class IntBox;
class IntOctagon;
class Simplex;
class FloatLine;
class ShapeBoundingDirections;

/**
 * Abstract base class for all convex tile shapes (Simplex, IntOctagon, IntBox).
 * A TileShape is a convex polygon defined as the intersection of half-planes,
 * each half-plane being the positive side of a directed border line.
 *
 * Port of the Java TileShape contract. The polymorphic operations
 * (intersection, cutout, cutout_from, offset, ...) are deferred in this C++
 * port because the concrete classes disagree on return types; they are
 * implemented per-class on the concrete types.
 */
class TileShape
{
public:
    virtual ~TileShape() = default;

    // ---- geometric queries -------------------------------------------------

    virtual bool is_empty() const = 0;
    virtual bool is_bounded() const = 0;
    virtual int dimension() const = 0;
    virtual int border_line_count() const = 0;
    virtual bool corner_is_bounded(int p_no) const = 0;
    virtual IntPoint corner(int p_no) const = 0;
    virtual FloatPoint corner_approx(int p_no) const = 0;
    virtual std::vector<FloatPoint> corner_approx_arr() const = 0;
    virtual Line border_line(int p_no) const = 0;
    virtual int border_line_index(const Line& p_line) const = 0;
    virtual bool is_IntBox() const = 0;
    virtual bool is_IntOctagon() const = 0;
    virtual IntBox bounding_box() const = 0;
    virtual Simplex to_Simplex() const = 0;
    virtual bool contains(const IntPoint& p_point) const = 0;
    virtual FloatPoint centre_of_gravity() const = 0;
    virtual double circumference() const = 0;

    /**
     * Calculates for this shape a surrounding regular tile shape with the
     * fixed directions of p_dirs. Implemented on the concrete tile types via
     * ShapeBoundingDirections::bounds. C++ returns IntOctagon for both the 4-
     * and 8-direction cases (an IntBox is representable as an octagon).
     */
    virtual IntOctagon bounding_shape(const ShapeBoundingDirections& p_dirs) const = 0;

    /**
     * Converts the physical instance of this shape to a simpler physical
     * instance, if possible (for example a Simplex to an IntOctagon).
     */
    virtual std::unique_ptr<TileShape> simplify() const = 0;
    virtual std::unique_ptr<TileShape> clone() const = 0;

    // ---- default methods (implemented on top of the pure virtuals) ---------

    bool is_outside(const IntPoint& p_point) const;
    bool contains_inside(const IntPoint& p_point) const;
    bool contains(const FloatPoint& p_point) const;
    bool contains(const FloatPoint& p_point, double p_tolerance) const;
    Side side_of_border(const FloatPoint& p_point, double p_tolerance) const;
    int contains_on_border_line_no(const IntPoint& p_point) const;

    /**
     * If p_point is equal to a corner of this shape, the number of that corner
     * is returned; -1 otherwise. Port of Java PolylineShape.equalsCorner().
     */
    int equals_corner(const IntPoint& p_point) const;

    /** Returns the previous border line or corner number of this shape. */
    int prev_no(int p_no) const;

    /** Returns the next border line or corner number of this shape. */
    int next_no(int p_no) const;

    /**
     * Calculates, if this shape and p_other have a common border piece and
     * returns the indices in this shape and p_other of the touching edge
     * lines. Otherwise an empty vector is returned. Used if the intersection
     * shape is 1-dimensional. Port of Java TileShape.touchingSides().
     */
    std::vector<int> touching_sides(const TileShape& p_other) const;

    /**
     * Returns the number of the border line hit by the ray from p_from_point
     * along p_direction, or -1 if p_from_point is outside this shape.
     * Mirrors Java TileShape.intersecting_border_line_no().
     */
    int intersecting_border_line_no(const Point& p_from_point,
                                     const Direction& p_direction) const;
    bool contains_on_border(const IntPoint& p_point) const;
    double area() const;
    double distance(const FloatPoint& p_point) const;
    double border_distance(const FloatPoint& p_point) const;
    double smallest_radius() const;
    std::optional<IntPoint> nearest_point(const IntPoint& p_from_point) const;
    std::optional<FloatPoint> nearest_point_approx(const FloatPoint& p_from_point) const;
    std::optional<IntPoint> nearest_border_point(const IntPoint& p_from_point) const;
    std::optional<FloatPoint> nearest_border_point_approx(const FloatPoint& p_from_point) const;
    std::vector<FloatPoint> nearest_border_points_approx(const FloatPoint& p_from_point, int p_count) const;
    int index_of_nearest_corner(const IntPoint& p_from_point) const;

    /**
     * Returns the index of the corner of this shape, so that all other points
     * of the shape are to the right of the line from p_from_point to this
     * corner. Port of Java PolylineShape.indexOfLeftMostCorner().
     */
    int index_of_left_most_corner(const FloatPoint& p_from_point) const;

    /**
     * Returns the index of the corner of this shape, so that all other points
     * of the shape are to the left of the line from p_from_point to this
     * corner. Port of Java PolylineShape.indexOfRightMostCorner().
     */
    int index_of_right_most_corner(const FloatPoint& p_from_point) const;

    std::optional<FloatLine> diagonal_corner_segment() const;

    /**
     * Returns a FloatLine result, so that result.a() is an approximation of
     * the left most corner of this shape when viewed from p_from_point, and
     * result.b() an approximation of the right most corner. Returns nothing
     * for an empty shape. Port of Java
     * PolylineShape.polarLineSegment().
     */
    std::optional<FloatLine> polar_line_segment(const FloatPoint& p_from_point) const;

    double length() const;
    Side side_of(const Line& p_line) const;
    double distance_to_the_left(const Line& p_line) const;
    std::vector<std::unique_ptr<TileShape>> split_to_convex() const;

    /**
     * Checks, if p_line_segment has a common point with the interior of this
     * shape. Port of Java TileShape.isIntersectedInteriorBy(LineSegment).
     */
    bool is_intersected_interior_by(const class LineSegment& p_line_segment) const;

    /**
     * Checks if the line segment defined by p_start_point, p_end_point and
     * p_line has a common point with the interior of this shape. Port of Java
     * TileShape.isIntersectedInteriorBy(Point, Point, Line).
     */
    bool is_intersected_interior_by(const IntPoint& p_start_point, const IntPoint& p_end_point,
                                    const Line& p_line) const;

    /**
     * Returns entrance points of p_polyline into this shape.
     * Each entry is {line_no, edge_no} where line_no is the index in
     * p_polyline.arr and edge_no is the border line index of this shape.
     * Mirrors Java TileShape.entrance_points(Polyline).
     */
    std::vector<std::pair<int, int>> entrance_points(const class Polyline& p_polyline) const;

    /**
     * Polymorphic offset — enlarges this shape by p_dist and returns the
     * result as a heap-allocated TileShape. Dispatched to the concrete
     * IntBox / IntOctagon / Simplex implementations. Headless routing
     * needs this for ShapeTraceEntries shove calculations.
     */
    std::unique_ptr<TileShape> offset(double p_dist) const;

    /**
     * Shrinks this shape by p_offset. If the shrunk shape is empty, the result
     * is the intersection of this shape with the bounding box of its centre of
     * gravity, so that the shape does not vanish completely. Port of Java
     * TileShape.shrink().
     */
    std::unique_ptr<TileShape> shrink(double p_offset) const;

    /**
     * Cuts p_shape out of this shape and divides the result into convex
     * pieces. Port of the Java TileShape.cutout: the actual division is
     * delegated to the concrete cutout_from pair (this may be a Simplex,
     * IntOctagon or IntBox at runtime), so the returned pieces may be any
     * mix of the concrete tile types.
     */
    std::vector<std::unique_ptr<TileShape>> cutout(const TileShape& p_shape) const;

    /**
     * Cuts out the parts of p_polyline in the interior of this shape and
     * returns the remaining pieces of the polyline. Pieces completely
     * contained in the border of this shape are not returned. Port of Java
     * TileShape.cutout(Polyline), which is the base of the spring-over
     * algorithm of TraceShover.
     */
    std::vector<class Polyline> cutout(const class Polyline& p_polyline) const;

    /**
     * Returns the smallest width of this convex shape. Dispatched to the
     * concrete IntBox / IntOctagon / Simplex implementations. Port of Java
     * TileShape.minWidth().
     */
    double min_width() const;

    /**
     * Returns the largest width of this convex shape. Dispatched to the
     * concrete IntBox / IntOctagon / Simplex implementations. Port of Java
     * TileShape.maxWidth().
     */
    double max_width() const;

    /**
     * Returns the smallest octagon with axis parallel and 45 degree border
     * lines containing this shape. Dispatched to the concrete IntOctagon /
     * Simplex implementations. Port of Java TileShape.boundingOctagon().
     */
    IntOctagon bounding_octagon() const;

    /**
     * Returns true, if p_box contains this shape. Dispatched to the concrete
     * IntBox / IntOctagon implementations; a Simplex is contained in the box
     * if all of its corners are. Port of Java TileShape.isContainedIn(IntBox),
     * which the shove algorithm uses to reject shapes outside the board.
     */
    bool is_contained_in(const IntBox& p_box) const;

    // ---- static factories ----------------------------------------------------

    /**
     * Creates a convex shape defined by the half-planes of p_line_arr and
     * simplifies the result to the simplest physical representation.
     */
    static std::unique_ptr<TileShape> get_instance(const std::vector<Line>& p_line_arr);

    /**
     * Returns the intersection of p_first and p_second, simplified to the
     * simplest physical representation. The C++ port of the Java
     * TileShape.intersection(TileShape) family, which dispatches on the
     * runtime types of both operands; the Simplex based computation produces
     * the same geometry for all concrete tile types and the result is
     * simplified to the concrete type afterwards.
     */
    static std::unique_ptr<TileShape> intersection_of(const TileShape& p_first, const TileShape& p_second);

    /**
     * Creates a TileShape from the corners of a convex IntPoint polygon.
     */
    static std::unique_ptr<TileShape> get_instance(const std::vector<IntPoint>& p_convex_polygon);

    /**
     * Creates a half-plane from a directed line.
     */
    static std::unique_ptr<TileShape> get_instance(const Line& p_line);

    /**
     * Creates a normalized IntOctagon from the input values.
     */
    static IntOctagon get_instance(int p_lx, int p_ly, int p_rx, int p_uy, int p_ulx, int p_lrx, int p_llx,
        int p_urx);

    /**
     * Creates a boxlike convex shape (IntOctagon).
     */
    static IntOctagon get_instance(int p_lower_left_x, int p_lower_left_y, int p_upper_right_x,
        int p_upper_right_y);

    /**
     * Creates the smallest IntBox containing p_point.
     */
    static IntBox get_instance(const IntPoint& p_point);
};

} // namespace planar
} // namespace geometry
} // namespace freerouting