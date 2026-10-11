#include "board/PullTightAlgoAnyAngle.h"

#include <algorithm>
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

PullTightAlgoAnyAngle::PullTightAlgoAnyAngle(RoutingBoard* p_board,
                                             const std::vector<int>& p_only_net_no_arr,
                                             datastructures::Stoppable* p_stoppable_thread,
                                             int p_time_limit,
                                             const gp::Point* p_keep_point,
                                             int p_keep_point_layer)
    : PullTightAlgo(p_board, p_only_net_no_arr, p_stoppable_thread, p_time_limit, p_keep_point,
                    p_keep_point_layer)
{
}

gp::Polyline PullTightAlgoAnyAngle::pull_tight(gp::Polyline p_polyline)
{
    gp::Polyline new_result = avoid_acid_traps(p_polyline);
    gp::Polyline prev_result;
    bool first_iteration = true;
    while ((first_iteration || !polylines_equal(new_result, prev_result))
           && !is_stop_requested())
    {
        first_iteration = false;
        prev_result = new_result;
        gp::Polyline tmp = skip_segments_of_length_0(prev_result);
        gp::Polyline tmp0 = reduce_lines(tmp);
        gp::Polyline tmp1 = skip_lines(tmp0);

        // reduce_corners replaces the previous 2 functions in intent, because
        // with consecutive corners closer than 1 grid point reduce_corners may
        // loop with smoothen_corners because of changing directions heavily.
        // Unlike reduce_corners, the above 2 functions do not
        // introduce new directions.

        gp::Polyline tmp2 = reduce_corners(tmp1);
        gp::Polyline tmp3 = reposition_lines(tmp2);
        new_result = smoothen_corners(tmp3);
    }
    return new_result;
}

gp::Polyline PullTightAlgoAnyAngle::reduce_corners(const gp::Polyline& p_polyline)
{
    if (p_polyline.arr.size() < 4)
    {
        return p_polyline;
    }
    const int last_index = static_cast<int>(p_polyline.arr.size()) - 4;

    std::vector<gp::Line> new_lines;
    new_lines.reserve(p_polyline.arr.size());
    new_lines.push_back(p_polyline.arr[0]);
    new_lines.push_back(p_polyline.arr[1]);

    int new_line_index = 1;

    bool polyline_changed = false;

    for (int i = 0; i <= last_index; ++i)
    {
        bool skip_line = false;
        const gp::FloatPoint new_a = new_lines[new_line_index - 1].intersection_approx(
            new_lines[new_line_index]);
        const gp::FloatPoint new_b = p_polyline.corner_approx(i + 2);
        const bool in_clip_shape = curr_clip_shape_ == nullptr
            || (curr_clip_shape_->contains(new_a) && curr_clip_shape_->contains(new_b)
                && curr_clip_shape_->contains(p_polyline.corner_approx(new_line_index)));

        std::optional<gp::Line> first_line;
        std::optional<gp::Line> last_line;
        const gp::Line middle_line(new_a.round(), new_b.round());
        if (in_clip_shape)
        {
            const gp::FloatPoint skip_corner =
                new_lines[new_line_index].intersection_approx(p_polyline.arr[i + 2]);
            bool ok = true;
            if (new_line_index == 1)
            {
                // first corner is always an IntPoint in C++ and must not be changed
                std::unique_ptr<gp::Direction> dir =
                    gp::Direction::get_instance(middle_line.direction());
                std::unique_ptr<gp::Direction> turned = dir->turn_45_degree(2);
                first_line = line_through_point(p_polyline.first_corner(), *turned);
                if (!first_line.has_value())
                {
                    ok = false;
                }
            }
            else
            {
                first_line = new_lines[new_line_index - 1];
            }
            if (i == last_index)
            {
                // last corner is always an IntPoint in C++ and must not be changed
                std::unique_ptr<gp::Direction> dir =
                    gp::Direction::get_instance(middle_line.direction());
                std::unique_ptr<gp::Direction> turned = dir->turn_45_degree(2);
                last_line = line_through_point(p_polyline.last_corner(), *turned);
                if (!last_line.has_value())
                {
                    ok = false;
                }
            }
            else
            {
                last_line = p_polyline.arr[i + 3];
            }

            // check, if the intersection of the first and middle line
            // is near new_a and the intersection of the middle and last
            // line is near new_b. There may be numerical stability
            // problems with near parallel lines.
            constexpr double check_dist = 100.0;
            if (ok && first_line.has_value() && last_line.has_value())
            {
                const gp::FloatPoint check_is =
                    first_line->intersection_approx(middle_line);
                if (check_is.distance_square(new_a) > check_dist)
                {
                    ok = false;
                }
            }
            if (ok && first_line.has_value() && last_line.has_value())
            {
                const gp::FloatPoint check_is =
                    middle_line.intersection_approx(*last_line);
                if (check_is.distance_square(new_b) > check_dist)
                {
                    ok = false;
                }
            }
            // The Java instanceof-IntPoint connection guards are unreachable
            // in C++ because corners are always IntPoints.
            gp::Polyline curr_polyline;
            bool have_polyline = false;
            if (ok && first_line.has_value() && last_line.has_value())
            {
                curr_polyline =
                    gp::Polyline({*first_line, middle_line, *last_line});
                have_polyline = true;
                if (curr_polyline.arr.size() != 3)
                {
                    ok = false;
                }
                const double length_before =
                    skip_corner.distance(new_a) + skip_corner.distance(new_b);
                const double length_after = curr_polyline.length_approx() + 1.5;
                // 1.5 added because of possible inaccuracy SQRT_2
                // by twice rounding.
                if (length_after >= length_before)
                // May happen from rounding to integer.
                // Prevent infinite loop.
                {
                    ok = false;
                }
            }

            if (ok && have_polyline)
            {
                std::unique_ptr<gp::TileShape> shape_to_check =
                    curr_polyline.offset_shape(curr_half_width_, 0);
                skip_line = shape_to_check != nullptr
                    && board_->check_trace_shape(*shape_to_check, curr_layer_, curr_net_no_arr_,
                                                 curr_cl_type_, contact_pin_set());
            }
        }
        if (skip_line && first_line.has_value() && last_line.has_value())
        {
            polyline_changed = true;
            new_lines.back() = middle_line;
            if (new_line_index == 1)
            {
                // make the first line perpendicular to the current line
                new_lines[0] = *first_line;
            }
            if (i == last_index)
            {
                // make the last line perpendicular to the current line
                ++new_line_index;
                new_lines.push_back(*last_line);
            }
            if (board_->changed_area() != nullptr)
            {
                board_->join_changed_area(new_a, curr_layer_);
                board_->join_changed_area(new_b, curr_layer_);
            }
        }
        else
        {
            ++new_line_index;
            new_lines.push_back(p_polyline.arr[i + 2]);
            if (i == last_index)
            {
                ++new_line_index;
                new_lines.push_back(p_polyline.arr[i + 3]);
            }
        }
        if (new_lines[new_line_index].is_parallel(new_lines[new_line_index - 1]))
        {
            // skip line, if it is parallel to the previous one
            new_lines.pop_back();
            --new_line_index;
        }
    }
    if (!polyline_changed)
    {
        return p_polyline;
    }
    return gp::Polyline(new_lines);
}

