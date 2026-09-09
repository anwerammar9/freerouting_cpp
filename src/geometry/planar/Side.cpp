#include "geometry/planar/Side.h"

namespace freerouting {
namespace geometry {
namespace planar {

Side SideUtils::of(double p_value) {
        if (p_value > 0) return Side::ON_THE_LEFT;
        if (p_value < 0) return Side::ON_THE_RIGHT;
        return Side::COLLINEAR;
    }

Side SideUtils::of(int p_value)
{
    return of(static_cast<double>(p_value));
}

std::string SideUtils::to_string(Side side)
{
        switch (side) {
            case Side::ON_THE_LEFT: return "ON_THE_LEFT";
            case Side::ON_THE_RIGHT: return "ON_THE_RIGHT";
            case Side::COLLINEAR: return "COLLINEAR";
            case Side::NO_SIDE: return "NO_SIDE";
            default: return "UNKNOWN";
        }
    }

Side SideUtils::negate(Side side)
{
        switch (side) {
            case Side::ON_THE_LEFT: return Side::ON_THE_RIGHT;
            case Side::ON_THE_RIGHT: return Side::ON_THE_LEFT;
            default: return side;
        }
    }

} // namespace planar
} // namespace geometry
} // namespace freerouting