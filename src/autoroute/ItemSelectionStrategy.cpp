#include "autoroute/ItemSelectionStrategy.h"

namespace freerouting {
namespace autoroute {

std::string to_string(ItemSelectionStrategy strategy) {
    switch (strategy) {
        case ItemSelectionStrategy::SEQUENTIAL:
            return "SEQUENTIAL";
        case ItemSelectionStrategy::RANDOM:
            return "RANDOM";
        case ItemSelectionStrategy::PRIORITIZED:
            return "PRIORITIZED";
        default:
            return "SEQUENTIAL";
    }
}

std::string get_strategy_description(ItemSelectionStrategy strategy) {
    switch (strategy) {
        case ItemSelectionStrategy::SEQUENTIAL:
            return "Process items in sequential order";
        case ItemSelectionStrategy::RANDOM:
            return "Process items in random order";
        case ItemSelectionStrategy::PRIORITIZED:
            return "Process items based on priority";
        default:
            return "Unknown item selection strategy";
    }
}

} // namespace autoroute
} // namespace freerouting
