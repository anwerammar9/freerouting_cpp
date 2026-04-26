#include "autoroute/AutorouteAttemptResult.h"

namespace freerouting {
namespace autoroute {

AutorouteAttemptResult::AutorouteAttemptResult()
        : state(AutorouteAttemptState::UNKNOWN), details("")
{
}

AutorouteAttemptResult::AutorouteAttemptResult(AutorouteAttemptState p_state)
        : state(p_state), details("")
{
}

AutorouteAttemptResult::AutorouteAttemptResult(AutorouteAttemptState p_state, const std::string& p_details)
        : state(p_state), details(p_details)
{
}

std::string AutorouteAttemptResult::to_string() const
{
    std::string result;
    switch (state)
    {
        case AutorouteAttemptState::UNKNOWN:
            result = "UNKNOWN";
            break;
        case AutorouteAttemptState::SKIPPED:
            result = "SKIPPED";
            break;
        case AutorouteAttemptState::NO_UNCONNECTED_NETS:
            result = "NO_UNCONNECTED_NETS";
            break;
        case AutorouteAttemptState::CONNECTED_TO_PLANE:
            result = "CONNECTED_TO_PLANE";
            break;
        case AutorouteAttemptState::ALREADY_CONNECTED:
            result = "ALREADY_CONNECTED";
            break;
        case AutorouteAttemptState::NO_CONNECTIONS:
            result = "NO_CONNECTIONS";
            break;
        case AutorouteAttemptState::ROUTED:
            result = "ROUTED";
            break;
        case AutorouteAttemptState::NOT_ROUTED:
            result = "NOT_ROUTED";
            break;
        case AutorouteAttemptState::FAILED:
            result = "FAILED";
            break;
        case AutorouteAttemptState::INSERT_ERROR:
            result = "INSERT_ERROR";
            break;
        default:
            result = "UNKNOWN";
            break;
    }

    for (char& c : result)
    {
        c = static_cast<char>(toupper(c));
    }

    return result + ": " + details;
}

} // namespace autoroute
} // namespace freerouting