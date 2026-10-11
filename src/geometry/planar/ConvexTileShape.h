#pragma once

#include <memory>

#include "geometry/planar/ConvexShape.h"
#include "geometry/planar/TileShape.h"

namespace freerouting::geometry::planar {

/**
 * A ConvexShape holding a TileShape.
 *
 * In Java TileShape implements ConvexShape, so a padstack (ConvexShape[] per
 * layer) can contain circles, boxes, octagons and simplices alike. In this C++
 * port TileShape is not derived from Shape; this adapter closes that gap
 * wherever a Java ConvexShape may be a TileShape (padstack shapes, pin
 * shapes). All queries are delegated to the wrapped tile shape.
 */
class ConvexTileShape : public ConvexShape
{
public:
    explicit ConvexTileShape(std::unique_ptr<TileShape> p_tile);
    ConvexTileShape(const ConvexTileShape& p_other);

    /** The wrapped tile shape. */
    const TileShape& tile() const { return *tile_; }

    // Shape
    IntBox bounding_box() const override;
    bool intersects(const Shape& p_other) const override;
    bool contains(const IntPoint& p_point) const override;
    std::string to_string() const override;
    double circumference() const override;
    double area() const override;
    FloatPoint centre_of_gravity() const override;
    bool is_outside(const IntPoint& p_point) const override;
    bool contains_inside(const IntPoint& p_point) const override;
    bool contains_on_border(const IntPoint& p_point) const override;
    bool contains(const FloatPoint& p_point) const override;
    double distance(const FloatPoint& p_point) const override;
    double border_distance(const FloatPoint& p_point) const override;
    std::unique_ptr<TileShape> bounding_tile() const override;
    double smallest_radius() const override;
    std::unique_ptr<Shape> enlarge(double p_offset) const override;
    bool intersects(const IntBox& p_box) const override;
    bool intersects(const IntOctagon& p_oct) const override;
    bool intersects(const Simplex& p_simplex) const override;
    bool intersects(const Circle& p_circle) const override;

    // ConvexShape
    std::unique_ptr<ConvexShape> offset(double p_distance) const override;
    std::unique_ptr<ConvexShape> shrink(double p_offset) const override;
    double max_width() const override;
    double min_width() const override;
    IntOctagon bounding_shape(const ShapeBoundingDirections& p_dirs) const override;

private:
    std::unique_ptr<TileShape> tile_;
};

} // namespace freerouting::geometry::planar
