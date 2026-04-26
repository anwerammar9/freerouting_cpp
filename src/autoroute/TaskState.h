#pragma once

namespace freerouting::autoroute {

enum class TaskState
{
    IDLE,
    STARTED,
    RUNNING,
    FINISHED,
    CANCELLED,
    TIMED_OUT
};

} // namespace freerouting::autoroute