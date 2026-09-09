#pragma once

#include <vector>
#include <memory>

namespace freerouting {

namespace board {
class RoutingBoard;
class Item;
}

namespace autoroute {

class MazeSearchRoute {
public:
    MazeSearchRoute(board::RoutingBoard* p_board);
    virtual ~MazeSearchRoute() = default;

    // Additional autorouting logic variables
    board::RoutingBoard* board_;

    bool calculate_route();
};

} // namespace autoroute
} // namespace freerouting
