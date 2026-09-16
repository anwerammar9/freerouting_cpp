#pragma once

#include <string>
#include <memory>

namespace freerouting::board {
class RoutingBoard;
}

namespace freerouting::autoroute {
class NamedAlgorithm;
}

namespace freerouting::autoroute::events {

class BoardSnapshotEvent {
public:
    BoardSnapshotEvent(freerouting::autoroute::NamedAlgorithm* p_source, board::RoutingBoard* p_board)
        : source_(p_source), board_(p_board) {}

    board::RoutingBoard* get_board() const { return board_; }
    NamedAlgorithm* get_source() const { return source_; }

private:
    NamedAlgorithm* source_;
    board::RoutingBoard* board_;
};

} // namespace freerouting::autoroute::events
