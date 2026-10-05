#pragma once

/**
 * Port of the Java SearchTreeObject (src/board/SearchTreeObject.java).
 * Interface for objects which can be stored in a ShapeSearchTree.
 */

#include "datastructures/ShapeTree.h"

namespace freerouting::board {

class SearchTreeObject : public freerouting::datastructures::ShapeTree::Storable
{
public:
    virtual ~SearchTreeObject() = default;

    /**
     * Returns, if this is an obstruction for the given net number.
     */
    virtual bool is_obstacle(int p_net_no) const = 0;

    /**
     * Returns, if this is a trace obstruction for the given net number.
     */
    virtual bool is_trace_obstacle(int p_net_no) const = 0;

    /**
     * Returns the layer of the shape with index p_index.
     */
    virtual int shape_layer(int p_index) const = 0;

    /**
     * Returns, if this object is a complete free space expansion room.
     * Used by ShapeSearchTree::complete_shape() to decide whether an already
     * completed free space room may be reused as ignore shape. Mirrors the
     * Java instanceof CompleteFreeSpaceExpansionRoom check.
     */
    virtual bool is_free_space_room() const { return false; }

    /**
     * Returns a unique id of this object, used as a deterministic tie-breaker
     * when sorting geometric objects (e.g. the room-neighbour comparator).
     * Port of Java SearchTreeObject.getId().
     */
    virtual int get_id() const = 0;
};

} // namespace freerouting::board