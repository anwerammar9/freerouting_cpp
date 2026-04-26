#pragma once

#include <string>

namespace freerouting {
namespace interactive {

enum class RatsNestItemType
{
    UNKNOWN,
    PIN,
    VIA,
    TRACE,
    CONDUCTION_AREA
};

std::string to_string(RatsNestItemType p_type);

struct RatsNestItemInfo
{
    RatsNestItemType type = RatsNestItemType::UNKNOWN;
    std::string component_name;
    std::string name;
    std::string text;
};

} // namespace interactive
} // namespace freerouting