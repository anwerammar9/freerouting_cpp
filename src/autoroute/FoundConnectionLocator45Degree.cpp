#include "autoroute/FoundConnectionLocator45Degree.h"

#include "autoroute/AutorouteEngine.h"
#include "autoroute/ExpansionDrill.h"
#include "autoroute/ExpansionRoom.h"
#include "autoroute/MazeSearchAlgo.h"
#include "board/Item.h"
#include "datastructures/Signum.h"
#include "geometry/planar/FloatLine.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/Side.h"
#include "geometry/planar/Simplex.h"
#include "geometry/planar/TileShape.h"
#include "logger/FRLogger.h"

#include <cmath>
#include <string>

namespace freerouting {
namespace autoroute {

using geometry::planar::FloatLine;
using geometry::planar::FloatPoint;
using geometry::planar::IntBox;
using geometry::planar::Side;
using geometry::planar::Simplex;
using geometry::planar::TileShape;

namespace {

FloatPoint round_to_integer(const FloatPoint& p_point)
{
    return p_point.round().to_float();
}

/** Java logs the corner calculation of these nets on trace level. */
bool is_trace_net(int p_net_no)
{
    return p_net_no == 33 || p_net_no == 66 || p_net_no == 67;
}

} // namespace

std::vector<FloatPoint> FoundConnectionLocator45Degree::calculate_next_trace_corners()
{
    std::vector<FloatPoint> result;

    if (current_to_door_index_ > current_target_door_index_)
    {
        if (is_trace_net(ctrl_->net_no))
        {
            logger::FRLogger::instance().trace("compare_trace_next_corners_raw net=" + std::to_string(ctrl_->net_no)
                                               + ", mode=45, branch=NO_MORE_DOORS, layer="
                                               + std::to_string(current_trace_layer_) + ", from_door="
                                               + std::to_string(current_from_door_index_) + ", to_door="
                                               + std::to_string(current_to_door_index_) + ", target_door="
                                               + std::to_string(current_target_door_index_) + ", result_size="
                                               + std::to_string(result.size()));
        }
        return result;
    }

    if (current_to_door_index_ - 1 < 0 || current_to_door_index_ - 1 >= static_cast<int>(backtrack_array_.size()))
    {
        logger::FRLogger::instance().warn("FoundConnectionLocator45Degree::calculate_next_trace_corners: from door index out of range");
        return result;
    }
    const BacktrackElement& current_from_info = backtrack_array_[current_to_door_index_ - 1];

    if (current_from_info.next_room == nullptr)
    {
        logger::FRLogger::instance().warn(
            "FoundConnectionLocator45Degree::calculate_next_trace_corners: nextRoom is null");
        return result;
    }

    const TileShape& room_shape = current_from_info.next_room->get_shape_tile();

    double trace_halfwidth = compensated_trace_half_width(current_trace_layer_);
    // add some tolerance for free space expansion rooms.
    double trace_halfwidth_add = trace_halfwidth + AutorouteEngine::TRACE_WIDTH_TOLERANCE;
    double shrink_offset;
    if (dynamic_cast<const ObstacleExpansionRoom*>(current_from_info.next_room) != nullptr)
    {
        shrink_offset = trace_halfwidth;
    }
    else
    {
        shrink_offset = trace_halfwidth_add;
    }

    std::unique_ptr<TileShape> shrinked_room_shape = room_shape.offset(-shrink_offset);
    if (!shrinked_room_shape)
    {
        shrinked_room_shape = room_shape.clone();
    }
    if (is_trace_net(ctrl_->net_no))
    {
        logger::FRLogger::instance().trace("compare_trace_room_shrink_raw net=" + std::to_string(ctrl_->net_no)
                                           + ", mode=45, layer=" + std::to_string(current_trace_layer_)
                                           + ", from_door=" + std::to_string(current_from_door_index_)
                                           + ", to_door=" + std::to_string(current_to_door_index_) + ", target_door="
                                           + std::to_string(current_target_door_index_) + ", next_room_type="
                                           + MazeSearchAlgo::describe_room(current_from_info.next_room)
                                           + ", shrinkOffset=" + std::to_string(shrink_offset) + ", room_empty="
                                           + std::to_string(room_shape.is_empty()) + ", shrinked_empty="
                                           + std::to_string(shrinked_room_shape->is_empty()) + ", current_from="
                                           + current_from_point_.to_string());
    }
    if (!shrinked_room_shape->is_empty())
    {
        // enter the shrunk room shape by a 45-degree angle first
        std::optional<FloatPoint> nearest_room_point_optional =
            shrinked_room_shape->nearest_point_approx(current_from_point_);
        if (!nearest_room_point_optional)
        {
            logger::FRLogger::instance().warn(
                "FoundConnectionLocator45Degree::calculate_next_trace_corners: no nearest point in the room shape");
            return result;
        }
        FloatPoint nearest_room_point = *nearest_room_point_optional;
        bool horizontal_first =
            calc_horizontal_first_from_door(current_from_info.door, current_from_point_, nearest_room_point);
        nearest_room_point = round_to_integer(nearest_room_point);
        result.push_back(
            calculate_additional_corner(current_from_point_, nearest_room_point, horizontal_first,
                                        angle_restriction_));
        result.push_back(nearest_room_point);
        current_from_point_ = nearest_room_point;
    }
    else
    {
        shrinked_room_shape = room_shape.clone();
    }

    if (current_to_door_index_ == current_target_door_index_)
    {
        if (!current_target_shape_)
        {
            logger::FRLogger::instance().warn(
                "FoundConnectionLocator45Degree::calculate_next_trace_corners: no target shape");
            return result;
        }
        std::optional<FloatPoint> nearest_point_optional = current_target_shape_->nearest_point_approx(current_from_point_);
        if (!nearest_point_optional)
        {
            logger::FRLogger::instance().warn(
                "FoundConnectionLocator45Degree::calculate_next_trace_corners: no nearest point in the target shape");
            return result;
        }
        FloatPoint nearest_point = round_to_integer(*nearest_point_optional);
        FloatPoint add_corner =
            calculate_additional_corner(current_from_point_, nearest_point, true, angle_restriction_);
        if (!shrinked_room_shape->contains(add_corner))
        {
            add_corner = calculate_additional_corner(current_from_point_, nearest_point, false, angle_restriction_);
        }
        result.push_back(add_corner);
        result.push_back(nearest_point);
        ++current_to_door_index_;
        if (is_trace_net(ctrl_->net_no))
        {
            logger::FRLogger::instance().trace("compare_trace_next_corners_raw net=" + std::to_string(ctrl_->net_no)
                                               + ", mode=45, branch=TARGET_DOOR, layer="
                                               + std::to_string(current_trace_layer_) + ", from_door="
                                               + std::to_string(current_from_door_index_) + ", to_door="
                                               + std::to_string(current_to_door_index_) + ", target_door="
                                               + std::to_string(current_target_door_index_) + ", result_size="
                                               + std::to_string(result.size()) + ", nearestPoint="
                                               + nearest_point.to_string() + ", addCorner=" + add_corner.to_string());
        }
        return result;
    }

    if (current_to_door_index_ < 0 || current_to_door_index_ >= static_cast<int>(backtrack_array_.size()))
    {
        logger::FRLogger::instance().warn(
            "FoundConnectionLocator45Degree::calculate_next_trace_corners: to door index out of range");
        return result;
    }
    const BacktrackElement& current_to_info = backtrack_array_[current_to_door_index_];
    auto* current_to_door = dynamic_cast<ExpansionDoor*>(current_to_info.door);
    if (current_to_door == nullptr)
    {
        logger::FRLogger::instance().warn(
            "FoundConnectionLocator45Degree::calculate_next_trace_corners: ExpansionDoor expected");
        return result;
    }

    FloatPoint nearest_to_door_point;
    if (current_to_door->get_dimension() == 2)
    {
        // May not happen in free angle routing mode because then corners are cut off.
        // The door shape is bound to the TileShape base, so that the Java
        // TileShape.shrink() is used and not the unrelated IntBox.shrink(int).
        const TileShape& to_door_shape = current_to_door->get_shape();

        std::unique_ptr<TileShape> shrinked_to_door_shape = to_door_shape.shrink(shrink_offset);
        std::optional<FloatPoint> nearest_to_door_point_optional =
            shrinked_to_door_shape ? shrinked_to_door_shape->nearest_point_approx(current_from_point_)
                                   : std::nullopt;
        if (!nearest_to_door_point_optional)
        {
            logger::FRLogger::instance().warn(
                "FoundConnectionLocator45Degree::calculate_next_trace_corners: no nearest point at the to door");
            return result;
        }
        nearest_to_door_point = round_to_integer(*nearest_to_door_point_optional);
    }
    else
    {
        std::vector<FloatLine> line_sections = current_to_door->get_section_segments(trace_halfwidth);
        if (current_to_info.section_no_of_door < 0
            || current_to_info.section_no_of_door >= static_cast<int>(line_sections.size()))
        {
            logger::FRLogger::instance().warn(
                "FoundConnectionLocator45Degree::calculate_next_trace_corners: lineSections inconsistent");
            return result;
        }
        const FloatLine& current_line_section = line_sections[current_to_info.section_no_of_door];
        nearest_to_door_point = current_line_section.nearest_segment_point(current_from_point_);

        bool nearest_to_door_point_ok = true;
        if (current_to_info.next_room != nullptr)
        {
            Simplex next_room_shape = current_to_info.next_room->get_shape_tile();
            // with IntBox or IntOctagon the next calculation will not work, because they have
            // border lines of length 0.
            std::vector<FloatPoint> nearest_points =
                next_room_shape.nearest_border_points_approx(nearest_to_door_point, 2);
            if (nearest_points.size() >= 2)
            {
                nearest_to_door_point_ok =
                    nearest_points[1].distance(nearest_to_door_point) >= trace_halfwidth_add;
            }
        }
        if (!nearest_to_door_point_ok)
        {
            // may be the room has an acute (45 degree) angle at a corner of the door
            nearest_to_door_point = current_line_section.a().middle_point(current_line_section.b());
        }
    }
    nearest_to_door_point = round_to_integer(nearest_to_door_point);
    bool horizontal_first =
        calc_horizontal_first_to_door(current_to_info.door, current_from_point_, nearest_to_door_point);
    result.push_back(calculate_additional_corner(current_from_point_, nearest_to_door_point, horizontal_first,
                                                 angle_restriction_));
    result.push_back(nearest_to_door_point);
    ++current_to_door_index_;
    if (is_trace_net(ctrl_->net_no))
    {
        logger::FRLogger::instance().trace("compare_trace_next_corners_raw net=" + std::to_string(ctrl_->net_no)
                                           + ", mode=45, branch=EXPANSION_DOOR, layer="
                                           + std::to_string(current_trace_layer_) + ", from_door="
                                           + std::to_string(current_from_door_index_) + ", to_door="
                                           + std::to_string(current_to_door_index_) + ", target_door="
                                           + std::to_string(current_target_door_index_) + ", result_size="
                                           + std::to_string(result.size()) + ", nearestToDoorPoint="
                                           + nearest_to_door_point.to_string() + ", horizontalFirst="
                                           + std::to_string(horizontal_first));
    }
    return result;
}

bool FoundConnectionLocator45Degree::calc_horizontal_first_from_door(const ExpandableObject* p_from_door,
                                                                    const FloatPoint& p_from_point,
                                                                    const FloatPoint& p_to_point)
{
    if (p_from_door == nullptr)
    {
        return false;
    }
    IntBox to_door_shape = p_from_door->get_shape();
    IntBox from_door_box = to_door_shape.bounding_box();
    if (p_from_door->get_dimension() != 1)
    {
        return from_door_box.height() >= from_door_box.width();
    }

    std::optional<FloatLine> door_line_segment_optional = to_door_shape.diagonal_corner_segment();
    if (!door_line_segment_optional)
    {
        return false;
    }
    const FloatLine& door_line_segment = *door_line_segment_optional;
    FloatPoint left_corner;
    FloatPoint right_corner;
    if (door_line_segment.a().x < door_line_segment.b().x
        || (door_line_segment.a().x == door_line_segment.b().x && door_line_segment.a().y <= door_line_segment.b().y))
    {
        left_corner = door_line_segment.a();
        right_corner = door_line_segment.b();
    }
    else
    {
        left_corner = door_line_segment.b();
        right_corner = door_line_segment.a();
    }
    double door_dx = right_corner.x - left_corner.x;
    double door_dy = right_corner.y - left_corner.y;
    double abs_door_dy = std::abs(door_dy);
    double door_max_width = std::max(door_dx, abs_door_dy);
    bool result;
    double door_half_max_width = 0.5 * door_max_width;
    if (from_door_box.width() <= door_half_max_width)
    {
        // door is about vertical
        result = true;
    }
    else if (from_door_box.height() <= door_half_max_width)
    {
        // door is about horizontal
        result = false;
    }
    else
    {
        double dx = p_to_point.x - p_from_point.x;
        double dy = p_to_point.y - p_from_point.y;
        if (left_corner.y < right_corner.y)
        {
            // door is about right diagonal
            if (datastructures::Signum::of(dx) == datastructures::Signum::of(dy))
            {
                result = std::abs(dx) > std::abs(dy);
            }
            else
            {
                result = std::abs(dx) < std::abs(dy);
            }
        }
        else
        {
            // door is about left diagonal
            if (datastructures::Signum::of(dx) == datastructures::Signum::of(dy))
            {
                result = std::abs(dx) < std::abs(dy);
            }
            else
            {
                result = std::abs(dx) > std::abs(dy);
            }
        }
    }
    return result;
}

bool FoundConnectionLocator45Degree::calc_horizontal_first_to_door(const ExpandableObject* p_to_door,
                                                                  const FloatPoint& p_from_point,
                                                                  const FloatPoint& p_to_point)
{
    if (p_to_door == nullptr)
    {
        return false;
    }
    IntBox to_door_shape = p_to_door->get_shape();
    IntBox from_door_box = to_door_shape.bounding_box();
    if (p_to_door->get_dimension() != 1)
    {
        return from_door_box.height() <= from_door_box.width();
    }

    std::optional<FloatLine> door_line_segment_optional = to_door_shape.diagonal_corner_segment();
    if (!door_line_segment_optional)
    {
        return false;
    }
    const FloatLine& door_line_segment = *door_line_segment_optional;
    FloatPoint left_corner;
    FloatPoint right_corner;
    if (door_line_segment.a().x < door_line_segment.b().x
        || (door_line_segment.a().x == door_line_segment.b().x && door_line_segment.a().y <= door_line_segment.b().y))
    {
        left_corner = door_line_segment.a();
        right_corner = door_line_segment.b();
    }
    else
    {
        left_corner = door_line_segment.b();
        right_corner = door_line_segment.a();
    }
    double door_dx = right_corner.x - left_corner.x;
    double door_dy = right_corner.y - left_corner.y;
    double abs_door_dy = std::abs(door_dy);
    double door_max_width = std::max(door_dx, abs_door_dy);
    bool result;
    double door_half_max_width = 0.5 * door_max_width;
    if (from_door_box.width() <= door_half_max_width)
    {
        // door is about vertical
        result = false;
    }
    else if (from_door_box.height() <= door_half_max_width)
    {
        // door is about horizontal
        result = true;
    }
    else
    {
        double dx = p_to_point.x - p_from_point.x;
        double dy = p_to_point.y - p_from_point.y;
        if (left_corner.y < right_corner.y)
        {
            // door is about right diagonal
            if (datastructures::Signum::of(dx) == datastructures::Signum::of(dy))
            {
                result = std::abs(dx) < std::abs(dy);
            }
            else
            {
                result = std::abs(dx) > std::abs(dy);
            }
        }
        else
        {
            // door is about left diagonal
            if (datastructures::Signum::of(dx) == datastructures::Signum::of(dy))
            {
                result = std::abs(dx) > std::abs(dy);
            }
            else
            {
                result = std::abs(dx) < std::abs(dy);
            }
        }
    }
    return result;
}

} // namespace autoroute
} // namespace freerouting