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
 * Implements the abstract class ShapeBoundingDirections as the 8 directions,
 * which are multiples of 45 degree. The class is a singleton with the only
 * instantiation INSTANCE.
 */
class FortyfiveDegreeBoundingDirections : public ShapeBoundingDirections
{
public:
    static const FortyfiveDegreeBoundingDirections INSTANCE;

    int count() const override { return 8; }

    IntOctagon bounds(const ConvexShape& p_shape) const override;
    IntOctagon bounds(const IntBox& p_box) const override;
    IntOctagon bounds(const IntOctagon& p_oct) const override;
    IntOctagon bounds(const Simplex& p_simplex) const override;
    IntOctagon bounds(const Circle& p_circle) const override;
    IntOctagon bounds(const PolygonShape& p_polygon) const override;

private:
    FortyfiveDegreeBoundingDirections() = default;
};

} // namespace planar
} // namespace geometry
} // namespace freerouting