#include "geometry/planar/FortyfiveDegreeDirection.h"

namespace freerouting::geometry::planar {

const IntDirection& get_direction(FortyfiveDegreeDirection p_direction)
{
    switch (p_direction)
    {
        case FortyfiveDegreeDirection::RIGHT: return IntDirection::RIGHT;
        case FortyfiveDegreeDirection::RIGHT45: return IntDirection::RIGHT45;
        case FortyfiveDegreeDirection::UP: return IntDirection::UP;
        case FortyfiveDegreeDirection::UP45: return IntDirection::UP45;
        case FortyfiveDegreeDirection::LEFT: return IntDirection::LEFT;
        case FortyfiveDegreeDirection::LEFT45: return IntDirection::LEFT45;
        case FortyfiveDegreeDirection::DOWN: return IntDirection::DOWN;
        case FortyfiveDegreeDirection::DOWN45: return IntDirection::DOWN45;
    }
    return IntDirection::NULL_DIR;
}

} // namespace freerouting::geometry::planar