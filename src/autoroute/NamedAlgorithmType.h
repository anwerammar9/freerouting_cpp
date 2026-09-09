#pragma once

#include <string>
#include <array>

namespace freerouting {
namespace autoroute {

enum class NamedAlgorithmType
{
    ROUTER,
    OPTIMIZER
};

/**
 * @brief Convert NamedAlgorithmType enum to string representation
 * 
 * @param type The algorithm type enum value
 * @return Human-readable string representation of the type
 */
std::string to_string(NamedAlgorithmType type);

/**
 * @brief Get description of what the algorithm type does
 * 
 * @param type The algorithm type enum value
 * @return Detailed description of the algorithm type's purpose
 */
std::string get_algorithm_description(NamedAlgorithmType type);

/**
 * @brief Check if the algorithm type is for routing
 * 
 * @param type The algorithm type enum value
 * @return true if the algorithm type is for routing
 */
constexpr bool is_router_algorithm(NamedAlgorithmType type) {
    return type == NamedAlgorithmType::ROUTER;
}

/**
 * @brief Check if the algorithm type is for optimization
 * 
 * @param type The algorithm type enum value
 * @return true if the algorithm type is for optimization
 */
constexpr bool is_optimizer_algorithm(NamedAlgorithmType type) {
    return type == NamedAlgorithmType::OPTIMIZER;
}

/**
 * @brief Get all algorithm types
 * 
 * @return Array of all algorithm types
 */
constexpr std::array<NamedAlgorithmType, 2> get_all_algorithm_types() {
    return {
        NamedAlgorithmType::ROUTER,
        NamedAlgorithmType::OPTIMIZER
    };
}

} // namespace autoroute
} // namespace freerouting