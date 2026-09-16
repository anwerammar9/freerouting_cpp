#pragma once

namespace freerouting::autoroute::events {

class TaskStateChangedEvent;

class TaskStateChangedEventListener {
public:
    virtual ~TaskStateChangedEventListener() = default;
    virtual void on_task_state_changed_event(const TaskStateChangedEvent& p_event) = 0;
};

} // namespace freerouting::autoroute::events
