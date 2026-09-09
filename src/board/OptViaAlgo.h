#pragma once

#include <memory>
#include <optional>
#include <vector>

namespace freerouting::autoroute {
struct ExpansionCostFactor;
} // namespace freerouting::autoroute

namespace freerouting::geometry::planar {
class FloatPoint;
class IntPoint;
class Point;
class Vector;
} // namespace freerouting::geometry::planar

namespace freerouting::board {

class RoutingBoard;
class Via;

/**
 * Contains functions for optimizing and improving via locations.
 * Mirrors Java OptViaAlgo.java (574 lines).
 */
class OptViaAlgo
{
public:
    OptViaAlgo() = delete;

    /**
     * Optimizes the location of a via connected to at most 2 traces according to
     * the trace costs on the layers of the connected traces. If p_trace_cost_arr
     * is null, the horizontal and vertical trace costs are set to 1.
     * Returns false, if the via was not changed.
     */
    static bool opt_via_location(RoutingBoard* p_board,
                                 Via* p_via,
                                 const std::vector<autoroute::ExpansionCostFactor>* p_trace_cost_arr,
                                 int p_trace_pull_tight_accuracy,
                                 int p_max_recursion_depth);

private:
    /**
     * Optimisations for vias with only 1 connected Trace (Plane or Fanout Vias).
     */
    static bool opt_plane_or_fanout_via(RoutingBoard* p_board,
                                        Via* p_via,
                                        int p_trace_pull_tight_accuracy,
                                        int p_max_recursion_depth);

    /**
     * Tries to move the via into the direction of p_to_location as far as possible.
     * Returns the new location of the via, or nullopt if no move was possible.
     */
    static std::optional<geometry::planar::IntPoint> reposition_via(
        RoutingBoard* p_board,
        Via* p_via,
        const geometry::planar::IntPoint& p_to_location,
        int p_trace_half_width,
        int p_trace_layer,
        int p_trace_cl_class);

    static bool reposition_via(RoutingBoard* p_board,
                               Via* p_via,
                               const geometry::planar::IntPoint& p_to_location,
                               int p_trace_half_width_1,
                               int p_trace_layer_1,
                               int p_trace_cl_class_1,
                               const geometry::planar::IntPoint& p_connect_location,
                               int p_trace_half_width_2,
                               int p_trace_layer_2,
                               int p_trace_cl_class_2);

    /**
     * Tries to reposition the via to a better location according to the trace
     * costs. Returns nullopt if no better location was found.
     */
    static std::optional<geometry::planar::IntPoint> reposition_via(
        RoutingBoard* p_board,
        Via* p_via,
        int p_first_trace_half_width,
        int p_first_trace_cl_class,
        int p_first_trace_layer,
        const autoroute::ExpansionCostFactor& p_first_trace_costs,
        const geometry::planar::IntPoint& p_first_trace_from_corner,
        int p_second_trace_half_width,
        int p_second_trace_cl_class,
        int p_second_trace_layer,
        const autoroute::ExpansionCostFactor& p_second_trace_costs,
        const geometry::planar::IntPoint& p_second_trace_from_corner);

    /**
     * Checks if two points are within the specified tolerance distance.
     * Uses Manhattan distance, matching DrillItem.get_normal_contacts().
     */
    static bool is_within_tolerance(const geometry::planar::Point& p_1,
                                    const geometry::planar::Point& p_2,
                                    int p_tolerance);
};

} // namespace freerouting::board
