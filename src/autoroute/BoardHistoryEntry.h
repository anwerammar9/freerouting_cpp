#ifndef AUTOROUTE_BOARD_HISTORY_ENTRY_H
#define AUTOROUTE_BOARD_HISTORY_ENTRY_H

#include <chrono>
#include <memory>
#include <string>

namespace freerouting::autoroute {

class RoutingBoard;

/**
 * Entry in the board history for tracking routing progress
 */
class BoardHistoryEntry
{
public:
    BoardHistoryEntry() = default;
    
    BoardHistoryEntry(const std::shared_ptr<RoutingBoard>& p_board, float p_score);
    
    const std::shared_ptr<RoutingBoard>& get_board() const { return board_; }
    float get_score() const { return score_; }
    const std::string& get_hash() const { return hash_; }
    int get_restore_count() const { return restore_count_; }
    void increment_restore_count() { restore_count_++; }
    
    auto get_timestamp() const { return timestamp_; }
    
    bool operator<(const BoardHistoryEntry& p_other) const;

private:
    std::shared_ptr<RoutingBoard> board_;
    float score_ = 0.0f;
    std::string hash_;
    int restore_count_ = 0;
    std::chrono::time_point<std::chrono::system_clock> timestamp_;
};

} // namespace freerouting::autoroute

#endif // AUTOROUTE_BOARD_HISTORY_ENTRY_H