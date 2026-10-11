#pragma once

#include <memory>
#include <optional>
#include <vector>

#include "board/SearchTreeObject.h"
#include "datastructures/ShapeTree.h"
#include "geometry/planar/Point.h"
#include "geometry/planar/Vector.h"

namespace freerouting::geometry::planar {
class TileShape;
}

namespace freerouting::board {
class Item;
class ShapeSearchTree;
}

namespace freerouting::autoroute {

class CompleteExpansionRoom;
class CompleteFreeSpaceExpansionRoom;
class ExpansionRoom;
class ObstacleExpansionRoom;

/**
 * The room neighbour implementation selected for an autoroute search tree.
 * Port of the Java enum SortedRoomNeighbours.CalculationMode.
 */
enum class CalculationMode
{
    ORTHOGONAL,
    DEGREE_45,
    ANY_ANGLE
};

/**
 * Selects the room neighbour implementation matching the bounding directions of
 * p_search_tree. Port of Java
 * SortedRoomNeighbours.selectCalculationMode(). Java tests the search tree
 * class; the C++ ShapeSearchTree is configured with bounding directions, so the
 * number of bounding directions selects the calculation mode.
 */
CalculationMode select_calculation_mode(const board::ShapeSearchTree* p_search_tree);

/**
 * Helper class to sort the doors of an expansion room counterclockwise around
 * the border of the room shape. Port of the Java inner class
 * SortedRoomNeighbours.SortedRoomNeighbour.
 */
class SortedRoomNeighbour
{
public:
    /**
     * p_room_shape is the shape of the room the neighbour was found for. It is
     * shared with the owning SortedRoomNeighbours instance, because the room
     * shape may be enlarged while the neighbours are still sorted.
     */
    SortedRoomNeighbour(board::SearchTreeObject* p_search_tree_object,
        std::shared_ptr<const geometry::planar::TileShape> p_neighbour_shape,
        std::shared_ptr<const geometry::planar::TileShape> p_intersection,
        int p_touching_side_no_of_room, int p_touching_side_no_of_neighbour_room,
        bool p_room_touch_is_corner, bool p_neighbour_room_touch_is_corner,
        std::shared_ptr<const geometry::planar::TileShape> p_room_shape);

    SortedRoomNeighbour(const SortedRoomNeighbour& p_other);
    SortedRoomNeighbour& operator=(const SortedRoomNeighbour& p_other);
    SortedRoomNeighbour(SortedRoomNeighbour&&) = default;
    SortedRoomNeighbour& operator=(SortedRoomNeighbour&&) = default;
    ~SortedRoomNeighbour() = default;

    /**
     * Distance tolerance used when comparing neighbour corners. Port of the
     * Java constant SortedRoomNeighbour.c_dist_tolerance.
     */
    static constexpr double C_DIST_TOLERANCE = 1;

    /**
     * Compare function for sorting the neighbours in counterclock sense around
     * the border of the room shape in ascending order.
     * Port of Java SortedRoomNeighbour.compareTo().
     */
    int compare_to(const SortedRoomNeighbour& p_other) const;

    /** Returns the first corner of the intersection shape with the neighbour. */
    const geometry::planar::IntPoint& first_corner() const;

    /** Returns the last corner of the intersection shape with the neighbour. */
    const geometry::planar::IntPoint& last_corner() const;

    /** The search tree object of the neighbour room. */
    board::SearchTreeObject* search_tree_object = nullptr;

    /** The shape of the neighbour room. */
    std::shared_ptr<const geometry::planar::TileShape> neighbour_shape;

    /** The intersection of this expansion room shape with the neighbour_shape. */
    std::shared_ptr<const geometry::planar::TileShape> intersection;

    /** The side number of this room, where it touches the neighbour. */
    int touching_side_no_of_room = 0;

    /** The side number of the neighbour room, where it touches this room. */
    int touching_side_no_of_neighbour_room = 0;

    /**
     * True, if the intersection of this room and the neighbour is equal to a
     * corner of this room.
     */
    bool room_touch_is_corner = false;

    /**
     * True, if the intersection of this room and the neighbour is equal to a
     * corner of the neighbour room.
     */
    bool neighbour_room_touch_is_corner = false;

private:
    std::shared_ptr<const geometry::planar::TileShape> room_shape_;

    mutable std::optional<geometry::planar::IntPoint> precalculated_first_corner_;
    mutable std::optional<geometry::planar::IntPoint> precalculated_last_corner_;
};

/**
 * To calculate the neighbour rooms of an expansion room. The neighbour rooms
 * will be sorted in counterclock sense around the border of the shape of room.
 * Overlapping neighbours containing an item may be stored in an unordered
 * list. Port of Java SortedRoomNeighbours (any-angle / base implementation).
 */
class SortedRoomNeighbours
{
public:
    /**
     * Creates the SortedRoomNeighbours instance of p_room. p_completed_room is
     * the room the neighbours are calculated for; it is not owned.
     */
    SortedRoomNeighbours(ExpansionRoom* p_from_room, CompleteExpansionRoom* p_completed_room);

    ~SortedRoomNeighbours() = default;

