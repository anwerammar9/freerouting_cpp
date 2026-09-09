#include "board/BoardObserverAdaptor.h"

namespace freerouting::board
{

void BoardObserverAdaptor::notify_deleted(Item* p_item)
{
    (void)p_item;
}

void BoardObserverAdaptor::notify_changed(Item* p_item)
{
    (void)p_item;
}

void BoardObserverAdaptor::notify_new(Item* p_item)
{
    (void)p_item;
}

void BoardObserverAdaptor::notify_moved(Component* p_component)
{
    (void)p_component;
}

void BoardObserverAdaptor::activate()
{
    active_ = true;
}

void BoardObserverAdaptor::deactivate()
{
    active_ = false;
}

bool BoardObserverAdaptor::is_active() const
{
    return active_;
}

} // namespace freerouting::board
