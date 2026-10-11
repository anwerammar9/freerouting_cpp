#include "board/PullTightAlgo45.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

#include "board/PolylineTrace.h"
#include "board/RoutingBoard.h"
#include "board/Trace.h"
#include "datastructures/Signum.h"
#include "geometry/planar/Direction.h"
#include "geometry/planar/Limits.h"
#include "logger/FRLogger.h"

namespace freerouting::board {

namespace gp = freerouting::geometry::planar;

namespace {

[[maybe_unused]] gp::IntPoint to_int_point(const gp::Point& p_point)
{
    if (const auto* int_point = dynamic_cast<const gp::IntPoint*>(&p_point))
    {
        return *int_point;
    }
    return p_point.to_float().round();
}

/**
 * Constructs the line through p_point with direction p_dir.
 * Exact when the direction is an IntDirection; otherwise returns nullopt.
 * Mirrors Java new Line(Point, Direction).
 */
[[maybe_unused]] std::optional<gp::Line> line_through_point(const gp::IntPoint& p_point,
                                           const gp::Direction& p_dir)
{
    if (const auto* int_dir = dynamic_cast<const gp::IntDirection*>(&p_dir))
    {
        return gp::Line::get_instance(p_point, *int_dir);
    }
    std::unique_ptr<gp::Vector> vec = p_dir.get_vector();
    std::unique_ptr<gp::Point> second = vec->add_to(p_point);
    if (const auto* second_int = dynamic_cast<const gp::IntPoint*>(second.get()))
    {
        if (*second_int == p_point)
        {
            return std::nullopt;
        }
        return gp::Line(p_point, *second_int);
    }
    return std::nullopt;
}

bool same_polyline(const gp::Polyline& p_first, const gp::Polyline& p_second)
{
    if (p_first.arr.size() != p_second.arr.size())
    {
        return false;
    }
    for (size_t i = 0; i < p_first.arr.size(); ++i)
    {
        if (!p_first.arr[i].equals(p_second.arr[i]))
        {
            return false;
        }
    }
    return true;
}

} // namespace

PullTightAlgo45::PullTightAlgo45(RoutingBoard* p_board,
                                 const std::vector<int>& p_only_net_no_arr,
                                 datastructures::Stoppable* p_stoppable_thread,
                                 int p_time_limit,
                                 const gp::Point* p_keep_point,
                                 int p_keep_point_layer)
    : PullTightAlgo(p_board, p_only_net_no_arr, p_stoppable_thread, p_time_limit, p_keep_point,
                    p_keep_point_layer)
{
}

gp::Polyline PullTightAlgo45::pull_tight(gp::Polyline p_polyline)
{
    gp::Polyline new_result = avoid_acid_traps(p_polyline);
    gp::Polyline prev_result;
    bool first_iteration = true;
    while ((first_iteration || !same_polyline(new_result, prev_result)) && !is_stop_requested())
    {
        first_iteration = false;
        prev_result = new_result;
        const gp::Polyline tmp1 = reduce_corners(prev_result);
        const gp::Polyline tmp2 = smoothen_corners(tmp1);
        new_result = reposition_lines(tmp2);
    }
    return new_result;
}

gp::Polyline PullTightAlgo45::reduce_corners(const gp::Polyline& p_polyline)
{
    if (p_polyline.corner_count() <= 3)
    {
        return p_polyline;
    }
    std::array<gp::IntPoint, 4> curr_corner;
    for (int i = 0; i < 4; ++i)
    {
        curr_corner[i] = p_polyline.corner(i);
    }
    std::array<bool, 4> curr_corner_in_clip_shape;
    for (int i = 0; i < 4; ++i)
    {
        curr_corner_in_clip_shape[i] =
            curr_clip_shape_ == nullptr || !curr_clip_shape_->is_outside(curr_corner[i]);
    }

    bool polyline_changed = false;
    std::vector<gp::IntPoint> new_corners;
    new_corners.reserve(p_polyline.arr.size() > 3 ? p_polyline.arr.size() - 3 : 0);
    new_corners.push_back(curr_corner[0]);
    std::array<gp::IntPoint, 2> curr_check_points;
    gp::IntPoint new_corner(0, 0);
    int corner_no = 3;
    while (corner_no < static_cast<int>(p_polyline.arr.size()) - 1)
    {
        bool corner_removed = false;
        curr_corner[3] = p_polyline.corner(corner_no);
        if (curr_corner[1] == curr_corner[2]
            || (corner_no < static_cast<int>(p_polyline.arr.size()) - 2
                && curr_corner[3].side_of(curr_corner[1], curr_corner[2])
                    == gp::Side::COLLINEAR))
        {
            // corners in the middle of a line can be skipped
            ++corner_no;
            curr_corner[2] = curr_corner[3];
            curr_corner_in_clip_shape[2] = curr_corner_in_clip_shape[3];
            if (corner_no < static_cast<int>(p_polyline.arr.size()) - 1)
            {
                curr_corner[3] = p_polyline.corner(corner_no);
            }
            polyline_changed = true;
        }
        curr_corner_in_clip_shape[3] =
            curr_clip_shape_ == nullptr || !curr_clip_shape_->is_outside(curr_corner[3]);
        if (curr_corner_in_clip_shape[1] && curr_corner_in_clip_shape[2]
            && curr_corner_in_clip_shape[3])
        {
            // translate the line from curr_corner[2] to curr_corner[1] to curr_corner[3]
            std::unique_ptr<gp::Vector> delta = curr_corner[3].difference_by(curr_corner[2]);
            std::unique_ptr<gp::Point> moved = curr_corner[1].translate_by(*delta);
            new_corner = to_int_point(*moved);
            if (curr_corner[3] == curr_corner[2])
            {
                // just remove multiple corner
                corner_removed = true;
            }
            else if (new_corner.side_of(curr_corner[0], curr_corner[1])
                     == gp::Side::COLLINEAR)
            {
                curr_check_points[0] = new_corner;
                curr_check_points[1] = curr_corner[1];
                gp::Polyline check_polyline = gp::Polyline::from_corners(
                    {curr_check_points[0], curr_check_points[1]});
                if (check_polyline.arr.size() == 3)
                {
                    std::unique_ptr<gp::TileShape> shape_to_check =
                        check_polyline.offset_shape(curr_half_width_, 0);
                    if (shape_to_check != nullptr
                        && board_->check_trace_shape(*shape_to_check, curr_layer_,
                                                     curr_net_no_arr_, curr_cl_type_, contact_pin_set()))
                    {
                        curr_check_points[1] = curr_corner[3];
                        if (curr_check_points[0] == curr_check_points[1])
                        {
                            corner_removed = true;
                        }
                        else
                        {
                            check_polyline = gp::Polyline::from_corners(
                                {curr_check_points[0], curr_check_points[1]});
                            if (check_polyline.arr.size() == 3)
                            {
                                shape_to_check =
                                    check_polyline.offset_shape(curr_half_width_, 0);
                                corner_removed = shape_to_check != nullptr
                                    && board_->check_trace_shape(*shape_to_check, curr_layer_,
                                                                 curr_net_no_arr_, curr_cl_type_,
                                                                 contact_pin_set());
                            }
                            else
                            {
                                corner_removed = true;
                            }
                        }
                    }
                }
                else
                {
                    corner_removed = true;
                }
            }
        }
        if (!corner_removed && curr_corner_in_clip_shape[0] && curr_corner_in_clip_shape[1]
            && curr_corner_in_clip_shape[2])
        {
            // the first try has failed. Try to translate the line from
            // corner_2 to corner_1 to corner_0
            std::unique_ptr<gp::Vector> delta = curr_corner[0].difference_by(curr_corner[1]);
            std::unique_ptr<gp::Point> moved = curr_corner[2].translate_by(*delta);
            new_corner = to_int_point(*moved);
            if (curr_corner[0] == curr_corner[1])
            {
                // just remove multiple corner
                corner_removed = true;
            }
            else if (new_corner.side_of(curr_corner[2], curr_corner[3])
                     == gp::Side::COLLINEAR)
            {
                curr_check_points[0] = new_corner;
                curr_check_points[1] = curr_corner[0];
                gp::Polyline check_polyline = gp::Polyline::from_corners(
                    {curr_check_points[0], curr_check_points[1]});
                if (check_polyline.arr.size() == 3)
                {
                    std::unique_ptr<gp::TileShape> shape_to_check =
                        check_polyline.offset_shape(curr_half_width_, 0);
                    if (shape_to_check != nullptr
                        && board_->check_trace_shape(*shape_to_check, curr_layer_,
                                                     curr_net_no_arr_, curr_cl_type_, contact_pin_set()))
                    {
                        curr_check_points[1] = curr_corner[2];
                        check_polyline = gp::Polyline::from_corners(
                            {curr_check_points[0], curr_check_points[1]});
                        if (check_polyline.arr.size() == 3)
                        {
                            shape_to_check =
                                check_polyline.offset_shape(curr_half_width_, 0);
                            corner_removed = shape_to_check != nullptr
                                && board_->check_trace_shape(*shape_to_check, curr_layer_,
                                                             curr_net_no_arr_, curr_cl_type_,
                                                             contact_pin_set());
                        }
                        else
                        {
                            corner_removed = true;
                        }
                    }
                }
                else
                {
                    corner_removed = true;
                }
            }
        }
        if (corner_removed)
        {
            polyline_changed = true;
            curr_corner[1] = new_corner;
            curr_corner_in_clip_shape[1] = curr_clip_shape_ == nullptr
                || !curr_clip_shape_->is_outside(curr_corner[1]);
            if (board_->changed_area() != nullptr)
            {
                board_->join_changed_area(new_corner.to_float(), curr_layer_);
                board_->join_changed_area(curr_corner[1].to_float(), curr_layer_);
                board_->join_changed_area(curr_corner[2].to_float(), curr_layer_);
            }
        }
        else
        {
            new_corners.push_back(curr_corner[1]);
            curr_corner[0] = curr_corner[1];
            curr_corner[1] = curr_corner[2];
            curr_corner_in_clip_shape[0] = curr_corner_in_clip_shape[1];
            curr_corner_in_clip_shape[1] = curr_corner_in_clip_shape[2];
        }
        curr_corner[2] = curr_corner[3];
        curr_corner_in_clip_shape[2] = curr_corner_in_clip_shape[3];
        ++corner_no;
    }
    if (!polyline_changed)
    {
        return p_polyline;
    }
    new_corners.push_back(curr_corner[1]);
    new_corners.push_back(curr_corner[2]);
    return gp::Polyline::from_corners(new_corners);
}

gp::Polyline PullTightAlgo45::smoothen_corners(const gp::Polyline& p_polyline)
{
    gp::Polyline result = p_polyline;
    bool polyline_changed = true;
    while (polyline_changed)
    {
        if (result.arr.size() < 4)
        {
            return result;
        }
        polyline_changed = false;
        std::vector<gp::Line> line_arr = result.arr;

        for (int i = 1; i < static_cast<int>(line_arr.size()) - 2; ++i)
        {
            std::unique_ptr<gp::Direction> d1 =
                gp::Direction::get_instance(line_arr[i].direction());
            std::unique_ptr<gp::Direction> d2 =
                gp::Direction::get_instance(line_arr[i + 1].direction());
            if (d1->is_multiple_of_45_degree() && d2->is_multiple_of_45_degree()
                && d1->projection(*d2) != datastructures::Signum::POSITIVE)
            {
                // there is a 90 degree or sharper angle
                std::optional<gp::Line> new_line = smoothen_corner(line_arr, i);
                if (!new_line.has_value())
                {
                    // the greedy smoothening couldn't change the polyline
                    new_line = smoothen_sharp_corner(line_arr, i);
                }
                if (new_line.has_value())
                {
                    polyline_changed = true;
                    // add the new line into the line array
                    line_arr.insert(line_arr.begin() + i + 1, *new_line);
                    ++i;
                }
            }
        }
        if (polyline_changed)
        {
            result = gp::Polyline(line_arr);
        }
    }
    return result;
}

std::optional<gp::Line> PullTightAlgo45::smoothen_sharp_corner(
    const std::vector<gp::Line>& p_line_arr, int p_no)
{
    gp::FloatPoint curr_corner = p_line_arr[p_no].intersection_approx(p_line_arr[p_no + 1]);
    if (curr_corner.x != static_cast<int>(curr_corner.x))
    {
        // intersection of 2 diagonal lines is not integer
        std::optional<gp::Line> result = smoothen_non_integer_corner(p_line_arr, p_no);
        if (result.has_value())
        {
            return result;
        }
    }
    const gp::FloatPoint prev_corner =
        p_line_arr[p_no].intersection_approx(p_line_arr[p_no - 1]);
    const gp::FloatPoint next_corner =
        p_line_arr[p_no + 1].intersection_approx(p_line_arr[p_no + 2]);

    std::unique_ptr<gp::Direction> prev_dir =
        gp::Direction::get_instance(p_line_arr[p_no].direction());
    std::unique_ptr<gp::Direction> next_dir =
        gp::Direction::get_instance(p_line_arr[p_no + 1].direction());
    std::unique_ptr<gp::Vector> dir_sum = prev_dir->get_vector()->add(*next_dir->get_vector());
    std::unique_ptr<gp::Direction> new_line_dir = gp::Direction::get_instance(*dir_sum);
    std::optional<gp::Line> translate_line =
        line_through_point(curr_corner.round(), *new_line_dir);
    if (!translate_line.has_value())
    {
        return std::nullopt;
    }
    double translate_dist = (gp::Limits::SQRT2 - 1.0) * curr_half_width_;
    const double prev_dist = std::abs(translate_line->signed_distance(prev_corner));
    const double next_dist = std::abs(translate_line->signed_distance(next_corner));
    translate_dist = std::min(translate_dist, prev_dist);
    translate_dist = std::min(translate_dist, next_dist);
    if (translate_dist < 0.99)
    {
        return std::nullopt;
    }
    translate_dist = std::max(translate_dist - 1.0, 1.0);
    if (translate_line->side_of(next_corner) == gp::Side::ON_THE_LEFT)
    {
        translate_dist = -translate_dist;
    }
    gp::Line result = translate_line->translate(translate_dist);
    if (board_->changed_area() != nullptr)
    {
        board_->join_changed_area(curr_corner, curr_layer_);
    }
    return result;
}

std::optional<gp::Line> PullTightAlgo45::smoothen_non_integer_corner(
    const std::vector<gp::Line>& p_line_arr, int p_no)
{
    const gp::Line& prev_line = p_line_arr[p_no];
    const gp::Line& next_line = p_line_arr[p_no + 1];
    if (prev_line.is_equal_or_opposite(next_line))
    {
        return std::nullopt;
    }
    if (!(prev_line.direction().is_diagonal() && next_line.direction().is_diagonal()))
    {
        return std::nullopt;
    }
    const gp::FloatPoint curr_corner = prev_line.intersection_approx(next_line);
    const gp::FloatPoint prev_corner = prev_line.intersection_approx(p_line_arr[p_no - 1]);
    const gp::FloatPoint next_corner = next_line.intersection_approx(p_line_arr[p_no + 2]);
    int new_x = 0;
    int new_y = 0;
    bool new_line_is_vertical = false;
    bool new_line_is_horizontal = false;
    if (prev_corner.x > curr_corner.x && next_corner.x > curr_corner.x)
    {
        new_x = static_cast<int>(std::ceil(curr_corner.x));
        new_y = static_cast<int>(std::ceil(curr_corner.y));
        new_line_is_vertical = true;
    }
    else if (prev_corner.x < curr_corner.x && next_corner.x < curr_corner.x)
    {
        new_x = static_cast<int>(std::floor(curr_corner.x));
        new_y = static_cast<int>(std::floor(curr_corner.y));
        new_line_is_vertical = true;
    }
    else if (prev_corner.y > curr_corner.y && next_corner.y > curr_corner.y)
    {
        new_x = static_cast<int>(std::ceil(curr_corner.x));
        new_y = static_cast<int>(std::ceil(curr_corner.y));
        new_line_is_horizontal = true;
    }
    else if (prev_corner.y < curr_corner.y && next_corner.y < curr_corner.y)
    {
        new_x = static_cast<int>(std::floor(curr_corner.x));
        new_y = static_cast<int>(std::floor(curr_corner.y));
        new_line_is_horizontal = true;
    }
    const gp::IntDirection* new_line_dir = nullptr;
    if (new_line_is_vertical)
    {
        new_line_dir =
            (prev_corner.y < next_corner.y) ? &gp::IntDirection::UP : &gp::IntDirection::DOWN;
    }
    else if (new_line_is_horizontal)
    {
        new_line_dir =
            (prev_corner.x < next_corner.x) ? &gp::IntDirection::RIGHT : &gp::IntDirection::LEFT;
    }
    else
    {
        return std::nullopt;
    }

    return gp::Line::get_instance(gp::IntPoint(new_x, new_y), *new_line_dir);
}

std::optional<gp::Line> PullTightAlgo45::smoothen_corner(
    const std::vector<gp::Line>& p_line_arr, int p_no)
{
    const gp::FloatPoint prev_corner =
        p_line_arr[p_no].intersection_approx(p_line_arr[p_no - 1]);
    const gp::FloatPoint curr_corner =
        p_line_arr[p_no].intersection_approx(p_line_arr[p_no + 1]);
    const gp::FloatPoint next_corner =
        p_line_arr[p_no + 1].intersection_approx(p_line_arr[p_no + 2]);

    std::unique_ptr<gp::Direction> prev_dir =
        gp::Direction::get_instance(p_line_arr[p_no].direction());
    std::unique_ptr<gp::Direction> next_dir =
        gp::Direction::get_instance(p_line_arr[p_no + 1].direction());
    std::unique_ptr<gp::Vector> dir_sum = prev_dir->get_vector()->add(*next_dir->get_vector());
    std::unique_ptr<gp::Direction> new_line_dir = gp::Direction::get_instance(*dir_sum);
    std::optional<gp::Line> translate_line =
        line_through_point(curr_corner.round(), *new_line_dir);
    if (!translate_line.has_value())
    {
        return std::nullopt;
    }
    const double prev_dist = std::abs(translate_line->signed_distance(prev_corner));
    const double next_dist = std::abs(translate_line->signed_distance(next_corner));
    if (prev_dist == 0.0 || next_dist == 0.0)
    {
        return std::nullopt;
    }
    double max_translate_dist = 0.0;
    gp::FloatPoint nearest_corner;
    if (prev_dist <= next_dist)
    {
        max_translate_dist = prev_dist;
        nearest_corner = prev_corner;
    }
    else
    {
        max_translate_dist = next_dist;
        nearest_corner = next_corner;
    }
    if (max_translate_dist < 1.0)
    {
        return std::nullopt;
    }
    max_translate_dist = std::max(max_translate_dist - 1.0, 1.0);
    if (translate_line->side_of(next_corner) == gp::Side::ON_THE_LEFT)
    {
        max_translate_dist = -max_translate_dist;
    }
    const gp::Line& first_check = p_line_arr[p_no];
    const gp::Line& third_check = p_line_arr[p_no + 1];
    double translate_dist = max_translate_dist;
    double delta_dist = max_translate_dist;
    const gp::Side side_of_nearest_corner = translate_line->side_of(nearest_corner);
    const int sign = datastructures::Signum::as_int(max_translate_dist);
    std::optional<gp::Line> result;
    while (std::abs(delta_dist) > min_translate_dist_)
    {
        bool check_ok = false;
        gp::Line new_line = translate_line->translate(translate_dist);
        const gp::Side new_line_side_of_nearest_corner = new_line.side_of(nearest_corner);
        if (new_line_side_of_nearest_corner == side_of_nearest_corner
            || new_line_side_of_nearest_corner == gp::Side::COLLINEAR)
        {
            const gp::Polyline tmp({first_check, new_line, third_check});

            if (tmp.arr.size() == 3)
            {
                std::unique_ptr<gp::TileShape> shape_to_check =
                    tmp.offset_shape(curr_half_width_, 0);
                if (shape_to_check != nullptr)
                {
                    check_ok = board_->check_trace_shape(*shape_to_check, curr_layer_,
                                                         curr_net_no_arr_, curr_cl_type_,
                                                         contact_pin_set());
                }
            }
            delta_dist /= 2.0;
            if (check_ok)
            {
                result = new_line;
                if (translate_dist == max_translate_dist)
                {
                    // biggest possible change
                    break;
                }
                translate_dist += delta_dist;
            }
            else
            {
                translate_dist -= delta_dist;
            }
        }
        else
        // moved a little bit to far at the first time
        // because of numerical inaccuracy
        {
            const double shorten_value = sign * 0.5;
            max_translate_dist -= shorten_value;
            translate_dist -= shorten_value;
            delta_dist -= shorten_value;
        }
    }
    if (result.has_value() && board_->changed_area() != nullptr)
    {
        board_->join_changed_area(first_check.intersection_approx(*result), curr_layer_);
        board_->join_changed_area(third_check.intersection_approx(*result), curr_layer_);
        board_->join_changed_area(curr_corner, curr_layer_);
    }
    return result;
}

std::unique_ptr<gp::Polyline> PullTightAlgo45::smoothen_start_corner_at_trace(
    PolylineTrace* p_trace)
{
    const gp::Polyline& trace_polyline = p_trace->polyline();
    const gp::IntPoint curr_end_corner = trace_polyline.corner(0);

    if (curr_clip_shape_ != nullptr && curr_clip_shape_->is_outside(curr_end_corner))
    {
        return nullptr;
    }

    const gp::IntPoint curr_prev_end_corner = trace_polyline.corner(1);
    gp::Side prev_corner_side = gp::Side::NO_SIDE;
    std::unique_ptr<gp::Direction> line_direction =
        gp::Direction::get_instance(trace_polyline.arr[1].direction());
    std::unique_ptr<gp::Direction> prev_line_direction =
        gp::Direction::get_instance(trace_polyline.arr[2].direction());

    bool acute_angle = false;
    bool bend = false;
    gp::FloatPoint other_trace_corner_approx;
    bool have_other_trace_corner = false;
    std::optional<gp::Line> other_trace_line;
    std::optional<gp::Line> other_prev_trace_line;

    ItemSet contact_list = p_trace->get_start_contacts();
    for (Item* curr_contact : contact_list)
    {
        auto* trace = dynamic_cast<PolylineTrace*>(curr_contact);
        if (trace == nullptr || curr_contact->is_shove_fixed())
        {
            return nullptr;
        }
        const gp::Polyline& contact_polyline = trace->polyline();
        gp::FloatPoint curr_other_corner;
        gp::Line curr_other_line = contact_polyline.arr[1];
        gp::Line curr_other_prev_line = contact_polyline.arr[2];
        if (contact_polyline.first_corner() == curr_end_corner)
        {
            curr_other_corner = contact_polyline.corner_approx(1);
        }
        else
        {
            const int curr_corner_no = contact_polyline.corner_count() - 2;
            curr_other_corner = contact_polyline.corner_approx(curr_corner_no);
            curr_other_line = contact_polyline.arr[curr_corner_no + 1].opposite();
            curr_other_prev_line = contact_polyline.arr[curr_corner_no];
        }
        const gp::Side curr_prev_corner_side =
            curr_other_line.side_of(curr_prev_end_corner);
        std::unique_ptr<gp::Direction> curr_other_dir =
            gp::Direction::get_instance(curr_other_line.direction());
        const datastructures::Signum curr_projection =
            line_direction->projection(*curr_other_dir);
        bool other_trace_found = false;
        if (curr_projection == datastructures::Signum::POSITIVE
            && curr_prev_corner_side != gp::Side::COLLINEAR)
        {
            if (curr_other_dir->is_orthogonal())
            {
                acute_angle = true;
                other_trace_found = true;
            }
        }
        else if (curr_projection == datastructures::Signum::ZERO
                 && trace_polyline.corner_count() > 2)
        {
            if (prev_line_direction->projection(*curr_other_dir)
                == datastructures::Signum::POSITIVE)
            {
                bend = true;
                other_trace_found = true;
            }
        }
        if (other_trace_found)
        {
            other_trace_corner_approx = curr_other_corner;
            have_other_trace_corner = true;
            other_trace_line = curr_other_line;
            prev_corner_side = curr_prev_corner_side;
            other_prev_trace_line = curr_other_prev_line;
        }
    }

    if (acute_angle && other_trace_line.has_value() && have_other_trace_corner)
    {
        std::unique_ptr<gp::Direction> other_dir =
            gp::Direction::get_instance(other_trace_line->direction());
        std::unique_ptr<gp::Direction> new_line_dir;
        if (prev_corner_side == gp::Side::ON_THE_LEFT)
        {
            new_line_dir = other_dir->turn_45_degree(2);
        }
        else
        {
            new_line_dir = other_dir->turn_45_degree(6);
        }
        std::optional<gp::Line> translate_line =
            line_through_point(curr_end_corner, *new_line_dir);
        if (!translate_line.has_value())
        {
            return nullptr;
        }
        double translate_dist = (gp::Limits::SQRT2 - 1.0) * curr_half_width_;
        const double prev_corner_dist =
            std::abs(translate_line->signed_distance(curr_prev_end_corner.to_float()));
        const double other_dist =
            std::abs(translate_line->signed_distance(other_trace_corner_approx));
        translate_dist = std::min(translate_dist, prev_corner_dist);
        translate_dist = std::min(translate_dist, other_dist);
        if (translate_dist >= 0.99)
        {
            translate_dist = std::max(translate_dist - 1.0, 1.0);
            if (translate_line->side_of(curr_prev_end_corner) == gp::Side::ON_THE_LEFT)
            {
                translate_dist = -translate_dist;
            }
            const gp::Line add_line = translate_line->translate(translate_dist);
            // construct the new trace polyline.
            std::vector<gp::Line> new_lines;
            new_lines.reserve(trace_polyline.arr.size() + 1);
            new_lines.push_back(*other_trace_line);
            new_lines.push_back(add_line);
            for (size_t i = 1; i < trace_polyline.arr.size(); ++i)
            {
                new_lines.push_back(trace_polyline.arr[i]);
            }
            return std::make_unique<gp::Polyline>(new_lines);
        }
    }
    else if (bend && other_trace_line.has_value() && other_prev_trace_line.has_value())
    {
        std::vector<gp::Line> check_line_arr;
        check_line_arr.reserve(trace_polyline.arr.size() + 1);
        check_line_arr.push_back(*other_prev_trace_line);
        check_line_arr.push_back(*other_trace_line);
        for (size_t i = 1; i < trace_polyline.arr.size(); ++i)
        {
            check_line_arr.push_back(trace_polyline.arr[i]);
        }
        std::optional<gp::Line> new_line = reposition_line(check_line_arr, 2);
        if (new_line.has_value())
        {
            std::vector<gp::Line> new_lines;
            new_lines.reserve(trace_polyline.arr.size());
            new_lines.push_back(*other_trace_line);
            new_lines.push_back(*new_line);
            for (size_t i = 2; i < trace_polyline.arr.size(); ++i)
            {
                new_lines.push_back(trace_polyline.arr[i]);
            }
            return std::make_unique<gp::Polyline>(new_lines);
        }
    }
    return nullptr;
}

std::unique_ptr<gp::Polyline> PullTightAlgo45::smoothen_end_corner_at_trace(
    PolylineTrace* p_trace)
{
    const gp::Polyline& trace_polyline = p_trace->polyline();
    const gp::IntPoint curr_end_corner = trace_polyline.last_corner();

    if (curr_clip_shape_ != nullptr && curr_clip_shape_->is_outside(curr_end_corner))
    {
        return nullptr;
    }

    const gp::IntPoint curr_prev_end_corner =
        trace_polyline.corner(trace_polyline.corner_count() - 2);
    gp::Side prev_corner_side = gp::Side::NO_SIDE;
    std::unique_ptr<gp::Direction> line_direction = gp::Direction::get_instance(
        trace_polyline.arr[trace_polyline.arr.size() - 2].direction());
    line_direction = line_direction->opposite();
    std::unique_ptr<gp::Direction> prev_line_direction = gp::Direction::get_instance(
        trace_polyline.arr[trace_polyline.arr.size() - 3].direction());
    prev_line_direction = prev_line_direction->opposite();

    bool acute_angle = false;
    bool bend = false;
    gp::FloatPoint other_trace_corner_approx;
    bool have_other_trace_corner = false;
    std::optional<gp::Line> other_trace_line;
    std::optional<gp::Line> other_prev_trace_line;

    ItemSet contact_list = p_trace->get_end_contacts();
    for (Item* curr_contact : contact_list)
    {
        auto* trace = dynamic_cast<PolylineTrace*>(curr_contact);
        if (trace == nullptr || curr_contact->is_shove_fixed())
        {
            return nullptr;
        }
        const gp::Polyline& contact_polyline = trace->polyline();
        gp::FloatPoint curr_other_corner;
        gp::Line curr_other_line = contact_polyline.arr[1];
        gp::Line curr_other_prev_line = contact_polyline.arr[2];
        if (contact_polyline.first_corner() == curr_end_corner)
        {
            curr_other_corner = contact_polyline.corner_approx(1);
        }
        else
        {
            const int curr_corner_no = contact_polyline.corner_count() - 2;
            curr_other_corner = contact_polyline.corner_approx(curr_corner_no);
            curr_other_line = contact_polyline.arr[curr_corner_no + 1].opposite();
            curr_other_prev_line = contact_polyline.arr[curr_corner_no];
        }
        const gp::Side curr_prev_corner_side =
            curr_other_line.side_of(curr_prev_end_corner);
        std::unique_ptr<gp::Direction> curr_other_dir =
            gp::Direction::get_instance(curr_other_line.direction());
        const datastructures::Signum curr_projection =
            line_direction->projection(*curr_other_dir);
        bool other_trace_found = false;
        if (curr_projection == datastructures::Signum::POSITIVE
            && curr_prev_corner_side != gp::Side::COLLINEAR)
        {
            if (curr_other_dir->is_orthogonal())
            {
                acute_angle = true;
                other_trace_found = true;
            }
        }
        else if (curr_projection == datastructures::Signum::ZERO
                 && trace_polyline.corner_count() > 2)
        {
            if (prev_line_direction->projection(*curr_other_dir)
                == datastructures::Signum::POSITIVE)
            {
                bend = true;
                other_trace_found = true;
            }
        }
        if (other_trace_found)
        {
            other_trace_corner_approx = curr_other_corner;
            have_other_trace_corner = true;
            other_trace_line = curr_other_line;
            prev_corner_side = curr_prev_corner_side;
            other_prev_trace_line = curr_other_prev_line;
        }
    }

    if (acute_angle && other_trace_line.has_value() && have_other_trace_corner)
    {
        std::unique_ptr<gp::Direction> other_dir =
            gp::Direction::get_instance(other_trace_line->direction());
        std::unique_ptr<gp::Direction> new_line_dir;
        if (prev_corner_side == gp::Side::ON_THE_LEFT)
        {
            new_line_dir = other_dir->turn_45_degree(6);
        }
        else
        {
            new_line_dir = other_dir->turn_45_degree(2);
        }
        std::optional<gp::Line> translate_line =
            line_through_point(curr_end_corner, *new_line_dir);
        if (!translate_line.has_value())
        {
            return nullptr;
        }
        double translate_dist = (gp::Limits::SQRT2 - 1.0) * curr_half_width_;
        const double prev_corner_dist =
            std::abs(translate_line->signed_distance(curr_prev_end_corner.to_float()));
        const double other_dist =
            std::abs(translate_line->signed_distance(other_trace_corner_approx));
        translate_dist = std::min(translate_dist, prev_corner_dist);
        translate_dist = std::min(translate_dist, other_dist);
        if (translate_dist >= 0.99)
        {
            translate_dist = std::max(translate_dist - 1.0, 1.0);
            if (translate_line->side_of(curr_prev_end_corner) == gp::Side::ON_THE_LEFT)
            {
                translate_dist = -translate_dist;
            }
            const gp::Line add_line = translate_line->translate(translate_dist);
            // construct the new trace polyline.
            std::vector<gp::Line> new_lines;
            new_lines.reserve(trace_polyline.arr.size() + 1);
            for (size_t i = 0; i + 1 < trace_polyline.arr.size(); ++i)
            {
                new_lines.push_back(trace_polyline.arr[i]);
            }
            new_lines.push_back(add_line);
            new_lines.push_back(*other_trace_line);
            return std::make_unique<gp::Polyline>(new_lines);
        }
    }
    else if (bend && other_trace_line.has_value() && other_prev_trace_line.has_value())
    {
        std::vector<gp::Line> check_line_arr;
        check_line_arr.reserve(trace_polyline.arr.size() + 1);
        for (size_t i = 0; i + 1 < trace_polyline.arr.size(); ++i)
        {
            check_line_arr.push_back(trace_polyline.arr[i]);
        }
        check_line_arr.push_back(*other_trace_line);
        check_line_arr.push_back(*other_prev_trace_line);
        std::optional<gp::Line> new_line = reposition_line(
            check_line_arr, static_cast<int>(trace_polyline.arr.size()) - 2);
        if (new_line.has_value())
        {
            std::vector<gp::Line> new_lines;
            new_lines.reserve(trace_polyline.arr.size());
            for (size_t i = 0; i + 2 < trace_polyline.arr.size(); ++i)
            {
                new_lines.push_back(trace_polyline.arr[i]);
            }
            new_lines.push_back(*new_line);
            new_lines.push_back(*other_trace_line);
            return std::make_unique<gp::Polyline>(new_lines);
        }
    }
    return nullptr;
}

} // namespace freerouting::board
