#pragma once

#include "board/BoardObservers.h"

namespace freerouting::board
{

/**
 * Empty adaptor implementing the BoardObservers interface.
 * Mirrors Java BoardObserverAdaptor.java.
 */
class BoardObserverAdaptor : public BoardObservers
{
public:
    BoardObserverAdaptor() = default;

    void notify_deleted(Item* p_item) override;
    void notify_changed(Item* p_item) override;
    void notify_new(Item* p_item) override;
    void notify_moved(Component* p_component) override;

    void activate() override;
    void deactivate() override;
    bool is_active() const override;

private:
    bool active_ = false;
};

} // namespace freerouting::board
