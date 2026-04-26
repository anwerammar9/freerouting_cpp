#include "core/RoutingJob.h"
#include "logger/FRLogger.h"

#include <sstream>
#include <iomanip>

namespace freerouting {
namespace core {

const std::string RoutingJob::DSN_FILE_EXTENSION = "dsn";
const std::string RoutingJob::BINARY_FILE_EXTENSION = "frb";

std::string to_string(RoutingJobState p_state)
{
    switch (p_state) {
        case RoutingJobState::PENDING:
            return "PENDING";
        case RoutingJobState::RUNNING:
            return "RUNNING";
        case RoutingJobState::COMPLETED:
            return "COMPLETED";
        case RoutingJobState::FAILED:
            return "FAILED";
        case RoutingJobState::CANCELLED:
            return "CANCELLED";
        default:
            return "UNKNOWN";
    }
}

std::string to_string(RoutingJobPriority p_priority)
{
    switch (p_priority) {
        case RoutingJobPriority::LOW:
            return "LOW";
        case RoutingJobPriority::NORMAL:
            return "NORMAL";
        case RoutingJobPriority::HIGH:
            return "HIGH";
        default:
            return "UNKNOWN";
    }
}

RoutingJob::RoutingJob()
    : created_at_(std::chrono::system_clock::now())
{
    logger::FRLogger::instance().info("Creating default RoutingJob");
}

RoutingJob::RoutingJob(const std::string& p_name)
    : name_(p_name), created_at_(std::chrono::system_clock::now())
{
    logger::FRLogger::instance().info("Creating RoutingJob with name: " + p_name);
}

RoutingJob::RoutingJob(const std::string& p_name, const std::string& p_input_file,
                       const std::string& p_output_file)
    : name_(p_name), input_file_(p_input_file), output_file_(p_output_file),
      created_at_(std::chrono::system_clock::now())
{
    logger::FRLogger::instance().info("Creating RoutingJob: " + p_name + 
                                     " (input: " + p_input_file + 
                                     ", output: " + p_output_file + ")");
}

std::string RoutingJob::to_string() const
{
    std::ostringstream oss;
    oss << "RoutingJob{"
        << "name='" << name_ << "'"
        << ", input_file='" << input_file_ << "'"
        << ", output_file='" << output_file_ << "'"
        << ", state=" << freerouting::core::to_string(state_)
        << ", priority=" << freerouting::core::to_string(priority_);
    
    // Format timestamp
    auto time_t = std::chrono::system_clock::to_time_t(created_at_);
    oss << ", created_at=" << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    
    oss << "}";
    return oss.str();
}

} // namespace core
} // namespace freerouting
