#pragma once

#include <string>
#include <array>

namespace freerouting {
namespace autoroute {

enum class TaskState
{
    IDLE,
    STARTED,
    RUNNING,
    FINISHED,
    CANCELLED,
    TIMED_OUT
};

/**
 * @brief Convert TaskState enum to string representation
 * 
 * @param state The task state enum value
 * @return Human-readable string representation of the state
 */
std::string to_string(TaskState state);

/**
 * @brief Get description of what the task state means
 * 
 * @param state The task state enum value
 * @return Detailed description of the task state's purpose
 */
std::string get_task_description(TaskState state);

/**
 * @brief Check if the task state represents an active task
 * 
 * @param state The task state enum value
 * @return true if the task is currently active (started or running)
 */
bool is_active_state(TaskState state);

/**
 * @brief Check if the task state represents a terminal state
 * 
 * @param state The task state enum value
 * @return true if the task has completed (finished, cancelled, or timed out)
 */
bool is_terminal_state(TaskState state);

/**
 * @brief Get all task states in order
 * 
 * @return Array of all task states in logical order
 */
constexpr std::array<TaskState, 6> get_all_task_states() {
    return {
        TaskState::IDLE,
        TaskState::STARTED,
        TaskState::RUNNING,
        TaskState::FINISHED,
        TaskState::CANCELLED,
        TaskState::TIMED_OUT
    };
}

} // namespace autoroute
} // namespace freerouting