    /**
     * Calculates the neighbour rooms of p_room. Overlapping objects are
     * collected from p_autoroute_search_tree, the touching ones are sorted in
     * counterclock sense and 1-dimensional doors to the neighbour rooms are
     * created. p_room_id_no is the id of a newly created complete free space
     * room. Port of Java SortedRoomNeighbours.calculateNeighbours().
     *
     * Returns nullptr if p_room is of an unexpected type.
     */
    static std::unique_ptr<SortedRoomNeighbours> calculate_neighbours(ExpansionRoom* p_room,
        int p_net_no, board::ShapeSearchTree* p_autoroute_search_tree, int p_room_id_no);

    /**
     * Creates the incomplete free space rooms for the border lines of p_room
     * which have no touching neighbour. Port of Java
     * SortedRoomNeighbours.calculateIncompleteRoomsWithEmptyNeighbours().
     */
    static void calculate_incomplete_rooms_with_empty_neighbours(ObstacleExpansionRoom* p_room,
        class AutorouteEngine* p_autoroute_engine);

    /**
     * Calculates the neighbour rooms of p_room and inserts the doors to the
     * new incomplete free space rooms. The shape of the completed room may be
     * changed (a corner may be cut off), and the from room p_room is enlarged
     * if a room edge without a touching neighbour is found. Returns the
     * completed expansion room, which is owned by the caller: a newly created
     * CompleteFreeSpaceExpansionRoom or p_room itself, if p_room is an
     * ObstacleExpansionRoom. Returns nullptr if p_room has an unexpected type.
     * Port of Java SortedRoomNeighbours.calculate().
     *
     * Divergence: the C++ ShapeSearchTree is configured with the bounding
     * directions instead of being a subclass, so Java SortedRoomNeighbours
     * complete() is mapped to this method: a search tree with 8 bounding
     * directions is dispatched to Sorted45DegreeRoomNeighbours, a search tree
     * with 4 bounding directions to SortedOrthogonalRoomNeighbours, and a tree
     * with any other direction set uses the any-angle implementation below.
     */
    static CompleteExpansionRoom* calculate(ExpansionRoom* p_room, class AutorouteEngine* p_autoroute_engine);

    /**
     * Checks that each side of the room shape has at least one touching
     * neighbour. Otherwise the shape of the from room is enlarged by removing
     * the empty room edge. Returns true if the shape of the from room was
     * changed. Port of Java SortedRoomNeighbours.tryRemoveEdge().
     */
    bool try_remove_edge(int p_net_no, board::ShapeSearchTree* p_autoroute_search_tree);

    /**
     * Calculates the incomplete free space rooms between the sorted
     * neighbours of this room together with the doors from the completed room
     * to them. The shape of the completed room may be changed (a corner may
     * be cut off). Port of Java
     * SortedRoomNeighbours.calculateNewIncompleteRooms().
     */
    void calculate_new_incomplete_rooms(class AutorouteEngine* p_autoroute_engine);

    /**
     * Door shape is expected to have dimension 1. Port of Java
     * SortedRoomNeighbours.insertDoorOk(ExpansionRoom, ExpansionRoom, TileShape).
     */
    static bool insert_door_ok(ExpansionRoom* p_room1, ExpansionRoom* p_room2,
        const geometry::planar::TileShape& p_door_shape);

    /**
     * Insert 1 dimensional doors of trace rooms only, if they are parallel to
     * the trace line. Port of Java
     * SortedRoomNeighbours.insertDoorOk(ObstacleExpansionRoom, Line).
     */
    static bool insert_door_ok(ObstacleExpansionRoom* p_room, const geometry::planar::Line& p_door_line);

    /**
     * Transfers the ownership of a complete free space expansion room created
     * by calculate_neighbours() to the caller. Returns nullptr if no new room
     * was created, i.e. if the from room was an obstacle expansion room.
     * Mirrors the fact that the Java calculateNeighbours() hands the new
     * CompleteFreeSpaceExpansionRoom to its caller.
     */
    CompleteFreeSpaceExpansionRoom* release_completed_room();

    const std::vector<SortedRoomNeighbour>& get_sorted_neighbours() const { return sorted_neighbours_; }
    CompleteExpansionRoom* get_completed_room() const { return completed_room_; }
    ExpansionRoom* get_from_room() const { return from_room_; }

    /**
     * The tree entries of the net dependent objects overlapping the room, whose
     * door calculation is delayed until the room shape does not change anymore.
     */
    const std::vector<datastructures::ShapeTree::TreeEntry>& get_own_net_objects() const
    {
        return own_net_objects_;
    }

private:
    void add_sorted_neighbour(board::SearchTreeObject* p_search_tree_object,
        const geometry::planar::TileShape& p_neighbour_shape,
        const geometry::planar::TileShape& p_intersection, int p_touching_side_no_of_room,
        int p_touching_side_no_of_neighbour_room, bool p_room_touch_is_corner,
        bool p_neighbour_room_touch_is_corner);

    /**
     * Java models the possibly missing door line of a degenerate door shape as
     * null; a disengaged optional p_door_line is rejected.
     */
    static bool insert_door_ok_impl(ObstacleExpansionRoom* p_room,
        const std::optional<geometry::planar::Line>& p_door_line);

    ExpansionRoom* from_room_ = nullptr;
    CompleteExpansionRoom* completed_room_ = nullptr;
    std::unique_ptr<CompleteFreeSpaceExpansionRoom> owned_completed_room_;
    std::shared_ptr<const geometry::planar::TileShape> room_shape_;
    std::vector<SortedRoomNeighbour> sorted_neighbours_;
    std::vector<datastructures::ShapeTree::TreeEntry> own_net_objects_;
};

} // namespace freerouting::autoroute
