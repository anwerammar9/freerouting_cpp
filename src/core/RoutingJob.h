#pragma once

#include <memory>
#include <string>
#include <vector>
#include <chrono>
#include <optional>

#include "settings/RouterSettings.h"
#include "settings/ProfileSettings.h"
#include "core/RoutingStage.h"

namespace freerouting {
namespace board { class RoutingBoard; }

namespace core {

enum class RoutingJobState
{
    INVALID,
    QUEUED,
    READY_TO_START,
    RUNNING,
    PAUSED,
    COMPLETED,
    TIMED_OUT,
    CANCELLED,
    TERMINATED
};

enum class RoutingJobPriority
{
    LOW,
    NORMAL,
    HIGH
};

std::string to_string(RoutingJobState p_state);
std::string to_string(RoutingJobPriority p_priority);

class RoutingJob
{
public:
    static const std::string DSN_FILE_EXTENSION;
    static const std::string BINARY_FILE_EXTENSION;

    RoutingJob();
    explicit RoutingJob(const std::string& p_name);
    RoutingJob(const std::string& p_name, const std::string& p_input_file,
             const std::string& p_output_file);

    const std::string& get_name() const { return name_; }
    void set_name(const std::string& p_name) { name_ = p_name; }

    const std::string& get_input_file() const { return input_file_; }
    void set_input_file(const std::string& p_file) { input_file_ = p_file; }

    const std::string& get_output_file() const { return output_file_; }
    void set_output_file(const std::string& p_file) { output_file_ = p_file; }

    RoutingJobState get_state() const { return state_; }
    void set_state(RoutingJobState p_state) { state_ = p_state; }

    RoutingJobPriority get_priority() const { return priority_; }
    void set_priority(RoutingJobPriority p_priority) { priority_ = p_priority; }

    const std::chrono::time_point<std::chrono::system_clock>& get_created_at() const { return created_at_; }

    void set_input(const std::string& p_file_path);
    bool try_to_set_output_file(const std::string& p_file_path);

    std::string to_string() const;

    // Public fields matching Java RoutingJob
    std::string session_id;
    RoutingStage stage = RoutingStage::IDLE;
    settings::RouterSettings router_settings;
    settings::DesignRulesCheckerSettings drc_settings;
    std::string drc_report_file;
    board::RoutingBoard* board = nullptr;

private:
    std::string name_;
    std::string input_file_;
    std::string output_file_;
    RoutingJobState state_ = RoutingJobState::INVALID;
    RoutingJobPriority priority_ = RoutingJobPriority::NORMAL;
    std::chrono::time_point<std::chrono::system_clock> created_at_;
};

} // namespace core
} // namespace freerouting