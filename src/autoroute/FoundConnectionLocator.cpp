#include "autoroute/FoundConnectionLocator.h"

#include "autoroute/AutorouteEngine.h"
#include "autoroute/ExpansionDrill.h"
#include "autoroute/ExpansionRoom.h"
#include "autoroute/FoundConnectionLocator45Degree.h"
#include "autoroute/FoundConnectionLocatorAnyAngle.h"
#include "autoroute/MazeSearchAlgo.h"
#include "board/ConductionArea.h"
#include "board/Item.h"
#include "board/ShapeSearchTree.h"
#include "geometry/planar/FloatLine.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/Side.h"
#include "logger/FRLogger.h"

#include <cmath>

namespace freerouting {
namespace autoroute {

using geometry::planar::FloatPoint;
using geometry::planar::IntPoint;

namespace {

/** Java logs the backtrack of these nets on trace level. */
bool is_trace_net(int p_net_no)
{
    return p_net_no == 33 || p_net_no == 66 || p_net_no == 67;
}

FloatPoint round_to_integer(const FloatPoint& p_point)
{
    return p_point.round().to_float();
}

FloatPoint ninety_degree_corner(const FloatPoint& p_from_point, const FloatPoint& p_to_point,
                                bool p_horizontal_first)
{
    double x;
    double y;
    if (p_horizontal_first)
    {
        x = p_to_point.x;
        y = p_from_point.y;
    }
    else
    {
        x = p_from_point.x;
        y = p_to_point.y;
    }
    return FloatPoint(x, y);
}

FloatPoint fortyfive_degree_corner(const FloatPoint& p_from_point, const FloatPoint& p_to_point,
                                   bool p_horizontal_first)
{
    double abs_dx = std::abs(p_to_point.x - p_from_point.x);
    double abs_dy = std::abs(p_to_point.y - p_from_point.y);
    double x;
    double y;

    if (abs_dx <= abs_dy)
    {
        if (p_horizontal_first)
        {
            x = p_to_point.x;
            if (p_to_point.y >= p_from_point.y)
            {
                y = p_from_point.y + abs_dx;
            }
            else
            {
                y = p_from_point.y - abs_dx;
            }
        }
        else
        {
            x = p_from_point.x;
            if (p_to_point.y > p_from_point.y)
            {
                y = p_to_point.y - abs_dx;
            }
            else
            {
                y = p_to_point.y + abs_dx;
            }
        }
    }
    else
    {
        if (p_horizontal_first)
        {
            y = p_from_point.y;
            if (p_to_point.x > p_from_point.x)
            {
                x = p_to_point.x - abs_dy;
            }
            else
            {
                x = p_to_point.x + abs_dy;
            }
        }
        else
        {
            y = p_to_point.y;
            if (p_to_point.x > p_from_point.x)
            {
                x = p_from_point.x + abs_dy;
            }
            else
            {
                x = p_from_point.x - abs_dy;
            }
        }
    }
    return FloatPoint(x, y);
}

} // namespace

std::unique_ptr<FoundConnectionLocator> FoundConnectionLocator::get_instance(
    const MazeSearchAlgo::Result* p_maze_result,
    const AutorouteControl& p_ctrl,
    board::ShapeSearchTree* p_search_tree,
    board::AngleRestriction p_angle_restriction,
    std::set<board::Item*>* p_ripped_item_list,
    std::map<board::Item*, int>* p_ripup_costs)
{
    if (p_maze_result == nullptr)
    {
        return nullptr;
    }
    std::unique_ptr<FoundConnectionLocator> result;
    if (p_angle_restriction == board::AngleRestriction::NINETY_DEGREE
        || p_angle_restriction == board::AngleRestriction::FORTYFIVE_DEGREE)
    {
        result = std::make_unique<FoundConnectionLocator45Degree>();
    }
    else
    {
        result = std::make_unique<FoundConnectionLocatorAnyAngle>();
    }
    result->initialize(*p_maze_result, p_ctrl, p_search_tree, p_angle_restriction, p_ripped_item_list,
                       p_ripup_costs);
    return result;
}

FloatPoint FoundConnectionLocator::calculate_additional_corner(const FloatPoint& p_from_point,
                                                               const FloatPoint& p_to_point,
                                                               bool p_horizontal_first,
                                                               board::AngleRestriction p_angle_restriction)
{
    FloatPoint result;
    if (p_angle_restriction == board::AngleRestriction::NINETY_DEGREE)
    {
        result = ninety_degree_corner(p_from_point, p_to_point, p_horizontal_first);
    }
    else if (p_angle_restriction == board::AngleRestriction::FORTYFIVE_DEGREE)
    {
        result = fortyfive_degree_corner(p_from_point, p_to_point, p_horizontal_first);
    }
    else
    {
        result = p_to_point;
    }
    return result;
}

bool FoundConnectionLocator::initialize(const MazeSearchAlgo::Result& p_maze_result,
                                        const AutorouteControl& p_ctrl,
                                        board::ShapeSearchTree* p_search_tree,
                                        board::AngleRestriction p_angle_restriction,
                                        std::set<board::Item*>* p_ripped_item_list,
                                        std::map<board::Item*, int>* p_ripup_costs)
{
    ctrl_ = &p_ctrl;
    angle_restriction_ = p_angle_restriction;
    backtrack_array_ = backtrack(p_maze_result, p_ctrl.net_no, p_ripped_item_list, p_ripup_costs);

    if (is_trace_net(ctrl_->net_no))
    {
        logger::FRLogger::instance().trace("compare_trace_backtrack_raw net=" + std::to_string(ctrl_->net_no)
                                           + ", size=" + std::to_string(backtrack_array_.size()));
        for (size_t i = 0; i < backtrack_array_.size(); i++)
        {
            const BacktrackElement& element = backtrack_array_[i];
            logger::FRLogger::instance().trace(
                "compare_trace_backtrack_raw net=" + std::to_string(ctrl_->net_no) + ", idx=" + std::to_string(i)
                + ", door_type=" + MazeSearchAlgo::describe_expandable(element.door) + ", section="
                + std::to_string(element.section_no_of_door) + ", next_room="
                + MazeSearchAlgo::describe_room(element.next_room));
        }
    }

    if (backtrack_array_.empty())
    {
        logger::FRLogger::instance().warn("FoundConnectionLocator: empty backtrack array");
        return false;
    }

    const BacktrackElement& start_info = backtrack_array_.back();
    start_door_ = dynamic_cast<TargetItemExpansionDoor*>(start_info.door);
    if (start_door_ == nullptr)
    {
        logger::FRLogger::instance().warn("FoundConnectionLocator: ItemExpansionDoor expected for startInfo.door");
        start_item = nullptr;
        start_layer = 0;
        target_item = nullptr;
        target_layer = 0;
        return false;
    }
    start_item = start_door_->item;
    start_layer = start_door_->get_complete_room() != nullptr
        ? start_door_->get_complete_room()->get_layer()
        : 0;

    current_from_door_index_ = 0;
    bool at_fanout_end = false;
    if (auto* current_destination_door = dynamic_cast<TargetItemExpansionDoor*>(p_maze_result.destination_door))
    {
        if (current_destination_door->get_complete_room() == nullptr)
        {
            logger::FRLogger::instance().warn("FoundConnectionLocator: target door without a room");
            return false;
        }
        target_item = current_destination_door->item;
        target_layer = current_destination_door->get_complete_room()->get_layer();

        std::optional<FloatPoint> starting_point = calculate_starting_point(current_destination_door, p_search_tree);
        if (!starting_point)
        {
            logger::FRLogger::instance().warn("FoundConnectionLocator: no starting point at the destination door");
            target_item = nullptr;
            target_layer = 0;
            return false;
        }
        current_from_point_ = *starting_point;
    }
    else if (auto* current_drill = dynamic_cast<ExpansionDrill*>(p_maze_result.destination_door))
    {
        // may happen only in case of fanout
        target_item = nullptr;
        current_from_point_ = current_drill->location.to_float();
        target_layer = current_drill->first_layer + p_maze_result.section_no_of_door;
        at_fanout_end = true;
    }
    else
    {
        logger::FRLogger::instance().warn("FoundConnectionLocator: unexpected type of destinationDoor");
        target_item = nullptr;
        target_layer = 0;
        return false;
    }
    current_trace_layer_ = target_layer;
    previous_from_point_ = current_from_point_;

    bool connection_done = false;
    while (!connection_done)
    {
        bool layer_changed = false;
        if (at_fanout_end)
        {
            // do not increase current_target_door_index_
            layer_changed = true;
        }
        else
        {
            current_target_door_index_ = current_from_door_index_ + 1;
            while (current_target_door_index_ < static_cast<int>(backtrack_array_.size()) && !layer_changed)
            {
                if (dynamic_cast<ExpansionDrill*>(backtrack_array_[current_target_door_index_].door) != nullptr)
                {
                    layer_changed = true;
                }
                else
                {
                    ++current_target_door_index_;
                }
            }
        }
        if (layer_changed)
        {
            // the next trace leads to a via
            auto* current_target_drill =
                dynamic_cast<ExpansionDrill*>(backtrack_array_[current_target_door_index_].door);
            if (current_target_drill == nullptr)
            {
                logger::FRLogger::instance().warn("FoundConnectionLocator: ExpansionDrill expected in the backtrack");
                return false;
            }
            current_target_shape_ =
                std::make_unique<geometry::planar::IntBox>(geometry::planar::TileShape::get_instance(
                    current_target_drill->location));
        }
        else
        {
            // the next trace leads to the final target
            connection_done = true;
            current_target_door_index_ = static_cast<int>(backtrack_array_.size()) - 1;
            const geometry::planar::TileShape* target_shape =
                get_trace_connection_shape(start_item, start_door_->tree_entry_no, p_search_tree);
            if (target_shape == nullptr)
            {
                logger::FRLogger::instance().warn("FoundConnectionLocator: no trace connection shape at the start item");
                return false;
            }
            current_target_shape_ = geometry::planar::TileShape::intersection_of(
                *target_shape, start_door_->get_complete_room()->get_shape_tile());
            if (!current_target_shape_)
            {
                logger::FRLogger::instance().warn("FoundConnectionLocator: empty target shape at the start item");
                return false;
            }
            if (current_target_shape_->dimension() >= 2)
            {
                // the target is a conduction area, make a safe connection
                // by shrinking the shape by the trace halfwidth.
                double trace_half_width = compensated_trace_half_width(start_door_->get_complete_room()->get_layer());
                std::unique_ptr<geometry::planar::TileShape> shrinked_shape =
                    current_target_shape_->offset(-trace_half_width);
                if (shrinked_shape && !shrinked_shape->is_empty())
                {
                    current_target_shape_ = std::move(shrinked_shape);
                }
            }
        }
        current_to_door_index_ = current_from_door_index_ + 1;
        ResultItem next_trace = calculate_next_trace(layer_changed, at_fanout_end);
        at_fanout_end = false;
        connection_items.push_back(next_trace);
    }

    valid_ = true;
    return true;
}

std::vector<FoundConnectionLocator::BacktrackElement> FoundConnectionLocator::backtrack(
    const MazeSearchAlgo::Result& p_maze_result,
    int p_net_no,
    std::set<board::Item*>* p_ripped_item_list,
    std::map<board::Item*, int>* p_ripup_costs)
{
    std::vector<BacktrackElement> result;
    CompleteExpansionRoom* current_next_room = nullptr;
    ExpandableObject* current_backtrack_door = p_maze_result.destination_door;
    if (current_backtrack_door == nullptr)
    {
        return result;
    }
    MazeSearchElement* current_maze_search_element =
        current_backtrack_door->get_maze_search_element(p_maze_result.section_no_of_door);
    if (current_maze_search_element == nullptr)
    {
        return result;
    }
    const bool debug_backtrack = p_net_no == 98;
    if (auto* door = dynamic_cast<TargetItemExpansionDoor*>(current_backtrack_door))
    {
        current_next_room = door->get_complete_room();
    }
    else if (auto* current_drill = dynamic_cast<ExpansionDrill*>(current_backtrack_door))
    {
        current_next_room = current_drill->room_arr[current_drill->first_layer + p_maze_result.section_no_of_door];
        if (current_maze_search_element->room_ripped)
        {
            for (CompleteExpansionRoom* tmp_room : current_drill->room_arr)
            {
                if (auto* room = dynamic_cast<ObstacleExpansionRoom*>(tmp_room))
                {
                    if (p_ripped_item_list != nullptr)
                    {
                        p_ripped_item_list->insert(room->get_item());
                    }
                    if (p_ripup_costs != nullptr)
                    {
                        (*p_ripup_costs)[room->get_item()] = current_maze_search_element->ripup_cost;
                    }
                }
            }
        }
    }
    BacktrackElement current_backtrack_element{current_backtrack_door, p_maze_result.section_no_of_door,
                                              current_next_room};
    for (;;)
    {
        result.push_back(current_backtrack_element);
        current_backtrack_door = current_maze_search_element->backtrack_door;
        if (current_backtrack_door == nullptr)
        {
            break;
        }
        int current_section_no = current_maze_search_element->section_no_of_backtrack_door;
        if (current_section_no >= current_backtrack_door->maze_search_element_count())
        {
            logger::FRLogger::instance().warn("FoundConnectionLocator: currentSectionNo to big");
            current_section_no = current_backtrack_door->maze_search_element_count() - 1;
        }
        if (current_section_no < 0)
        {
            logger::FRLogger::instance().warn("FoundConnectionLocator: negative section number in the backtrack");
            break;
        }
        if (auto* current_drill = dynamic_cast<ExpansionDrill*>(current_backtrack_door))
        {
            if (current_section_no < static_cast<int>(current_drill->room_arr.size()))
            {
                current_next_room = current_drill->room_arr[current_section_no];
            }
            else
            {
                current_next_room = nullptr;
            }
        }
        else
        {
            current_next_room = current_backtrack_door->other_room(current_next_room);
        }
        current_maze_search_element = current_backtrack_door->get_maze_search_element(current_section_no);
        if (current_maze_search_element == nullptr)
        {
            logger::FRLogger::instance().warn("FoundConnectionLocator: missing maze search element in the backtrack");
            break;
        }
        current_backtrack_element = BacktrackElement{current_backtrack_door, current_section_no, current_next_room};
        if (debug_backtrack)
        {
            int obstacle_id = -1;
            if (auto* obst = dynamic_cast<ObstacleExpansionRoom*>(current_next_room))
            {
                obstacle_id = obst->get_item() != nullptr ? obst->get_item()->get_id() : -1;
            }
            logger::FRLogger::instance().trace(
                "BACKTRACK_STEP net=" + std::to_string(p_net_no) + ", door_type="
                + MazeSearchAlgo::describe_expandable(current_backtrack_door) + ", section="
                + std::to_string(current_section_no) + ", roomRipped="
                + std::to_string(current_maze_search_element->room_ripped) + ", ripupCost="
                + std::to_string(current_maze_search_element->ripup_cost) + ", next_room_type="
                + MazeSearchAlgo::describe_room(current_next_room) + ", obstacle_id=" + std::to_string(obstacle_id));
        }
        if (current_maze_search_element->room_ripped)
        {
            if (auto* room = dynamic_cast<ObstacleExpansionRoom*>(current_next_room))
            {
                if (p_ripped_item_list != nullptr)
                {
                    p_ripped_item_list->insert(room->get_item());
                }
                if (p_ripup_costs != nullptr)
                {
                    (*p_ripup_costs)[room->get_item()] = current_maze_search_element->ripup_cost;
                }
            }
        }
    }
    return result;
}

std::optional<FloatPoint> FoundConnectionLocator::calculate_starting_point(TargetItemExpansionDoor* p_from_door,
                                                                          board::ShapeSearchTree* p_search_tree)
{
    const geometry::planar::TileShape* connection_shape =
        get_trace_connection_shape(p_from_door->item, p_from_door->tree_entry_no, p_search_tree);
    if (connection_shape == nullptr)
    {
        return std::nullopt;
    }
    std::unique_ptr<geometry::planar::TileShape> intersection =
        geometry::planar::TileShape::intersection_of(*connection_shape,
                                                     p_from_door->get_complete_room()->get_shape_tile());
    if (!intersection)
    {
        return std::nullopt;
    }
    return round_to_integer(intersection->centre_of_gravity());
}

const geometry::planar::TileShape* FoundConnectionLocator::get_trace_connection_shape(
    board::Item* p_item, int p_index, board::ShapeSearchTree* p_search_tree)
{
    if (p_item == nullptr || p_search_tree == nullptr)
    {
        return nullptr;
    }
    if (!is_connectable(p_item))
    {
        return nullptr;
    }
    return p_item->get_tree_shape(*p_search_tree, p_index);
}

bool FoundConnectionLocator::is_connectable(const board::Item* p_item)
{
    // C++ analogue of the Java `instanceof Connectable` guard: the C++
    // Connectable interface is a dead draft implemented by no real item, so a
    // connectable object is a routeable item (trace, drill) or a conduction
    // area. This mirrors MazeSearchAlgo::is_connectable().
    if (p_item == nullptr)
    {
        return false;
    }
    return p_item->is_trace() || p_item->is_via() || p_item->is_pin()
        || dynamic_cast<const board::ConductionArea*>(p_item) != nullptr;
}

double FoundConnectionLocator::compensated_trace_half_width(int p_layer) const
{
    // The C++ AutorouteControl can be left uninitialised (empty arrays), while
    // the Java code would raise an IndexOutOfBoundsException. A missing entry
    // is reported once and the neutral value is returned, mirroring
    // MazeSearchAlgo::compensated_trace_half_width_for_layer().
    if (ctrl_ == nullptr)
    {
        return 0.0;
    }
    if (p_layer < 0 || p_layer >= static_cast<int>(ctrl_->compensated_trace_half_width.size()))
    {
        return 0.0;
    }
    return ctrl_->compensated_trace_half_width[p_layer];
}

FoundConnectionLocator::ResultItem FoundConnectionLocator::calculate_next_trace(bool p_layer_changed,
                                                                               bool p_at_fanout_end)
{
    std::vector<FloatPoint> corner_list;
    corner_list.push_back(current_from_point_);
    if (!p_at_fanout_end)
    {
        FloatPoint adjusted_start_corner = adjust_start_corner();
        if (!(adjusted_start_corner == current_from_point_))
        {
            FloatPoint add_corner = calculate_additional_corner(current_from_point_, adjusted_start_corner, true,
                                                                angle_restriction_);
            corner_list.push_back(add_corner);
            corner_list.push_back(adjusted_start_corner);
            previous_from_point_ = current_from_point_;
            current_from_point_ = adjusted_start_corner;
        }
    }
    FloatPoint prev_corner = current_from_point_;
    for (;;)
    {
        std::vector<FloatPoint> next_corners = calculate_next_trace_corners();
        if (next_corners.empty())
        {
            break;
        }
        for (const FloatPoint& current_next_corner : next_corners)
        {
            if (!(current_next_corner == prev_corner))
            {
                corner_list.push_back(current_next_corner);
                previous_from_point_ = current_from_point_;
                current_from_point_ = current_next_corner;
                prev_corner = current_next_corner;
            }
        }
    }

    int next_layer = current_trace_layer_;
    if (p_layer_changed)
    {
        current_from_door_index_ = current_target_door_index_ + 1;
        if (current_from_door_index_ >= static_cast<int>(backtrack_array_.size()))
        {
            logger::FRLogger::instance().warn("FoundConnectionLocator: from door index out of range");
        }
        else
        {
            CompleteExpansionRoom* next_room = backtrack_array_[current_from_door_index_].next_room;
            if (next_room != nullptr)
            {
                next_layer = next_room->get_layer();
            }
        }
    }

    // Round the new trace corners to Integer.
    std::vector<IntPoint> rounded_corner_list;
    bool has_prev_point = false;
    IntPoint prev_point;
    for (const FloatPoint& corner : corner_list)
    {
        IntPoint current_point = corner.round();
        if (!has_prev_point || !(current_point == prev_point))
        {
            rounded_corner_list.push_back(current_point);
            prev_point = current_point;
            has_prev_point = true;
        }
    }

    // Construct the result item
    ResultItem result;
    result.corners = rounded_corner_list;
    result.layer = current_trace_layer_;

    if (is_trace_net(ctrl_->net_no))
    {
        logger::FRLogger::instance().trace(
            "compare_trace_next_trace_raw net=" + std::to_string(ctrl_->net_no) + ", traceLayer="
            + std::to_string(current_trace_layer_) + ", nextLayer=" + std::to_string(next_layer)
            + ", cornerCount=" + std::to_string(result.corners.size()) + ", from_door="
            + std::to_string(current_from_door_index_) + ", to_door=" + std::to_string(current_to_door_index_)
            + ", target_door=" + std::to_string(current_target_door_index_));
    }

    current_trace_layer_ = next_layer;
    return result;
}

FloatPoint FoundConnectionLocator::adjust_start_corner() const
{
    if (current_from_door_index_ < 0 || current_from_door_index_ >= static_cast<int>(backtrack_array_.size()))
    {
        return current_from_point_;
    }
    const BacktrackElement& current_from_info = backtrack_array_[current_from_door_index_];
    if (current_from_info.next_room == nullptr)
    {
        return current_from_point_;
    }
    double trace_half_width = compensated_trace_half_width(current_trace_layer_);
    const geometry::planar::TileShape& room_shape = current_from_info.next_room->get_shape_tile();
    std::unique_ptr<geometry::planar::TileShape> shrinked_room_shape = room_shape.offset(-trace_half_width);
    if (!shrinked_room_shape || shrinked_room_shape->is_empty() || shrinked_room_shape->contains(current_from_point_))
    {
        return current_from_point_;
    }
    std::optional<FloatPoint> nearest = shrinked_room_shape->nearest_point_approx(current_from_point_);
    if (!nearest)
    {
        return current_from_point_;
    }
    return round_to_integer(*nearest);
}

} // namespace autoroute
} // namespace freerouting