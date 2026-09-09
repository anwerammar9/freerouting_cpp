#pragma once

#include "datastructures/Observers.h"

namespace freerouting::board
{

class Item;
class Component;

/**
 * Interface for the observers of the board. The observers are informed about
 * changes in the board. Mirrors Java BoardObservers.java.
 */
class BoardObservers : public datastructures::Observers<Item*>
{
public:
    virtual ~BoardObservers() = default;

    /**
     * Enable the observers to synchronize the moved component.
     */
    virtual void notify_moved(Component* p_component) = 0;
};

} // namespace freerouting::board
