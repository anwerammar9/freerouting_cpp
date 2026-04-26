#include "StopRequestState.h"

namespace freerouting {
namespace core {

std::string to_string(StopRequestState state) {
    switch (state) {
        case StopRequestState::NONE:
            return "NONE";
        case StopRequestState::AUTO_ROUTER_ONLY:
            return "AUTO_ROUTER_ONLY";
        case StopRequestState::ALL:
            return "ALL";
        default:
            return "NONE";
    }
}

std::string get_stop_description(StopRequestState state) {
    switch (state) {
        case StopRequestState::NONE:
            return "No stop is requested";
        case StopRequestState::AUTO_ROUTER_ONLY:
            return "Only the auto-router is requested to stop";
        case StopRequestState::ALL:
            return "The entire thread is requested to stop";
        default:
            return "Unknown stop request state";
    }
}

} // namespace core
} // namespace freerouting
