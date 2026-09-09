#pragma once

#include "geometry/planar/Point.h"

namespace freerouting::geometry::planar {

/**
 * Describes functionality of an ellipse in the plane. Does not implement the
 * ConvexShape interface, because coordinates are float.
 */
class Ellipse
{
public:
    const FloatPoint center;
    /**
     * Rotation of the ellipse in radian normed to 0 {@literal <=} rotation {@literal <} pi
     */
    const double rotation;
    const double bigger_radius;
    const double smaller_radius;

    /**
     * Creates a new instance of Ellipse.
     */
    Ellipse(const FloatPoint& p_center,
            double p_rotation,
            double p_radius_1,
            double p_radius_2);
};

} // namespace freerouting::geometry::planar