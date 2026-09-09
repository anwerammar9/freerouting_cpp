#include "autoroute/InsertFoundConnectionAlgo.h"
#include "autoroute/LocateFoundConnectionAlgo.h"
#include "board/RoutingBoard.h"
#include "logger/FRLogger.h"

#include <vector>

namespace freerouting::autoroute {

InsertFoundConnectionAlgo::InsertFoundConnectionAlgo(board::RoutingBoard* p_board, const AutorouteControl& p_ctrl)
    : board_(p_board), ctrl_(&p_ctrl)
{
}

bool InsertFoundConnectionAlgo::insert_via(int p_from_layer, int p_to_layer)
{
    (void)p_from_layer;
    (void)p_to_layer;
    return true;
}

bool InsertFoundConnectionAlgo::insert_trace(LocateResultItem& p_trace)
{
    (void)p_trace;
    return true;
}

bool InsertFoundConnectionAlgo::insert(std::unique_ptr<LocateResult> p_connection,
                                   board::RoutingBoard* p_board,
                                   const AutorouteControl& p_ctrl)
{
    (void)p_board;
    (void)p_ctrl;

    if (!p_connection || p_connection->connection_items.empty())
    {
        freerouting::logger::FRLogger::instance().warn("InsertFoundConnectionAlgo: no connection to insert");
        return false;
    }

    InsertFoundConnectionAlgo inserter(p_board, p_ctrl);

    for (auto& trace_item : p_connection->connection_items)
    {
        if (!inserter.insert_trace(trace_item))
        {
            freerouting::logger::FRLogger::instance().warn("InsertFoundConnectionAlgo: failed to insert trace");
            return false;
        }
    }

    return true;
}

} // namespace freerouting::autoroute