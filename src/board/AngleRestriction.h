#pragma once

#include <string>

namespace freerouting {
namespace board {

enum class AngleRestriction
{
    NONE = 0,
    FORTYFIVE_DEGREE = 1,
    NINETY_DEGREE = 2
};

std::string to_string(AngleRestriction p_angle);
int to_int(AngleRestriction p_angle);
AngleRestriction from_int(int p_value);

} // namespace board
} // namespace freerouting