#ifndef AUTOROUTE_BOARD_UPDATE_STRATEGY_H
#define AUTOROUTE_BOARD_UPDATE_STRATEGY_H

namespace freerouting::autoroute {

/**
 * Strategy for updating the board during autorouting
 */
enum class BoardUpdateStrategy {
    GREEDY,
    GLOBAL_OPTIMAL,
    HYBRID
};

} // namespace freerouting::autoroute

#endif // AUTOROUTE_BOARD_UPDATE_STRATEGY_H