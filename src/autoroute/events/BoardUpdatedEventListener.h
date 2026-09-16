#pragma once

namespace freerouting::autoroute::events {

class BoardUpdatedEvent;

class BoardUpdatedEventListener {
public:
    virtual ~BoardUpdatedEventListener() = default;
    virtual void on_board_updated_event(const BoardUpdatedEvent& p_event) = 0;
};

} // namespace freerouting::autoroute::events
