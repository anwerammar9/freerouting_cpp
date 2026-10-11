#pragma once

#include <array>
#include <memory>
#include <vector>

#include "board/SearchTreeObject.h"
#include "geometry/planar/IntBox.h"

namespace freerouting::board {
class ShapeSearchTree;
}

namespace freerouting::autoroute {

class AutorouteEngine;
class CompleteExpansionRoom;
class CompleteFreeSpaceExpansionRoom;
class ExpansionRoom;
class ObstacleExpansionRoom;

/**
 * Helper class to sort the doors of an orthogonal expansion room counterclockwise
 * around the border of the room shape. Port of the Java inner class
 * SortedOrthogonalRoomNeighbours.SortedRoomNeighbour.
 *
 * The first and the last touching side of the room are derived from the
 * intersection box. The Java inner class additionally marks the sides of the
 * room whose interior touches an obstacle; p_edge_interior_touches_obstacle
 * points to that flag array of the owning SortedOrthogonalRoomNeighbours
 * instance.
 */
class SortedOrthogonalRoomNeighbour
{
public:
    SortedOrthogonalRoomNeighbour(board::SearchTreeObject* p_search_tree_object,
        const geometry::planar::IntBox& p_shape, const geometry::planar::IntBox& p_intersection,
        const geometry::planar::IntBox& p_room_shape, std::array<bool, 4>* p_edge_interior_touches_obstacle);

    /**
     * Compare function for sorting the neighbours in counterclock sense around
     * the border of the room shape in ascending order. Port of Java
     * SortedRoomNeighbour.compareTo().
     */
    int compare_to(const SortedOrthogonalRoomNeighbour& p_other) const;

    /** The search tree object of the neighbour room. */
    board::SearchTreeObject* search_tree_object = nullptr;

    /** The shape of the neighbour room. */
    geometry::planar::IntBox shape;

    /** The intersection of this expansion room shape with the neighbour shape. */
    geometry::planar::IntBox intersection;

    /** The first side of the room shape, where the neighbour shape touches. */
    int first_touching_side = -1;

    /** The last side of the room shape, where the neighbour shape touches. */
    int last_touching_side = -1;

private:
    /** Port of the Java flag update block of the SortedRoomNeighbour constructor. */
    void mark_touching_sides();

    /**
     * The room box the touching sides were calculated from. Java holds a
     * reference to the shape of the owning instance; a copy is used here,
     * because the neighbours are only used while the room shape of the
     * neighbour calculation stays unchanged (a changed room shape restarts the
     * calculation).
     */
    geometry::planar::IntBox room_shape_;
    std::array<bool, 4>* edge_interior_touches_obstacle_ = nullptr;
};

/**
 * To calculate the neighbour rooms of an expansion room in a search tree with
 * orthogonal (90 degree) bounding directions. The room shapes are handled as
 * IntBoxes, so the neighbours are calculated on the four sides of the room box.
 * Port of the Java SortedOrthogonalRoomNeighbours.
 */
class SortedOrthogonalRoomNeighbours
{
public:
    /**
     * Calculates the neighbour rooms of p_room and inserts the doors to the new
     * incomplete free space rooms. The from room p_room is enlarged if a room
     * edge without a touching neighbour is found. Returns the completed
     * expansion room, which is owned by the caller: a newly created
     * CompleteFreeSpaceExpansionRoom or p_room itself, if p_room is an
     * ObstacleExpansionRoom. Returns nullptr if p_room has an unexpected type.
     * Port of Java SortedOrthogonalRoomNeighbours.calculate().
     */
    static CompleteExpansionRoom* calculate(ExpansionRoom* p_room, AutorouteEngine* p_autoroute_engine);

    /**
     * Calculates all touching neighbours of p_room and sorts them in
     * counterclock sense around the border of the room shape. Port of Java
     * SortedOrthogonalRoomNeighbours.calculateNeighbours().
     *
     * Returns nullptr if p_room is of an unexpected type or its shape is not an
     * IntBox.
     */
    static std::unique_ptr<SortedOrthogonalRoomNeighbours> calculate_neighbours(ExpansionRoom* p_room,
        int p_net_no, board::ShapeSearchTree* p_autoroute_search_tree, int p_room_id_no);

