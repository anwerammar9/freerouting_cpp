#ifndef AUTOROUTE_ITEM_SELECTION_STRATEGY_H
#define AUTOROUTE_ITEM_SELECTION_STRATEGY_H

namespace freerouting::autoroute {

/**
 * Strategy for selecting items during autorouting
 */
enum class ItemSelectionStrategy {
    SEQUENTIAL,
    RANDOM,
    PRIORITIZED
};

} // namespace freerouting::autoroute

#endif // AUTOROUTE_ITEM_SELECTION_STRATEGY_H