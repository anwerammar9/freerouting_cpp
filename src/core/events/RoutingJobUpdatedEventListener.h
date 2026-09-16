#pragma once

namespace freerouting::core::events {

class RoutingJobUpdatedEvent;

/**
 * Listener interface for routing job updates.
 * Mirrors Java RoutingJobUpdatedEventListener.java.
 */
class RoutingJobUpdatedEventListener {
public:
    virtual ~RoutingJobUpdatedEventListener() = default;
    virtual void on_routing_job_updated(const RoutingJobUpdatedEvent& p_event) = 0;
};

} // namespace freerouting::core::events