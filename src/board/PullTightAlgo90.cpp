#include "board/PullTightAlgo90.h"

#include "board/RoutingBoard.h"

namespace freerouting::board {

namespace {

bool same_polyline(const geometry::planar::Polyline& p_first,
                   const geometry::planar::Polyline& p_second)
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

PullTightAlgo90::PullTightAlgo90(RoutingBoard* p_board,
                                 const std::vector<int>& p_only_net_no_arr,
                                 datastructures::Stoppable* p_stoppable_thread,
                                 int p_time_limit,
                                 const geometry::planar::Point* p_keep_point,
                                 int p_keep_point_layer)
    : PullTightAlgo(p_board, p_only_net_no_arr, p_stoppable_thread, p_time_limit, p_keep_point,
                    p_keep_point_layer)
{
}

geometry::planar::Polyline PullTightAlgo90::pull_tight(geometry::planar::Polyline p_polyline)
{
    geometry::planar::Polyline new_result = avoid_acid_traps(p_polyline);
    geometry::planar::Polyline prev_result;
    bool first_iteration = true;
    while ((first_iteration || !same_polyline(new_result, prev_result))
           && !is_stop_requested())
    {
        first_iteration = false;
        prev_result = new_result;
        const geometry::planar::Polyline tmp1 = try_skip_second_corner(prev_result);
        const geometry::planar::Polyline tmp2 = try_skip_corners(tmp1);
        new_result = reposition_lines(tmp2);
    }
    return new_result;
}

geometry::planar::Polyline PullTightAlgo90::try_skip_second_corner(
    const geometry::planar::Polyline& p_polyline)
{
    if (p_polyline.arr.size() < 5)
    {
        return p_polyline;
    }
    const std::vector<geometry::planar::Line> check_lines = {p_polyline.arr[1], p_polyline.arr[0],
                                           p_polyline.arr[3], p_polyline.arr[4]};
    const geometry::planar::Polyline check_polyline(check_lines);
    if (check_polyline.arr.size() != 4
        || (curr_clip_shape_ != nullptr
            && curr_clip_shape_->is_outside(check_polyline.corner(1).to_float().round())))
    {
        return p_polyline;
    }
    for (int i = 0; i < 2; ++i)
    {
        std::unique_ptr<geometry::planar::TileShape> shape_to_check = check_polyline.offset_shape(curr_half_width_, i);
        if (shape_to_check == nullptr
            || !board_->check_trace_shape(*shape_to_check, curr_layer_, curr_net_no_arr_,
                                          curr_cl_type_, contact_pin_set()))
        {
            return p_polyline;
        }
    }
    // now the second corner can be skipped.
    std::vector<geometry::planar::Line> new_lines;
    new_lines.reserve(p_polyline.arr.size() - 1);
    new_lines.push_back(p_polyline.arr[1]);
    new_lines.push_back(p_polyline.arr[0]);
    for (size_t i = 3; i < p_polyline.arr.size(); ++i)
    {
        new_lines.push_back(p_polyline.arr[i]);
    }
    return geometry::planar::Polyline(new_lines);
}

geometry::planar::Polyline PullTightAlgo90::try_skip_corners(
    const geometry::planar::Polyline& p_polyline)
{
    std::vector<geometry::planar::Line> new_lines;
    new_lines.reserve(p_polyline.arr.size() + 1);
    new_lines.push_back(p_polyline.arr[0]);
    new_lines.push_back(p_polyline.arr[1]);
    bool polyline_changed = false;
    bool second_last_corner_skipped = false;
    for (int i = 5; i <= static_cast<int>(p_polyline.arr.size()); ++i)
    {
        bool skip_lines = false;
        const bool in_clip_shape =
            curr_clip_shape_ == nullptr
            || !curr_clip_shape_->is_outside(
                p_polyline.corner_approx(i - 3).round());
        std::vector<geometry::planar::Line> check_lines;
        check_lines.reserve(4);
        if (in_clip_shape)
        {
            check_lines.push_back(new_lines[new_lines.size() - 2]);
            check_lines.push_back(new_lines.back());
            check_lines.push_back(p_polyline.arr[i - 1]);
            if (i < static_cast<int>(p_polyline.arr.size()))
            {
                check_lines.push_back(p_polyline.arr[i]);
            }
            else
            {
                // use as concluding line the second last line
                check_lines.push_back(p_polyline.arr[i - 2]);
            }
            const geometry::planar::Polyline check_polyline(check_lines);
            skip_lines = check_polyline.arr.size() == 4
                && (curr_clip_shape_ == nullptr
                    || !curr_clip_shape_->is_outside(
                        check_polyline.corner_approx(1).round()));
            if (skip_lines)
            {
                std::unique_ptr<geometry::planar::TileShape> shape_to_check =
                    check_polyline.offset_shape(curr_half_width_, 0);
                skip_lines = shape_to_check != nullptr
                    && board_->check_trace_shape(*shape_to_check, curr_layer_, curr_net_no_arr_,
                                                 curr_cl_type_, contact_pin_set());
            }
            if (skip_lines)
            {
                std::unique_ptr<geometry::planar::TileShape> shape_to_check =
                    check_polyline.offset_shape(curr_half_width_, 1);
                skip_lines = shape_to_check != nullptr
                    && board_->check_trace_shape(*shape_to_check, curr_layer_, curr_net_no_arr_,
                                                 curr_cl_type_, contact_pin_set());
            }
        }
        if (skip_lines)
        {
            if (i == static_cast<int>(p_polyline.arr.size()))
            {
                second_last_corner_skipped = true;
            }
            if (board_->changed_area() != nullptr)
            {
                board_->join_changed_area(check_lines[1].intersection_approx(check_lines[2]),
                                          curr_layer_);
                board_->join_changed_area(
                    p_polyline.arr[i - 2].intersection_approx(p_polyline.arr[i - 3]),
                    curr_layer_);
            }
            polyline_changed = true;
            ++i;
        }
        else
        {
            new_lines.push_back(p_polyline.arr[i - 3]);
        }
    }
    if (!polyline_changed)
    {
        return p_polyline;
    }
    if (second_last_corner_skipped)
    {
        // The second last corner of p_polyline was skipped
        new_lines.push_back(p_polyline.arr[p_polyline.arr.size() - 1]);
        new_lines.push_back(p_polyline.arr[p_polyline.arr.size() - 2]);
    }
    else
    {
        for (int i = 3; i > 0; --i)
        {
            new_lines.push_back(
                p_polyline.arr[p_polyline.arr.size() - static_cast<size_t>(i)]);
        }
    }
    return geometry::planar::Polyline(new_lines);
}

std::unique_ptr<geometry::planar::Polyline> PullTightAlgo90::smoothen_start_corner_at_trace(
    PolylineTrace* p_trace)
{
    (void)p_trace;
    return nullptr;
}

std::unique_ptr<geometry::planar::Polyline> PullTightAlgo90::smoothen_end_corner_at_trace(
    PolylineTrace* p_trace)
{
    (void)p_trace;
    return nullptr;
}

} // namespace freerouting::board
