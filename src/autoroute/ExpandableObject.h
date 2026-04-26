#pragma once

#include <memory>
#include "geometry/planar/TileShape.h"

namespace freerouting {
namespace autoroute {

class CompleteExpansionRoom;
class MazeSearchElement;

/**
 * @brief An object which can be expanded by the maze expansion algorithm
 * 
 * This interface defines the contract for objects that can be expanded
 * during the maze expansion algorithm used in autorouting.
 */
class ExpandableObject {
public:
    virtual ~ExpandableObject() = default;

    /**
     * @brief Calculates the intersection of the shapes of the 2 objects belonging to this door
     * 
     * @return The tile shape representing the object's shape
     */
    virtual std::shared_ptr<TileShape> get_shape() const = 0;

    /**
     * @brief Returns the dimension of the intersection of the shapes of the 2 objects belonging to this door
     * 
     * @return The dimension value
     */
    virtual int get_dimension() const = 0;

    /**
     * @brief Returns the other room to p_room if this is a door and the other room is a CompleteExpansionRoom
     * 
     * @param p_room The room to find the other room for
     * @return Pointer to the other room, or nullptr if not applicable
     */
    virtual CompleteExpansionRoom* other_room(CompleteExpansionRoom* p_room) = 0;

    /**
     * @brief Returns the count of MazeSearchElements in this expandable object
     * 
     * @return The number of maze search elements
     */
    virtual int maze_search_element_count() const = 0;

    /**
     * @brief Returns the p_no-th MazeSearchElement in this expandable object
     * 
     * @param p_no The index of the maze search element to retrieve
     * @return Pointer to the maze search element
     */
    virtual MazeSearchElement* get_maze_search_element(int p_no) = 0;

    /**
     * @brief Resets this ExpandableObject for autorouting the next connection
     */
    virtual void reset() = 0;
};

} // namespace autoroute
} // namespace freerouting
