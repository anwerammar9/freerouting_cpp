#pragma once

#include <memory>
#include <vector>

#include "board/ForcedPadAlgo.h"

namespace freerouting::geometry::planar {
class Circle;
class FloatPoint;
class Point;
class Simplex;
class TileShape;
} // namespace freerouting::geometry::planar

namespace freerouting::rules {
class ViaInfo;
} // namespace freerouting::rules

namespace freerouting::board {

class Padstack;
class RoutingBoard;

/**
 * Class with static functions for checking and inserting forced vias.
 * Mirrors Java ForcedViaAlgo.java (223 lines).
 */
class ForcedViaAlgo
{
public:
    ForcedViaAlgo() = delete;

    /**
     * Checks, if a Via is possible at the input layer after evtl. shoving aside obstacle traces.
     * p_room_shape is used for calculating the from_side. p_trace_half_width and
     * p_trace_clearance_class are used to check that a trace of the trace width can start at
     * the via location.
     */
    static ForcedPadAlgo::CheckDrillResult check_layer(double p_via_radius,
                                                      int p_cl_class,
                                                      bool p_attach_smd_allowed,
                                                      const geometry::planar::TileShape& p_room_shape,
                                                      const geometry::planar::Point& p_location,
                                                      int p_layer,
                                                      const std::vector<int>& p_net_no_arr,
                                                      int p_max_recursion_depth,
                                                      int p_max_via_recursion_depth,
                                                      RoutingBoard* p_board,
                                                      int p_trace_half_width = 0,
                                                      int p_trace_clearance_class = 1);

    /**
     * Checks, if a Via is possible with the input parameter after evtl. shoving aside obstacle traces.
     *
     * p_trace_pen_halfwidth_arr and p_trace_clearance_class_index are provided
     * to make space for starting a trace in case the trace width is bigger
     * than the via shape. If p_trace_pen_halfwidth_arr is nullptr, that check
     * is skipped (Java passes null in that case).
     */
    static bool check(const std::shared_ptr<rules::ViaInfo>& p_via_info,
                      const geometry::planar::Point& p_location,
                      const std::vector<int>& p_net_no_arr,
                      int p_max_recursion_depth,
                      int p_max_via_recursion_depth,
                      RoutingBoard* p_board,
                      const std::vector<int>* p_trace_pen_halfwidth_arr = nullptr,
                      int p_trace_clearance_class_index = 1);

    /**
     * Shoves aside traces, so that a via with the input parameters can be inserted without clearance violations.
     * If the shove failed, the database may be damaged, so that an undo becomes necessary.
     * p_trace_clearance_class_no and p_trace_pen_halfwidth_arr is provided to make space for starting a trace
     * in case the trace width is bigger than the via shape. Returns false, if the forced via failed.
     */
    static bool insert(const std::shared_ptr<rules::ViaInfo>& p_via_info,
                       const geometry::planar::Point& p_location,
                       const std::vector<int>& p_net_no_arr,
                       int p_trace_clearance_class_no,
                       const std::vector<int>& p_trace_pen_halfwidth_arr,
                       int p_max_recursion_depth,
                       int p_max_via_recursion_depth,
                       RoutingBoard* p_board);

private:
    /**
     * Hole-clearance substitute shape for a copper-less layer of a via padstack:
     * the drill still passes through, so other copper must stay holeClearance
     * away from it. Returns null when the rule is off or no drill radius is
     * known. Port of Java ForcedViaInserter.holeCheckShape().
     */
    static std::unique_ptr<geometry::planar::Circle> hole_check_shape(
        const Padstack* p_padstack,
        const geometry::planar::Point& p_location,
        RoutingBoard* p_board);

    static CalcFromSide calculate_from_side(const geometry::planar::FloatPoint& p_via_location,
                                            const geometry::planar::TileShape& p_via_shape,
                                            const geometry::planar::Simplex& p_room_shape,
                                            double p_dist,
                                            bool p_is_90_degree);
};

} // namespace freerouting::board
