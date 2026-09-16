#pragma once

namespace freerouting::autoroute::events {

class BoardSnapshotEvent;

class BoardSnapshotEventListener {
public:
    virtual ~BoardSnapshotEventListener() = default;
    virtual void on_board_snapshot_event(const BoardSnapshotEvent& p_event) = 0;
};

} // namespace freerouting::autoroute::events
