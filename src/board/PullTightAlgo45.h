#pragma once

#include <memory>

#include "board/PullTightAlgo.h"

namespace freerouting::board {

/**
 * Pull-tight algorithm for 45-degree traces.
 * Mirrors Java PullTightAlgo45.java (623 lines).
 */
class PullTightAlgo45 : public PullTightAlgo
{
public:
    PullTightAlgo45(RoutingBoard* p_board,
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
     * Tries to reduce the amount of corners of p_polyline. Returns p_polyline
     * if nothing was changed.
     */
    geometry::planar::Polyline reduce_corners(const geometry::planar::Polyline& p_polyline);

    /**
     * Smoothens the 90 degree corners of p_polyline to 45 degree.
     */
    geometry::planar::Polyline smoothen_corners(const geometry::planar::Polyline& p_polyline);

    /**
     * Adds a line to smoothen a corner without clearance check (tiny cut-off).
     * Returns nullopt if not possible.
     */
    std::optional<geometry::planar::Line> smoothen_sharp_corner(
        const std::vector<geometry::planar::Line>& p_line_arr, int p_no);

    /**
     * Smoothens a non integer corner of two diagonal lines with a short axis
     * parallel line. Returns nullopt if not possible.
     */
    std::optional<geometry::planar::Line> smoothen_non_integer_corner(
        const std::vector<geometry::planar::Line>& p_line_arr, int p_no);

    /**
     * Adds a line to smoothen a corner with clearance check. Returns nullopt
     * if not possible.
     */
    std::optional<geometry::planar::Line> smoothen_corner(
        const std::vector<geometry::planar::Line>& p_line_arr, int p_no);
};

} // namespace freerouting::board
