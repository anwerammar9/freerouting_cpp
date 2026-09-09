#include "board/ForcedPadAlgo.h"
#include "board/ShapeTraceEntries.h"
#include "board/ShoveTraceAlgo.h"
#include "board/CalcShapeAndFromSide.h"
#include "board/MoveDrillItemAlgo.h"
#include "board/Via.h"
#include "board/Pin.h"
#include "board/PolylineTrace.h"
#include "board/SearchTreeManager.h"
#include "board/ShapeSearchTree.h"
#include "board/RoutingBoard.h"
#include "board/BasicBoard.h"
#include "geometry/planar/Point.h"
#include "geometry/planar/Vector.h"
#include "geometry/planar/Polyline.h"
#include "geometry/planar/IntOctagon.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/Direction.h"
#include "geometry/planar/OrthogonalBoundingDirections.h"
#include "geometry/planar/FortyfiveDegreeBoundingDirections.h"
#include "geometry/planar/ShapeBoundingDirections.h"
#include "logger/FRLogger.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace freerouting::board {

ForcedPadAlgo::ForcedPadAlgo(RoutingBoard* p_board)
    : board_(p_board)
{
}

ForcedPadAlgo::CheckDrillResult ForcedPadAlgo::check_forced_pad(const geometry::planar::TileShape& p_pad_shape,
                                                                 const CalcFromSide* p_from_side,
                                                                 int p_layer,
                                                                 const std::vector<int>& p_net_no_arr,
                                                                 int p_cl_type,
                                                                 bool p_copper_sharing_allowed,
                                                                 const std::vector<Item*>& p_ignore_items,
                                                                 int p_max_recursion_depth,
                                                                 int p_max_via_recursion_depth,
                                                                 bool p_check_only_front,
                                                                 const datastructures::TimeLimit* p_time_limit)
{
    if (!p_pad_shape.bounding_box().is_contained_in(board_->get_bounding_box()))
    {
        // get_outline() returns const; the failing-obstacle slot only tracks
        // the pointer for diagnostics, so const_cast is safe here.
        board_->set_shove_failing_obstacle(const_cast<BoardOutline*>(board_->get_outline()));
        return CheckDrillResult::NOT_DRILLABLE;
    }

    ShapeSearchTree* search_tree = board_->search_tree_manager().get_default_tree();
    ShapeTraceEntries shape_entries(p_pad_shape, p_layer, p_net_no_arr, p_cl_type, p_from_side, board_);

    std::vector<int> empty_net_arr;
    std::set<Item*, TreeStorableLess> obstacles_set = search_tree->overlapping_items_with_clearance(p_pad_shape, p_layer, empty_net_arr, p_cl_type);
    std::vector<Item*> obstacles(obstacles_set.begin(), obstacles_set.end());

    if (!p_ignore_items.empty())
    {
        std::vector<Item*> filtered_obstacles;
        filtered_obstacles.reserve(obstacles.size());
        for (Item* item : obstacles)
        {
            if (std::find(p_ignore_items.begin(), p_ignore_items.end(), item) == p_ignore_items.end())
            {
                filtered_obstacles.push_back(item);
            }
        }
        obstacles = std::move(filtered_obstacles);
    }

    bool obstacles_shovable = shape_entries.store_items(obstacles, true, p_copper_sharing_allowed);
    if (!obstacles_shovable)
    {
        board_->set_shove_failing_obstacle(shape_entries.get_found_obstacle());
        return CheckDrillResult::NOT_DRILLABLE;
    }

    for (Via* curr_shove_via : shape_entries.shove_via_list)
    {
        if (p_max_via_recursion_depth <= 0)
        {
            board_->set_shove_failing_obstacle(curr_shove_via);
            return CheckDrillResult::NOT_DRILLABLE;
        }

        std::vector<geometry::planar::IntPoint> new_via_center = MoveDrillItemAlgo::try_shove_via_points(
            p_pad_shape, p_layer, curr_shove_via, p_cl_type, false, board_);

        if (new_via_center.empty())
        {
            board_->set_shove_failing_obstacle(curr_shove_via);
            return CheckDrillResult::NOT_DRILLABLE;
        }

        geometry::planar::IntVector delta(
            new_via_center[0].x - curr_shove_via->get_center().x,
            new_via_center[0].y - curr_shove_via->get_center().y);

        std::vector<Item*> ignore_items;
        if (!MoveDrillItemAlgo::check(curr_shove_via, delta, p_max_recursion_depth,
                                      p_max_via_recursion_depth - 1, ignore_items, board_, p_time_limit))
        {
            return CheckDrillResult::NOT_DRILLABLE;
        }
    }

    CheckDrillResult result = CheckDrillResult::DRILLABLE;
    if (p_copper_sharing_allowed)
    {
        for (Item* curr_obstacle : obstacles)
        {
            if (dynamic_cast<Pin*>(curr_obstacle))
            {
                result = CheckDrillResult::DRILLABLE_WITH_ATTACH_SMD;
                break;
            }
        }
    }

    int trace_piece_count = shape_entries.substitute_trace_count();
    if (trace_piece_count == 0)
    {
        return result;
    }

    if (p_max_recursion_depth <= 0)
    {
        board_->set_shove_failing_obstacle(shape_entries.get_found_obstacle());
        return CheckDrillResult::NOT_DRILLABLE;
    }

    if (shape_entries.stack_depth() > 1)
    {
        board_->set_shove_failing_obstacle(shape_entries.get_found_obstacle());
        return CheckDrillResult::NOT_DRILLABLE;
    }

    ShoveTraceAlgo shove_trace_algo(board_);
    bool is_orthogonal_mode = dynamic_cast<const geometry::planar::IntBox*>(&p_pad_shape) != nullptr;

    while (true)
    {
        PolylineTrace* curr_substitute_trace = shape_entries.next_substitute_trace_piece();
        if (curr_substitute_trace == nullptr)
        {
            break;
        }

        for (int i = 0; i < curr_substitute_trace->tile_shape_count(); ++i)
        {
            const geometry::planar::Line& curr_line = curr_substitute_trace->polyline().arr[i + 1];
            const geometry::planar::IntVector curr_int_vec = curr_line.direction();
            std::unique_ptr<geometry::planar::Direction> curr_dir =
                geometry::planar::Direction::get_instance(curr_int_vec);

            bool is_in_front;
            if (p_check_only_front)
            {
                is_in_front = in_front_of_pad(curr_line, p_pad_shape, p_from_side->no,
                                              curr_substitute_trace->get_half_width(), true);
            }
            else
            {
                is_in_front = true;
            }

            if (is_in_front)
            {
                CalcShapeAndFromSide curr(*curr_substitute_trace, i, is_orthogonal_mode, true);
                if (!shove_trace_algo.check(curr.shape(), curr.from_side(), curr_dir.get(), p_layer,
                                            curr_substitute_trace->get_net_nos(),
                                            curr_substitute_trace->get_clearance_class(),
                                            p_max_recursion_depth - 1,
                                            p_max_via_recursion_depth,
                                            0,
                                            p_time_limit))
                {
                    return CheckDrillResult::NOT_DRILLABLE;
                }
            }
        }
    }

    return result;
}

