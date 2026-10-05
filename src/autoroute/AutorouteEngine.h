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
class DrillPageArray;
class ExpansionRoom;

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
     * Returns the 2 dimensional array of drill pages used by the maze search.
     * Mirrors Java AutorouteEngine.drillPageArray. The array is nullptr for an
     * engine without a board.
     */
    DrillPageArray* drill_page_array() { return drill_page_array_.get(); }

    /**
     * Invalidates all drill pages intersecting with p_shape, so that they are
     * recalculated at the next call of get_drills(). Mirrors Java
     * AutorouteEngine.invalidateDrillPages().
     */
    void invalidate_drill_pages(const geometry::planar::IntBox& p_shape);

    /**
     * Returns true if the room database of this engine is maintained across
     * autoroute passes. Port of Java AutorouteEngine.maintainDatabase.
     */
    bool get_maintain_database() const { return maintain_database_; }

    /**
     * Adds p_room to the internal database of incomplete expansion rooms of
     * this engine. Takes ownership of p_room. Mirrors Java
     * AutorouteEngine.addIncompleteExpansionRoom().
     */
    IncompleteFreeSpaceExpansionRoom* add_incomplete_expansion_room(IncompleteFreeSpaceExpansionRoom* p_room);

    /**
     * Creates and registers an incomplete free space expansion room with an
     * exact Simplex shape and a contained shape. Mirrors Java
     * AutorouteEngine.addIncompleteExpansionRoom(TileShape, int, TileShape).
     */
    IncompleteFreeSpaceExpansionRoom* add_incomplete_expansion_room(const geometry::planar::Simplex& p_shape,
        int p_layer, const geometry::planar::Simplex& p_contained_shape);

    /**
     * Returns the first incomplete expansion room of the engine database or
     * nullptr if the database is empty. Port of Java
     * AutorouteEngine.getFirstIncompleteExpansionRoom().
     */
    IncompleteFreeSpaceExpansionRoom* get_first_incomplete_expansion_room();

    /**
     * Removes all doors of p_room from both sides of the connection. The
     * incomplete free space rooms reached through the doors are removed from
     * the engine database as well. Port of Java
     * AutorouteEngine.removeAllDoors().
     */
    void remove_all_doors(ExpansionRoom* p_room);

    /**
     * Removes a complete expansion room from the room database and from the
     * autoroute search tree and creates a new incomplete expansion room for
     * every neighbour of the removed room, so that the free space of the
     * removed room stays reachable. Port of Java
     * AutorouteEngine.removeCompleteExpansionRoom().
     */
    void remove_complete_expansion_room(CompleteFreeSpaceExpansionRoom* p_room);

    /**
     * Completes the shapes of the neighbour rooms of p_room, so that the doors
     * of p_room will not change later on. An incomplete free space neighbour
     * is completed, an obstacle room without calculated doors gets its doors
     * calculated. Port of Java
     * AutorouteEngine.completeNeighbourRooms().
     */
    void complete_neighbour_rooms(CompleteExpansionRoom* p_room);

    /**
     * Returns the complete free space expansion rooms of the room database
     * which have a target door to one of the items in p_items. Port of Java
     * AutorouteEngine.getRoomsWithTargetItems(Set<Item>). The result is ordered
     * by the room id, mirroring the Java TreeSet.
     */
    std::vector<CompleteFreeSpaceExpansionRoom*> get_rooms_with_target_items(
        const std::set<board::Item*>& p_items) const;

    /**
     * Completes the shape of p_room and returns the resulting complete free
     * space expansion rooms. p_room is removed from the engine database if it
     * was registered. Mirrors Java AutorouteEngine.completeExpansionRoom().
     *
     * The completion runs through ShapeSearchTree::complete_shape()
     * (WP-10 Increment C): a door to a 2-dimensional neighbour
     * CompleteFreeSpaceExpansionRoom supplies the ignore object and from-door
     * shape, the free space is computed by the search tree and every resulting
     * 2-dimensional piece becomes a CompleteFreeSpaceExpansionRoom registered
     * in the engine database and stored in the autoroute search tree.
     *
     * The start shape is bounded by p_room's own shape (or, for sub-2D drill
     * unit shapes, an inflation of it), so a room never grows beyond the free
     * space inside that bound — the C++ rooms are IntBox based and the Java
     * "whole-plane start shape" would split an otherwise empty board into
     * several sections. Room splitting around intersecting obstacles is
     * faithfully reproduced. Door calculation (Java SortedRoomNeighbours.complete)
     * is not ported yet.
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

    /**
     * The pages of expansion drills used by the maze search to change the layer.
     * Mirrors Java AutorouteEngine.drillPageArray.
     */
    std::unique_ptr<DrillPageArray> drill_page_array_;
    
    // Lazy initialization of search tree
    void ensure_search_tree_initialized();

    /**
     * Removes all net dependent complete free space expansion rooms from the
     * database. Mirrors the net-change handling of Java
     * AutorouteEngine.initConnection().
     */
    void remove_net_dependent_rooms();

    /**
     * Removes p_room from the engine database of incomplete expansion rooms if
     * it was registered. The doors of p_room are removed first. Mirrors Java
     * AutorouteEngine.removeIncompleteExpansionRoom().
     */
    void remove_incomplete_expansion_room(IncompleteExpansionRoom* p_room);

    /**
     * Calculates the doors of the completed free space room of p_piece on layer
     * p_layer, adds the room to the room database and to the autoroute search
     * tree and appends it to p_result. Returns the new room or nullptr, if the
     * room calculation did not create a 2-dimensional complete free space room.
     * Port of Java AutorouteEngine.addCompleteRoom() together with
     * calculateDoors().
     */
    CompleteFreeSpaceExpansionRoom* add_complete_room(int p_layer,
        const board::ShapeSearchTree::CompleteShapePiece& p_piece,
        std::vector<CompleteFreeSpaceExpansionRoom*>& p_result);

    std::vector<std::unique_ptr<IncompleteFreeSpaceExpansionRoom>> incomplete_expansion_rooms_;
    std::vector<std::unique_ptr<CompleteFreeSpaceExpansionRoom>> complete_expansion_rooms_;
    int expansion_room_instance_count_ = 0;
};

} // namespace freerouting::autoroute

#endif // AUTOROUTE_AUTOROUTE_ENGINE_H