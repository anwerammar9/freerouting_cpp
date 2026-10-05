#ifndef AUTOROUTE_MAZE_SEARCH_ELEMENT_H
#define AUTOROUTE_MAZE_SEARCH_ELEMENT_H

#include "autoroute/ExpandableObject.h"
#include "geometry/planar/FloatLine.h"

#include <cstdint>
#include <vector>

namespace freerouting::autoroute {

class ExpansionDoor;
class ExpansionRoom;
class CompleteExpansionRoom;

class MazeSearchElement {
public:
    enum class Adjustment {
        NONE,
        LEFT,
        RIGHT
    };

    MazeSearchElement();

    bool is_occupied = false;
    ExpandableObject* backtrack_door = nullptr;
    int section_no_of_backtrack_door = 0;
    bool room_ripped = false;
    Adjustment adjustment = Adjustment::NONE;

    /**
     * The ripup cost paid to enter this door's room via the maze search. Zero
     * when room_ripped is false. Port of Java MazeSearchElement.ripupCost.
     */
    int ripup_cost = 0;

    void reset();

    static constexpr int SECTION_COUNT_MAX = 64;
    static void reset_section_array(std::vector<MazeSearchElement>& p_arr);
};

/**
 * Information for the maze expand algorithm contained in expansion doors and
 * drills while the maze expanding algorithm is in progress. Port of Java
 * MazeListElement.
 */
class MazeListElement {
public:
    /** The door or drill belonging to this MazeListElement. */
    ExpandableObject* door = nullptr;

    /** The section number of the door (or the layer of the drill). */
    int section_no_of_door = 0;

    /** The door, from which this door was expanded. */
    ExpandableObject* backtrack_door = nullptr;

    /** The section number of the backtrack door. */
    int section_no_of_backtrack_door = 0;

    /** The weighted distance to the start of the expansion. */
    double expansion_value = 0.0;

    /**
     * The expansion value plus the shortest distance to a destination. The
     * list is sorted in ascending order by this value.
     */
    double sorting_value = 0.0;

    /** The next room, which will be expanded from this maze search element. */
    CompleteExpansionRoom* next_room = nullptr;

    /**
     * Point of the region of the expansion door, which has the shortest
     * distance to the backtrack door.
     */
    geometry::planar::FloatLine shape_entry;

    bool room_ripped = false;
    MazeSearchElement::Adjustment adjustment = MazeSearchElement::Adjustment::NONE;
    bool already_checked = false;

    /**
     * The ripup cost paid to enter the next_room through this door. Non-zero
     * only when room_ripped is true and this element was directly created by
     * expand_to_door_section with a positive add_costs.
     */
    int ripup_cost = 0;

    /**
     * Creates a new instance of MazeListElement. Port of the Java
     * MazeListElement constructor.
     */
    MazeListElement(ExpandableObject* p_door,
                    int p_section_no_of_door,
                    ExpandableObject* p_backtrack_door,
                    int p_section_no_of_backtrack_door,
                    double p_expansion_value,
                    double p_sorting_value,
                    CompleteExpansionRoom* p_next_room,
                    const geometry::planar::FloatLine& p_shape_entry,
                    bool p_room_ripped,
                    MazeSearchElement::Adjustment p_adjustment,
                    bool p_already_checked);

    /**
     * Orders the maze expansion list by the sorting value, then the expansion
     * value, then the door id and finally the section number. Port of Java
     * MazeListElement.compareTo(); two elements comparing equal are the same
     * list entry.
     */
    int compare_to(const MazeListElement& p_other) const;

    bool operator<(const MazeListElement& p_other) const { return compare_to(p_other) < 0; }
};

} // namespace freerouting::autoroute

#endif // AUTOROUTE_MAZE_SEARCH_ELEMENT_H