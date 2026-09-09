#ifndef GEOMETRY_PLANAR_SIDE_H
#define GEOMETRY_PLANAR_SIDE_H

#include "geometry/planar/Point.h"

#include <string>

namespace freerouting {
namespace geometry {
namespace planar {

class SideUtils
{
public:
    static Side of(double p_value);
    static Side of(int p_value);
    static std::string to_string(Side side);
    static Side negate(Side side);
};

} // namespace planar
} // namespace geometry
} // namespace freerouting

#endif