bool ForcedPadAlgo::forced_pad(const geometry::planar::TileShape& p_pad_shape,
                               const CalcFromSide* p_from_side,
                               int p_layer,
                               const std::vector<int>& p_net_no_arr,
                               int p_cl_type,
                               bool p_copper_sharing_allowed,
                               const std::vector<Item*>& p_ignore_items,
                               int p_max_recursion_depth,
                               int p_max_via_recursion_depth)
{
    if (p_pad_shape.is_empty())
    {
        logger::FRLogger::instance().warn("ForcedPadAlgo.forced_pad: p_pad_shape is empty");
        return true;
    }

    if (!p_pad_shape.bounding_box().is_contained_in(board_->get_bounding_box()))
    {
        board_->set_shove_failing_obstacle(const_cast<BoardOutline*>(board_->get_outline()));
        return false;
    }

    if (!MoveDrillItemAlgo::shove_vias(p_pad_shape, p_from_side, p_layer, p_net_no_arr, p_cl_type,
                                       p_ignore_items, p_max_recursion_depth, p_max_via_recursion_depth,
                                       false, board_))
    {
        return false;
    }

    ShapeSearchTree* search_tree = board_->search_tree_manager().get_default_tree();
    ShapeTraceEntries shape_entries(p_pad_shape, p_layer, p_net_no_arr, p_cl_type, p_from_side, board_);

    std::vector<int> empty_net_arr;
    std::set<Item*, TreeStorableLess> obstacles_set = search_tree->overlapping_items_with_clearance(p_pad_shape, p_layer, empty_net_arr, p_cl_type);
    std::vector<Item*> obstacles(obstacles_set.begin(), obstacles_set.end());

    if (!p_ignore_items.empty())
    {
        std::vector<Item*> filtered_obstacles;
        filtered_obstacles.reserve(obstacles.size());
        for (Item* item : obstacles)
        {
            if (std::find(p_ignore_items.begin(), p_ignore_items.end(), item) == p_ignore_items.end())
            {
                filtered_obstacles.push_back(item);
            }
        }
        obstacles = std::move(filtered_obstacles);
    }

    bool obstacles_shovable = shape_entries.store_items(obstacles, true, p_copper_sharing_allowed) && shape_entries.shove_via_list.empty();
    if (!obstacles_shovable)
    {
        board_->set_shove_failing_obstacle(shape_entries.get_found_obstacle());
        return false;
    }

    int trace_piece_count = shape_entries.substitute_trace_count();
    if (trace_piece_count == 0)
    {
        return true;
    }

    if (p_max_recursion_depth <= 0)
    {
        board_->set_shove_failing_obstacle(shape_entries.get_found_obstacle());
        return false;
    }

    bool tails_exist_before = board_->contains_trace_tails(obstacles, p_net_no_arr);
    shape_entries.cutout_traces(obstacles);

    bool is_orthogonal_mode = dynamic_cast<const geometry::planar::IntBox*>(&p_pad_shape) != nullptr;
    ShoveTraceAlgo shove_trace_algo(board_);

    while (true)
    {
        PolylineTrace* curr_substitute_trace = shape_entries.next_substitute_trace_piece();
        if (curr_substitute_trace == nullptr)
        {
            break;
        }

        if (curr_substitute_trace->first_corner().to_float()
            == curr_substitute_trace->last_corner().to_float())
        {
            continue;
        }

        std::vector<int> curr_net_no_arr = curr_substitute_trace->get_net_nos();

        for (int i = 0; i < curr_substitute_trace->tile_shape_count(); ++i)
        {
            CalcShapeAndFromSide curr(*curr_substitute_trace, i, is_orthogonal_mode, false);
            if (!shove_trace_algo.insert(curr.shape(), curr.from_side(), p_layer,
                                         curr_net_no_arr,
                                         curr_substitute_trace->get_clearance_class(),
                                         p_ignore_items,
                                         p_max_recursion_depth - 1,
                                         p_max_via_recursion_depth,
                                         0))
            {
                return false;
            }
        }

        for (int i = 0; i < curr_substitute_trace->corner_count(); ++i)
        {
            board_->join_changed_area(curr_substitute_trace->polyline().corner_approx(i), p_layer);
        }

        const geometry::planar::Point* end_corners[2] = {nullptr, nullptr};
        if (!tails_exist_before)
        {
            end_corners[0] = &curr_substitute_trace->first_corner();
            end_corners[1] = &curr_substitute_trace->last_corner();
        }

        board_->add_item(curr_substitute_trace);

        // TODO: pass the changed-area clip shape once ChangedArea exposes a
        // per-layer shape (Java: board.changed_area.get_area(p_layer)).
        const geometry::planar::IntOctagon* opt_area = nullptr;

        try
        {
            curr_substitute_trace->normalize(opt_area);
        }
        catch (const std::exception& e)
        {
            logger::FRLogger::instance().error("Couldn't normalize trace: " + std::string(e.what()));
        }

        if (!tails_exist_before)
        {
            for (int i = 0; i < 2; ++i)
            {
                if (end_corners[i] != nullptr)
                {
                    Trace* tail = board_->get_trace_tail(*end_corners[i], p_layer, curr_net_no_arr);
                    if (tail != nullptr)
                    {
                        std::vector<Item*> connection_items = tail->get_connection_items(Item::StopConnectionOption::VIA);
                        board_->remove_items(connection_items);
                        for (int curr_net_no : curr_net_no_arr)
                        {
                            board_->combine_traces(curr_net_no);
                        }
                    }
                }
            }
        }
    }

    return true;
}

