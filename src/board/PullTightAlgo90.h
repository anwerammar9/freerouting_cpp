#pragma once

#include <memory>

#include "board/PullTightAlgo.h"

namespace freerouting::board {

/**
 * Pull-tight algorithm for 90-degree (orthogonal) traces.
 * Mirrors Java PullTightAlgo90.java (144 lines).
 */
class PullTightAlgo90 : public PullTightAlgo
{
public:
    PullTightAlgo90(RoutingBoard* p_board,
                    const std::vector<int>& p_only_net_no_arr,
                    datastructures::Stoppable* p_stoppable_thread,
                    int p_time_limit,
                    const geometry::planar::Point* p_keep_point,
                    int p_keep_point_layer);

    geometry::planar::Polyline pull_tight(geometry::planar::Polyline p_polyline) override;
    std::unique_ptr<geometry::planar::Polyline> smoothen_start_corner_at_trace(
        PolylineTrace* p_trace) override;
    std::unique_ptr<geometry::planar::Polyline> smoothen_end_corner_at_trace(
        PolylineTrace* p_trace) override;

private:
    /**
     * Tries to skip the second corner of p_polyline. Returns p_polyline if
     * nothing was changed.
     */
    geometry::planar::Polyline try_skip_second_corner(
        const geometry::planar::Polyline& p_polyline);

    /**
     * Tries to reduce the amount of corners of p_polyline. Returns p_polyline
     * if nothing was changed.
     */
    geometry::planar::Polyline try_skip_corners(const geometry::planar::Polyline& p_polyline);
};

} // namespace freerouting::board