gp::Polyline PullTightAlgoAnyAngle::smoothen_corners(const gp::Polyline& p_polyline)
{
    if (p_polyline.arr.size() < 4)
    {
        return p_polyline;
    }
    bool polyline_changed = false;
    std::vector<gp::Line> line_arr = p_polyline.arr;

    for (int i = 0; i < static_cast<int>(line_arr.size()) - 3; ++i)
    {
        std::optional<gp::Line> new_line = smoothen_corner(line_arr, i);
        if (new_line.has_value())
        {
            polyline_changed = true;
            // add the new line into the line array
            line_arr.insert(line_arr.begin() + i + 2, *new_line);
            ++i;
        }
    }
    if (!polyline_changed)
    {
        return p_polyline;
    }
    return gp::Polyline(line_arr);
}

gp::Polyline PullTightAlgoAnyAngle::reposition_lines(const gp::Polyline& p_polyline)
{
    if (p_polyline.arr.size() < 5)
    {
        return p_polyline;
    }
    bool polyline_changed = false;
    std::vector<gp::Line> line_arr = p_polyline.arr;
    for (int i = 0; i < static_cast<int>(line_arr.size()) - 4; ++i)
    {
        std::optional<gp::Line> new_line = reposition_line(line_arr, i);
        if (new_line.has_value())
        {
            polyline_changed = true;
            line_arr[i + 2] = *new_line;
            if (line_arr[i + 2].is_parallel(line_arr[i + 1])
                || line_arr[i + 2].is_parallel(line_arr[i + 3]))
            {
                // calculation of corners not possible before skipping
                // parallel lines
                break;
            }
        }
    }
    if (!polyline_changed)
    {
        return p_polyline;
    }
    return gp::Polyline(line_arr);
}