CalcFromSide ForcedPadAlgo::calc_from_side(const geometry::planar::TileShape& p_shape,
                                                           const geometry::planar::Point& p_shape_center,
                                                           int p_layer,
                                                           int p_offset,
                                                           int p_cl_class)
{
    std::vector<int> empty_arr;
    std::unique_ptr<geometry::planar::TileShape> offset_shape = p_shape.offset(p_offset);

    for (int i = 0; i < offset_shape->border_line_count(); ++i)
    {
        geometry::planar::IntOctagon check_shape = calc_check_shape_for_from_side(p_shape, p_shape_center, offset_shape->border_line(i));

        if (board_->check_trace_shape(check_shape, p_layer, empty_arr, p_cl_class, nullptr))
        {
            // Java passes a null border point here; use origin as placeholder.
            return CalcFromSide(i, geometry::planar::FloatPoint(0.0, 0.0));
        }
    }

    for (int i = 0; i < offset_shape->border_line_count(); ++i)
    {
        geometry::planar::IntOctagon check_shape = calc_check_shape_for_from_side(p_shape, p_shape_center, offset_shape->border_line(i));
        if (board_->check_trace_shape(check_shape, p_layer, empty_arr, 0, nullptr))
        {
            // Java passes a null border point here; use origin as placeholder.
            return CalcFromSide(i, geometry::planar::FloatPoint(0.0, 0.0));
        }
    }

    return CalcFromSide::NOT_CALCULATED;
}

