#pragma once

#include "autoroute/NamedAlgorithmType.h"
#include "autoroute/events/BoardSnapshotEvent.h"
#include "autoroute/events/BoardSnapshotEventListener.h"
#include "autoroute/events/BoardUpdatedEvent.h"
#include "autoroute/events/BoardUpdatedEventListener.h"
#include "autoroute/events/TaskStateChangedEvent.h"
#include "autoroute/events/TaskStateChangedEventListener.h"
#include <string>
#include <vector>
#include <memory>

namespace freerouting::board {
class RoutingBoard;
}

namespace freerouting::autoroute {

class NamedAlgorithm {
public:
    NamedAlgorithm() = default;
    virtual ~NamedAlgorithm() = default;

    virtual std::string get_id() const = 0;
    virtual std::string get_name() const = 0;
    virtual std::string get_version() const = 0;
    virtual std::string get_description() const = 0;
    virtual NamedAlgorithmType get_type() const = 0;

    void add_board_snapshot_event_listener(events::BoardSnapshotEventListener* p_listener);
    void fire_board_snapshot_event(board::RoutingBoard* p_board);

    void add_board_updated_event_listener(events::BoardUpdatedEventListener* p_listener);
    void fire_board_updated_event(int32_t p_routed_count, int32_t p_incomplete_count,
                                  board::RoutingBoard* p_board);

    void add_task_state_changed_event_listener(events::TaskStateChangedEventListener* p_listener);
    void fire_task_state_changed_event(events::TaskStateChangedEvent p_event);

private:
    std::vector<events::BoardSnapshotEventListener*> board_snapshot_listeners_;
    std::vector<events::BoardUpdatedEventListener*> board_updated_listeners_;
    std::vector<events::TaskStateChangedEventListener*> task_state_changed_listeners_;
};

} // namespace freerouting::autoroute
