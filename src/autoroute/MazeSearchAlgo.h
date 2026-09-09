#ifndef AUTOROUTE_MAZE_SEARCH_ALGO_H
#define AUTOROUTE_MAZE_SEARCH_ALGO_H

#include "autoroute/MazeSearchElement.h"
#include "autoroute/ExpansionRoom.h"
#include "autoroute/DestinationDistance.h"
#include "autoroute/AutorouteControl.h"
#include "geometry/planar/Point.h"
#include "board/Item.h"

#include <vector>
#include <queue>
#include <memory>
#include <set>

namespace freerouting::board {
class RoutingBoard;
class ShapeSearchTree;
}

namespace freerouting::datastructures {
class Stoppable;
}

namespace freerouting::autoroute {

class MazeSearchAlgo {
public:
    struct Result {
        ExpansionDoor* destination_door = nullptr;
        int section_no_of_destination_door = 0;
    };

    MazeSearchAlgo(board::RoutingBoard* p_board, datastructures::Stoppable* p_stoppable);
    ~MazeSearchAlgo();

    static std::unique_ptr<MazeSearchAlgo> get_instance(board::RoutingBoard* p_board,
                                                         datastructures::Stoppable* p_stoppable);

    bool init(const std::set<board::Item*>& p_start_items,
              const std::set<board::Item*>& p_dest_items,
              board::ShapeSearchTree* p_search_tree);

    Result find_connection();
    bool occupy_next_element();
    std::vector<ExpansionDoor*> reconstruct_path(const Result& p_result);

    void set_net_no(int p_net_no) { net_no_ = p_net_no; }
    void set_control(const AutorouteControl& p_ctrl) { control_ = p_ctrl; }

    bool is_initialized() const { return initialized_; }

private:
    board::RoutingBoard* board_ = nullptr;
    datastructures::Stoppable* stoppable_ = nullptr;
    int net_no_ = 0;
    AutorouteControl control_;
    bool initialized_ = false;

    std::priority_queue<MazeListElement, std::vector<MazeListElement>, MazeListElementComparator> expansion_list_;
    
    std::unique_ptr<DestinationDistance> destination_distance_;
    board::ShapeSearchTree* search_tree_ = nullptr;

    std::set<board::Item*> start_items_;
    std::set<board::Item*> dest_items_;

    bool is_destination(ExpansionDoor* p_door) const;
    bool is_stop_requested() const;

    void expand_to_neighbours(const MazeListElement& p_element);
    void expand_to_room_neighbours(ExpansionRoom* p_room, ExpansionDoor* p_backtrack_door,
                                   int p_section_no_of_backtrack_door, double p_expansion_value,
                                   bool p_room_ripped, MazeSearchElement::Adjustment p_adjustment);
};

} // namespace freerouting::autoroute

#endif // AUTOROUTE_MAZE_SEARCH_ALGO_H