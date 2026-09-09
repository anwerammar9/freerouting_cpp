#pragma once

#include "geometry/planar/ShapeBoundingDirections.h"

namespace freerouting {
namespace geometry {
namespace planar {

class IntBox;
class Simplex;
class Circle;
class PolygonShape;

/**
 * Implements the abstract class ShapeBoundingDirections as the 4 orthogonal
 * directions. The class is a singleton with the only instantiation INSTANCE.
 */
class OrthogonalBoundingDirections : public ShapeBoundingDirections
{
public:
    static const OrthogonalBoundingDirections INSTANCE;

    int count() const override { return 4; }

    IntOctagon bounds(const ConvexShape& p_shape) const override;
    IntOctagon bounds(const IntBox& p_box) const override;
    IntOctagon bounds(const IntOctagon& p_oct) const override;
    IntOctagon bounds(const Simplex& p_simplex) const override;
    IntOctagon bounds(const Circle& p_circle) const override;
    IntOctagon bounds(const PolygonShape& p_polygon) const override;

private:
    OrthogonalBoundingDirections() = default;
};

} // namespace planar
} // namespace geometry
} // namespace freerouting