#pragma once

#include <memory>

namespace freerouting {
namespace board {

enum class FixedState
{
    NOT_FIXED,
    SHOVE_FIXED,
    USER_FIXED,
    SYSTEM_FIXED,
    FROM_DESIGN
};

} // namespace board
} // namespace freerouting