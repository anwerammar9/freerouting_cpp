#pragma once

#include <string>
#include <memory>

namespace freerouting::board {
class RoutingBoard;
}

namespace freerouting::core {
struct RouterCounters;
}

namespace freerouting::autoroute {
class NamedAlgorithm;
}

namespace freerouting::autoroute::events {

class BoardUpdatedEvent {
public:
    BoardUpdatedEvent(freerouting::autoroute::NamedAlgorithm* p_source, int32_t p_routed_count,
                      int32_t p_incomplete_count, board::RoutingBoard* p_board)
        : source_(p_source), routed_count_(p_routed_count),
          incomplete_count_(p_incomplete_count), board_(p_board) {}

    board::RoutingBoard* get_board() const { return board_; }
    int32_t get_routed_count() const { return routed_count_; }
    int32_t get_incomplete_count() const { return incomplete_count_; }
    NamedAlgorithm* get_source() const { return source_; }

private:
    NamedAlgorithm* source_;
    int32_t routed_count_;
    int32_t incomplete_count_;
    board::RoutingBoard* board_;
};

} // namespace freerouting::autoroute::events
