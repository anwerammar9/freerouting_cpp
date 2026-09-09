#include "board/ConductionArea.h"

#include <set>

#include "board/BasicBoard.h"
#include "board/ItemSelectionFilter.h"
#include "board/Trace.h"
#include "board/Via.h"
#include "datastructures/ShapeTree.h"
#include "geometry/planar/Shape.h"
#include "geometry/planar/Point.h"
#include "geometry/planar/TileShape.h"
#include "logger/FRLogger.h"

namespace freerouting::board {

namespace {

/**
 * Checks, whether p_shape contains p_point, accepting both integer and float points.
 */
bool shape_contains_point(const geometry::planar::TileShape& p_shape, const geometry::planar::Point& p_point)
{
    if (const auto* int_point = dynamic_cast<const geometry::planar::IntPoint*>(&p_point))
    {
        return p_shape.contains(*int_point);
    }
    if (const auto* float_point = dynamic_cast<const geometry::planar::FloatPoint*>(&p_point))
    {
        return p_shape.contains(*float_point);
    }
    return false;
}

} // namespace

ConductionArea::ConductionArea(const std::shared_ptr<geometry::planar::Area>& p_area,
                               int p_layer,
                               const geometry::planar::Vector& p_translation,
                               double p_rotation_in_degree,
                               bool p_side_changed,
                               const std::vector<int>& p_net_no_arr,
                               int p_clearance_class,
                               int p_id_no,
                               int p_component_no,
                               const std::string& p_name,
                               bool p_is_obstacle,
                               FixedState p_fixed_state,
                               BasicBoard* p_board)
    : ObstacleArea(p_area, p_layer, p_translation, p_rotation_in_degree, p_side_changed, p_net_no_arr,
                   p_clearance_class, p_id_no, p_component_no, p_name, p_fixed_state, p_board),
      is_obstacle_(p_is_obstacle)
{
}

std::unique_ptr<Item> ConductionArea::copy(int p_id_no) const
{
    if (net_nos_.size() != 1)
    {
        logger::FRLogger::instance().warn(
            "ConductionArea.copy not yet implemented for areas with more than 1 net");
        return nullptr;
    }
    return std::make_unique<ConductionArea>(get_relative_area(), get_layer(), *get_translation(),
                                            get_rotation_in_degree(), get_side_changed(), net_nos_, clearance_class_,
                                            p_id_no, component_no_, get_name(), is_obstacle_, fixed_state_, board_);
}

std::vector<Item*> ConductionArea::get_normal_contacts() const
{
    std::vector<Item*> result;
    std::set<Item*, TreeStorableLess> result_set;
    for (int i = 0; i < tile_shape_count(); ++i)
    {
        geometry::planar::IntOctagon current_shape = get_tile_shape(i);
        std::set<SearchTreeObject*, TreeStorableLess> overlaps =
            board_->overlapping_objects(current_shape, get_layer());
        for (SearchTreeObject* p_curr_ob : overlaps)
        {
            auto* curr_item = dynamic_cast<Item*>(p_curr_ob);
            if (curr_item == nullptr || curr_item == this)
            {
                continue;
            }
            if (!curr_item->shares_net(this) || !curr_item->shares_layer(this))
            {
                continue;
            }
            if (const auto* curr_trace = dynamic_cast<const Trace*>(curr_item))
            {
                if (shape_contains_point(current_shape, curr_trace->first_corner())
                    || shape_contains_point(current_shape, curr_trace->last_corner()))
                {
                    result_set.insert(curr_item);
                }
            }
            else if (const auto* curr_drill_item = dynamic_cast<const DrillItem*>(curr_item))
            {
                if (shape_contains_point(current_shape, curr_drill_item->get_center()))
                {
                    result_set.insert(curr_item);
                }
            }
        }
    }
    result.insert(result.end(), result_set.begin(), result_set.end());
    return result;
}

std::vector<std::shared_ptr<geometry::planar::Point>> ConductionArea::get_ratsnest_corners() const
{
    std::vector<std::shared_ptr<geometry::planar::Point>> result;
    std::vector<geometry::planar::FloatPoint> corners = get_area()->corner_approx_arr();
    result.reserve(corners.size());
    for (const geometry::planar::FloatPoint& curr_corner : corners)
    {
        result.push_back(std::make_shared<geometry::planar::IntPoint>(curr_corner.round()));
    }
    return result;
}

std::shared_ptr<geometry::planar::TileShape> ConductionArea::get_trace_connection_shape(
    const datastructures::ShapeTree& p_search_tree, int p_index) const
{
    if (p_index < 0 || p_index >= tree_shape_count(p_search_tree))
    {
        logger::FRLogger::instance().warn("ConductionArea.get_trace_connection_shape p_index out of range");
        return nullptr;
    }
    const geometry::planar::TileShape* p_shape = get_tree_shape(p_search_tree, p_index);
    if (p_shape == nullptr)
    {
        return nullptr;
    }
    return std::shared_ptr<geometry::planar::TileShape>(p_shape->clone());
}

bool ConductionArea::is_obstacle(const Item* p_other) const
{
    if (!is_obstacle_)
    {
        return false;
    }
    return ObstacleArea::is_obstacle(p_other);
}

bool ConductionArea::is_trace_obstacle(int p_net_no) const
{
    return is_obstacle_ && !contains_net(p_net_no);
}

bool ConductionArea::is_drillable(int p_net_no) const
{
    return !is_obstacle_ || contains_net(p_net_no);
}

bool ConductionArea::is_selected_by_filter(const ItemSelectionFilter& p_filter) const
{
    if (!is_selected_by_fixed_filter(p_filter))
    {
        return false;
    }
    return p_filter.is_selected(ItemSelectionFilter::SelectableChoices::CONDUCTION);
}

} // namespace freerouting::board