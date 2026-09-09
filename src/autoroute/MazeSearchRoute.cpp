#include "autoroute/MazeSearchRoute.h"
#include "board/RoutingBoard.h"
#include "logger/FRLogger.h"

namespace freerouting {
namespace autoroute {

MazeSearchRoute::MazeSearchRoute(board::RoutingBoard* p_board)
    : board_(p_board)
{
}

bool MazeSearchRoute::calculate_route()
{
    // Stub for the complex maze routing algorithm
    freerouting::logger::FRLogger::instance().info("MazeSearchRoute::calculate_route() called (stub)");
    return false;
}

} // namespace autoroute
} // namespace freerouting
