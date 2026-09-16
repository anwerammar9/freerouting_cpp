#pragma once

#include <string>
#include <unordered_map>

namespace freerouting::core {

enum class RoutingJobState
{
    INVALID,
    QUEUED,
    READY_TO_START,
    RUNNING,
    PAUSED,
    COMPLETED,
    TIMED_OUT,
    CANCELLED,
    TERMINATED
};

inline std::string to_string(RoutingJobState p_state)
{
    switch (p_state)
    {
        case RoutingJobState::INVALID: return "INVALID";
        case RoutingJobState::QUEUED: return "QUEUED";
        case RoutingJobState::READY_TO_START: return "READY_TO_START";
        case RoutingJobState::RUNNING: return "RUNNING";
        case RoutingJobState::PAUSED: return "PAUSED";
        case RoutingJobState::COMPLETED: return "COMPLETED";
        case RoutingJobState::TIMED_OUT: return "TIMED_OUT";
        case RoutingJobState::CANCELLED: return "CANCELLED";
        case RoutingJobState::TERMINATED: return "TERMINATED";
        default: return "UNKNOWN";
    }
}

} // namespace freerouting::core
