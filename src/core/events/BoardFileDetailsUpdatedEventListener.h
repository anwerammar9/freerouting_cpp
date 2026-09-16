#pragma once

namespace freerouting::core::events {

class BoardFileDetailsUpdatedEvent;

/**
 * Listener interface for board file detail updates.
 * Mirrors Java BoardFileDetailsUpdatedEventListener.java.
 */
class BoardFileDetailsUpdatedEventListener {
public:
    virtual ~BoardFileDetailsUpdatedEventListener() = default;
    virtual void on_board_file_details_updated(const BoardFileDetailsUpdatedEvent& p_event) = 0;
};

} // namespace freerouting::core::events