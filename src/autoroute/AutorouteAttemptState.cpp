#include "autoroute/AutorouteAttemptState.h"

namespace freerouting {
namespace autoroute {

std::string to_string(AutorouteAttemptState state) {
    switch (state) {
        case AutorouteAttemptState::UNKNOWN:
            return "UNKNOWN";
        case AutorouteAttemptState::SKIPPED:
            return "SKIPPED";
        case AutorouteAttemptState::NO_UNCONNECTED_NETS:
            return "NO_UNCONNECTED_NETS";
        case AutorouteAttemptState::CONNECTED_TO_PLANE:
            return "CONNECTED_TO_PLANE";
        case AutorouteAttemptState::ALREADY_CONNECTED:
            return "ALREADY_CONNECTED";
        case AutorouteAttemptState::NO_CONNECTIONS:
            return "NO_CONNECTIONS";
        case AutorouteAttemptState::ROUTED:
            return "ROUTED";
        case AutorouteAttemptState::FAILED:
            return "FAILED";
        case AutorouteAttemptState::INSERT_ERROR:
            return "INSERT_ERROR";
        default:
            return "UNKNOWN";
    }
}

std::string get_state_description(AutorouteAttemptState state) {
    switch (state) {
        case AutorouteAttemptState::UNKNOWN:
            return "Unknown result";
        case AutorouteAttemptState::SKIPPED:
            return "Item was skipped";
        case AutorouteAttemptState::NO_UNCONNECTED_NETS:
            return "Item has no unconnected nets";
        case AutorouteAttemptState::CONNECTED_TO_PLANE:
            return "Item is connected to a conduction plane";
        case AutorouteAttemptState::ALREADY_CONNECTED:
            return "Item is already connected";
        case AutorouteAttemptState::NO_CONNECTIONS:
            return "The item has no connections to nets";
        case AutorouteAttemptState::ROUTED:
            return "Item was successfully routed";
        case AutorouteAttemptState::FAILED:
            return "Routing failed";
        case AutorouteAttemptState::INSERT_ERROR:
            return "Error inserting item";
        default:
            return "Unknown autoroute attempt state";
    }
}

} // namespace autoroute
} // namespace freerouting
