#include "board/AngleRestriction.h"

namespace freerouting {
namespace board {

std::string to_string(AngleRestriction p_angle)
{
    switch (p_angle)
    {
        case AngleRestriction::NONE: return "NONE";
        case AngleRestriction::FORTYFIVE_DEGREE: return "FORTYFIVE_DEGREE";
        case AngleRestriction::NINETY_DEGREE: return "NINETY_DEGREE";
        default: return "NONE";
    }
}

int to_int(AngleRestriction p_angle)
{
    return static_cast<int>(p_angle);
}

AngleRestriction from_int(int p_value)
{
    if (p_value < 0 || p_value > 2)
    {
        return AngleRestriction::NONE;
    }
    return static_cast<AngleRestriction>(p_value);
}

} // namespace board
} // namespace freerouting