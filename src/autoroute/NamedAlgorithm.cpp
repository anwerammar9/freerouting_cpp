#include "autoroute/NamedAlgorithm.h"
#include "board/RoutingBoard.h"

namespace freerouting::autoroute {

void NamedAlgorithm::add_board_snapshot_event_listener(events::BoardSnapshotEventListener* p_listener)
{
    if (p_listener)
    {
        board_snapshot_listeners_.push_back(p_listener);
    }
}

void NamedAlgorithm::fire_board_snapshot_event(board::RoutingBoard* p_board)
{
    events::BoardSnapshotEvent event(this, p_board);
    for (auto* listener : board_snapshot_listeners_)
    {
        if (listener)
        {
            listener->on_board_snapshot_event(event);
        }
    }
}

void NamedAlgorithm::add_board_updated_event_listener(events::BoardUpdatedEventListener* p_listener)
{
    if (p_listener)
    {
        board_updated_listeners_.push_back(p_listener);
    }
}

void NamedAlgorithm::fire_board_updated_event(int32_t p_routed_count, int32_t p_incomplete_count,
                                               board::RoutingBoard* p_board)
{
    events::BoardUpdatedEvent event(this, p_routed_count, p_incomplete_count, p_board);
    for (auto* listener : board_updated_listeners_)
    {
        if (listener)
        {
            listener->on_board_updated_event(event);
        }
    }
}

void NamedAlgorithm::add_task_state_changed_event_listener(events::TaskStateChangedEventListener* p_listener)
{
    if (p_listener)
    {
        task_state_changed_listeners_.push_back(p_listener);
    }
}

void NamedAlgorithm::fire_task_state_changed_event(events::TaskStateChangedEvent p_event)
{
    for (auto* listener : task_state_changed_listeners_)
    {
        if (listener)
        {
            listener->on_task_state_changed_event(p_event);
        }
    }
}

} // namespace freerouting::autoroute
