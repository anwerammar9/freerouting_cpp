#pragma once

namespace freerouting::core {
class RoutingJob;
}

namespace freerouting::logger {
class LogEntry;
}

namespace freerouting::core::events {

/**
 * Notifies listeners that a log entry has been added to a routing job.
 * Mirrors Java RoutingJobLogEntryAddedEvent.java.
 */
class RoutingJobLogEntryAddedEvent {
public:
    RoutingJobLogEntryAddedEvent(void* p_source, RoutingJob* p_job, logger::LogEntry* p_log_entry)
        : source_(p_source), job_(p_job), log_entry_(p_log_entry) {}

    void* get_source() const { return source_; }

    RoutingJob* get_job() const { return job_; }

    logger::LogEntry* get_log_entry() const { return log_entry_; }

private:
    void* source_;
    RoutingJob* job_;
    logger::LogEntry* log_entry_;
};

} // namespace freerouting::core::events