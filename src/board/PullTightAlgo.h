#pragma once

#include <memory>
#include <optional>
#include <set>
#include <vector>

#include "geometry/planar/IntOctagon.h"
#include "geometry/planar/Point.h"
#include "geometry/planar/Polyline.h"
#include "geometry/planar/Vector.h"

namespace freerouting::autoroute {
struct ExpansionCostFactor;
} // namespace freerouting::autoroute

namespace freerouting::datastructures {
class Stoppable;
class TimeLimit;
} // namespace freerouting::datastructures

namespace freerouting::geometry::planar {
class Direction;
} // namespace freerouting::geometry::planar

namespace freerouting::board {

class Pin;
class PolylineTrace;
class RoutingBoard;

/**
 * Class with functionality for optimising traces and vias.
 * Mirrors Java PullTightAlgo.java (471 lines).
 */
class PullTightAlgo
{
public:
    virtual ~PullTightAlgo();

    /**
     * Returns a new PullTightAlgo instance for the board's angle restriction.
     * If p_clip_shape != nullptr, optimizing is restricted to that shape.
     * Mirrors Java PullTightAlgo.get_instance().
     */
    static std::unique_ptr<PullTightAlgo> get_instance(
        RoutingBoard* p_board,
        const std::vector<int>& p_only_net_no_arr,
        const geometry::planar::IntOctagon* p_clip_shape,
        int p_min_translate_dist,
        datastructures::Stoppable* p_stoppable_thread,
        int p_time_limit,
        const geometry::planar::Point* p_keep_point,
        int p_keep_point_layer);

    /**
     * Function for optimizing the route in an internal marked area.
     * p_trace_cost_arr is used for optimizing vias and may be null.
     */
    void opt_changed_area(const std::vector<autoroute::ExpansionCostFactor>* p_trace_cost_arr);

    /**
     * Function for optimizing a single trace polygon. p_contact_pins are the
     * pins at the end corners of p_polyline.
     */
    geometry::planar::Polyline pull_tight(const geometry::planar::Polyline& p_polyline,
                                          int p_layer,
                                          int p_half_width,
                                          const std::vector<int>& p_net_no_arr,
                                          int p_cl_type,
                                          const std::set<Pin*>& p_contact_pins);

    /**
     * Smoothens acute angles with contact traces. Returns true if changed.
     */
    bool smoothen_end_corners_at_trace(PolylineTrace* p_trace);

    /**
     * Splits the traces containing the keep point. Returns true if split.
     */
    bool split_traces_at_keep_point();

    const std::vector<int>& only_net_nos() const { return only_net_no_arr_; }

    virtual geometry::planar::Polyline pull_tight(geometry::planar::Polyline p_polyline) = 0;
    virtual std::unique_ptr<geometry::planar::Polyline> smoothen_start_corner_at_trace(
        PolylineTrace* p_trace) = 0;
    virtual std::unique_ptr<geometry::planar::Polyline> smoothen_end_corner_at_trace(
        PolylineTrace* p_trace) = 0;

protected:
    PullTightAlgo(RoutingBoard* p_board,
                  const std::vector<int>& p_only_net_no_arr,
                  datastructures::Stoppable* p_stoppable_thread,
                  int p_time_limit,
                  const geometry::planar::Point* p_keep_point,
                  int p_keep_point_layer);

    /**
     * Terminates the pull tight algorithm, if the user has made a stop request.
     */
    bool is_stop_requested() const;

    /**
     * Tries to shorten p_polyline by relocating its lines.
     */
    virtual geometry::planar::Polyline reposition_lines(
        const geometry::planar::Polyline& p_polyline);

    /**
     * Tries to reposition the line with index p_no to make the polyline
     * consisting of p_line_arr shorter. Returns nullopt if impossible.
     */
    virtual std::optional<geometry::planar::Line> reposition_line(
        const std::vector<geometry::planar::Line>& p_line_arr, int p_no);

    /**
     * Constructs the line through p_point with direction p_dir.
     * Exact for IntDirection; otherwise nullopt. Mirrors Java
     * new Line(Point, Direction).
     */
    static std::optional<geometry::planar::Line> line_through_point(
        const geometry::planar::IntPoint& p_point,
        const geometry::planar::Direction& p_dir);

    /**
     * Converts any point to IntPoint, rounding float locations.
     */
    static geometry::planar::IntPoint to_int_point(const geometry::planar::Point& p_point);

    /**
     * Content equality of two polylines (line count + Line::equals).
     */
    static bool polylines_equal(const geometry::planar::Polyline& p_first,
                                const geometry::planar::Polyline& p_second);

    /**
     * Tries to skip line segments of length 0.
     */
    geometry::planar::Polyline skip_segments_of_length_0(
        const geometry::planar::Polyline& p_polyline);

    /**
     * Wraps around pins of the own net to avoid acid traps. Currently disabled
     * in Java (returns input); ported as-is.
     */
    geometry::planar::Polyline avoid_acid_traps(const geometry::planar::Polyline& p_polyline);

    /**
     * Returns the contact pins for clearance checks, or nullptr when no pins
     * are exempt. Preserves the Java null-vs-empty distinction: a cleared or
     * never-set pin set disables the pin rule entirely.
     */
    const std::set<Pin*>* contact_pin_set() const
    {
        return contact_pins_.empty() ? nullptr : &contact_pins_;
    }

    static constexpr double c_max_cos_angle = 0.999;
    static constexpr double c_min_corner_dist_square = 0.9;

    RoutingBoard* board_ = nullptr;
    std::vector<int> only_net_no_arr_;
    int curr_layer_ = 0;
    int curr_half_width_ = 0;
    std::vector<int> curr_net_no_arr_;
    int curr_cl_type_ = 0;
    std::unique_ptr<geometry::planar::IntOctagon> curr_clip_shape_;
    std::set<Pin*> contact_pins_;
    int min_translate_dist_ = 100;

private:
    bool smoothen_end_corners_at_trace_1(PolylineTrace* p_trace);
    std::unique_ptr<geometry::planar::Polyline> smoothen_end_corners_at_trace_2(
        PolylineTrace* p_trace);

    datastructures::Stoppable* stoppable_thread_ = nullptr;
    std::unique_ptr<datastructures::TimeLimit> time_limit_;
    std::optional<geometry::planar::FloatPoint> keep_point_;
    int keep_point_layer_ = -1;
};

} // namespace freerouting::board
