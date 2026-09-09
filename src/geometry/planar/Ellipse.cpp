#include "geometry/planar/Ellipse.h"

#include <algorithm>
#include <cmath>

namespace freerouting::geometry::planar {

namespace {

constexpr double PI = 3.14159265358979323846;

double normalize_rotation(double p_rotation, double p_radius_1, double p_radius_2)
{
    double curr_rotation = (p_radius_1 >= p_radius_2) ? p_rotation
                                                      : p_rotation + 0.5 * PI;
    while (curr_rotation >= PI)
    {
        curr_rotation -= PI;
    }
    while (curr_rotation < 0.0)
    {
        curr_rotation += PI;
    }
    return curr_rotation;
}

} // namespace

Ellipse::Ellipse(const FloatPoint& p_center,
                 double p_rotation,
                 double p_radius_1,
                 double p_radius_2)
    : center(p_center),
      rotation(normalize_rotation(p_rotation, p_radius_1, p_radius_2)),
      bigger_radius(std::max(p_radius_1, p_radius_2)),
      smaller_radius(std::min(p_radius_1, p_radius_2))
{
}

} // namespace freerouting::geometry::planar