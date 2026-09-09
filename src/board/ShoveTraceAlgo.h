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

/**
 * Headless shove trace algorithm – mirrors Java ShoveTraceAlgo.java (593 lines).
 * Full spring_over / via shove paths are stubbed for headless; the core
 * TileShape shove via ShapeTraceEntries is ported.
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

    // Spring over obstacles – headless minimal (returns p_polyline if no obstacle, else nullptr)
    geometry::planar::Polyline spring_over_obstacles(const geometry::planar::Polyline& p_polyline,
                                                     int p_half_width,
                                                     int p_layer,
                                                     const std::vector<int>& p_net_no_arr,
                                                     int p_cl_type,
                                                     const std::set<Pin*>& p_contact_pins);

private:
    std::vector<Item*> get_ignore_items_at_tie_pins(const geometry::planar::TileShape& p_trace_shape,
                                                    int p_layer,
                                                    const std::vector<int>& p_net_no_arr);

    geometry::planar::Polyline spring_over(const geometry::planar::Polyline& p_polyline,
                                           int p_half_width,
                                           int p_layer,
                                           const std::vector<int>& p_net_no_arr,
                                           int p_cl_type,
                                           bool p_over_connected_pins,
                                           int p_recursion_depth,
                                           const std::set<Pin*>* p_contact_pins);

    RoutingBoard* board_ = nullptr;
};

} // namespace freerouting::board