geometry::planar::IntOctagon ForcedPadAlgo::calc_check_shape_for_from_side(const geometry::planar::TileShape& p_shape,
                                                                           const geometry::planar::Point& p_shape_center,
                                                                           const geometry::planar::Line& p_border_line)
{
    geometry::planar::FloatPoint shape_center = p_shape_center.to_float();
    geometry::planar::FloatPoint offset_projection = shape_center.projection_approx(p_border_line);

    geometry::planar::IntPoint center_int;
    if (const auto* center_as_int = dynamic_cast<const geometry::planar::IntPoint*>(&p_shape_center))
    {
        center_int = *center_as_int;
    }
    else
    {
        center_int = shape_center.round();
    }
    geometry::planar::IntPoint offset_int = offset_projection.round();
    geometry::planar::IntVector curr_dir = p_border_line.direction();

    std::vector<geometry::planar::Line> line_arr;
    line_arr.push_back(geometry::planar::Line(center_int, geometry::planar::IntPoint(center_int.x + curr_dir.x, center_int.y + curr_dir.y)));
    line_arr.push_back(geometry::planar::Line(center_int, geometry::planar::IntPoint(center_int.x - curr_dir.y, center_int.y + curr_dir.x)));
    line_arr.push_back(geometry::planar::Line(offset_int, geometry::planar::IntPoint(center_int.x + curr_dir.x, center_int.y + curr_dir.y)));

    geometry::planar::Polyline check_line(line_arr);
    std::unique_ptr<geometry::planar::TileShape> result = check_line.offset_shape(1, 0);
    if (result)
    {
        return result->bounding_shape(geometry::planar::OrthogonalBoundingDirections::INSTANCE);
    }
    return geometry::planar::IntOctagon::EMPTY;
}

