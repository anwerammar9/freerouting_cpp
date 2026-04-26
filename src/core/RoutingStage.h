#pragma once

#include <string>
#include <array>

namespace freerouting {
namespace core {

/**
 * @brief The different stages a routing job can be in
 * 
 * This enum defines the various stages that a routing job progresses
 * through during the autorouting process, from idle state through
 * active routing to optimization phase.
 */
enum class RoutingStage {
    IDLE,          ///< The job hasn't started yet
    ROUTING,       ///< The board is currently being processed by the auto-router
    OPTIMIZATION   ///< The board is routed, and it is currently being optimized
};

/**
 * @brief Convert RoutingStage enum to string representation
 * 
 * @param stage The routing stage enum value
 * @return Human-readable string representation of the stage
 */
std::string to_string(RoutingStage stage);

/**
 * @brief Get description of what happens at each stage
 * 
 * @param stage The routing stage enum value
 * @return Detailed description of the stage's purpose
 */
std::string get_stage_description(RoutingStage stage);

/**
 * @brief Check if a stage represents active work
 * 
 * @param stage The routing stage enum value
 * @return true if the stage involves active processing
 */
constexpr bool is_active_stage(RoutingStage stage) {
    return stage == RoutingStage::ROUTING || stage == RoutingStage::OPTIMIZATION;
}

/**
 * @brief Get all routing stages in order
 * 
 * @return Array of all routing stages in logical order
 */
constexpr std::array<RoutingStage, 3> get_all_stages() {
    return {
        RoutingStage::IDLE,
        RoutingStage::ROUTING,
        RoutingStage::OPTIMIZATION
    };
}

} // namespace core
} // namespace freerouting
