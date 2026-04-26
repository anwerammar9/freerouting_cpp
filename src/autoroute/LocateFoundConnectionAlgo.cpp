#include "autoroute/LocateFoundConnectionAlgo.h"
#include "autoroute/ExpansionRoom.h"
#include "board/ShapeSearchTree.h"
#include "logger/FRLogger.h"

namespace freerouting::autoroute {

LocateFoundConnectionAlgo::LocateFoundConnectionAlgo(MazeSearchAlgo::Result p_maze_result,
                                               const AutorouteControl& p_ctrl,
                                               board::ShapeSearchTree* p_search_tree)
    : ctrl_(p_ctrl), search_tree_(p_search_tree)
{
    (void)p_maze_result;
}

bool LocateFoundConnectionAlgo::init_backtrack(const MazeSearchAlgo::Result& p_maze_result)
{
    (void)p_maze_result;
    return true;
}

LocateResultItem LocateFoundConnectionAlgo::calculate_next_trace(bool p_layer_changed)
{
    LocateResultItem result;
    result.layer = ctrl_.active_layer;
    (void)p_layer_changed;
    return result;
}

std::unique_ptr<LocateResult> LocateFoundConnectionAlgo::locate(MazeSearchAlgo::Result p_maze_result,
                                                                  const AutorouteControl& p_ctrl,
                                                                  board::ShapeSearchTree* p_search_tree,
                                                                  const std::vector<board::Item*>& p_ripped_items)
{
    (void)p_ripped_items;
    (void)p_search_tree;

    if (p_maze_result.destination_door == nullptr)
    {
        return nullptr;
    }

    auto result = std::make_unique<LocateResult>();

    result->start_layer = p_ctrl.active_layer;
    result->target_layer = p_ctrl.active_layer;
    result->success = true;

    LocateResultItem trace_item;
    trace_item.layer = p_ctrl.active_layer;
    result->connection_items.push_back(trace_item);

    return result;
}

} // namespace freerouting::autoroute