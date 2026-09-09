#include "board/CalcFromSide.h"

#include <cmath>
#include <limits>
#include <optional>
#include <vector>

#include "geometry/planar/FloatLine.h"
#include "geometry/planar/LineSegment.h"
#include "geometry/planar/Polyline.h"
#include "geometry/planar/TileShape.h"
#include "logger/FRLogger.h"

namespace freerouting::board
{

const CalcFromSide CalcFromSide::NOT_CALCULATED =
    CalcFromSide(-1, geometry::planar::FloatPoint(0.0, 0.0));

CalcFromSide::CalcFromSide(int p_no, const geometry::planar::FloatPoint& p_border_intersection)
    : no(p_no), border_intersection(p_border_intersection)
{
}

CalcFromSide::CalcFromSide(const geometry::planar::Polyline& p_polyline, int p_no,
                           const geometry::planar::TileShape& p_shape)
{
    int fromside_no = -1;
    geometry::planar::FloatPoint intersection;
    bool border_intersection_found = false;

    for (int curr_no = p_no; curr_no > 0; curr_no--)
    {
        geometry::planar::LineSegment curr_seg(p_polyline, curr_no);
        const std::vector<int> intersections = curr_seg.border_intersections(p_shape);
        if (!intersections.empty())
        {
            fromside_no = intersections[0];
            intersection =
                curr_seg.get_line().intersection_approx(p_shape.border_line(fromside_no));
            border_intersection_found = true;
            break;
        }
    }
    if (!border_intersection_found)
    {
        // The first corner of p_polyline is inside p_shape.
        geometry::planar::FloatPoint from_point = p_polyline.corner_approx(0);
        geometry::planar::Line check_line = p_polyline.arr[1];
        double min_dist = std::numeric_limits<double>::max();
        int edge_count = p_shape.border_line_count();
        for (int i = 0; i < edge_count; i++)
        {
            geometry::planar::Line curr_line = p_shape.border_line(i);
            geometry::planar::FloatPoint curr_intersection =
                check_line.intersection_approx(curr_line);
            double curr_dist = std::abs(curr_intersection.distance(from_point));
            if (curr_dist < min_dist)
            {
                fromside_no = i;
                intersection = curr_intersection;
                min_dist = curr_dist;
            }
        }
    }
    no = fromside_no;
    border_intersection = intersection;
}

CalcFromSide::CalcFromSide(const geometry::planar::IntPoint& p_from_point,
                           const geometry::planar::TileShape& p_shape)
{
    const std::optional<geometry::planar::IntPoint> border_projection =
        p_shape.nearest_border_point(p_from_point);
    if (border_projection.has_value())
    {
        no = p_shape.contains_on_border_line_no(*border_projection);
        border_intersection = border_projection->to_float();
    }
    else
    {
        no = -1;
        border_intersection = geometry::planar::FloatPoint(0.0, 0.0);
    }
    if (no < 0)
    {
        logger::FRLogger::instance().warn("CalcFromSide: this.no >= 0 expected");
    }
}

CalcFromSide::CalcFromSide(const geometry::planar::LineSegment& p_line_segment,
                           const geometry::planar::TileShape& p_shape,
                           bool p_shove_to_the_left)
{
    const geometry::planar::FloatPoint start_corner = p_line_segment.start_point_approx();
    const geometry::planar::FloatPoint end_corner = p_line_segment.end_point_approx();
    const int border_line_count = p_shape.border_line_count();
    const geometry::planar::Line check_line = p_line_segment.get_line();
    const geometry::planar::FloatPoint first_corner = p_shape.corner_approx(0);
    geometry::planar::Side prev_side = check_line.side_of(first_corner);
    int front_side_no = -1;

    for (int i = 1; i <= border_line_count; i++)
    {
        const geometry::planar::FloatPoint next_corner =
            (i == border_line_count) ? first_corner : p_shape.corner_approx(i);
        const geometry::planar::Side next_side = check_line.side_of(next_corner);
        if (prev_side != next_side)
        {
            const geometry::planar::FloatPoint curr_intersection =
                p_shape.border_line(i - 1).intersection_approx(check_line);
            if (curr_intersection.distance_square(start_corner)
                < curr_intersection.distance_square(end_corner))
            {
                front_side_no = i - 1;
                break;
            }
        }
        prev_side = next_side;
    }
    if (front_side_no < 0)
    {
        double min_distance = std::numeric_limits<double>::max();
        int nearest_side = 0;
        for (int i = 0; i < border_line_count; i++)
        {
            geometry::planar::FloatLine border_line(p_shape.border_line(i).a.to_float(),
                                                    p_shape.border_line(i).b.to_float());
            const geometry::planar::FloatPoint projection =
                border_line.perpendicular_projection(start_corner);
            const geometry::planar::FloatPoint side_start = p_shape.corner_approx(i);
            const geometry::planar::FloatPoint side_end =
                p_shape.corner_approx((i + 1) % border_line_count);
            if (projection.is_contained_in_box(side_start, side_end, 0.01))
            {
                const double distance = start_corner.distance(projection);
                if (distance < min_distance)
                {
                    min_distance = distance;
                    nearest_side = i;
                    border_intersection = projection;
                }
            }
        }
        no = (p_shove_to_the_left ? (nearest_side + 2)
                                  : (nearest_side + border_line_count - 2))
             % border_line_count;
        border_intersection = p_shape.corner_approx(no).middle_point(
            p_shape.corner_approx((no + 1) % border_line_count));
        return;
    }
    no = (p_shove_to_the_left ? (front_side_no + 2) : (front_side_no + border_line_count - 2))
         % border_line_count;
    border_intersection = p_shape.corner_approx(no).middle_point(
        p_shape.corner_approx((no + 1) % border_line_count));
}

} // namespace freerouting::board