#pragma once

#include "geometry/planar/Direction.h"

namespace freerouting::geometry::planar {

/**
 * Enum for the eight 45-degree directions starting from right in
 * counterclock sense to down45.
 */
enum class FortyfiveDegreeDirection
{
    RIGHT,
    RIGHT45,
    UP,
    UP45,
    LEFT,
    LEFT45,
    DOWN,
    DOWN45
};

/**
 * Returns the IntDirection belonging to p_direction.
 */
const IntDirection& get_direction(FortyfiveDegreeDirection p_direction);

} // namespace freerouting::geometry::planar