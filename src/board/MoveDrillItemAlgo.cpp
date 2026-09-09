#include "board/MoveDrillItemAlgo.h"
#include "board/ForcedPadAlgo.h"
#include "board/ShapeTraceEntries.h"
#include "board/ShapeSearchTree.h"
#include "board/SearchTreeManager.h"
#include "board/Via.h"
#include "board/Trace.h"
#include "board/ConductionArea.h"
#include "board/RoutingBoard.h"
#include "board/BasicBoard.h"
#include "rules/BoardRules.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/IntOctagon.h"
#include "geometry/planar/TileShape.h"
#include "geometry/planar/Point.h"
#include "geometry/planar/Vector.h"
#include "geometry/planar/OrthogonalBoundingDirections.h"
#include "geometry/planar/FortyfiveDegreeBoundingDirections.h"
#include "geometry/planar/ShapeBoundingDirections.h"
#include "logger/FRLogger.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>

namespace freerouting::board {

bool MoveDrillItemAlgo::check(DrillItem* p_drill_item,
                              const geometry::planar::Vector& p_vector,
                              int p_max_recursion_depth,
                              int p_max_via_recursion_depth,
                              const std::vector<Item*>& p_ignore_items,
                              RoutingBoard* p_board,
                              const datastructures::TimeLimit* p_time_limit)
{
    if (p_time_limit != nullptr && p_time_limit->limit_exceeded())
    {
        return false;
    }
    if (p_drill_item->is_shove_fixed())
    {
        return false;
    }

    // Check, that p_drillitem is only connected to traces.
    std::vector<Item*> contact_list = p_drill_item->get_normal_contacts();
    for (Item* curr_contact : contact_list)
    {
        if (!(dynamic_cast<Trace*>(curr_contact) || dynamic_cast<ConductionArea*>(curr_contact)))
        {
            return false;
        }
    }

    std::vector<Item*> ignore_items;
    if (p_ignore_items.empty())
    {
        ignore_items = {};
    }
    else
    {
        ignore_items = p_ignore_items;
    }
    ignore_items.push_back(p_drill_item);

    ForcedPadAlgo forced_pad_algo(p_board);
    bool attach_allowed = false;
    if (auto* via = dynamic_cast<Via*>(p_drill_item))
    {
        attach_allowed = via->is_attach_allowed();
    }

    ShapeSearchTree* search_tree = p_board->search_tree_manager().get_default_tree();
    for (int curr_layer = p_drill_item->first_layer(); curr_layer <= p_drill_item->last_layer(); ++curr_layer)
    {
        int curr_ind = curr_layer - p_drill_item->first_layer();
        const geometry::planar::TileShape* curr_shape = p_drill_item->get_tree_shape(*search_tree, curr_ind);
        if (curr_shape == nullptr)
        {
            continue;
        }
        const geometry::planar::ShapeBoundingDirections& dirs = (p_board->get_rules().get_trace_angle_restriction() == rules::AngleRestriction::RIGHT)
            ? static_cast<const geometry::planar::ShapeBoundingDirections&>(geometry::planar::OrthogonalBoundingDirections::INSTANCE)
            : static_cast<const geometry::planar::ShapeBoundingDirections&>(geometry::planar::FortyfiveDegreeBoundingDirections::INSTANCE);
        geometry::planar::IntOctagon shape_oct = curr_shape->bounding_shape(dirs);
        geometry::planar::FloatPoint fp = p_vector.to_float();
        geometry::planar::IntVector int_vec(static_cast<int64_t>(std::round(fp.x)),
                                            static_cast<int64_t>(std::round(fp.y)));
        geometry::planar::IntOctagon translated_oct = shape_oct.translate_by(int_vec);
        CalcFromSide from_side(p_drill_item->get_center(), translated_oct);
        if (forced_pad_algo.check_forced_pad(translated_oct, &from_side, curr_layer,
                                             p_drill_item->get_net_nos(),
                                             p_drill_item->get_clearance_class(),
                                             attach_allowed, ignore_items,
                                             p_max_recursion_depth,
                                             p_max_via_recursion_depth,
                                             true, p_time_limit) == ForcedPadAlgo::CheckDrillResult::NOT_DRILLABLE)
        {
            return false;
        }
    }
    return true;
}

bool MoveDrillItemAlgo::insert(DrillItem* p_drill_item,
                                const geometry::planar::Vector& p_vector,
                                int p_max_recursion_depth,
                                int p_max_via_recursion_depth,
                                const geometry::planar::IntOctagon* p_tidy_region,
                                RoutingBoard* p_board)
{
    if (p_drill_item->is_shove_fixed())
    {
        return false;
    }

    bool attach_allowed = false;
    if (auto* via = dynamic_cast<Via*>(p_drill_item))
    {
        attach_allowed = via->is_attach_allowed();
    }
    ForcedPadAlgo forced_pad_algo(p_board);
    std::vector<Item*> ignore_items;
    ignore_items.push_back(p_drill_item);

    ShapeSearchTree* search_tree = p_board->search_tree_manager().get_default_tree();
    geometry::planar::IntOctagon tidy_region_copy;
    if (p_tidy_region)
    {
        tidy_region_copy = *p_tidy_region;
    }
    for (int curr_layer = p_drill_item->first_layer(); curr_layer <= p_drill_item->last_layer(); ++curr_layer)
    {
        int curr_ind = curr_layer - p_drill_item->first_layer();
        const geometry::planar::TileShape* curr_shape = p_drill_item->get_tree_shape(*search_tree, curr_ind);
        if (curr_shape == nullptr)
        {
            continue;
        }
        const geometry::planar::ShapeBoundingDirections& dirs = (p_board->get_rules().get_trace_angle_restriction() == rules::AngleRestriction::RIGHT)
            ? static_cast<const geometry::planar::ShapeBoundingDirections&>(geometry::planar::OrthogonalBoundingDirections::INSTANCE)
            : static_cast<const geometry::planar::ShapeBoundingDirections&>(geometry::planar::FortyfiveDegreeBoundingDirections::INSTANCE);
        geometry::planar::IntOctagon shape_oct = curr_shape->bounding_shape(dirs);
        geometry::planar::FloatPoint fp = p_vector.to_float();
        geometry::planar::IntVector int_vec(static_cast<int64_t>(std::round(fp.x)),
                                            static_cast<int64_t>(std::round(fp.y)));
        geometry::planar::IntOctagon translated_oct = shape_oct.translate_by(int_vec);
        if (p_tidy_region != nullptr)
        {
            tidy_region_copy = tidy_region_copy.union_with(translated_oct);
        }
        CalcFromSide from_side(p_drill_item->get_center(), translated_oct);
        if (!forced_pad_algo.forced_pad(translated_oct, &from_side, curr_layer,
                                        p_drill_item->get_net_nos(),
                                        p_drill_item->get_clearance_class(),
                                        attach_allowed, ignore_items,
                                        p_max_recursion_depth,
                                        p_max_via_recursion_depth))
        {
            return false;
        }
        geometry::planar::IntBox curr_bounding_box = curr_shape->bounding_box();
        for (int j = 0; j < 4; ++j)
        {
            p_board->join_changed_area(curr_bounding_box.corner_approx(j), curr_layer);
        }
    }
    // Move the drill item by updating its center
    const geometry::planar::IntPoint& old_center = p_drill_item->get_center();
    geometry::planar::FloatPoint fp2 = p_vector.to_float();
    geometry::planar::IntPoint new_center(old_center.x + static_cast<int64_t>(std::round(fp2.x)),
                                          old_center.y + static_cast<int64_t>(std::round(fp2.y)));
    p_drill_item->set_center(new_center);
    return true;
}

bool MoveDrillItemAlgo::shove_vias(const geometry::planar::TileShape& p_obstacle_shape,
                                    const CalcFromSide* p_from_side,
                                    int p_layer,
                                    const std::vector<int>& p_net_no_arr,
                                    int p_cl_type,
                                    const std::vector<Item*>& p_ignore_items,
                                    int p_max_recursion_depth,
                                    int p_max_via_recursion_depth,
                                    bool p_copper_sharing_allowed,
                                    RoutingBoard* p_board)
{
    ShapeSearchTree* search_tree = p_board->search_tree_manager().get_default_tree();
    ShapeTraceEntries shape_entries(p_obstacle_shape, p_layer, p_net_no_arr, p_cl_type, p_from_side, p_board);

    std::vector<int> empty_net_arr;
    std::set<Item*, TreeStorableLess> obstacles_set = search_tree->overlapping_items_with_clearance(p_obstacle_shape, p_layer, empty_net_arr, p_cl_type);
    std::vector<Item*> obstacles(obstacles_set.begin(), obstacles_set.end());

    if (!shape_entries.store_items(obstacles, false, p_copper_sharing_allowed))
    {
        return true;
    }
    if (!p_ignore_items.empty())
    {
        std::vector<Via*> filtered;
        filtered.reserve(shape_entries.shove_via_list.size());
        for (Via* via : shape_entries.shove_via_list)
        {
            if (std::find(p_ignore_items.begin(), p_ignore_items.end(), via) == p_ignore_items.end())
            {
                filtered.push_back(via);
            }
        }
        shape_entries.shove_via_list = std::move(filtered);
    }
    if (shape_entries.shove_via_list.empty())
    {
        return true;
    }
    double shape_radius = 0.5 * p_obstacle_shape.bounding_box().min_width();
    for (Via* curr_via : shape_entries.shove_via_list)
    {
        if (curr_via->shares_net_no(p_net_no_arr))
        {
            continue;
        }
        if (p_max_via_recursion_depth <= 0)
        {
            return true;
        }
std::vector<geometry::planar::IntPoint> try_via_centers = try_shove_via_points(p_obstacle_shape, p_layer, curr_via, p_cl_type, true, p_board);
        geometry::planar::IntPoint* new_via_center = nullptr;
        const geometry::planar::TileShape* via_shape_on_layer = curr_via->get_tree_shape_on_layer(search_tree, p_layer);
        if (!via_shape_on_layer)
        {
            continue;
        }
        double max_dist = 0.5 * via_shape_on_layer->bounding_box().max_width() + shape_radius;
        double max_dist_square = max_dist * max_dist;
        geometry::planar::IntPoint curr_via_center = curr_via->get_center();
        geometry::planar::FloatPoint check_via_center = curr_via_center.to_float();
        int64_t chosen_dx = 0, chosen_dy = 0;
        bool found = false;
        for (size_t i = 0; i < try_via_centers.size(); ++i)
        {
            if (i == 0 || check_via_center.distance_square(try_via_centers[i].to_float()) <= max_dist_square)
            {
                std::vector<Item*> ignore_items;
                if (!p_ignore_items.empty())
                {
                    ignore_items = p_ignore_items;
                }
                int64_t dx = try_via_centers[i].x - curr_via_center.x;
                int64_t dy = try_via_centers[i].y - curr_via_center.y;
                geometry::planar::IntVector rel_coor(dx, dy);
                // No time limit here because the item database is already changed.
                bool shove_ok = check(curr_via, rel_coor, p_max_recursion_depth,
                                      p_max_via_recursion_depth - 1, ignore_items, p_board, nullptr);
                if (shove_ok)
                {
                    new_via_center = const_cast<geometry::planar::IntPoint*>(&try_via_centers[i]);
                    chosen_dx = dx;
                    chosen_dy = dy;
                    found = true;
                    break;
                }
            }
        }
        if (!found)
        {
            continue;
        }
        geometry::planar::IntVector chosen_rel_coor(chosen_dx, chosen_dy);
        if (!insert(curr_via, chosen_rel_coor, p_max_recursion_depth, p_max_via_recursion_depth - 1, nullptr, p_board))
        {
            return false;
        }
    }
    return true;
}

std::vector<geometry::planar::IntPoint> MoveDrillItemAlgo::try_shove_via_points(
    const geometry::planar::TileShape& p_obstacle_shape,
    int p_layer,
    Via* p_via,
    int p_cl_class_no,
    bool p_extended_check,
    RoutingBoard* p_board)
{
    ShapeSearchTree* search_tree = p_board->search_tree_manager().get_default_tree();
    const geometry::planar::TileShape* curr_via_shape = p_via->get_tree_shape_on_layer(search_tree, p_layer);
    if (curr_via_shape == nullptr)
    {
        return {};
    }
    bool is_int_octagon = p_obstacle_shape.is_IntOctagon();
    double clearance_value = p_board->get_rules().get_clearance_matrix().get_value(p_cl_class_no, p_via->get_clearance_class(), p_layer);
    double shove_distance;
    if (p_board->get_rules().get_trace_angle_restriction() == rules::AngleRestriction::RIGHT || is_int_octagon)
    {
        shove_distance = 0.5 * curr_via_shape->bounding_box().max_width();
        if (!search_tree->is_clearance_compensation_used())
        {
            shove_distance += clearance_value;
        }
    }
    else
    {
        // a different algorithm is used for calculating the new via centers
        shove_distance = 0;
        if (!search_tree->is_clearance_compensation_used())
        {
            // enlarge p_obstacle_shape and curr_via_shape by half of the clearance value to synchronize
            // with the check algorithm in ShapeSearchTree.overlapping_tree_entries_with_clearance
            shove_distance += 0.5 * clearance_value;
        }
    }

    // The additional constant 2 is an empirical value for the tolerance in case of diagonal
    // shoving.
    shove_distance += 2;

    geometry::planar::IntPoint curr_via_center = p_via->get_center();
    std::vector<geometry::planar::IntPoint> try_via_centers;
    int try_count = 1;
    if (p_board->get_rules().get_trace_angle_restriction() == rules::AngleRestriction::RIGHT)
    {
        geometry::planar::IntBox curr_offset_box = p_obstacle_shape.bounding_box().offset(static_cast<int>(std::ceil(shove_distance)));
        if (p_extended_check)
        {
            try_count = 2;
        }
        try_via_centers = curr_offset_box.nearest_border_projections(curr_via_center, try_count);
    }
    else if (is_int_octagon)
    {
        const geometry::planar::ShapeBoundingDirections& dirs = geometry::planar::FortyfiveDegreeBoundingDirections::INSTANCE;
        geometry::planar::IntOctagon curr_offset_octagon = p_obstacle_shape.bounding_shape(dirs).enlarge(static_cast<int>(std::ceil(shove_distance)));
        if (p_extended_check)
        {
            try_count = 4;
        }
        try_via_centers = curr_offset_octagon.nearest_border_projections(curr_via_center, try_count);
    }
    else
    {
        std::unique_ptr<geometry::planar::TileShape> curr_offset_shape = p_obstacle_shape.offset(shove_distance);
        const geometry::planar::TileShape* via_shape = curr_via_shape;
        if (!search_tree->is_clearance_compensation_used())
        {
            std::unique_ptr<geometry::planar::TileShape> enlarged_via_shape = curr_via_shape->offset(0.5 * clearance_value);
            via_shape = enlarged_via_shape.get();
        }
        if (p_extended_check)
        {
            try_count = 4;
        }
        // Use nearest_border_projections on the offset shape if it's IntBox or IntOctagon, 
        // otherwise use a simplified approach
        if (curr_offset_shape->is_IntBox())
        {
            const geometry::planar::IntBox& box = static_cast<const geometry::planar::IntBox&>(*curr_offset_shape);
            try_via_centers = box.nearest_border_projections(curr_via_center, try_count);
        }
        else if (curr_offset_shape->is_IntOctagon())
        {
            const geometry::planar::IntOctagon& oct = static_cast<const geometry::planar::IntOctagon&>(*curr_offset_shape);
            try_via_centers = oct.nearest_border_projections(curr_via_center, try_count);
        }
        else
        {
            // Fallback: just return the current center
            try_via_centers.push_back(curr_via_center);
        }
    }
    return try_via_centers;
}

} // namespace freerouting::board