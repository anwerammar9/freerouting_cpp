#include "interactive/RatsNestItemInfo.h"

namespace freerouting {
namespace interactive {

std::string to_string(RatsNestItemType p_type)
{
    switch (p_type)
    {
        case RatsNestItemType::UNKNOWN: return "UNKNOWN";
        case RatsNestItemType::PIN: return "PIN";
        case RatsNestItemType::VIA: return "VIA";
        case RatsNestItemType::TRACE: return "TRACE";
        case RatsNestItemType::CONDUCTION_AREA: return "CONDUCTION_AREA";
        default: return "UNKNOWN";
    }
}

} // namespace interactive
} // namespace freerouting