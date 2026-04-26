#pragma once

#include <string>
#include <array>

namespace freerouting {
namespace autoroute {

/**
 * @brief The possible results of auto-routing a connection
 * 
 * This enum defines the various states that can result from
 * attempting to autoroute a connection, from successful routing
 * to various failure conditions.
 */
enum class AutorouteAttemptState {
    UNKNOWN,              ///< Unknown result
    SKIPPED,              ///< Item was skipped
    NO_UNCONNECTED_NETS,  ///< Item has no unconnected nets
    CONNECTED_TO_PLANE,   ///< Item is connected to a conduction plane
    ALREADY_CONNECTED,    ///< Item is already connected
    NO_CONNECTIONS,       ///< The item has no connections to nets
    NOT_ROUTED,          ///< Item was not routed
    ROUTED,               ///< Item was successfully routed
    FAILED,               ///< Routing failed
    INSERT_ERROR          ///< Error inserting item
};

/**
 * @brief Convert AutorouteAttemptState enum to string representation
 * 
 * @param state The autoroute attempt state enum value
 * @return Human-readable string representation of the state
 */
std::string to_string(AutorouteAttemptState state);

/**
 * @brief Get description of what the state means
 * 
 * @param state The autoroute attempt state enum value
 * @return Detailed description of the state's meaning
 */
std::string get_state_description(AutorouteAttemptState state);

/**
 * @brief Check if the state represents a successful routing attempt
 * 
 * @param state The autoroute attempt state enum value
 * @return true if the routing was successful
 */
constexpr bool is_successful(AutorouteAttemptState state) {
    return state == AutorouteAttemptState::ROUTED || 
           state == AutorouteAttemptState::ALREADY_CONNECTED ||
           state == AutorouteAttemptState::CONNECTED_TO_PLANE;
}

/**
 * @brief Check if the state represents a failure condition
 * 
 * @param state The autoroute attempt state enum value
 * @return true if the routing failed
 */
constexpr bool is_failure(AutorouteAttemptState state) {
    return state == AutorouteAttemptState::FAILED || 
           state == AutorouteAttemptState::INSERT_ERROR;
}

/**
 * @brief Check if the state represents a skipped or not-attempted routing
 * 
 * @param state The autoroute attempt state enum value
 * @return true if routing was skipped or not attempted
 */
constexpr bool is_skipped(AutorouteAttemptState state) {
    return state == AutorouteAttemptState::SKIPPED || 
           state == AutorouteAttemptState::NO_UNCONNECTED_NETS ||
           state == AutorouteAttemptState::NO_CONNECTIONS;
}

/**
 * @brief Check if the state represents a completed routing attempt
 * 
 * @param state The autoroute attempt state enum value
 * @return true if the routing attempt was completed (success or failure)
 */
constexpr bool is_completed(AutorouteAttemptState state) {
    return is_successful(state) || is_failure(state);
}

/**
 * @brief Get all autoroute attempt states
 * 
 * @return Array of all autoroute attempt states
 */
constexpr std::array<AutorouteAttemptState, 9> get_all_states() {
    return {
        AutorouteAttemptState::UNKNOWN,
        AutorouteAttemptState::SKIPPED,
        AutorouteAttemptState::NO_UNCONNECTED_NETS,
        AutorouteAttemptState::CONNECTED_TO_PLANE,
        AutorouteAttemptState::ALREADY_CONNECTED,
        AutorouteAttemptState::NO_CONNECTIONS,
        AutorouteAttemptState::ROUTED,
        AutorouteAttemptState::FAILED,
        AutorouteAttemptState::INSERT_ERROR
    };
}

} // namespace autoroute
} // namespace freerouting
