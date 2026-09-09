#pragma once

#include "geometry/planar/IntOctagon.h"

namespace freerouting {
namespace geometry {
namespace planar {

class ConvexShape;
class IntBox;
class Simplex;
class Circle;
class PolygonShape;

/**
 * Describes the functionality for the fixed directions of a RegularTileShape.
 * The C++ port returns IntOctagon (an IntBox is representable as an octagon
 * via IntBox::to_IntOctagon), so a single superset return type covers both the
 * 4- and 8-direction bounding results.
 */
class ShapeBoundingDirections
{
public:
    virtual ~ShapeBoundingDirections();

    /**
     * Returns the count of the fixed directions.
     */
    virtual int count() const = 0;

    /**
     * Calculates for an arbitrary ConvexShape a surrounding regular tile shape
     * with these fixed directions. Used in the implementation of the search trees.
     */
    virtual IntOctagon bounds(const ConvexShape& p_shape) const = 0;

    /**
     * Auxiliary function to implement the same function with parameter type ConvexShape.
     */
    virtual IntOctagon bounds(const IntBox& p_box) const = 0;

    /**
     * Auxiliary function to implement the same function with parameter type ConvexShape.
     */
    virtual IntOctagon bounds(const IntOctagon& p_oct) const = 0;

    /**
     * Auxiliary function to implement the same function with parameter type ConvexShape.
     */
    virtual IntOctagon bounds(const Simplex& p_simplex) const = 0;

    /**
     * Auxiliary function to implement the same function with parameter type ConvexShape.
     */
    virtual IntOctagon bounds(const Circle& p_circle) const = 0;

    /**
     * Auxiliary function to implement the same function with parameter type ConvexShape.
     */
    virtual IntOctagon bounds(const PolygonShape& p_polygon) const = 0;
};

} // namespace planar
} // namespace geometry
} // namespace freerouting