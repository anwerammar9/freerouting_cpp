#pragma once

#include <vector>

#include "geometry/planar/Point.h"

namespace freerouting::geometry::planar {

/**
 * A Polygon is a list of points in the plane, where no 2 consecutive points
 * may be equal and no 3 consecutive points collinear.
 */
class Polygon
{
public:
    /**
     * Creates a polygon from p_point_arr. Multiple points and points, which are
     * collinear with its previous and next point will be removed.
     */
    explicit Polygon(const std::vector<IntPoint>& p_point_arr);

    /**
     * Returns the array of corners of this polygon.
     */
    std::vector<IntPoint> corner_array() const;

    /**
     * Reverts the order of the corners of this polygon.
     */
    Polygon revert_corners() const;

    /**
     * Returns the winding number of this polygon, treated as closed. It will be
     * {@literal >} 0, if the corners are in counterclock sense, and {@literal <} 0,
     * if the corners are in clockwise sense.
     */
    int winding_number_after_closing() const;

private:
    std::vector<IntPoint> corners_;
};

} // namespace freerouting::geometry::planar