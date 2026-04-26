#ifndef AUTOROUTE_LOCATE_FOUND_CONNECTION_ALGO_H
#define AUTOROUTE_LOCATE_FOUND_CONNECTION_ALGO_H

#include <vector>
#include <memory>

#include "autoroute/MazeSearchAlgo.h"
#include "autoroute/AutorouteControl.h"
#include "geometry/planar/Point.h"
#include "board/Item.h"

namespace freerouting::board {
class ShapeSearchTree;
}

namespace freerouting::autoroute {

struct LocateResultItem
{
    int layer = 0;
    int section_no = 0;
};

struct LocateResult
{
    std::vector<LocateResultItem> connection_items;
    board::Item* start_item = nullptr;
    int start_layer = 0;
    board::Item* target_item = nullptr;
    int target_layer = 0;
    bool success = false;
};

class LocateFoundConnectionAlgo
{
public:
    static std::unique_ptr<LocateResult> locate(MazeSearchAlgo::Result p_maze_result,
                                        const AutorouteControl& p_ctrl,
                                        board::ShapeSearchTree* p_search_tree,
                                        const std::vector<board::Item*>& p_ripped_items);

protected:
    LocateFoundConnectionAlgo(MazeSearchAlgo::Result p_maze_result,
                               const AutorouteControl& p_ctrl,
                               board::ShapeSearchTree* p_search_tree);

    AutorouteControl ctrl_;
    board::ShapeSearchTree* search_tree_ = nullptr;
    std::vector<LocateResultItem> connection_items_;
    board::Item* start_item_ = nullptr;
    int start_layer_ = 0;
    board::Item* target_item_ = nullptr;
    int target_layer_ = 0;

    std::vector<geometry::planar::FloatPoint> backtrack_points_;
    int current_from_index_ = 0;
    int current_to_index_ = 0;

    bool init_backtrack(const MazeSearchAlgo::Result& p_maze_result);
    LocateResultItem calculate_next_trace(bool p_layer_changed);
};

} // namespace freerouting::autoroute

#endif // AUTOROUTE_LOCATE_FOUND_CONNECTION_ALGO_H