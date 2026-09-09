#pragma once

#include <string>
#include <array>

namespace freerouting {
namespace autoroute {

/**
 * @brief Strategy for updating the board during autorouting
 * 
 * This enum defines different approaches for updating the board state
 * during the autorouting process, from immediate greedy updates
 * to global optimal solutions.
 */
enum class BoardUpdateStrategy {
    GREEDY,          ///< Update immediately with local optimal choices
    GLOBAL_OPTIMAL, ///< Wait for global optimal solution before updating
    HYBRID           ///< Combination of greedy and global optimal approaches
};

/**
 * @brief Convert BoardUpdateStrategy enum to string representation
 * 
 * @param strategy The board update strategy enum value
 * @return Human-readable string representation of the strategy
 */
std::string to_string(BoardUpdateStrategy strategy);

/**
 * @brief Get description of what the strategy does
 * 
 * @param strategy The board update strategy enum value
 * @return Detailed description of the strategy's approach
 */
std::string get_strategy_description(BoardUpdateStrategy strategy);

/**
 * @brief Check if the strategy uses immediate updates
 * 
 * @param strategy The board update strategy enum value
 * @return true if the strategy updates the board immediately
 */
constexpr bool uses_immediate_updates(BoardUpdateStrategy strategy) {
    return strategy == BoardUpdateStrategy::GREEDY;
}

/**
 * @brief Check if the strategy uses global optimization
 * 
 * @param strategy The board update strategy enum value
 * @return true if the strategy uses global optimization
 */
constexpr bool uses_global_optimization(BoardUpdateStrategy strategy) {
    return strategy == BoardUpdateStrategy::GLOBAL_OPTIMAL || strategy == BoardUpdateStrategy::HYBRID;
}

/**
 * @brief Get all board update strategies
 * 
 * @return Array of all board update strategies
 */
constexpr std::array<BoardUpdateStrategy, 3> get_all_strategies() {
    return {
        BoardUpdateStrategy::GREEDY,
        BoardUpdateStrategy::GLOBAL_OPTIMAL,
        BoardUpdateStrategy::HYBRID
    };
}

} // namespace autoroute
} // namespace freerouting