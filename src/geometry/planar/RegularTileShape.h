#pragma once

#include <memory>

#include "geometry/planar/TileShape.h"

namespace freerouting {
namespace geometry {
namespace planar {

class IntBox;
class IntOctagon;

/**
 * Abstract base class for TileShapes whose border lines may have only
 * directions out of a fixed set of regular directions (orthogonal or
 * multiple-of-45-degree), as for example axis parallel box shapes.
 *
 * Port of the Java RegularTileShape (geometry/planar/RegularTileShape.java).
 * In this C++ port IntBox and IntOctagon inherit from this class, mirroring
 * the Java hierarchy. The polymorphic compare/union ops accept the base type
 * and dispatch on the runtime type of the operand.
 */
class RegularTileShape : public TileShape
{
public:
    ~RegularTileShape() override = default;

    // Keep the base contains() overloads visible next to the overloads below.
    using TileShape::contains;

    /**
     * Compares the edgelines of index p_edge_no of this regular TileShape and
     * p_other. Returns Side.ON_THE_LEFT, if the edgeline of this shape is to
     * the left of the edgeline of p_other; Side.COLLINEAR, if the edgelines
     * are equal, and Side.ON_THE_RIGHT, if this edgeline is to the right of
     * the edgeline of p_other.
     */
    virtual Side compare(const RegularTileShape& p_other, int p_edge_no) const = 0;

    /**
     * Calculates the smallest RegularTileShape containing this shape and
     * p_other. The runtime type of the result (IntBox or IntOctagon) depends
     * on the runtime type of the argument.
     */
    virtual std::unique_ptr<RegularTileShape> union_with(const RegularTileShape& p_other) const = 0;

    /**
     * Returns true, if p_other is completely contained in this shape.
     */
    virtual bool contains(const RegularTileShape& p_other) const = 0;

    /**
     * Returns true, if this shape and p_other have a non-empty intersection.
     */
    virtual bool intersects(const RegularTileShape& p_other) const = 0;

    // ---- auxiliary operations on the concrete operand types -----------------

    virtual Side compare(const IntBox& p_other, int p_edge_no) const = 0;
    virtual Side compare(const IntOctagon& p_other, int p_edge_no) const = 0;
    virtual bool is_contained_in(const IntBox& p_other) const = 0;
    virtual bool is_contained_in(const IntOctagon& p_other) const = 0;
};

} // namespace planar
} // namespace geometry
} // namespace freerouting