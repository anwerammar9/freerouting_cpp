#include "autoroute/TaskState.h"

namespace freerouting {
namespace autoroute {

bool is_running_state(TaskState state) {
    return state == TaskState::STARTED || state == TaskState::RUNNING;
}

bool is_terminal_state(TaskState state) {
    return state == TaskState::FINISHED || state == TaskState::CANCELLED || state == TaskState::TIMED_OUT;
}

} // namespace autoroute
} // namespace freerouting