#pragma once

namespace freerouting::core::events {

class RoutingJobLogEntryAddedEvent;

/**
 * Listener interface for routing job log entry additions.
 * Mirrors Java RoutingJobLogEntryAddedEventListener.java.
 */
class RoutingJobLogEntryAddedEventListener {
public:
    virtual ~RoutingJobLogEntryAddedEventListener() = default;
    virtual void on_log_entry_added(const RoutingJobLogEntryAddedEvent& p_event) = 0;
};

} // namespace freerouting::core::events