gp::Polyline PullTightAlgoAnyAngle::reduce_lines(const gp::Polyline& p_polyline)
{
    if (p_polyline.arr.size() < 6)
    {
        return p_polyline;
    }
    bool polyline_changed = false;
    std::vector<gp::Line> line_arr = p_polyline.arr;
    for (int i = 2; i < static_cast<int>(line_arr.size()) - 2; ++i)
    {
        const gp::FloatPoint prev_corner =
            line_arr[i - 2].intersection_approx(line_arr[i - 1]);
        const gp::FloatPoint next_corner =
            line_arr[i + 1].intersection_approx(line_arr[i + 2]);
        const bool in_clip_shape = curr_clip_shape_ == nullptr
            || (curr_clip_shape_->contains(prev_corner)
                && curr_clip_shape_->contains(next_corner));
        if (!in_clip_shape)
        {
            continue;
        }
        const gp::Line& translate_line = line_arr[i];
        const double prev_dist = translate_line.signed_distance(prev_corner);
        const double next_dist = translate_line.signed_distance(next_corner);
        if (datastructures::Signum::of(prev_dist) != datastructures::Signum::of(next_dist))
        // the 2 corners are on different sides of the translate_line
        {
            continue;
        }
        double translate_dist = 0.0;
        if (std::abs(prev_dist) < std::abs(next_dist))
        {
            translate_dist = prev_dist;
        }
        else
        {
            translate_dist = next_dist;
        }
        if (translate_dist == 0.0)
        {
            // line segment may have length 0
            continue;
        }
        const gp::Side line_side = translate_line.side_of(prev_corner);
        gp::Line new_line = translate_line.translate(-translate_dist);
        // make sure, we have crossed the nearest_corner;
        const int sign = datastructures::Signum::as_int(translate_dist);
        gp::Side new_line_side_of_prev_corner = new_line.side_of(prev_corner);
        gp::Side new_line_side_of_next_corner = new_line.side_of(next_corner);
        while (new_line_side_of_prev_corner == line_side
               && new_line_side_of_next_corner == line_side)
        {
            translate_dist += sign * 0.5;
            new_line = translate_line.translate(-translate_dist);
            new_line_side_of_prev_corner = new_line.side_of(prev_corner);
            new_line_side_of_next_corner = new_line.side_of(next_corner);
        }
        int crossed_corners_before_count = 0;
        int crossed_corners_after_count = 0;
        if (new_line_side_of_prev_corner != line_side)
        {
            ++crossed_corners_before_count;
        }
        if (new_line_side_of_next_corner != line_side)
        {
            ++crossed_corners_after_count;
        }
        // check, that we haven't crossed both corners
        if (crossed_corners_before_count > 1 || crossed_corners_after_count > 1)
        {
            continue;
        }
        // check, that next_nearest_corner and nearest_corner are on
        // different sides of new_line;
        if (crossed_corners_before_count > 0)
        {
            if (i < 3)
            {
                continue;
            }
            const gp::FloatPoint prev_prev_corner =
                line_arr[i - 3].intersection_approx(line_arr[i - 2]);
            if (new_line.side_of(prev_prev_corner) != line_side)
            {
                continue;
            }
        }
        if (crossed_corners_after_count > 0)
        {
            if (i >= static_cast<int>(line_arr.size()) - 3)
            {
                continue;
            }
            const gp::FloatPoint next_next_corner =
                line_arr[i + 2].intersection_approx(line_arr[i + 3]);
            if (new_line.side_of(next_next_corner) != line_side)
            {
                continue;
            }
        }
        std::vector<gp::Line> curr_lines;
        curr_lines.reserve(line_arr.size());
        const int keep_before_ind = i - crossed_corners_before_count;
        for (int k = 0; k < keep_before_ind; ++k)
        {
            curr_lines.push_back(line_arr[k]);
        }
        curr_lines.push_back(new_line);
        for (int k = i + 1 + crossed_corners_after_count;
             k < static_cast<int>(line_arr.size()); ++k)
        {
            curr_lines.push_back(line_arr[k]);
        }
        gp::Polyline tmp(curr_lines);
        bool check_ok = false;
        if (tmp.arr.size() == curr_lines.size())
        {
            std::unique_ptr<gp::TileShape> shape_to_check =
                tmp.offset_shape(curr_half_width_, keep_before_ind - 1);
            if (shape_to_check != nullptr)
            {
                check_ok = board_->check_trace_shape(*shape_to_check, curr_layer_,
                                                     curr_net_no_arr_, curr_cl_type_, contact_pin_set());
            }
        }
        if (check_ok)
        {
            if (board_->changed_area() != nullptr)
            {
                board_->join_changed_area(prev_corner, curr_layer_);
                board_->join_changed_area(next_corner, curr_layer_);
            }
            polyline_changed = true;
            line_arr = curr_lines;
            --i;
        }
    }
    if (!polyline_changed)
    {
        return p_polyline;
    }
    return gp::Polyline(line_arr);
}

