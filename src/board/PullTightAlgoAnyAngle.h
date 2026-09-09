#pragma once

#include <memory>

#include "board/PullTightAlgo.h"

namespace freerouting::board {

/**
 * Pull-tight algorithm for any-angle traces.
 * Mirrors Java PullTightAlgoAnyAngle.java (954 lines).
 */
class PullTightAlgoAnyAngle : public PullTightAlgo
{
public:
    PullTightAlgoAnyAngle(RoutingBoard* p_board,
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

protected:
    geometry::planar::Polyline reposition_lines(
        const geometry::planar::Polyline& p_polyline) override;
    std::optional<geometry::planar::Line> reposition_line(
        const std::vector<geometry::planar::Line>& p_line_arr, int p_start_no) override;

private:
    static constexpr double SKIP_LENGTH = 10.0;

    /**
     * Tries to reduce the corner count by replacing two consecutive lines.
     */
    geometry::planar::Polyline reduce_corners(const geometry::planar::Polyline& p_polyline);

    /**
     * Tries to reduce the number of lines by moving lines parallel beyond the
     * intersection of the next or previous lines.
     */
    geometry::planar::Polyline reduce_lines(const geometry::planar::Polyline& p_polyline);

    /**
     * Tries to smoothen p_polyline by cutting off corners.
     */
    geometry::planar::Polyline smoothen_corners(const geometry::planar::Polyline& p_polyline);

    /**
     * Cuts off the corner at p_start_no + 1 with clearance check.
     * Returns nullopt if not possible.
     */
    std::optional<geometry::planar::Line> smoothen_corner(
        const std::vector<geometry::planar::Line>& p_line_arr, int p_start_no);

    /**
     * Tries to skip 1 or 2 lines. Returns p_polyline if nothing changed.
     */
    geometry::planar::Polyline skip_lines(const geometry::planar::Polyline& p_polyline);
};

} // namespace freerouting::board