    /**
     * Creates the incomplete free space rooms for the 4 border lines of p_room,
     * which have no touching neighbour. Port of Java
     * SortedOrthogonalRoomNeighbours.calculateIncompleteRoomsWithEmptyNeighbours().
     */
    static void calculate_incomplete_rooms_with_empty_neighbours(ObstacleExpansionRoom* p_room,
        AutorouteEngine* p_autoroute_engine);

    /**
     * Checks that each side of the room box has at least one touching
     * neighbour. Otherwise the shape of the from room is enlarged by removing
     * the empty room edge. Returns true if the shape of the from room was
     * changed. Port of Java
     * SortedOrthogonalRoomNeighbours.tryRemoveEdge().
     */
    bool try_remove_edge(int p_net_no, board::ShapeSearchTree* p_autoroute_search_tree);

    /**
     * Calculates the incomplete free space rooms between the sorted
     * neighbours of this room together with the doors from the completed room to
     * them. Port of Java
     * SortedOrthogonalRoomNeighbours.calculateNewIncompleteRooms().
     */
    void calculate_new_incomplete_rooms(AutorouteEngine* p_autoroute_engine);

    const std::vector<SortedOrthogonalRoomNeighbour>& get_sorted_neighbours() const
    {
        return sorted_neighbours_;
    }
    CompleteExpansionRoom* get_completed_room() const { return completed_room_; }
    ExpansionRoom* get_from_room() const { return from_room_; }

    /**
     * For each of the 4 sides of the room box: true, if the interior of the
     * side touches an obstacle.
     */
    const std::array<bool, 4>& get_edge_interior_touches_obstacle() const
    {
        return edge_interior_touches_obstacle_;
    }

    /** The box shape of the completed room. */
    const geometry::planar::IntBox& get_room_shape() const { return room_shape_; }

    /** True, if the from room of this instance is an obstacle expansion room. */
    bool is_obstacle_expansion_room() const { return is_obstacle_expansion_room_; }

    /**
     * Transfers the ownership of a complete free space expansion room created by
     * calculate_neighbours() to the caller. Returns nullptr if no new room was
     * created, i.e. if the from room was an obstacle expansion room.
     */
    CompleteFreeSpaceExpansionRoom* release_completed_room();

private:
    SortedOrthogonalRoomNeighbours(ExpansionRoom* p_from_room, CompleteExpansionRoom* p_completed_room,
        const geometry::planar::IntBox& p_room_shape);

    void add_sorted_neighbour(board::SearchTreeObject* p_search_tree_object,
        const geometry::planar::IntBox& p_neighbour_shape, const geometry::planar::IntBox& p_intersection);

    /**
     * Replaces the border line p_remove_edge_no of p_room_box by an unbounded
     * border line. Port of Java
     * SortedOrthogonalRoomNeighbours.removeBorderLine().
     */
    static geometry::planar::IntBox remove_border_line(
        const geometry::planar::IntBox& p_room_box, int p_remove_edge_no);

    /**
     * Creates a new incomplete room with the box p_ll_x..p_ur_y, if it is
     * 2-dimensional and its intersection with this room is not empty.
     * Port of Java SortedOrthogonalRoomNeighbours.insertIncompleteRoom().
     */
    void insert_incomplete_room(
        AutorouteEngine* p_autoroute_engine, int64_t p_ll_x, int64_t p_ll_y, int64_t p_ur_x, int64_t p_ur_y);

    ExpansionRoom* from_room_ = nullptr;
    CompleteExpansionRoom* completed_room_ = nullptr;
    std::unique_ptr<CompleteFreeSpaceExpansionRoom> owned_completed_room_;
    geometry::planar::IntBox room_shape_;
    std::vector<SortedOrthogonalRoomNeighbour> sorted_neighbours_;
    std::array<bool, 4> edge_interior_touches_obstacle_{};
    bool is_obstacle_expansion_room_ = false;
};

} // namespace freerouting::autoroute
