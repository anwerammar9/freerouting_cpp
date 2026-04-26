#include "autoroute/BoardUpdateStrategy.h"

namespace freerouting {
namespace autoroute {

std::string to_string(BoardUpdateStrategy strategy) {
    switch (strategy) {
        case BoardUpdateStrategy::GREEDY:
            return "GREEDY";
        case BoardUpdateStrategy::GLOBAL_OPTIMAL:
            return "GLOBAL_OPTIMAL";
        case BoardUpdateStrategy::HYBRID:
            return "HYBRID";
        default:
            return "GREEDY";
    }
}

std::string get_strategy_description(BoardUpdateStrategy strategy) {
    switch (strategy) {
        case BoardUpdateStrategy::GREEDY:
            return "Update immediately with local optimal choices";
        case BoardUpdateStrategy::GLOBAL_OPTIMAL:
            return "Wait for global optimal solution before updating";
        case BoardUpdateStrategy::HYBRID:
            return "Combination of greedy and global optimal approaches";
        default:
            return "Unknown board update strategy";
    }
}

} // namespace autoroute
} // namespace freerouting
