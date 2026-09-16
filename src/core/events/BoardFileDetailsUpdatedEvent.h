#pragma once

namespace freerouting::core {
class BoardFileDetails;
}

namespace freerouting::core::events {

/**
 * Notifies listeners that the details of a board file have been updated.
 * Mirrors Java BoardFileDetailsUpdatedEvent.java.
 */
class BoardFileDetailsUpdatedEvent {
public:
    BoardFileDetailsUpdatedEvent(void* p_source, BoardFileDetails* p_details)
        : source_(p_source), details_(p_details) {}

    void* get_source() const { return source_; }

    BoardFileDetails* get_details() const { return details_; }

private:
    void* source_;
    BoardFileDetails* details_;
};

} // namespace freerouting::core::events