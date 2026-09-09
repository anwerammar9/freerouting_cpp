#pragma once

#include <vector>

#include "geometry/planar/Vector.h"

namespace freerouting::board {

class Component;
class Item;
class RoutingBoard;

/**
 * Class for moving a group of items on the board
 * Mirrors Java MoveComponent.java (164 lines).
 */
class MoveComponent
{
public:
    /**
     * Creates a new instance of MoveComponent
     */
    MoveComponent(Item* p_item,
                  const geometry::planar::Vector& p_translate_vector,
                  int p_max_recursion_depth,
                  int p_max_via_recursion_depth);

    /**
     * Checks, if all items in the group can be moved by shoving obstacle trace aside without creating clearance violations.
     */
    bool check();

    /**
     * Moves all items in the group by the translate vector and shoves aside obstacle traces.
     * Returns false, if that was not possible without creating clearance violations.
     * In this case an undo may be necessary.
     */
    bool insert(int p_tidy_width, int p_pull_tight_accuracy);

private:
    geometry::planar::IntVector translate_vector_;
    int max_recursion_depth_;
    int max_via_recursion_depth_;
    RoutingBoard* board_ = nullptr;
    std::vector<Item*> item_group_arr_;
    bool all_items_movable_ = true;
    Component* component_ = nullptr;

    struct SortedItem
    {
        Item* item = nullptr;
        double projection = 0.0;

        bool operator<(const SortedItem& p_other) const
        {
            return projection < p_other.projection;
        }
    };
};

} // namespace freerouting::board
