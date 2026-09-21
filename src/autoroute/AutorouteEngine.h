#ifndef AUTOROUTE_AUTOROUTE_ENGINE_H
#define AUTOROUTE_AUTOROUTE_ENGINE_H

#include <memory>
#include <vector>
#include <string>
#include <set>

#include "autoroute/AutorouteControl.h"
#include "autoroute/AutorouteAttemptResult.h"
#include "board/Item.h"
#include "board/ShapeSearchTree.h"

namespace freerouting::board {
class RoutingBoard;
}

namespace freerouting::datastructures {
class TimeLimit;
class Stoppable;
}

namespace freerouting::autoroute {

class IncompleteExpansionRoom;
class IncompleteFreeSpaceExpansionRoom;
class CompleteExpansionRoom;
class CompleteFreeSpaceExpansionRoom;

class AutorouteEngine {
public:
    AutorouteEngine(board::RoutingBoard* p_board, int p_trace_clearance_class_no, bool p_maintain_database);
    ~AutorouteEngine();

    void set_stoppable(datastructures::Stoppable* p_stoppable) { stoppable_ = p_stoppable; }
    void set_time_limit(int64_t p_time_ms);
    void set_net_no(int p_net_no) { net_no_ = p_net_no; }

    void init_connection(int p_net_no, datastructures::Stoppable* p_stoppable, int64_t p_time_limit_ms);

    bool route_all(int p_iteration_limit, datastructures::Stoppable* p_stoppable);
    void reset();
    void clear();

    int get_net_no() const;
    int generate_room_id_no();

    bool is_stop_requested() const;

    AutorouteAttemptResult autoroute_connection(const std::set<board::Item*>& p_start_set,
                                                const std::set<board::Item*>& p_dest_set,
                                                const AutorouteControl& p_ctrl);

    board::ShapeSearchTree* get_autoroute_search_tree()
    {
        ensure_search_tree_initialized();
        return autoroute_search_tree_.get();
    }

    /** Returns the routing board of this autoroute engine. Mirrors Java AutorouteEngine.board. */
    board::RoutingBoard* board() { return board_; }

    /**
     * Adds p_room to the internal database of incomplete expansion rooms of
     * this engine. Takes ownership of p_room. Mirrors Java
     * AutorouteEngine.addIncompleteExpansionRoom().
     */
    IncompleteFreeSpaceExpansionRoom* add_incomplete_expansion_room(IncompleteFreeSpaceExpansionRoom* p_room);

    /**
     * Completes the shape of p_room and returns the resulting complete free
     * space expansion rooms. p_room itself is not modified or retained.
     * Mirrors Java AutorouteEngine.completeExpansionRoom().
     *
     * Note (WP-9): the full ShapeSearchTree.complete_shape() free-space
     * expansion (Java blocker "MazeSearchAlgo" dependency 6) is not ported
     * yet. This port completes p_room as a single CompleteFreeSpaceExpansionRoom
     * with p_room's current 2-dimensional shape and registers it in the engine
     * database. The result is the exact empty-board behaviour of Java; rooms
     * that would be split around obstacles need the complete_shape port.
     */
    std::vector<CompleteFreeSpaceExpansionRoom*> complete_expansion_room(IncompleteExpansionRoom* p_room);

    static constexpr int TRACE_WIDTH_TOLERANCE = 2;

    std::string to_string() const { return "AutorouteEngine"; }

private:
    board::RoutingBoard* board_ = nullptr;
    bool maintain_database_ = false;
    datastructures::Stoppable* stoppable_ = nullptr;
    int net_no_ = 0;
    int trace_clearance_class_no_ = 0;
    std::unique_ptr<datastructures::TimeLimit> time_limit_;

    std::unique_ptr<board::ShapeSearchTree> autoroute_search_tree_;
    
    // Lazy initialization of search tree
    void ensure_search_tree_initialized();

    /**
     * Removes all net dependent complete free space expansion rooms from the
     * database. Mirrors the net-change handling of Java
     * AutorouteEngine.initConnection().
     */
    void remove_net_dependent_rooms();

    /**
     * Removes a complete expansion room from the database and the search tree.
     * Mirrors Java AutorouteEngine.removeCompleteExpansionRoom() without the
     * neighbour split (C++ rooms currently have no doors).
     */
    void remove_complete_expansion_room(CompleteFreeSpaceExpansionRoom* p_room);

    std::vector<std::unique_ptr<IncompleteFreeSpaceExpansionRoom>> incomplete_expansion_rooms_;
    std::vector<std::unique_ptr<CompleteFreeSpaceExpansionRoom>> complete_expansion_rooms_;
    int expansion_room_instance_count_ = 0;
};

} // namespace freerouting::autoroute

#endif // AUTOROUTE_AUTOROUTE_ENGINE_H