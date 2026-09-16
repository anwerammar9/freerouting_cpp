#pragma once

#include <algorithm>
#include <string>

namespace freerouting::core {

enum class RoutingJobPriority
{
    LOWEST,
    LOW,
    BELOW_NORMAL,
    NORMAL,
    ABOVE_NORMAL,
    HIGH,
    HIGHEST
};

inline float to_value(RoutingJobPriority p_priority)
{
    switch (p_priority)
    {
        case RoutingJobPriority::LOWEST: return 0.0f;
        case RoutingJobPriority::LOW: return 2.0f;
        case RoutingJobPriority::BELOW_NORMAL: return 4.0f;
        case RoutingJobPriority::NORMAL: return 5.0f;
        case RoutingJobPriority::ABOVE_NORMAL: return 6.0f;
        case RoutingJobPriority::HIGH: return 8.0f;
        case RoutingJobPriority::HIGHEST: return 10.0f;
        default: return 5.0f;
    }
}

inline std::string to_string(RoutingJobPriority p_priority)
{
    switch (p_priority)
    {
        case RoutingJobPriority::LOWEST: return "LOWEST";
        case RoutingJobPriority::LOW: return "LOW";
        case RoutingJobPriority::BELOW_NORMAL: return "BELOW_NORMAL";
        case RoutingJobPriority::NORMAL: return "NORMAL";
        case RoutingJobPriority::ABOVE_NORMAL: return "ABOVE_NORMAL";
        case RoutingJobPriority::HIGH: return "HIGH";
        case RoutingJobPriority::HIGHEST: return "HIGHEST";
        default: return "NORMAL";
    }
}

} // namespace freerouting::core
