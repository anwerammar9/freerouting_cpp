#pragma once

#include <vector>
#include <memory>
#include <optional>
#include <set>

#include "geometry/planar/Point.h"
#include "geometry/planar/Polyline.h"
#include "geometry/planar/TileShape.h"

namespace freerouting::datastructures
{
class TimeLimit;
}

namespace freerouting::geometry::planar
{
class LineSegment;
class Direction;
}

namespace freerouting::board
{

class RoutingBoard;
class Item;
class Pin;
class CalcFromSide;
class ShapeSearchTree;

/**
 * Headless shove trace algorithm.
 *
 * This class mirrors the Java class
 * app.freerouting.board.optimize.TraceShover (875 lines). The Java baseline
 * calls the same class ShoveTraceAlgo in an older revision; the method
 * signatures below are the ones of the current java/app/freerouting/board/
 * optimize/TraceShover.java.
 *
 * Ported so far: spring_over() and spring_over_obstacles() (the full Java
 * port of Java TraceShover.springOver/springOverObstacles) plus the tie-pin
 * obstacle filter getIgnoreItemsAtTiePins().
 *
 * Still stubbed (to be ported in a later increment):
 *  - check(TileShape, ShapeEntrySide, Direction, ...): the via shove check,
 *    the boundary-box check and the substitute-trace recursion
 *  - insert(TileShape, ShapeEntrySide, ...): the board mutation (cutout of
 *    the obstacle traces and insertion of the substitute traces)
 *  - the static check(RoutingBoard, LineSegment, ...)
 */
class ShoveTraceAlgo
{
public:
    explicit ShoveTraceAlgo(RoutingBoard* p_board);

    // Static check for a line segment shove – returns max trace length that can be shoved.
    static double check(RoutingBoard* p_board,
                        const geometry::planar::LineSegment& p_line_segment,
                        bool p_shove_to_the_left,
                        int p_layer,
                        const std::vector<int>& p_net_no_arr,
                        int p_trace_half_width,
                        int p_cl_type,
                        int p_max_recursion_depth,
                        int p_max_via_recursion_depth);

    // Instance check with direction and spring_over depth
    bool check(const geometry::planar::TileShape& p_trace_shape,
               const CalcFromSide* p_from_side,
               const geometry::planar::Direction* p_dir,
               int p_layer,
               const std::vector<int>& p_net_no_arr,
               int p_cl_type,
               int p_max_recursion_depth,
               int p_max_via_recursion_depth,
               int p_max_spring_over_recursion_depth,
               const datastructures::TimeLimit* p_time_limit);

    // Insert trace by shoving obstacles – headless stub returns false if vias need shoving
    bool insert(const geometry::planar::TileShape& p_trace_shape,
                const CalcFromSide* p_from_side,
                int p_layer,
                const std::vector<int>& p_net_no_arr,
                int p_cl_type,
                const std::vector<Item*>& p_ignore_items,
                int p_max_recursion_depth,
                int p_max_via_recursion_depth,
                int p_max_spring_over_recursion_depth);

    /**
     * Returns the items which are contacts of same-net pins under p_trace_shape
     * on p_layer and therefore have to be ignored as obstacles (tie pins).
     * Port of Java TraceShover.getIgnoreItemsAtTiePins().
     */
    std::vector<Item*> get_ignore_items_at_tie_pins(const geometry::planar::TileShape& p_trace_shape,
                                                    int p_layer,
                                                    const std::vector<int>& p_net_no_arr);

    /**
     * Result of the private spring_over() helper. Java signals its two
     * outcomes by returning either null (failure) or the input polyline object
     * itself (no obstacle in the way); the C++ port uses the optional and this
     * flag instead of object identity.
     */
    struct SpringOverResult
    {
        /** The new polyline, or std::nullopt if the spring over failed. */
        std::optional<geometry::planar::Polyline> polyline;

        /** False if the input polyline was returned unchanged (Java identity). */
        bool changed = false;
    };

    /**
     * Spring over obstacles – wraps the polyline around the obstacles in
     * counterclock sense. Port of Java TraceShover.springOver().
     */
    SpringOverResult spring_over(const geometry::planar::Polyline& p_polyline,
                                 int p_half_width,
                                 int p_layer,
                                 const std::vector<int>& p_net_no_arr,
                                 int p_cl_type,
                                 bool p_over_connected_pins,
                                 int p_recursion_depth,
                                 const std::set<Pin*>* p_contact_pins);

    /**
     * Wraps p_polyline around the obstacles of both turning directions and
     * returns the shorter result. Returns std::nullopt, if neither direction
     * could be found. Port of Java TraceShover.springOverObstacles().
     *
     * p_contact_pins == nullptr mirrors the Java null argument, which makes
     * the spring over treat own net pins as non obstacles; an empty set makes
     * every own net pin an obstacle instead.
     */
    std::optional<geometry::planar::Polyline> spring_over_obstacles(
        const geometry::planar::Polyline& p_polyline,
        int p_half_width,
        int p_layer,
        const std::vector<int>& p_net_no_arr,
        int p_cl_type,
        const std::set<Pin*>* p_contact_pins);

private:
    /**
     * Returns the items in the search tree overlapping p_shape with clearance,
     * as a plain vector (Java uses the returned collection directly).
     */
    static std::vector<Item*> to_item_vector(ShapeSearchTree* p_search_tree,
                                            const geometry::planar::TileShape& p_shape,
                                            int p_layer,
                                            int p_cl_type);

    /** Returns p_item_arr without the items contained in p_removed_arr. */
    static std::vector<Item*> remove_items(const std::vector<Item*>& p_item_arr,
                                          const std::vector<Item*>& p_removed_arr);

    RoutingBoard* board_ = nullptr;
};

} // namespace freerouting::board
