#pragma once

#include <string>
#include <array>

namespace freerouting {
namespace autoroute {

/**
 * @brief Strategy for selecting items during autorouting
 * 
 * This enum defines different approaches for selecting items
 * during the autorouting process, from sequential processing
 * to random or prioritized selection.
 */
enum class ItemSelectionStrategy {
    SEQUENTIAL,   ///< Process items in sequential order
    RANDOM,       ///< Process items in random order
    PRIORITIZED   ///< Process items based on priority
};

/**
 * @brief Convert ItemSelectionStrategy enum to string representation
 * 
 * @param strategy The item selection strategy enum value
 * @return Human-readable string representation of the strategy
 */
std::string to_string(ItemSelectionStrategy strategy);

/**
 * @brief Get description of what the strategy does
 * 
 * @param strategy The item selection strategy enum value
 * @return Detailed description of the strategy's approach
 */
std::string get_strategy_description(ItemSelectionStrategy strategy);

/**
 * @brief Check if the strategy uses deterministic ordering
 * 
 * @param strategy The item selection strategy enum value
 * @return true if the strategy uses deterministic ordering
 */
constexpr bool uses_deterministic_ordering(ItemSelectionStrategy strategy) {
    return strategy == ItemSelectionStrategy::SEQUENTIAL || strategy == ItemSelectionStrategy::PRIORITIZED;
}

/**
 * @brief Check if the strategy uses priority-based selection
 * 
 * @param strategy The item selection strategy enum value
 * @return true if the strategy uses priority for selection
 */
constexpr bool uses_priority_selection(ItemSelectionStrategy strategy) {
    return strategy == ItemSelectionStrategy::PRIORITIZED;
}

/**
 * @brief Get all item selection strategies
 * 
 * @return Array of all item selection strategies
 */
constexpr std::array<ItemSelectionStrategy, 3> get_all_selection_strategies() {
    return {
        ItemSelectionStrategy::SEQUENTIAL,
        ItemSelectionStrategy::RANDOM,
        ItemSelectionStrategy::PRIORITIZED
    };
}

} // namespace autoroute
} // namespace freerouting