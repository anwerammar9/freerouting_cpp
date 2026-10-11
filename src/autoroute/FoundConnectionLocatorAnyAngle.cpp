#include "autoroute/FoundConnectionLocatorAnyAngle.h"

#include "autoroute/AutorouteEngine.h"
#include "autoroute/ExpansionRoom.h"
#include "autoroute/MazeSearchAlgo.h"
#include "geometry/planar/FloatLine.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/Point.h"
#include "geometry/planar/Side.h"
#include "geometry/planar/TileShape.h"
#include "logger/FRLogger.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace freerouting {
namespace autoroute {

using geometry::planar::FloatLine;
using geometry::planar::FloatPoint;
using geometry::planar::IntPoint;
using geometry::planar::Side;
using geometry::planar::TileShape;

namespace {

/** Java logs the corner calculation of these nets on trace level. */
bool is_trace_net(int p_net_no)
{
    return p_net_no == 33 || p_net_no == 66 || p_net_no == 67;
}

} // namespace

std::vector<FloatPoint> FoundConnectionLocatorAnyAngle::calculate_next_trace_corners()
{
    std::vector<FloatPoint> result;
    if (current_to_door_index_ >= current_target_door_index_)
    {
        if (current_to_door_index_ == current_target_door_index_)
        {
            if (!current_target_shape_)
            {
                logger::FRLogger::instance().warn(
                    "FoundConnectionLocatorAnyAngle::calculate_next_trace_corners: no target shape");
                return result;
            }
            std::optional<IntPoint> nearest_point_int =
                current_target_shape_->nearest_point(current_from_point_.round());
            if (!nearest_point_int)
            {
                logger::FRLogger::instance().warn(
                    "FoundConnectionLocatorAnyAngle::calculate_next_trace_corners: no nearest target point");
                return result;
            }
            FloatPoint nearest_point = nearest_point_int->to_float();
            ++current_to_door_index_;
            result.push_back(nearest_point);
        }
        return result;
    }

    double trace_halfwidth_exact = compensated_trace_half_width(current_trace_layer_);
    double trace_halfwidth_max = trace_halfwidth_exact + AutorouteEngine::TRACE_WIDTH_TOLERANCE;
    const double trace_halfwidth_middle = trace_halfwidth_exact + C_TOLERANCE;

    if (current_to_door_index_ < 0 || current_to_door_index_ >= static_cast<int>(backtrack_array_.size()))
    {
        logger::FRLogger::instance().warn(
            "FoundConnectionLocatorAnyAngle::calculate_next_trace_corners: to door index out of range");
        return result;
    }
    const BacktrackElement& current_to_info = backtrack_array_[current_to_door_index_];
    std::optional<FloatPoint> door_left_corner = calc_door_left_corner(current_to_info);
    std::optional<FloatPoint> door_right_corner = calc_door_right_corner(current_to_info);
    if (!door_left_corner || !door_right_corner)
    {
        logger::FRLogger::instance().warn(
            "FoundConnectionLocatorAnyAngle::calculate_next_trace_corners: no door corners");
        return result;
    }
    if (current_from_point_.side_of(*door_left_corner, *door_right_corner) != Side::ON_THE_RIGHT)
    {
        // the door is already crossed at this.from_point
        if (current_from_point_.scalar_product(previous_from_point_, *door_left_corner) >= 0)
        {
            // Also the left corner of the door is passed.
            // That may not be the case if the door line is crossed almost parallel.
            door_left_corner = std::nullopt;
        }
        if (current_from_point_.scalar_product(previous_from_point_, *door_right_corner) >= 0)
        {
            // Also the right corner of the door is passed.
            door_right_corner = std::nullopt;
        }
        if (!door_left_corner && !door_right_corner)
        {
            // The door is completely passed.
            ++current_to_door_index_;
            result.push_back(current_from_point_);
            return result;
        }
    }

    // Calculate the visibility range for a trace line from current_from_point
    // through the interval from left_most_visible_point to right_most_visible_point,
    // by advancing the door index as far as possible, so that still something is visible.

    bool end_of_trace = false;
    std::optional<FloatPoint> left_tangent_point;
    std::optional<FloatPoint> right_tangent_point;
    int new_door_ind = current_to_door_index_;
    int left_ind = new_door_ind;
    int right_ind = new_door_ind;
    int current_door_ind = current_to_door_index_ + 1;
    std::optional<FloatPoint> result_corner;

    // construct a maximum length straight line through the doors

    for (;;)
    {
        left_tangent_point = current_from_point_.right_tangential_point(*door_left_corner, trace_halfwidth_max);
        if (door_left_corner && !left_tangent_point)
        {
            logger::FRLogger::instance().trace(
                "FoundConnectionLocatorAnyAngle::calculate_next_trace_corners: left tangent point is null");
            left_tangent_point = door_left_corner;
        }
        right_tangent_point = current_from_point_.left_tangential_point(*door_right_corner, trace_halfwidth_max);
        if (door_right_corner && !right_tangent_point)
        {
            logger::FRLogger::instance().trace(
                "FoundConnectionLocatorAnyAngle::calculate_next_trace_corners: right tangent point is null");
            right_tangent_point = door_right_corner;
        }
        if (left_tangent_point && right_tangent_point
            && right_tangent_point->side_of(current_from_point_, *left_tangent_point) != Side::ON_THE_RIGHT)
        {
            // The gap between left_most_visible_point and right_most_visible_point is too small
            // for a trace with the current half width.

            if (!door_left_corner || !door_right_corner)
            {
                // one of the corners was already passed, so the gap check does not apply
                ++current_door_ind;
                continue;
            }
            double left_corner_distance = door_left_corner->distance(current_from_point_);
            double right_corner_distance = door_right_corner->distance(current_from_point_);
            if (left_corner_distance <= right_corner_distance)
            {
                new_door_ind = left_ind;
                result_corner = left_turn_next_corner(current_from_point_, trace_halfwidth_max, door_left_corner,
                                                      door_right_corner);
            }
            else
            {
                new_door_ind = right_ind;
                result_corner = right_turn_next_corner(current_from_point_, trace_halfwidth_max, door_right_corner,
                                                       door_left_corner);
            }
            break;
        }
        if (current_door_ind >= current_target_door_index_)
        {
            end_of_trace = true;
            break;
        }
        if (current_door_ind < 0 || current_door_ind >= static_cast<int>(backtrack_array_.size()))
        {
            logger::FRLogger::instance().warn(
                "FoundConnectionLocatorAnyAngle::calculate_next_trace_corners: next door index out of range");
            return result;
        }
        const BacktrackElement& next_to_info = backtrack_array_[current_door_ind];
        std::optional<FloatPoint> next_left_corner = calc_door_left_corner(next_to_info);
        std::optional<FloatPoint> next_right_corner = calc_door_right_corner(next_to_info);
        if (!next_left_corner || !next_right_corner)
        {
            logger::FRLogger::instance().warn(
                "FoundConnectionLocatorAnyAngle::calculate_next_trace_corners: no next door corners");
            return result;
        }
        if (current_from_point_.side_of(*next_left_corner, *next_right_corner) != Side::ON_THE_RIGHT)
        {
            // the door may be already crossed at this.from_point
            if (!door_left_corner
                && current_from_point_.scalar_product(previous_from_point_, *next_left_corner) >= 0)
            {
                // Also the left corner of the door is passed.
                // That may not be the case if the door line is crossed almost parallel.
                next_left_corner = std::nullopt;
            }
            if (!door_right_corner
                && current_from_point_.scalar_product(previous_from_point_, *next_right_corner) >= 0)
            {
                // Also the right corner of the door is passed.
                next_right_corner = std::nullopt;
            }
            if (!next_left_corner && !next_right_corner)
            {
                // The door is completely passed.
                // Should not happen because the previous door was not passed completely.
                logger::FRLogger::instance().trace(
                    "FoundConnectionLocatorAnyAngle::calculate_next_trace_corners: next door passed unexpected");
                ++current_to_door_index_;
                result.push_back(current_from_point_);
                return result;
            }
        }
        if (door_left_corner && door_right_corner)
        {
            // otherwise the following sideOf conditions may not be correct
            // even if all parameter points are defined
            if (next_left_corner->side_of(current_from_point_, *door_right_corner) == Side::ON_THE_RIGHT)
            {
                // bend to the right
                new_door_ind = right_ind + 1;
                result_corner = right_turn_next_corner(current_from_point_, trace_halfwidth_max, door_right_corner,
                                                       next_left_corner);
                break;
            }

            if (next_right_corner->side_of(current_from_point_, *door_left_corner) == Side::ON_THE_LEFT)
            {
                // bend to the left
                new_door_ind = left_ind + 1;
                result_corner = left_turn_next_corner(current_from_point_, trace_halfwidth_max, door_left_corner,
                                                      next_right_corner);
                break;
            }
        }
        bool visability_range_gets_smaller_on_the_right_side = !door_right_corner;
        if (door_right_corner && next_right_corner
            && next_right_corner->side_of(current_from_point_, *door_right_corner) != Side::ON_THE_RIGHT)
        {
            std::optional<FloatPoint> current_tangential_point =
                current_from_point_.left_tangential_point(*next_right_corner, trace_halfwidth_max);
            if (current_tangential_point)
            {
                FloatLine check_line(current_from_point_, *current_tangential_point);
                if (std::abs(check_line.segment_distance(*door_right_corner)) >= trace_halfwidth_max)
                {
                    visability_range_gets_smaller_on_the_right_side = true;
                }
            }
        }
        if (visability_range_gets_smaller_on_the_right_side)
        {
            // The visibility range gets smaller on the right side.
            door_right_corner = next_right_corner;
            right_ind = current_door_ind;
        }
        bool visability_range_gets_smaller_on_the_left_side = !door_left_corner;
        if (door_left_corner && next_left_corner
            && next_left_corner->side_of(current_from_point_, *door_left_corner) != Side::ON_THE_LEFT)
        {
            std::optional<FloatPoint> current_tangential_point =
                current_from_point_.right_tangential_point(*next_left_corner, trace_halfwidth_max);
            if (current_tangential_point)
            {
                FloatLine check_line(current_from_point_, *current_tangential_point);
                if (std::abs(check_line.segment_distance(*door_left_corner)) >= trace_halfwidth_max)
                {
                    visability_range_gets_smaller_on_the_left_side = true;
                }
            }
        }
        if (visability_range_gets_smaller_on_the_left_side)
        {
            // The visibility range gets smaller on the left side.
            door_left_corner = next_left_corner;
            left_ind = current_door_ind;
        }
        ++current_door_ind;
    }

    if (end_of_trace)
    {
        if (!current_target_shape_)
        {
            logger::FRLogger::instance().warn(
                "FoundConnectionLocatorAnyAngle::calculate_next_trace_corners: no target shape at the end of the trace");
            return result;
        }
        std::optional<IntPoint> nearest_point_int =
            current_target_shape_->nearest_point(current_from_point_.round());
        if (!nearest_point_int)
        {
            logger::FRLogger::instance().warn(
                "FoundConnectionLocatorAnyAngle::calculate_next_trace_corners: no nearest target point");
            return result;
        }
        FloatPoint nearest_point = nearest_point_int->to_float();
        result_corner = nearest_point;
        if (left_tangent_point && nearest_point.side_of(current_from_point_, *left_tangent_point) == Side::ON_THE_LEFT)
        {
            // The nearest target point is to the left of the visible range, add another corner
            new_door_ind = left_ind + 1;
            FloatPoint target_right_corner =
                current_target_shape_->corner_approx(current_target_shape_->index_of_right_most_corner(current_from_point_));
            std::optional<FloatPoint> current_corner =
                right_left_tangential_point(current_from_point_, target_right_corner, *door_left_corner,
                                            trace_halfwidth_max);
            if (current_corner)
            {
                result_corner = current_corner;
                end_of_trace = false;
            }
        }
        else if (right_tangent_point && door_right_corner
                 && nearest_point.side_of(current_from_point_, *right_tangent_point) == Side::ON_THE_RIGHT)
        {
            // The nearest target point is to the right of the visible range, add another corner
            FloatPoint target_left_corner =
                current_target_shape_->corner_approx(current_target_shape_->index_of_left_most_corner(current_from_point_));
            new_door_ind = right_ind + 1;
            std::optional<FloatPoint> current_corner =
                left_right_tangential_point(current_from_point_, target_left_corner, *door_right_corner,
                                             trace_halfwidth_max);
            if (current_corner)
            {
                result_corner = current_corner;
                end_of_trace = false;
            }
        }
    }
    if (end_of_trace)
    {
        new_door_ind = current_target_door_index_;
    }

    if (!result_corner)
    {
        logger::FRLogger::instance().warn(
            "FoundConnectionLocatorAnyAngle::calculate_next_trace_corners: no result corner");
        return result;
    }

    // Check clearance violation with the previous door shapes
    // and correct them in this case.

    FloatLine check_line(current_from_point_, *result_corner);
    int check_from_door_index = std::max(current_to_door_index_ - 5, current_from_door_index_ + 1);
    std::optional<FloatPoint> corrected_result;
    int corrected_door_ind = 0;
    for (int i = check_from_door_index; i < new_door_ind; i++)
    {
        if (i < 0 || i >= static_cast<int>(backtrack_array_.size()))
        {
            continue;
        }
        std::optional<FloatPoint> current_left_corner = calc_door_left_corner(backtrack_array_[i]);
        if (current_left_corner)
        {
            double current_distance = check_line.segment_distance(*current_left_corner);
            if (std::abs(current_distance) < trace_halfwidth_middle)
            {
                std::optional<FloatPoint> current_corrected_result =
                    right_left_tangential_point(check_line.a(), check_line.b(), *current_left_corner,
                                                trace_halfwidth_max);
                if (current_corrected_result
                    && (!corrected_result
                        || current_corrected_result->side_of(current_from_point_, *corrected_result)
                               == Side::ON_THE_RIGHT))
                {
                    corrected_door_ind = i;
                    corrected_result = current_corrected_result;
                }
            }
        }
        std::optional<FloatPoint> current_right_corner = calc_door_right_corner(backtrack_array_[i]);
        if (current_right_corner)
        {
            double current_distance = check_line.segment_distance(*current_right_corner);
            if (std::abs(current_distance) < trace_halfwidth_middle)
            {
                std::optional<FloatPoint> current_corrected_result =
                    left_right_tangential_point(check_line.a(), check_line.b(), *current_right_corner,
                                                trace_halfwidth_max);
                if (current_corrected_result
                    && (!corrected_result
                        || current_corrected_result->side_of(current_from_point_, *corrected_result)
                               == Side::ON_THE_LEFT))
                {
                    corrected_door_ind = i;
                    corrected_result = current_corrected_result;
                }
            }
        }
    }
    if (corrected_result)
    {
        result_corner = corrected_result;
        new_door_ind = std::max(corrected_door_ind, current_to_door_index_);
    }

    current_to_door_index_ = new_door_ind;
    if (result_corner && !(*result_corner == current_from_point_))
    {
        result.push_back(*result_corner);
    }
    if (is_trace_net(ctrl_->net_no))
    {
        logger::FRLogger::instance().trace("compare_trace_next_corners_raw net=" + std::to_string(ctrl_->net_no)
                                           + ", layer=" + std::to_string(current_trace_layer_) + ", from_door="
                                           + std::to_string(current_from_door_index_) + ", to_door="
                                           + std::to_string(current_to_door_index_) + ", target_door="
                                           + std::to_string(current_target_door_index_) + ", endOfTrace="
                                           + std::to_string(end_of_trace) + ", corrected="
                                           + std::to_string(corrected_result.has_value()) + ", result_size="
                                           + std::to_string(result.size()) + ", resultCorner="
                                           + result_corner->to_string() + ", current_from="
                                           + current_from_point_.to_string());
    }
    return result;
}

std::optional<FloatPoint> FoundConnectionLocatorAnyAngle::calc_door_left_corner(
    const BacktrackElement& p_to_info)
{
    if (p_to_info.door == nullptr)
    {
        return std::nullopt;
    }
    CompleteExpansionRoom* from_room = p_to_info.door->other_room(p_to_info.next_room);
    if (from_room == nullptr)
    {
        return std::nullopt;
    }
    FloatPoint pole = from_room->get_shape_tile().centre_of_gravity();
    const TileShape& current_to_door_shape = p_to_info.door->get_shape();
    int left_most_corner_no = current_to_door_shape.index_of_left_most_corner(pole);
    return current_to_door_shape.corner_approx(left_most_corner_no);
}

std::optional<FloatPoint> FoundConnectionLocatorAnyAngle::calc_door_right_corner(
    const BacktrackElement& p_to_info)
{
    if (p_to_info.door == nullptr)
    {
        return std::nullopt;
    }
    CompleteExpansionRoom* from_room = p_to_info.door->other_room(p_to_info.next_room);
    if (from_room == nullptr)
    {
        return std::nullopt;
    }
    FloatPoint pole = from_room->get_shape_tile().centre_of_gravity();
    const TileShape& current_to_door_shape = p_to_info.door->get_shape();
    int right_most_corner_no = current_to_door_shape.index_of_right_most_corner(pole);
    return current_to_door_shape.corner_approx(right_most_corner_no);
}

std::optional<FloatPoint> FoundConnectionLocatorAnyAngle::right_turn_next_corner(
    const FloatPoint& p_from_corner,
    double p_dist,
    const std::optional<FloatPoint>& p_to_corner,
    const std::optional<FloatPoint>& p_next_corner)
{
    if (!p_to_corner || !p_next_corner)
    {
        return p_from_corner;
    }
    std::optional<FloatPoint> current_tangential_point =
        p_from_corner.left_tangential_point(*p_to_corner, p_dist);
    if (!current_tangential_point)
    {
        logger::FRLogger::instance().trace(
            "FoundConnectionLocatorAnyAngle::right_turn_next_corner: left tangential point is null");
        return p_from_corner;
    }
    const FloatLine first_line(p_from_corner, *current_tangential_point);
    current_tangential_point = p_to_corner->right_tangential_point(*p_next_corner, 2 * p_dist + C_TOLERANCE);
    if (!current_tangential_point)
    {
        logger::FRLogger::instance().trace(
            "FoundConnectionLocatorAnyAngle::right_turn_next_corner: right tangential point is null");
        return p_from_corner;
    }
    FloatLine second_line(*p_to_corner, *current_tangential_point);
    second_line = second_line.translate(p_dist);
    return first_line.intersection(second_line);
}

std::optional<FloatPoint> FoundConnectionLocatorAnyAngle::left_turn_next_corner(
    const FloatPoint& p_from_corner,
    double p_dist,
    const std::optional<FloatPoint>& p_to_corner,
    const std::optional<FloatPoint>& p_next_corner)
{
    if (!p_to_corner || !p_next_corner)
    {
        return p_from_corner;
    }
    std::optional<FloatPoint> current_tangential_point =
        p_from_corner.right_tangential_point(*p_to_corner, p_dist);
    if (!current_tangential_point)
    {
        logger::FRLogger::instance().trace(
            "FoundConnectionLocatorAnyAngle::left_turn_next_corner: right tangential point is null");
        return p_from_corner;
    }
    const FloatLine first_line(p_from_corner, *current_tangential_point);
    current_tangential_point = p_to_corner->left_tangential_point(*p_next_corner, 2 * p_dist + C_TOLERANCE);
    if (!current_tangential_point)
    {
        logger::FRLogger::instance().trace(
            "FoundConnectionLocatorAnyAngle::left_turn_next_corner: left tangential point is null");
        return p_from_corner;
    }
    FloatLine second_line(*p_to_corner, *current_tangential_point);
    second_line = second_line.translate(-p_dist);
    return first_line.intersection(second_line);
}

std::optional<FloatPoint> FoundConnectionLocatorAnyAngle::right_left_tangential_point(
    const FloatPoint& p_from_point,
    const FloatPoint& p_to_point,
    const FloatPoint& p_center,
    double p_dist)
{
    std::optional<FloatPoint> current_tangential_point = p_from_point.right_tangential_point(p_center, p_dist);
    if (!current_tangential_point)
    {
        logger::FRLogger::instance().trace(
            "FoundConnectionLocatorAnyAngle::right_left_tangential_point: right tangential point is null");
        return std::nullopt;
    }
    FloatLine first_line(p_from_point, *current_tangential_point);
    current_tangential_point = p_to_point.left_tangential_point(p_center, p_dist);
    if (!current_tangential_point)
    {
        logger::FRLogger::instance().trace(
            "FoundConnectionLocatorAnyAngle::right_left_tangential_point: left tangential point is null");
        return std::nullopt;
    }
    FloatLine second_line(p_to_point, *current_tangential_point);
    return first_line.intersection(second_line);
}

std::optional<FloatPoint> FoundConnectionLocatorAnyAngle::left_right_tangential_point(
    const FloatPoint& p_from_point,
    const FloatPoint& p_to_point,
    const FloatPoint& p_center,
    double p_dist)
{
    std::optional<FloatPoint> current_tangential_point = p_from_point.left_tangential_point(p_center, p_dist);
    if (!current_tangential_point)
    {
        logger::FRLogger::instance().trace(
            "FoundConnectionLocatorAnyAngle::left_right_tangential_point: left tangential point is null");
        return std::nullopt;
    }
    FloatLine first_line(p_from_point, *current_tangential_point);
    current_tangential_point = p_to_point.right_tangential_point(p_center, p_dist);
    if (!current_tangential_point)
    {
        logger::FRLogger::instance().trace(
            "FoundConnectionLocatorAnyAngle::left_right_tangential_point: right tangential point is null");
        return std::nullopt;
    }
    FloatLine second_line(p_to_point, *current_tangential_point);
    return first_line.intersection(second_line);
}

} // namespace autoroute
} // namespace freerouting