std::optional<gp::Line> PullTightAlgoAnyAngle::smoothen_corner(
    const std::vector<gp::Line>& p_line_arr, int p_start_no)
{
    if (static_cast<int>(p_line_arr.size()) - p_start_no < 4)
    {
        return std::nullopt;
    }
    const gp::FloatPoint curr_corner =
        p_line_arr[p_start_no + 1].intersection_approx(p_line_arr[p_start_no + 2]);
    if (curr_clip_shape_ != nullptr && !curr_clip_shape_->contains(curr_corner))
    {
        return std::nullopt;
    }
    const double cosinus_angle = p_line_arr[p_start_no + 1].direction().cos_angle(
        p_line_arr[p_start_no + 2].direction());
    if (cosinus_angle > c_max_cos_angle)
    // lines are already nearly parallel, don't divide angle any further
    // because of problems with numerical stability
    {
        return std::nullopt;
    }
    const gp::FloatPoint prev_corner =
        p_line_arr[p_start_no].intersection_approx(p_line_arr[p_start_no + 1]);
    const gp::FloatPoint next_corner =
        p_line_arr[p_start_no + 2].intersection_approx(p_line_arr[p_start_no + 3]);

    // create a line approximately through curr_corner, whose
    // direction is about the middle of the directions of the
    // previous and the next line.
    // Translations of this line are used to cut off the corner.
    std::unique_ptr<gp::Direction> prev_dir =
        gp::Direction::get_instance(p_line_arr[p_start_no + 1].direction());
    std::unique_ptr<gp::Direction> next_dir =
        gp::Direction::get_instance(p_line_arr[p_start_no + 2].direction());
    std::unique_ptr<gp::Direction> middle_dir = prev_dir->middle_approx(*next_dir);
    std::optional<gp::Line> translate_line =
        line_through_point(curr_corner.round(), *middle_dir);
    if (!translate_line.has_value())
    {
        return std::nullopt;
    }
    const double prev_dist = translate_line->signed_distance(prev_corner);
    const double next_dist = translate_line->signed_distance(next_corner);
    gp::FloatPoint nearest_point;
    double max_translate_dist = 0.0;
    if (std::abs(prev_dist) < std::abs(next_dist))
    {
        nearest_point = prev_corner;
        max_translate_dist = prev_dist;
    }
    else
    {
        nearest_point = next_corner;
        max_translate_dist = next_dist;
    }
    if (std::abs(max_translate_dist) < 1.0)
    {
        return std::nullopt;
    }
    std::vector<gp::Line> curr_lines;
    curr_lines.reserve(p_line_arr.size() + 1);
    for (int k = 0; k < p_start_no + 2; ++k)
    {
        curr_lines.push_back(p_line_arr[k]);
    }
    for (int k = p_start_no + 2; k < static_cast<int>(p_line_arr.size()); ++k)
    {
        curr_lines.push_back(p_line_arr[k]);
    }
    double translate_dist = max_translate_dist;
    double delta_dist = max_translate_dist;
    const gp::Side side_of_nearest_point = translate_line->side_of(nearest_point);
    const int sign = datastructures::Signum::as_int(max_translate_dist);
    std::optional<gp::Line> result;
    while (std::abs(delta_dist) > min_translate_dist_)
    {
        bool check_ok = false;
        gp::Line new_line = translate_line->translate(translate_dist);
        const gp::Side new_line_side_of_nearest_point = new_line.side_of(nearest_point);
        if (new_line_side_of_nearest_point == side_of_nearest_point
            || new_line_side_of_nearest_point == gp::Side::COLLINEAR)
        {
            curr_lines[p_start_no + 2] = new_line;
            gp::Polyline tmp(curr_lines);

            if (tmp.arr.size() == curr_lines.size())
            {
                std::unique_ptr<gp::TileShape> shape_to_check =
                    tmp.offset_shape(curr_half_width_, p_start_no + 1);
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
                result = curr_lines[p_start_no + 2];
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
    if (!result.has_value())
    {
        return std::nullopt;
    }

    if (board_->changed_area() != nullptr)
    {
        const gp::FloatPoint new_prev_corner =
            curr_lines[p_start_no].intersection_approx(curr_lines[p_start_no + 1]);
        const gp::FloatPoint new_next_corner =
            curr_lines[p_start_no + 3].intersection_approx(curr_lines[p_start_no + 4]);
        board_->join_changed_area(new_prev_corner, curr_layer_);
        board_->join_changed_area(new_next_corner, curr_layer_);
    }
    return result;
}

std::optional<gp::Line> PullTightAlgoAnyAngle::reposition_line(
    const std::vector<gp::Line>& p_line_arr, int p_start_no)
{
    if (static_cast<int>(p_line_arr.size()) - p_start_no < 5)
    {
        return std::nullopt;
    }
    if (curr_clip_shape_ != nullptr)
    // check, that the corners of the line to translate are inside
    // the clip shape
    {
        for (int i = 1; i < 3; ++i)
        {
            const gp::FloatPoint curr_corner =
                p_line_arr[p_start_no + i].intersection_approx(p_line_arr[p_start_no + i + 1]);
            if (!curr_clip_shape_->contains(curr_corner))
            {
                return std::nullopt;
            }
        }
    }
    const gp::Line& translate_line = p_line_arr[p_start_no + 2];
    gp::FloatPoint prev_corner =
        p_line_arr[p_start_no].intersection_approx(p_line_arr[p_start_no + 1]);
    gp::FloatPoint next_corner =
        p_line_arr[p_start_no + 3].intersection_approx(p_line_arr[p_start_no + 4]);
    double prev_dist = translate_line.signed_distance(prev_corner);
    int corners_skipped_before = 0;
    int corners_skipped_after = 0;
    constexpr double c_epsilon = 0.001;
    while (std::abs(prev_dist) < c_epsilon)
    // move also all lines through the start corner of the line to translate
    {
        ++corners_skipped_before;
        const int curr_no = p_start_no - corners_skipped_before;
        if (curr_no < 0)
        // the first corner is on the line to translate
        {
            return std::nullopt;
        }
        prev_corner = p_line_arr[curr_no].intersection_approx(p_line_arr[curr_no + 1]);
        prev_dist = translate_line.signed_distance(prev_corner);
    }
    double next_dist = translate_line.signed_distance(next_corner);
    while (std::abs(next_dist) < c_epsilon)
    // move also all lines through the end corner of the line to translate
    {
        ++corners_skipped_after;
        const int curr_no = p_start_no + 3 + corners_skipped_after;
        if (curr_no >= static_cast<int>(p_line_arr.size()) - 2)
        // the last corner is on the line to translate
        {
            return std::nullopt;
        }
        next_corner = p_line_arr[curr_no].intersection_approx(p_line_arr[curr_no + 1]);
        next_dist = translate_line.signed_distance(next_corner);
    }
    if (datastructures::Signum::of(prev_dist) != datastructures::Signum::of(next_dist))
    // the 2 corners are at different sides of translate_line
    {
        return std::nullopt;
    }
    gp::FloatPoint nearest_point;
    double max_translate_dist = 0.0;
    if (std::abs(prev_dist) < std::abs(next_dist))
    {
        nearest_point = prev_corner;
        max_translate_dist = prev_dist;
    }
    else
    {
        nearest_point = next_corner;
        max_translate_dist = next_dist;
    }
    std::vector<gp::Line> curr_lines;
    curr_lines.reserve(p_line_arr.size());
    for (int k = 0; k < p_start_no + 2; ++k)
    {
        curr_lines.push_back(p_line_arr[k]);
    }
    for (int k = p_start_no + 3; k < static_cast<int>(p_line_arr.size()); ++k)
    {
        curr_lines.push_back(p_line_arr[k]);
    }
    double translate_dist = max_translate_dist;
    double delta_dist = max_translate_dist;
    const gp::Side side_of_nearest_point = translate_line.side_of(nearest_point);
    const int sign = datastructures::Signum::as_int(max_translate_dist);
    std::optional<gp::Line> result;
    bool first_time = true;
    while (first_time || std::abs(delta_dist) > min_translate_dist_)
    {
        bool check_ok = false;
        gp::Line new_line = translate_line.translate(-translate_dist);
        if (first_time && std::abs(translate_dist) < 1.0)
        {
            if (new_line.equals(translate_line))
            {
                // try the parallel line through the nearest_point
                const gp::IntPoint rounded_nearest_point = nearest_point.round();
                if (nearest_point.distance(rounded_nearest_point.to_float())
                    < std::abs(translate_dist))
                {
                    std::unique_ptr<gp::Direction> rounded_dir =
                        gp::Direction::get_instance(translate_line.direction());
                    std::optional<gp::Line> through_nearest =
                        line_through_point(rounded_nearest_point, *rounded_dir);
                    if (through_nearest.has_value())
                    {
                        new_line = *through_nearest;
                    }
                }
                first_time = false;
            }
            if (new_line.equals(translate_line))
            {
                return std::nullopt;
            }
        }
        const gp::Side new_line_side_of_nearest_point = new_line.side_of(nearest_point);
        if (new_line_side_of_nearest_point == side_of_nearest_point
            || new_line_side_of_nearest_point == gp::Side::COLLINEAR)
        {
            first_time = false;
            curr_lines[p_start_no + 2] = new_line;
            // corners_skipped_before > 0 or corners_skipped_after > 0
            // happens very rarely. But this handling seems to be
            // important because there are situations which no other
            // tightening function can solve. For example when 3 or more
            // consecutive corners are equal.
            gp::Line prev_translated_line = new_line;
            for (int i = 0; i < corners_skipped_before; ++i)
            // Translate the previous lines onto or past the
            // intersection of new_line with the first untranslated line.
            {
                const int prev_line_no = p_start_no + 1 - corners_skipped_before;
                const gp::FloatPoint curr_prev_corner =
                    prev_translated_line.intersection_approx(curr_lines[prev_line_no]);
                const gp::Line& curr_translate_line = p_line_arr[p_start_no + 1 - i];
                const double curr_translate_dist =
                    curr_translate_line.signed_distance(curr_prev_corner);
                prev_translated_line = curr_translate_line.translate(-curr_translate_dist);
                curr_lines[p_start_no + 1 - i] = prev_translated_line;
            }
            prev_translated_line = new_line;
            for (int i = 0; i < corners_skipped_after; ++i)
            // Translate the next lines onto or past the
            // intersection of new_line with the first untranslated line.
            {
                const int next_line_no = p_start_no + 3 + corners_skipped_after;
                const gp::FloatPoint curr_next_corner =
                    prev_translated_line.intersection_approx(curr_lines[next_line_no]);
                const gp::Line& curr_translate_line = p_line_arr[p_start_no + 3 + i];
                const double curr_translate_dist =
                    curr_translate_line.signed_distance(curr_next_corner);
                prev_translated_line = curr_translate_line.translate(-curr_translate_dist);
                curr_lines[p_start_no + 3 + i] = prev_translated_line;
            }
            gp::Polyline tmp(curr_lines);

            if (tmp.arr.size() == curr_lines.size())
            {
                std::unique_ptr<gp::TileShape> shape_to_check =
                    tmp.offset_shape(curr_half_width_, p_start_no + 1);
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
                result = curr_lines[p_start_no + 2];
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
    if (!result.has_value())
    {
        return std::nullopt;
    }

    if (board_->changed_area() != nullptr)
    {
        const gp::FloatPoint new_prev_corner =
            curr_lines[p_start_no].intersection_approx(curr_lines[p_start_no + 1]);
        const gp::FloatPoint new_next_corner =
            curr_lines[p_start_no + 3].intersection_approx(curr_lines[p_start_no + 4]);
        board_->join_changed_area(new_prev_corner, curr_layer_);
        board_->join_changed_area(new_next_corner, curr_layer_);
    }
    return result;
}

gp::Polyline PullTightAlgoAnyAngle::skip_lines(const gp::Polyline& p_polyline)
{
    for (int i = 1; i < static_cast<int>(p_polyline.arr.size()) - 3; ++i)
    {
        for (int j = 0; j <= 1; ++j)
        {
            gp::FloatPoint corner1;
            gp::FloatPoint corner2;
            gp::Line curr_line = p_polyline.arr[0];
            bool have_line = false;
            if (j == 0) // try to skip the line before the i+2-th line
            {
                curr_line = p_polyline.arr[i + 2];
                have_line = true;
                corner1 = p_polyline.corner_approx(i);
                corner2 = p_polyline.corner_approx(i - 1);
            }
            else // try to skip the line after i-th line
            {
                curr_line = p_polyline.arr[i];
                have_line = true;
                corner1 = p_polyline.corner_approx(i + 1);
                corner2 = p_polyline.corner_approx(i + 2);
            }
            if (!have_line)
            {
                continue;
            }
            const bool in_clip_shape = curr_clip_shape_ == nullptr
                || (curr_clip_shape_->contains(corner1)
                    && curr_clip_shape_->contains(corner2));
            if (!in_clip_shape)
            {
                continue;
            }

            gp::Side side1 = curr_line.side_of(corner1);
            gp::Side side2 = curr_line.side_of(corner2);
            if (side1 != side2)
            // the two corners are on different sides of the line
            {
                gp::Polyline reduced_polyline = p_polyline.skip_lines(i + 1, i + 1);
                if (reduced_polyline.arr.size() == p_polyline.arr.size() - 1)
                {
                    int shape_no = i - 1;
                    if (j == 0)
                    {
                        ++shape_no;
                    }
                    std::unique_ptr<gp::TileShape> shape_to_check =
                        reduced_polyline.offset_shape(curr_half_width_, shape_no);
                    if (shape_to_check != nullptr
                        && board_->check_trace_shape(*shape_to_check, curr_layer_,
                                                     curr_net_no_arr_, curr_cl_type_, contact_pin_set()))
                    {
                        if (board_->changed_area() != nullptr)
                        {
                            board_->join_changed_area(corner1, curr_layer_);
                            board_->join_changed_area(corner2, curr_layer_);
                        }
                        return reduced_polyline;
                    }
                }
            }
            // now try skipping 2 lines
            if (i >= static_cast<int>(p_polyline.arr.size()) - 4)
            {
                break;
            }
            gp::FloatPoint corner3;
            if (j == 1)
            {
                corner3 = p_polyline.corner_approx(i + 3);
            }
            else
            {
                corner3 = p_polyline.corner_approx(i + 1);
            }
            if (curr_clip_shape_ != nullptr && !curr_clip_shape_->contains(corner3))
            {
                continue;
            }
            if (j == 0)
            // curr_line is 1 line later than in the case skipping 1 line
            // when coming from behind
            {
                curr_line = p_polyline.arr[i + 3];
                side1 = curr_line.side_of(corner1);
                side2 = curr_line.side_of(corner2);
            }
            else
            {
                side1 = curr_line.side_of(corner3);
            }
            if (side1 != side2)
            // the two corners are on different sides of the line
            {
                gp::Polyline reduced_polyline = p_polyline.skip_lines(i + 1, i + 2);
                if (reduced_polyline.arr.size() == p_polyline.arr.size() - 2)
                {
                    int shape_no = i - 1;
                    if (j == 0)
                    {
                        ++shape_no;
                    }
                    std::unique_ptr<gp::TileShape> shape_to_check =
                        reduced_polyline.offset_shape(curr_half_width_, shape_no);
                    if (shape_to_check != nullptr
                        && board_->check_trace_shape(*shape_to_check, curr_layer_,
                                                     curr_net_no_arr_, curr_cl_type_, contact_pin_set()))
                    {
                        if (board_->changed_area() != nullptr)
                        {
                            board_->join_changed_area(corner1, curr_layer_);
                            board_->join_changed_area(corner2, curr_layer_);
                            board_->join_changed_area(corner3, curr_layer_);
                        }
                        return reduced_polyline;
                    }
                }
            }
        }
    }
    return p_polyline;
}

std::unique_ptr<gp::Polyline> PullTightAlgoAnyAngle::smoothen_start_corner_at_trace(
    PolylineTrace* p_trace)
{
    const gp::Polyline& trace_polyline = p_trace->polyline();
    const gp::IntPoint curr_end_corner = trace_polyline.corner(0);

    if (curr_clip_shape_ != nullptr && curr_clip_shape_->is_outside(curr_end_corner))
    {
        return nullptr;
    }

    gp::IntPoint curr_prev_end_corner = trace_polyline.corner(1);
    // C++ corners are always IntPoints, so a short non-integer start segment
    // as in Java skip_short_segment cannot occur; kept explicit.
    const bool skip_short_segment = false;
    int start_line_no = 1;
    if (skip_short_segment)
    {
        if (trace_polyline.corner_count() < 3)
        {
            return nullptr;
        }
        curr_prev_end_corner = trace_polyline.corner(2);
        ++start_line_no;
    }
    gp::Side prev_corner_side = gp::Side::NO_SIDE;
    std::unique_ptr<gp::Direction> line_direction =
        gp::Direction::get_instance(trace_polyline.arr[start_line_no].direction());
    std::unique_ptr<gp::Direction> prev_line_direction =
        gp::Direction::get_instance(trace_polyline.arr[start_line_no + 1].direction());

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
        if (contact_polyline.first_corner().equals(&curr_end_corner))
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
            acute_angle = true;
            other_trace_found = true;
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
    int new_line_count = static_cast<int>(trace_polyline.arr.size()) + 1;
    int diff = 1;
    if (skip_short_segment)
    {
        --new_line_count;
        --diff;
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
            new_lines.reserve(new_line_count);
            new_lines.push_back(*other_trace_line);
            new_lines.push_back(add_line);
            for (int k = 2 - diff; k < static_cast<int>(trace_polyline.arr.size()); ++k)
            {
                new_lines.push_back(trace_polyline.arr[k]);
            }
            return std::make_unique<gp::Polyline>(new_lines);
        }
    }
    else if (bend && other_trace_line.has_value() && other_prev_trace_line.has_value())
    {
        std::vector<gp::Line> check_line_arr;
        check_line_arr.reserve(new_line_count);
        check_line_arr.push_back(*other_prev_trace_line);
        check_line_arr.push_back(*other_trace_line);
        for (int k = 2 - diff; k < static_cast<int>(trace_polyline.arr.size()); ++k)
        {
            check_line_arr.push_back(trace_polyline.arr[k]);
        }
        std::optional<gp::Line> new_line = reposition_line(check_line_arr, 0);
        if (new_line.has_value())
        {
            std::vector<gp::Line> new_lines;
            new_lines.reserve(trace_polyline.arr.size());
            new_lines.push_back(*other_trace_line);
            new_lines.push_back(*new_line);
            for (size_t k = 2; k < trace_polyline.arr.size(); ++k)
            {
                new_lines.push_back(trace_polyline.arr[k]);
            }
            return std::make_unique<gp::Polyline>(new_lines);
        }
    }
    return nullptr;
}

std::unique_ptr<gp::Polyline> PullTightAlgoAnyAngle::smoothen_end_corner_at_trace(
    PolylineTrace* p_trace)
{
    const gp::Polyline& trace_polyline = p_trace->polyline();
    const gp::IntPoint curr_end_corner = trace_polyline.last_corner();

    if (curr_clip_shape_ != nullptr && curr_clip_shape_->is_outside(curr_end_corner))
    {
        return nullptr;
    }

    gp::IntPoint curr_prev_end_corner =
        trace_polyline.corner(trace_polyline.corner_count() - 2);
    // C++ corners are always IntPoints; see smoothen_start_corner_at_trace.
    const bool skip_short_segment = false;
    int end_line_no = static_cast<int>(trace_polyline.arr.size()) - 2;
    if (skip_short_segment)
    {
        if (trace_polyline.corner_count() < 3)
        {
            return nullptr;
        }
        curr_prev_end_corner = trace_polyline.corner(trace_polyline.corner_count() - 3);
        --end_line_no;
    }
    gp::Side prev_corner_side = gp::Side::NO_SIDE;
    std::unique_ptr<gp::Direction> line_direction =
        gp::Direction::get_instance(trace_polyline.arr[end_line_no].direction());
    line_direction = line_direction->opposite();
    std::unique_ptr<gp::Direction> prev_line_direction =
        gp::Direction::get_instance(trace_polyline.arr[end_line_no].direction());
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
        if (contact_polyline.corner_count() > 2)
        {
            gp::FloatPoint curr_other_corner;
            gp::Line curr_other_line = contact_polyline.arr[1];
            gp::Line curr_other_prev_line = contact_polyline.arr[2];
            if (contact_polyline.first_corner().equals(&curr_end_corner))
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
                acute_angle = true;
                other_trace_found = true;
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
    }

    int new_line_count = static_cast<int>(trace_polyline.arr.size()) + 1;
    int diff = 0;
    if (skip_short_segment)
    {
        --new_line_count;
        ++diff;
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
            new_lines.reserve(new_line_count);
            for (int k = 0; k < static_cast<int>(trace_polyline.arr.size()) - 1; ++k)
            {
                new_lines.push_back(trace_polyline.arr[k]);
            }
            new_lines.push_back(add_line);
            new_lines.push_back(*other_trace_line);
            return std::make_unique<gp::Polyline>(new_lines);
        }
    }
    else if (bend && other_trace_line.has_value() && other_prev_trace_line.has_value())
    {
        std::vector<gp::Line> check_line_arr;
        check_line_arr.reserve(new_line_count);
        for (int k = diff; k < static_cast<int>(trace_polyline.arr.size()); ++k)
        {
            check_line_arr.push_back(trace_polyline.arr[k]);
        }
        check_line_arr.push_back(*other_trace_line);
        check_line_arr.push_back(*other_prev_trace_line);
        std::optional<gp::Line> new_line =
            reposition_line(check_line_arr,
                            static_cast<int>(check_line_arr.size()) - 5);
        if (new_line.has_value())
        {
            std::vector<gp::Line> new_lines;
            new_lines.reserve(trace_polyline.arr.size());
            for (size_t k = 0; k + 2 < trace_polyline.arr.size(); ++k)
            {
                new_lines.push_back(trace_polyline.arr[k]);
            }
            new_lines.push_back(*new_line);
            new_lines.push_back(*other_trace_line);
            return std::make_unique<gp::Polyline>(new_lines);
        }
    }
    return nullptr;
}

} // namespace freerouting::board
