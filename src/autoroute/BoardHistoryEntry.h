#pragma once

#include <chrono>
#include <memory>
#include <string>

namespace freerouting {
namespace autoroute {

class RoutingBoard;
class RouterScoringSettings;
class BoardStatistics;

/**
 * @brief Entry in the board history for tracking routing progress
 * 
 * This class stores information about a board state during routing,
 * including the board itself, its score, hash, statistics, and timestamp.
 * It implements comparable interface for sorting by score.
 */
class BoardHistoryEntry
{
public:
    /**
     * @brief Default constructor (deleted - board must be provided)
     */
    BoardHistoryEntry() = delete;
    
    /**
     * @brief Construct a new board history entry
     * 
     * @param board The routing board to store
     * @param scoringSettings The scoring settings for calculating the score
     */
    BoardHistoryEntry(std::shared_ptr<RoutingBoard> board, 
                     const RouterScoringSettings& scoringSettings);
    
    // Accessors
    const std::shared_ptr<RoutingBoard>& get_board() const { return board_; }
    float get_score() const { return score_; }
    const std::string& get_hash() const { return hash_; }
    const std::shared_ptr<BoardStatistics>& get_statistics() const { return statistics_; }
    std::chrono::time_point<std::chrono::system_clock> get_timestamp() const { return timestamp_; }
    int get_restore_count() const { return restore_count_; }
    
    // Mutators
    void increment_restore_count() { restore_count_++; }
    
    /**
     * @brief Comparison operator for sorting by score
     * 
     * @param other The other entry to compare with
     * @return true if this entry has a lower score than the other
     */
    bool operator<(const BoardHistoryEntry& other) const {
        return score_ < other.score_;
    }
    
    /**
     * @brief Three-way comparison operator (C++20)
     * 
     * @param other The other entry to compare with
     * @return Strong ordering result
     */
    auto operator<=>(const BoardHistoryEntry& other) const {
        return score_ <=> other.score_;
    }

private:
    std::shared_ptr<RoutingBoard> board_;
    float score_;
    std::string hash_;
    std::shared_ptr<BoardStatistics> statistics_;
    std::chrono::time_point<std::chrono::system_clock> timestamp_;
    int restore_count_ = 0;
};

} // namespace autoroute
} // namespace freerouting