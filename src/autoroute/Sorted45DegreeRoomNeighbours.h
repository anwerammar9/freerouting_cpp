#pragma once

#include <array>
#include <memory>
#include <vector>

#include "board/SearchTreeObject.h"
#include "geometry/planar/IntOctagon.h"

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
 * Helper class to sort the doors of a 45 degree expansion room counterclockwise
 * around the border of the room shape. Port of the Java inner class
 * Sorted45DegreeRoomNeighbours.SortedRoomNeighbour.
 *
 * The first and the last touching side of the room are derived from the
 * intersection octagon. The Java inner class additionally marks the sides of the
 * room whose interior touches an obstacle; p_edge_interior_touches_obstacle
 * points to that flag array of the owning Sorted45DegreeRoomNeighbours
 * instance.
 */
class Sorted45DegreeRoomNeighbour
{
public:
    Sorted45DegreeRoomNeighbour(board::SearchTreeObject* p_search_tree_object,
        const geometry::planar::IntOctagon& p_shape, const geometry::planar::IntOctagon& p_intersection,
        const geometry::planar::IntOctagon& p_room_shape, std::array<bool, 8>* p_edge_interior_touches_obstacle);

    /**
     * Compare function for sorting the neighbours in counterclock sense around
     * the border of the room shape in ascending order. Port of Java
     * SortedRoomNeighbour.compareTo().
     */
    int compare_to(const Sorted45DegreeRoomNeighbour& p_other) const;

    /** The search tree object of the neighbour room. */
    board::SearchTreeObject* search_tree_object = nullptr;

    /** The bounding octagon of the shape of the neighbour room. */
    geometry::planar::IntOctagon shape;

    /** The intersection of this expansion room shape with the neighbour shape. */
    geometry::planar::IntOctagon intersection;

    /** The first side of the room shape, where the neighbour shape touches. */
    int first_touching_side = -1;

    /** The last side of the room shape, where the neighbour shape touches. */
    int last_touching_side = -1;

private:
    /** Port of the Java flag update loop of the SortedRoomNeighbour constructor. */
    void mark_touching_sides();

    /**
     * The room shape the touching sides were calculated from. Java holds a
     * reference to the shape of the owning instance; a copy is used here,
     * because the neighbours are only used while the room shape of the
     * neighbour calculation stays unchanged (a changed room shape restarts the
     * calculation).
     */
    geometry::planar::IntOctagon room_shape_;
    std::array<bool, 8>* edge_interior_touches_obstacle_ = nullptr;
};

/**
 * To calculate the neighbour rooms of an expansion room in a search tree with
 * 45 degree bounding directions. The room shapes are handled as IntOctagons, so
 * the neighbours are calculated on the eight sides of the room octagon. Port of
 * the Java Sorted45DegreeRoomNeighbours.
 */
class Sorted45DegreeRoomNeighbours
{
public:
    /**
     * Calculates the neighbour rooms of p_room and inserts the doors to the new
     * incomplete free space rooms. The shape of the completed room may change if
     * a corner of the room is cut off, and the from room p_room is enlarged if a
     * room edge without a touching neighbour is found. Returns the completed
     * expansion room, which is owned by the caller: a newly created
     * CompleteFreeSpaceExpansionRoom or p_room itself, if p_room is an
     * ObstacleExpansionRoom. Returns nullptr if p_room has an unexpected type.
     * Port of Java Sorted45DegreeRoomNeighbours.calculate().
     */
    static CompleteExpansionRoom* calculate(ExpansionRoom* p_room, AutorouteEngine* p_autoroute_engine);

    /**
     * Calculates all touching neighbours of p_room and sorts them in
     * counterclock sense around the border of the room shape. Port of Java
     * Sorted45DegreeRoomNeighbours.calculateNeighbours().
     *
     * Returns nullptr if p_room is of an unexpected type.
     */
    static std::unique_ptr<Sorted45DegreeRoomNeighbours> calculate_neighbours(ExpansionRoom* p_room, int p_net_no,
        board::ShapeSearchTree* p_autoroute_search_tree, int p_room_id_no);

