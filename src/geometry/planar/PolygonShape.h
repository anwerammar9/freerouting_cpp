#pragma once

#include <memory>
#include <optional>
#include <vector>

#include "geometry/planar/PolylineShape.h"
#include "geometry/planar/IntBox.h"

namespace freerouting {
namespace geometry {
namespace planar {

class Circle;
class Simplex;
class IntOctagon;
class Line;
class Polygon;
class TileShape;
class Polyline;
class ShapeBoundingDirections;

/**
 * Shape described by a closed polygon of corner points. The corners are ordered
 * in counterclock sense around the border of the shape. The corners are
 * normalized, so that the corner with the lowest y-value comes first; in case
 * of equal y-value the corner with the lowest x-value comes first.
 *
 * Port of the Java PolygonShape (geometry/planar/PolygonShape.java).
 */
class PolygonShape : public PolylineShape
{
public:
    std::vector<IntPoint> corners;

    PolygonShape() = default;

    /**
     * Creates a normalized PolygonShape from p_corner_arr, mirroring the Java
     * constructor: the corner order is reverted to counterclock sense if
     * necessary, duplicated and collinear corner points are removed and the
     * corner with lowest (y, x) comes first.
     */
    explicit PolygonShape(const std::vector<IntPoint>& p_corner_arr);

    /**
     * Creates a normalized PolygonShape from the corners of p_polygon,
     * mirroring the Java constructor.
     */
    explicit PolygonShape(const Polygon& p_polygon);

    ~PolygonShape() override = default;

    IntBox bounding_box() const override;
    IntOctagon bounding_octagon() const override;

    bool intersects(const Shape& other) const override;
    bool intersects(const Circle& p_circle) const override;
    bool intersects(const Simplex& p_simplex) const override;
    bool intersects(const IntOctagon& p_oct) const override;
    bool intersects(const IntBox& p_box) const override;

    bool contains(const IntPoint& point) const override;
    bool contains(const FloatPoint& p_point) const override;
    bool contains_inside(const IntPoint& p_point) const override;
    bool contains_inside(const FloatPoint& p_point) const override;
    bool is_outside(const IntPoint& p_point) const override;
    bool is_outside(const FloatPoint& p_point) const override;
    bool contains_on_border(const IntPoint& p_point) const override;
    bool contains_on_border(const FloatPoint& p_point) const override;

    bool is_empty() const override;
    bool is_bounded() const override;
    int dimension() const override;
    bool corner_is_bounded(int p_no) const override;
    int border_line_count() const override;
    IntPoint corner(int p_no) const override;
    Line border_line(int p_no) const override;

    IntOctagon bounding_shape(const ShapeBoundingDirections& p_dirs) const override;

    bool is_convex() const;
    PolygonShape convex_hull() const;
    std::unique_ptr<TileShape> bounding_tile() const override;

    double area() const override;
    double border_distance(const FloatPoint& p_point) const override;
    double distance(const FloatPoint& p_point) const override;
    double smallest_radius() const override;
    std::optional<FloatPoint> nearest_point_approx(const FloatPoint& p_from_point) const;

    std::unique_ptr<Shape> enlarge(double p_offset) const override;
    std::vector<std::unique_ptr<Polyline>> cutout(const Polyline& p_polyline) const override;

    PolygonShape turn_90_degree(int p_factor, const IntPoint& p_pole) const;
    PolygonShape rotate_approx(double p_angle, const FloatPoint& p_pole) const;
    PolygonShape translate_by(const Vector& p_vector) const;
    PolygonShape mirror_horizontal(const IntPoint& p_pole) const;
    PolygonShape mirror_vertical(const IntPoint& p_pole) const;

    const std::vector<std::unique_ptr<TileShape>>& split_to_convex() const override;

    std::string to_string() const override;

private:
    std::vector<std::unique_ptr<TileShape>> split_to_convex_recu() const;

    mutable std::shared_ptr<std::vector<std::unique_ptr<TileShape>>> precalculated_convex_pieces;
};

} // namespace planar
} // namespace geometry
} // namespace freerouting