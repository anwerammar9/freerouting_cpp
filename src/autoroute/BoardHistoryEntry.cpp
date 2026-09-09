#include "autoroute/BoardHistoryEntry.h"
#include "autoroute/RoutingBoard.h"
#include "settings/RouterScoringSettings.h"
#include "core/scoring/BoardStatistics.h"

#include <chrono>

namespace freerouting {
namespace autoroute {

BoardHistoryEntry::BoardHistoryEntry(std::shared_ptr<RoutingBoard> board, 
                                     const RouterScoringSettings& scoringSettings)
    : board_(std::move(board))
    , timestamp_(std::chrono::system_clock::now())
{
    if (!board_) {
        throw std::invalid_argument("Board cannot be null");
    }
    
    // Get statistics from the board
    statistics_ = board_->get_statistics();
    
    // Calculate normalized score
    score_ = statistics_->get_normalized_score(scoringSettings);
    
    // Get board hash
    hash_ = board_->get_hash();
    
    // Initialize restore count
    restore_count_ = 0;
}

} // namespace autoroute
} // namespace freerouting