    /**
     * Checks that each side of the room octagon has at least one touching
     * neighbour. Otherwise the room shape is improved by removing the not
     * touching border lines. Returns true if the shape of the from room was
     * changed. Port of Java
     * Sorted45DegreeRoomNeighbours.tryRemoveEdgeLine().
     */
    bool try_remove_edge_line(int p_net_no, board::ShapeSearchTree* p_autoroute_search_tree);

    /**
     * Calculates the incomplete free space rooms between the sorted
     * neighbours of this room together with the doors from the completed room to
     * them. Port of Java
     * Sorted45DegreeRoomNeighbours.calculateNewIncompleteRooms().
     */
    void calculate_new_incomplete_rooms(AutorouteEngine* p_autoroute_engine);

    const std::vector<Sorted45DegreeRoomNeighbour>& get_sorted_neighbours() const { return sorted_neighbours_; }
    CompleteExpansionRoom* get_completed_room() const { return completed_room_; }
    ExpansionRoom* get_from_room() const { return from_room_; }

    /**
     * For each of the 8 sides of the room octagon: true, if the interior of the
     * side touches an obstacle.
     */
    const std::array<bool, 8>& get_edge_interior_touches_obstacle() const
    {
        return edge_interior_touches_obstacle_;
    }

    /** The bounding octagon of the shape of the completed room. */
    const geometry::planar::IntOctagon& get_room_shape() const { return room_shape_; }

    /**
     * Transfers the ownership of a complete free space expansion room created by
     * calculate_neighbours() to the caller. Returns nullptr if no new room was
     * created, i.e. if the from room was an obstacle expansion room.
     */
    CompleteFreeSpaceExpansionRoom* release_completed_room();

private:
    Sorted45DegreeRoomNeighbours(ExpansionRoom* p_from_room, CompleteExpansionRoom* p_completed_room);

    void add_sorted_neighbour(board::SearchTreeObject* p_search_tree_object,
        const geometry::planar::IntOctagon& p_neighbour_shape,
        const geometry::planar::IntOctagon& p_intersection);

    /**
     * Replaces every octagon side, whose interior does not touch an obstacle, by
     * an unbounded side. Port of Java
     * Sorted45DegreeRoomNeighbours.removeNotTouchingBorderLines().
     */
    static geometry::planar::IntOctagon remove_not_touching_border_lines(
        const geometry::planar::IntOctagon& p_room_oct, const std::array<bool, 8>& p_edge_interior_touches_obstacle);

    /**
     * Calculates an incomplete room for each edge side from p_from_side_index to
     * p_to_side_index. Port of Java
     * Sorted45DegreeRoomNeighbours.calculateEdgeIncompleteRoomsOfObstacleExpansionRoom().
     */
    void calculate_edge_incomplete_rooms_of_obstacle_expansion_room(int p_from_side_index, int p_to_side_index,
        AutorouteEngine* p_autoroute_engine);

    /**
     * Inserts a new incomplete room with an octagon shape. Port of Java
     * Sorted45DegreeRoomNeighbours.insertIncompleteRoom().
     */
    void insert_incomplete_room(AutorouteEngine* p_autoroute_engine, int64_t p_left_x, int64_t p_bottom_y,
        int64_t p_right_x, int64_t p_top_y, int64_t p_upper_left_diagonal_x, int64_t p_lower_right_diagonal_x,
        int64_t p_lower_left_diagonal_x, int64_t p_upper_right_diagonal_x);

    /**
     * Calculates the incomplete rooms of an obstacle expansion room between two
     * neighbours. Port of Java
     * Sorted45DegreeRoomNeighbours.calculateNewIncompleteRoomsForObstacleExpansionRoom().
     */
    void calculate_new_incomplete_rooms_for_obstacle_expansion_room(
        const Sorted45DegreeRoomNeighbour& p_prev_neighbour, const Sorted45DegreeRoomNeighbour& p_next_neighbour,
        AutorouteEngine* p_autoroute_engine);

    ExpansionRoom* from_room_ = nullptr;
    CompleteExpansionRoom* completed_room_ = nullptr;
    std::unique_ptr<CompleteFreeSpaceExpansionRoom> owned_completed_room_;
    geometry::planar::IntOctagon room_shape_;
    std::vector<Sorted45DegreeRoomNeighbour> sorted_neighbours_;
    std::array<bool, 8> edge_interior_touches_obstacle_{};
};

} // namespace freerouting::autoroute
