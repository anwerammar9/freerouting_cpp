#pragma once

#include <string>
#include <array>

namespace freerouting {
namespace core {

/**
 * @brief Defines the stop state of a StoppableThread
 * 
 * This enum controls the granularity of stop requests for background
 * threads, allowing either selective stopping of just the autorouter
 * or complete termination of the thread.
 */
enum class StopRequestState {
    NONE,              ///< No stop is requested
    AUTO_ROUTER_ONLY,  ///< Only the auto-router is requested to stop
    ALL                ///< The entire thread is requested to stop
};

/**
 * @brief Convert StopRequestState enum to string representation
 * 
 * @param state The stop request state enum value
 * @return Human-readable string representation of the state
 */
std::string to_string(StopRequestState state);

/**
 * @brief Get description of what the stop request means
 * 
 * @param state The stop request state enum value
 * @return Detailed description of the stop request's impact
 */
std::string get_stop_description(StopRequestState state);

/**
 * @brief Check if the stop request affects the autorouter
 * 
 * @param state The stop request state enum value
 * @return true if the autorouter should stop
 */
constexpr bool should_stop_autorouter(StopRequestState state) {
    return state == StopRequestState::AUTO_ROUTER_ONLY || state == StopRequestState::ALL;
}

/**
 * @brief Check if the stop request affects the entire thread
 * 
 * @param state The stop request state enum value
 * @return true if the entire thread should stop
 */
constexpr bool should_stop_thread(StopRequestState state) {
    return state == StopRequestState::ALL;
}

/**
 * @brief Get all stop request states
 * 
 * @return Array of all stop request states in order of severity
 */
constexpr std::array<StopRequestState, 3> get_all_stop_states() {
    return {
        StopRequestState::NONE,
        StopRequestState::AUTO_ROUTER_ONLY,
        StopRequestState::ALL
    };
}

} // namespace core
} // namespace freerouting
