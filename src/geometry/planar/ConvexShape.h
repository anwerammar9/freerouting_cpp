#pragma once

#include <memory>

#include "geometry/planar/Shape.h"

namespace freerouting::geometry::planar {

class IntOctagon;
class ShapeBoundingDirections;

/**
 * A shape is defined as convex, if for each line segment with both endpoints
 * contained in the shape the whole segment is contained completely in the shape.
 */
class ConvexShape : public Shape
{
public:
    /**
     * Calculates the offset shape by p_distance. If p_distance {@literal >} 0,
     * the shape will be enlarged, else the result shape will be smaller.
     */
    virtual std::unique_ptr<ConvexShape> offset(double p_distance) const = 0;

    /**
     * Shrinks the shape by p_offset. The result shape will not be empty.
     */
    virtual std::unique_ptr<ConvexShape> shrink(double p_offset) const = 0;

    /**
     * Returns the maximum diameter of the shape.
     */
    virtual double max_width() const = 0;

    /**
     * Returns the minimum diameter of the shape.
     */
    virtual double min_width() const = 0;

    /**
     * Returns a surrounding regular tile shape with the fixed directions of p_dirs.
     */
    virtual IntOctagon bounding_shape(const ShapeBoundingDirections& p_dirs) const = 0;
};

} // namespace freerouting::geometry::planar