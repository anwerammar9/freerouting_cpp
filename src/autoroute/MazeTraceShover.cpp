#include "autoroute/MazeTraceShover.h"

#include "autoroute/AutorouteControl.h"
#include "autoroute/ExpansionRoom.h"
#include "board/Item.h"
#include "board/PolylineTrace.h"
#include "board/RoutingBoard.h"
#include "board/ShoveTraceAlgo.h"
#include "board/Trace.h"
#include "board/Via.h"
#include "geometry/planar/Direction.h"
#include "geometry/planar/FloatLine.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/LineSegment.h"
#include "geometry/planar/Point.h"
#include "geometry/planar/Polyline.h"
#include "geometry/planar/TileShape.h"
#include "geometry/planar/Vector.h"
#include "logger/FRLogger.h"

#include <algorithm>
#include <limits>
#include <vector>

namespace freerouting::autoroute {

using geometry::planar::FloatLine;
using geometry::planar::FloatPoint;
using geometry::planar::IntBox;
using geometry::planar::IntDirection;
using geometry::planar::IntPoint;
using geometry::planar::IntVector;
using geometry::planar::Line;
using geometry::planar::LineSegment;
using geometry::planar::Polyline;
using geometry::planar::Side;
using geometry::planar::TileShape;
using logger::FRLogger;

namespace {

/**
 * Reads p_value[0] of a control array, or p_default if the array does not
 * describe the requested layer. The Java code would raise an
 * IndexOutOfBoundsException; returning a neutral value keeps an
 * incompletely configured control from crashing the search.
 */
double trace_half_width_or(const std::vector<int>& p_value, int p_layer, double p_default)
{
    if (p_layer < 0 || static_cast<size_t>(p_layer) >= p_value.size())
    {
        return p_default;
    }
    return p_value[p_layer];
}

} // namespace

bool MazeTraceShover::check_shove_trace_line(const MazeListElement& p_list_element,
    ObstacleExpansionRoom* p_obstacle_room,
    board::RoutingBoard* p_board,
    const AutorouteControl& p_ctrl,
    bool p_shove_to_the_left,
    std::vector<DoorSection>& p_to_door_list)
{
    if (p_obstacle_room == nullptr || p_board == nullptr)
    {
        return true;
    }
    auto* from_door = dynamic_cast<ExpansionDoor*>(p_list_element.door);
    if (from_door == nullptr)
    {
        return true;
    }
    auto* obstacle_trace = dynamic_cast<board::PolylineTrace*>(p_obstacle_room->get_item());
    if (obstacle_trace == nullptr)
    {
        return true;
    }
    int trace_layer = p_obstacle_room->get_layer();
    // Only traces with the same halfwidth and the same clearance class can be
    // shoved.
    const double ctrl_trace_half_width =
        trace_half_width_or(p_ctrl.trace_half_width, trace_layer, 0.0);
    if (static_cast<double>(obstacle_trace->get_half_width()) != ctrl_trace_half_width
        || obstacle_trace->get_clearance_class() != p_ctrl.trace_clearance_class_no)
    {
        return true;
    }
    const double compensated_trace_half_width =
        trace_half_width_or(p_ctrl.compensated_trace_half_width, trace_layer, 0.0);
    const IntBox from_door_shape = from_door->get_shape();
    if (from_door_shape.max_width() < 2 * compensated_trace_half_width)
    {
        return true;
    }
    int trace_corner_no = p_obstacle_room->get_index_in_item();

    const Polyline& trace_polyline = obstacle_trace->polyline();
    const int line_count = static_cast<int>(trace_polyline.arr.size());

    // Check if trace_corner_no allows access to indices up to
    // trace_corner_no + 2. Stale indices can occur when traces are modified
    // during routing (pull-tight, shoving, etc.).
    if (trace_corner_no < 0 || trace_corner_no >= line_count - 2)
    {
        return false;
    }
    const std::vector<ExpansionDoor*> room_doors = p_obstacle_room->get_doors();
    // The side of the trace line seen from the doors to expand.
    // Used to determine, if a door is on the right side to put it into the
    // door list.
    LineSegment shove_line_segment = LineSegment::from_endpoints(IntPoint(0, 0), IntPoint(0, 0));
    if (from_door->get_dimension() == 2)
    {
        // Shove from a link door into the direction of the other link door.
        // The static_cast to the base class selects the ExpansionRoom overload
        // of other_room(), which is the one Java resolves to here; the
        // CompleteExpansionRoom overload would null the result for a door to
        // an incomplete free space room.
        ExpansionRoom* other_room =
            from_door->other_room(static_cast<ExpansionRoom*>(p_obstacle_room));
        auto* other_obstacle_room = dynamic_cast<ObstacleExpansionRoom*>(other_room);
        if (other_obstacle_room == nullptr)
        {
            return false;
        }
        if (!end_points_matching(*obstacle_trace, other_obstacle_room->get_item()))
        {
            return false;
        }
        FloatPoint door_center = from_door_shape.centre_of_gravity();
        FloatPoint corner1 = trace_polyline.corner_approx(trace_corner_no);
        FloatPoint corner2 = trace_polyline.corner_approx(trace_corner_no + 1);
        if (corner1.distance_square(corner2) < 1)
        {
            // shove_line_segment may be reduced to a point
            return false;
        }
        bool shove_into_direction_of_trace_start =
            door_center.distance_square(corner2) < door_center.distance_square(corner1);
        shove_line_segment = LineSegment(trace_polyline, trace_corner_no + 1);
        if (shove_into_direction_of_trace_start)
        {
            // Shove from the endpoint to the start point of the line segment
            shove_line_segment = shove_line_segment.opposite();
        }
    }
    else
    {
        ExpansionRoom* from_room =
            from_door->other_room(static_cast<ExpansionRoom*>(p_obstacle_room));
        if (from_room == nullptr)
        {
            return true;
        }
        FloatPoint from_point = from_room->get_shape().centre_of_gravity();
        Line shove_trace_line = trace_polyline.arr[trace_corner_no + 1];
        std::optional<FloatLine> door_line_segment = from_door_shape.diagonal_corner_segment();
        if (!door_line_segment.has_value())
        {
            // Divergence: Java dereferences the null result of
            // diagonalCornerSegment() here and throws. An empty from door
            // shape cannot be shoved, so the search continues with the other
            // door sections instead.
            FRLogger::instance().trace(
                "MazeTraceShover.check_shove_trace_line: from door shape is empty");
            return true;
        }
        Side side_of_trace_line = shove_trace_line.side_of(door_line_segment->a(), 0.0);

        std::optional<FloatLine> polar_line_segment = from_door_shape.polar_line_segment(from_point);
        if (!polar_line_segment.has_value())
        {
            return true;
        }

        bool door_line_swapped = polar_line_segment->b().distance_square(door_line_segment->a())
            < polar_line_segment->a().distance_square(door_line_segment->a());

        bool section_ok;
        // Shove only from the right most section to the right or from the left
        // most section to the left.
        const double shape_entry_check_distance = compensated_trace_half_width + 5;
        const double check_dist_square = shape_entry_check_distance * shape_entry_check_distance;

        if ((p_shove_to_the_left && !door_line_swapped) || (!p_shove_to_the_left && door_line_swapped))
        {
            section_ok = p_list_element.section_no_of_door == p_list_element.door->maze_search_element_count() - 1
                && (p_list_element.shape_entry.a().distance_square(door_line_segment->b()) <= check_dist_square
                    || p_list_element.shape_entry.b().distance_square(door_line_segment->b())
                        <= check_dist_square);
        }
        else
        {
            section_ok = p_list_element.section_no_of_door == 0
                && (p_list_element.shape_entry.a().distance_square(door_line_segment->a()) <= check_dist_square
                    || p_list_element.shape_entry.b().distance_square(door_line_segment->a())
                        <= check_dist_square);
        }
        if (!section_ok)
        {
            return false;
        }

        // Create the line segment for shoving.
        FloatLine shrinked_line_segment = polar_line_segment->shrink_segment(compensated_trace_half_width);
        const IntVector trace_dir = shove_trace_line.direction();
        IntDirection perpendicular_direction = IntDirection(trace_dir.x, trace_dir.y).turn_45_degree_int(2);
        if (side_of_trace_line == Side::ON_THE_LEFT)
        {
            if (p_shove_to_the_left)
            {
                Line start_closing_line =
                    Line::get_instance(shrinked_line_segment.b().round(), perpendicular_direction);
                shove_line_segment = LineSegment(start_closing_line, trace_polyline.arr[trace_corner_no + 1],
                    trace_polyline.arr[trace_corner_no + 2]);
            }
            else
            {
                Line start_closing_line =
                    Line::get_instance(shrinked_line_segment.a().round(), perpendicular_direction);
                shove_line_segment = LineSegment(start_closing_line, trace_polyline.arr[trace_corner_no + 1].opposite(),
                    trace_polyline.arr[trace_corner_no].opposite());
            }
        }
        else
        {
            if (p_shove_to_the_left)
            {
                Line start_closing_line =
                    Line::get_instance(shrinked_line_segment.b().round(), perpendicular_direction);
                shove_line_segment = LineSegment(start_closing_line, trace_polyline.arr[trace_corner_no + 1].opposite(),
                    trace_polyline.arr[trace_corner_no].opposite());
            }
            else
            {
                Line start_closing_line =
                    Line::get_instance(shrinked_line_segment.a().round(), perpendicular_direction);
                shove_line_segment = LineSegment(start_closing_line, trace_polyline.arr[trace_corner_no + 1],
                    trace_polyline.arr[trace_corner_no + 2]);
            }
        }
    }

    const int trace_half_width = static_cast<int>(ctrl_trace_half_width);
    const std::vector<int> net_numbers{p_ctrl.net_no};

    double shove_width = p_board->check_trace_segment(
        shove_line_segment, trace_layer, net_numbers, trace_half_width, p_ctrl.trace_clearance_class_no, true);
    const double no_conflict = static_cast<double>(std::numeric_limits<int>::max());
    bool segment_shortened = false;
    if (shove_width < no_conflict)
    {
        // Shorten shove_line_segment
        shove_width = shove_width - 1;
        if (shove_width <= 0)
        {
            return true;
        }
        shove_line_segment = shove_line_segment.change_length_approx(shove_width);
        segment_shortened = true;
    }

    FloatPoint from_corner = shove_line_segment.start_point_approx();
    FloatPoint to_corner = shove_line_segment.end_point_approx();
    bool segment_is_point = from_corner.distance_square(to_corner) < 0.1;

    if (!segment_is_point)
    {
        // Divergence: Java calls TraceShover.check(), the C++ port of which
        // is ShoveTraceAlgo::check(). The C++ check only approximates the
        // clearance analysis of the Java original (no via/spring-over
        // recursion), so a successful Java shove may be refused here.
        shove_width = board::ShoveTraceAlgo::check(p_board, shove_line_segment, p_shove_to_the_left, trace_layer,
            net_numbers, trace_half_width, p_ctrl.trace_clearance_class_no, p_ctrl.max_shove_trace_recursion_depth,
            p_ctrl.max_shove_via_recursion_depth);

        if (shove_width <= 0)
        {
            return true;
        }
    }

    // Put the doors on this side of the room into p_to_door_list.
    if (segment_shortened)
    {
        shove_width = std::min(shove_width, from_corner.distance(to_corner));
    }

    Line shove_line = shove_line_segment.get_line();

    // from_door_compare_distance is used to check, that a door is between
    // from_door and the end point of the shove line.
    double from_door_compare_distance;
    if (from_door->get_dimension() == 2 || segment_is_point)
    {
        from_door_compare_distance = std::numeric_limits<double>::max();
    }
    else
    {
        from_door_compare_distance = to_corner.distance_square(from_door_shape.corner(0).to_float());
    }

    for (ExpansionDoor* current_door : room_doors)
    {
        if (current_door == from_door)
        {
            continue;
        }
        if (auto* room = dynamic_cast<ObstacleExpansionRoom*>(current_door->first_room_))
        {
            auto* room1 = dynamic_cast<ObstacleExpansionRoom*>(current_door->second_room_);
            if (room1 != nullptr)
            {
                board::Item* first_room_item = room->get_item();
                board::Item* second_room_item = room1->get_item();
                if (first_room_item != second_room_item)
                {
                    // There may be topological problems at a trace fork.
                    continue;
                }
            }
        }
        IntBox current_door_shape = current_door->get_shape();
        if (current_door->get_dimension() == 2 && shove_width >= no_conflict)
        {
            bool add_link_door = current_door_shape.contains(to_corner);
            if (add_link_door)
            {
                std::vector<FloatLine> line_sections =
                    current_door->get_section_segments(compensated_trace_half_width);
                if (line_sections.empty())
                {
                    continue;
                }
                p_to_door_list.push_back(DoorSection{current_door, 0, line_sections[0]});
            }
        }
        else if (!segment_is_point)
        {
            // Now current_door is 1-dimensional.
            // Check, that current_door is on the same border line as from_door.
            std::optional<FloatLine> current_door_segment = current_door_shape.diagonal_corner_segment();
            if (!current_door_segment.has_value())
            {
                FRLogger::instance().trace(
                    "MazeTraceShover.check_shove_trace_line: door shape is empty");
                continue;
            }
            Side start_corner_side_of_trace_line = shove_line.side_of(current_door_segment->a(), 0.0);
            Side end_corner_side_of_trace_line = shove_line.side_of(current_door_segment->b(), 0.0);
            if (p_shove_to_the_left)
            {
                if (start_corner_side_of_trace_line != Side::ON_THE_LEFT
                    || end_corner_side_of_trace_line != Side::ON_THE_LEFT)
                {
                    continue;
                }
            }
            else
            {
                if (start_corner_side_of_trace_line != Side::ON_THE_RIGHT
                    || end_corner_side_of_trace_line != Side::ON_THE_RIGHT)
                {
                    continue;
                }
            }
            std::optional<FloatLine> current_door_line = current_door_shape.polar_line_segment(from_corner);
            if (!current_door_line.has_value())
            {
                continue;
            }
            FloatPoint current_door_nearest_corner;
            if (current_door_line->a().distance_square(from_corner)
                <= current_door_line->b().distance_square(from_corner))
            {
                current_door_nearest_corner = current_door_line->a();
            }
            else
            {
                current_door_nearest_corner = current_door_line->b();
            }
            if (to_corner.distance_square(current_door_nearest_corner) >= from_door_compare_distance)
            {
                // current_door is not located into the direction of to_corner.
                continue;
            }
            FloatPoint current_door_projection = current_door_nearest_corner.projection_approx(shove_line);

            if (current_door_projection.distance(from_corner) + compensated_trace_half_width <= shove_width)
            {
                std::vector<FloatLine> line_sections =
                    current_door->get_section_segments(compensated_trace_half_width);
                for (size_t i = 0; i < line_sections.size(); i++)
                {
                    FloatLine current_line_section = line_sections[i];
                    FloatPoint current_section_nearest_corner;
                    if (current_line_section.a().distance_square(from_corner)
                        <= current_line_section.b().distance_square(from_corner))
                    {
                        current_section_nearest_corner = current_line_section.a();
                    }
                    else
                    {
                        current_section_nearest_corner = current_line_section.b();
                    }
                    FloatPoint current_section_projection =
                        current_section_nearest_corner.projection_approx(shove_line);
                    if (current_section_projection.distance(from_corner) <= shove_width)
                    {
                        p_to_door_list.push_back(
                            DoorSection{current_door, static_cast<int>(i), current_line_section});
                    }
                }
            }
        }
    }
    return true;
}

bool MazeTraceShover::end_points_matching(const board::PolylineTrace& p_trace, const board::Item* p_from_item)
{
    if (p_from_item == &p_trace)
    {
        return true;
    }
    if (!p_trace.shares_net(p_from_item))
    {
        return false;
    }
    bool points_matching;
    if (auto* item = dynamic_cast<const board::DrillItem*>(p_from_item))
    {
        const geometry::planar::IntPoint& from_center = item->get_center();
        points_matching = from_center.equals(&p_trace.first_corner()) || from_center.equals(&p_trace.last_corner());
    }
    else if (auto* from_trace = dynamic_cast<const board::PolylineTrace*>(p_from_item))
    {
        const geometry::planar::Point& trace_first = p_trace.first_corner();
        const geometry::planar::Point& trace_last = p_trace.last_corner();
        const geometry::planar::Point& from_first = from_trace->first_corner();
        const geometry::planar::Point& from_last = from_trace->last_corner();
        points_matching = trace_first.equals(&from_first) || trace_first.equals(&from_last)
            || trace_last.equals(&from_first) || trace_last.equals(&from_last);
    }
    else
    {
        points_matching = false;
    }
    return points_matching;
}

} // namespace freerouting::autoroute
