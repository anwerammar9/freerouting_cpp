#include "autoroute/TaskState.h"

namespace freerouting {
namespace autoroute {

std::string to_string(TaskState state) {
    switch (state) {
        case TaskState::IDLE:     return "IDLE";
        case TaskState::STARTED:  return "STARTED";
        case TaskState::RUNNING:  return "RUNNING";
        case TaskState::FINISHED: return "FINISHED";
        case TaskState::CANCELLED: return "CANCELLED";
        case TaskState::TIMED_OUT: return "TIMED_OUT";
    }
    return "UNKNOWN";
}

std::string get_task_description(TaskState state) {
    switch (state) {
        case TaskState::IDLE:     return "Task has not started yet";
        case TaskState::STARTED:  return "Task has been started";
        case TaskState::RUNNING:  return "Task is currently running";
        case TaskState::FINISHED: return "Task completed successfully";
        case TaskState::CANCELLED: return "Task was cancelled";
        case TaskState::TIMED_OUT: return "Task timed out";
    }
    return "Unknown task state";
}

bool is_active_state(TaskState state) {
    return state == TaskState::STARTED || state == TaskState::RUNNING;
}

bool is_terminal_state(TaskState state) {
    return state == TaskState::FINISHED || state == TaskState::CANCELLED || state == TaskState::TIMED_OUT;
}

} // namespace autoroute
} // namespace freerouting