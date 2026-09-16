#pragma once

namespace freerouting::core {
class RoutingJob;
}

namespace freerouting::core::events {

/**
 * Notifies listeners that a routing job has been updated.
 * Mirrors Java RoutingJobUpdatedEvent.java.
 */
class RoutingJobUpdatedEvent {
public:
    RoutingJobUpdatedEvent(void* p_source, RoutingJob* p_job)
        : source_(p_source), job_(p_job) {}

    void* get_source() const { return source_; }

    RoutingJob* get_job() const { return job_; }

private:
    void* source_;
    RoutingJob* job_;
};

} // namespace freerouting::core::events