bool ForcedPadAlgo::in_front_of_pad(const geometry::planar::Line& p_line,
                                    const geometry::planar::TileShape& p_pad_shape,
                                    int p_from_side,
                                    int p_width,
                                    bool p_with_sides)
{
    if (!p_pad_shape.is_IntOctagon())
    {
        return true;
    }

    geometry::planar::IntOctagon pad_octagon = p_pad_shape.bounding_shape(geometry::planar::FortyfiveDegreeBoundingDirections::INSTANCE);

    const geometry::planar::IntPoint* line_a = dynamic_cast<const geometry::planar::IntPoint*>(&p_line.a);
    const geometry::planar::IntPoint* line_b = dynamic_cast<const geometry::planar::IntPoint*>(&p_line.b);

    if (!line_a || !line_b)
    {
        return true;
    }

    double diag_width = p_width * std::sqrt(2.0);
    bool result = false;

    switch (p_from_side)
    {
        case 0:
        {
            result = std::min(line_a->y, line_b->y) >= pad_octagon.top + p_width
                     || std::max(line_a->x - line_a->y, line_b->x - line_b->y) <= pad_octagon.left_top - diag_width
                     || std::min(line_a->x + line_a->y, line_b->x + line_b->y) >= pad_octagon.right_top + diag_width;
            if (p_with_sides && !result)
            {
                result = std::max(line_a->x, line_b->x) <= pad_octagon.left - p_width
                         && std::min(line_a->x - line_a->y, line_b->x - line_b->y) <= pad_octagon.left_top - diag_width
                         || std::min(line_a->x, line_b->x) >= pad_octagon.right + p_width
                         && std::min(line_a->x + line_a->y, line_b->x + line_b->y) >= pad_octagon.right_top + diag_width;
            }
            break;
        }
        case 1:
        {
            result = std::min(line_a->y, line_b->y) >= pad_octagon.top + p_width
                     || std::max(line_a->x - line_a->y, line_b->x - line_b->y) <= pad_octagon.left_top - diag_width
                     || std::max(line_a->x, line_b->x) <= pad_octagon.left - p_width;
            if (p_with_sides && !result)
            {
                result = std::min(line_a->x, line_b->x) <= pad_octagon.left - p_width
                         && std::max(line_a->x + line_a->y, line_b->x + line_b->y) <= pad_octagon.left_bottom - diag_width
                         || std::max(line_a->y, line_b->y) >= pad_octagon.top + p_width
                         && std::min(line_a->x + line_a->y, line_b->x + line_b->y) >= pad_octagon.right_top + diag_width;
            }
            break;
        }
        case 2:
        {
            result = std::max(line_a->x, line_b->x) <= pad_octagon.left - p_width
                     || std::max(line_a->x - line_a->y, line_b->x - line_b->y) <= pad_octagon.left_top - diag_width
                     || std::max(line_a->x + line_a->y, line_b->x + line_b->y) <= pad_octagon.left_bottom - diag_width;
            if (p_with_sides && !result)
            {
                result = std::max(line_a->y, line_b->y) <= pad_octagon.bottom - p_width
                         && std::min(line_a->x + line_a->y, line_b->x + line_b->y) <= pad_octagon.left_bottom - diag_width
                         || std::min(line_a->y, line_b->y) >= pad_octagon.top + p_width
                         && std::min(line_a->x - line_a->y, line_b->x - line_b->y) <= pad_octagon.left_top - diag_width;
            }
            break;
        }
        case 3:
        {
            result = std::max(line_a->x, line_b->x) <= pad_octagon.left - p_width
                     || std::max(line_a->y, line_b->y) <= pad_octagon.bottom - p_width
                     || std::max(line_a->x + line_a->y, line_b->x + line_b->y) <= pad_octagon.left_bottom - diag_width;
            if (p_with_sides && !result)
            {
                result = std::min(line_a->y, line_b->y) <= pad_octagon.bottom - p_width
                         && std::min(line_a->x - line_a->y, line_b->x - line_b->y) >= pad_octagon.right_bottom + diag_width
                         || std::min(line_a->x, line_b->x) <= pad_octagon.left - p_width
                         && std::max(line_a->x - line_a->y, line_b->x - line_b->y) <= pad_octagon.left_top - diag_width;
            }
            break;
        }
        case 4:
        {
            result = std::max(line_a->y, line_b->y) <= pad_octagon.bottom - p_width
                     || std::max(line_a->x + line_a->y, line_b->x + line_b->y) <= pad_octagon.left_bottom - diag_width
                     || std::min(line_a->x - line_a->y, line_b->x - line_b->y) >= pad_octagon.right_bottom + diag_width;
            if (p_with_sides && !result)
            {
                result = std::min(line_a->x, line_b->x) >= pad_octagon.right + p_width
                         && std::max(line_a->x - line_a->y, line_b->x - line_b->y) >= pad_octagon.right_bottom + diag_width
                         || std::max(line_a->x, line_b->x) <= pad_octagon.left - p_width
                         && std::min(line_a->x + line_a->y, line_b->x + line_b->y) <= pad_octagon.left_bottom - diag_width;
            }
            break;
        }
        case 5:
        {
            result = std::max(line_a->y, line_b->y) <= pad_octagon.bottom - p_width
                     || std::min(line_a->x, line_b->x) >= pad_octagon.right + p_width
                     || std::min(line_a->x - line_a->y, line_b->x - line_b->y) >= pad_octagon.right_bottom + diag_width;
            if (p_with_sides && !result)
            {
                result = std::max(line_a->x, line_b->x) >= pad_octagon.right + p_width
                         && std::min(line_a->x + line_a->y, line_b->x + line_b->y) >= pad_octagon.right_top + diag_width
                         || std::min(line_a->y, line_b->y) <= pad_octagon.bottom - p_width
                         && std::max(line_a->x + line_a->y, line_b->x + line_b->y) <= pad_octagon.left_bottom - diag_width;
            }
            break;
        }
        case 6:
        {
            result = std::min(line_a->x, line_b->x) >= pad_octagon.right + p_width
                     || std::min(line_a->x + line_a->y, line_b->x + line_b->y) >= pad_octagon.right_top + diag_width
                     || std::min(line_a->x - line_a->y, line_b->x - line_b->y) >= pad_octagon.right_bottom + diag_width;
            if (p_with_sides && !result)
            {
                result = std::max(line_a->y, line_b->y) <= pad_octagon.bottom - p_width
                         && std::max(line_a->x - line_a->y, line_b->x - line_b->y) >= pad_octagon.right_bottom + diag_width
                         || std::min(line_a->y, line_b->y) >= pad_octagon.top + p_width
                         && std::max(line_a->x + line_a->y, line_b->x + line_b->y) >= pad_octagon.right_top + diag_width;
            }
            break;
        }
        case 7:
        {
            result = std::min(line_a->y, line_b->y) >= pad_octagon.top + p_width
                     || std::min(line_a->x + line_a->y, line_b->x + line_b->y) >= pad_octagon.right_top + diag_width
                     || std::min(line_a->x, line_b->x) >= pad_octagon.right + p_width;
            if (p_with_sides && !result)
            {
                result = std::max(line_a->y, line_b->y) >= pad_octagon.top + p_width
                         && std::max(line_a->x - line_a->y, line_b->x - line_b->y) <= pad_octagon.left_top - diag_width
                         || std::max(line_a->x, line_b->x) >= pad_octagon.right + p_width
                         && std::min(line_a->x - line_a->y, line_b->x - line_b->y) >= pad_octagon.right_bottom + diag_width;
            }
            break;
        }
        default:
        {
            logger::FRLogger::instance().warn("ForcedPadAlgo.in_front_of_pad: p_from_side out of range");
            result = true;
            break;
        }
    }

    return result;
}

} // namespace freerouting::board