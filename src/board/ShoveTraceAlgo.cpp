#include "board/ShoveTraceAlgo.h"

#include "board/BoardOutline.h"
#include "board/CalcFromSide.h"
#include "board/CalcShapeAndFromSide.h"
#include "board/RoutingBoard.h"
#include "board/SearchTreeManager.h"
#include "board/ShapeSearchTree.h"
#include "board/ShapeTraceEntries.h"
#include "board/MoveDrillItemAlgo.h"
#include "board/FixedState.h"
#include "board/ChangedArea.h"
#include "board/Trace.h"
#include "board/PolylineTrace.h"
#include "board/Via.h"
#include "board/Pin.h"
#include "board/ConductionArea.h"
#include "board/ObstacleArea.h"
#include "board/ViaObstacleArea.h"
#include "board/ComponentObstacleArea.h"
#include "board/Item.h"
#include "board/SearchTreeObject.h"
#include "geometry/planar/LineSegment.h"
#include "geometry/planar/Polyline.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/IntOctagon.h"
#include "geometry/planar/TileShape.h"
#include "geometry/planar/Direction.h"
#include "datastructures/TimeLimit.h"
#include "logger/FRLogger.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace freerouting::board
{

namespace
{

/**
 * Java RoutingBoard.removeItems() unlinks the items from the item list and
 * evicts their search tree entries (BoardItemRepository.removeItem). The C++
 * RoutingBoard::remove_items() only unlinks the item list, so the tree
 * eviction is done here, following the pattern of PolylineTrace::split().
 */
void remove_items_from_board(RoutingBoard* p_board, const std::vector<Item*>& p_items)
{
    if (p_board == nullptr) return;
    for (Item* item : p_items)
    {
        if (item == nullptr) continue;
        p_board->remove_item(item);
        p_board->get_search_tree_manager()->remove(item);
    }
}

} // namespace

ShoveTraceAlgo::ShoveTraceAlgo(RoutingBoard* p_board) : board_(p_board) {}

double ShoveTraceAlgo::check(RoutingBoard* p_board,
                             const geometry::planar::LineSegment& p_line_segment,
                             bool p_shove_to_the_left,
                             int p_layer,
                             const std::vector<int>& p_net_no_arr,
                             int p_trace_half_width,
                             int p_cl_type,
                             int p_max_recursion_depth,
                             int p_max_via_recursion_depth)
{
    // Port of Java TraceShover.check(RoutingBoard, LineSegment, ...) at
    // TraceShover.java:57-225.
    if (!p_board) return 0;
    auto* search_tree = p_board->get_search_tree_manager()->get_default_tree();
    int half_width = p_trace_half_width;
    if (search_tree && search_tree->is_clearance_compensation_used())
    {
        half_width += search_tree->clearance_compensation_value(p_cl_type, p_layer);
    }
    geometry::planar::Polyline polyline = p_line_segment.to_polyline();
    std::vector<std::unique_ptr<geometry::planar::TileShape>> trace_shapes = polyline.offset_shapes(half_width);
    if (trace_shapes.size() != 1)
    {
        logger::FRLogger::instance().warn("ShoveTraceAlgo.check: trace_shape count 1 expected");
        return 0;
    }
    const geometry::planar::TileShape* trace_shape = trace_shapes[0].get();
    if (trace_shape->is_empty())
    {
        logger::FRLogger::instance().warn("ShoveTraceAlgo.check: trace_shape is empty");
        return 0;
    }
    if (!trace_shape->is_contained_in(p_board->get_bounding_box()))
    {
        return 0;
    }
    CalcFromSide from_side(p_line_segment, *trace_shape, p_shove_to_the_left);
    ShapeTraceEntries entries(*trace_shape, p_layer, p_net_no_arr, p_cl_type, &from_side, p_board);
    std::vector<Item*> obstacles_vec = to_item_vector(search_tree, *trace_shape, p_layer, p_cl_type);
    if (!entries.store_items(obstacles_vec, false, true) || entries.trace_tails_in_shape())
    {
        return 0;
    }
    const int trace_piece_count = entries.substitute_trace_count();
    if (entries.stack_depth() > 1)
    {
        return 0;
    }

    const geometry::planar::FloatPoint start_corner_approx = p_line_segment.start_point_approx();
    const geometry::planar::FloatPoint end_corner_approx = p_line_segment.end_point_approx();
    const double segment_length = end_corner_approx.distance(start_corner_approx);

    const auto& cl_matrix = p_board->get_rules().get_clearance_matrix();
    double result = std::numeric_limits<int>::max();

    // check, if the obstacle vias can be shoved
    for (Via* current_shove_via : entries.shove_via_list)
    {
        if (current_shove_via == nullptr) continue;
        if (current_shove_via->shares_net_no(p_net_no_arr))
        {
            continue;
        }
        bool shove_via_ok = false;
        if (p_max_via_recursion_depth > 0)
        {
            std::vector<geometry::planar::IntPoint> new_via_centers =
                MoveDrillItemAlgo::try_shove_via_points(*trace_shape, p_layer, current_shove_via,
                                                         p_cl_type, false, p_board);
            if (new_via_centers.empty())
            {
                return 0;
            }
            std::unique_ptr<geometry::planar::Vector> delta =
                new_via_centers[0].difference_by(current_shove_via->get_center());
            std::vector<Item*> ignore_items;
            if (delta != nullptr &&
                MoveDrillItemAlgo::check(current_shove_via, *delta, p_max_recursion_depth,
                                         p_max_via_recursion_depth - 1, ignore_items, p_board,
                                         nullptr))
            {
                shove_via_ok = true;
            }
        }
        if (!shove_via_ok)
        {
            // The via cannot be shoved aside, so the shove is only possible up
            // to the near border of the via.
            geometry::planar::FloatPoint via_center_approx =
                current_shove_via->get_center().to_float();
            double projection = start_corner_approx.scalar_product(end_corner_approx, via_center_approx);
            projection /= segment_length;
            const geometry::planar::TileShape* via_tree_shape =
                current_shove_via->get_tree_shape_on_layer(search_tree, p_layer);
            double via_radius =
                via_tree_shape != nullptr ? 0.5 * via_tree_shape->bounding_box().max_width() : 0.0;
            double current_ok_length = projection - via_radius - half_width;
            if (!search_tree->is_clearance_compensation_used())
            {
                current_ok_length -= cl_matrix.get_value(p_cl_type, current_shove_via->get_clearance_class(),
                                                         p_layer, true);
            }
            if (current_ok_length <= 0)
            {
                return 0;
            }
            result = std::min(result, current_ok_length);
        }
    }
    if (trace_piece_count == 0)
    {
        return result;
    }
    if (p_max_recursion_depth <= 0)
    {
        return 0;
    }

    // check the segments of the substitute traces in front of the input line
    std::unique_ptr<geometry::planar::Direction> line_direction =
        geometry::planar::Direction::get_instance(p_line_segment.get_line().direction());
    for (;;)
    {
        std::unique_ptr<PolylineTrace> current_substitute_trace(entries.next_substitute_trace_piece());
        if (current_substitute_trace == nullptr)
        {
            break;
        }
        for (int i = 0; i < current_substitute_trace->tile_shape_count(); ++i)
        {
            geometry::planar::LineSegment current_line_segment(current_substitute_trace->polyline(), i + 1);
            if (p_shove_to_the_left)
            {
                // swap the line segment to get the correct shove length in
                // case it is smaller than the length of the whole line segment
                current_line_segment = current_line_segment.opposite();
            }
            std::unique_ptr<geometry::planar::Direction> current_direction =
                geometry::planar::Direction::get_instance(current_line_segment.get_line().direction());
            const bool is_in_front = line_direction->equals(current_direction.get());
            if (!is_in_front)
            {
                continue;
            }
            double shove_ok_length =
                check(p_board, current_line_segment, p_shove_to_the_left, p_layer,
                      current_substitute_trace->get_net_nos(), current_substitute_trace->get_half_width(),
                      current_substitute_trace->get_clearance_class(), p_max_recursion_depth - 1,
                      p_max_via_recursion_depth);
            if (shove_ok_length < std::numeric_limits<int>::max())
            {
                if (shove_ok_length <= 0)
                {
                    return 0;
                }
                double projection =
                    std::min(start_corner_approx.scalar_product(end_corner_approx,
                                                               current_line_segment.start_point_approx()),
                             start_corner_approx.scalar_product(end_corner_approx,
                                                               current_line_segment.end_point_approx()));
                projection /= segment_length;
                double current_ok_length = shove_ok_length + projection - half_width -
                                           current_substitute_trace->get_half_width();
                if (search_tree->is_clearance_compensation_used())
                {
                    current_ok_length -= search_tree->clearance_compensation_value(
                        current_substitute_trace->get_clearance_class(), p_layer);
                }
                else
                {
                    current_ok_length -= cl_matrix.get_value(p_cl_type,
                                                            current_substitute_trace->get_clearance_class(),
                                                            p_layer, true);
                }
                if (current_ok_length <= 0)
                {
                    return 0;
                }
                result = std::min(current_ok_length, result);
            }
            break;
        }
    }
    return result;
}

bool ShoveTraceAlgo::check(const geometry::planar::TileShape& p_trace_shape,
                           const CalcFromSide* p_from_side,
                           const geometry::planar::Direction* p_dir,
                           int p_layer,
                           const std::vector<int>& p_net_no_arr,
                           int p_cl_type,
                           int p_max_recursion_depth,
                           int p_max_via_recursion_depth,
                           int p_max_spring_over_recursion_depth,
                           const datastructures::TimeLimit* p_time_limit)
{
    // Port of Java TraceShover.check(TileShape, ShapeEntrySide, Direction, ...)
    // at TraceShover.java:231-411.
    if (p_time_limit != nullptr && p_time_limit->limit_exceeded()) return false;
    if (p_trace_shape.is_empty())
    {
        logger::FRLogger::instance().warn("ShoveTraceAlgo.check: p_trace_shape is empty");
        return true;
    }
    if (!p_trace_shape.is_contained_in(board_->get_bounding_box()))
    {
        // Java board.setShoveFailingObstacle(board.getOutline()); the C++
        // BoardOutline is not a board::Item, so no obstacle can be reported.
        board_->set_shove_failing_obstacle(nullptr);
        return false;
    }
    ShapeTraceEntries entries(p_trace_shape, p_layer, p_net_no_arr, p_cl_type, p_from_side, board_);
    auto* search_tree = board_->get_search_tree_manager()->get_default_tree();
    std::vector<Item*> obstacles_vec = to_item_vector(search_tree, p_trace_shape, p_layer, p_cl_type);
    obstacles_vec = remove_items(obstacles_vec, get_ignore_items_at_tie_pins(p_trace_shape, p_layer, p_net_no_arr));
    if (!entries.store_items(obstacles_vec, false, true))
    {
        board_->set_shove_failing_obstacle(entries.get_found_obstacle());
        return false;
    }
    const int trace_piece_count = entries.substitute_trace_count();
    if (entries.stack_depth() > 1)
    {
        board_->set_shove_failing_obstacle(entries.get_found_obstacle());
        return false;
    }
    const double shape_radius = 0.5 * p_trace_shape.bounding_box().min_width();

    // check, if the obstacle vias can be shoved
    for (Via* current_shove_via : entries.shove_via_list)
    {
        if (current_shove_via == nullptr) continue;
        if (current_shove_via->shares_net_no(p_net_no_arr))
        {
            continue;
        }
        if (p_max_via_recursion_depth <= 0)
        {
            board_->set_shove_failing_obstacle(current_shove_via);
            return false;
        }
        geometry::planar::FloatPoint current_shove_via_center = current_shove_via->get_center().to_float();
        std::vector<geometry::planar::IntPoint> try_via_centers =
            MoveDrillItemAlgo::try_shove_via_points(p_trace_shape, p_layer, current_shove_via,
                                                    p_cl_type, true, board_);
        const geometry::planar::TileShape* via_tree_shape =
            current_shove_via->get_tree_shape_on_layer(search_tree, p_layer);
        double max_dist = (via_tree_shape != nullptr
                               ? 0.5 * via_tree_shape->bounding_box().max_width()
                               : 0.0) +
                          shape_radius;
        const double max_dist_square = max_dist * max_dist;
        bool shove_via_ok = false;
        for (size_t i = 0; i < try_via_centers.size(); ++i)
        {
            if (i == 0 ||
                current_shove_via_center.distance_square(try_via_centers[i].to_float()) <= max_dist_square)
            {
                std::unique_ptr<geometry::planar::Vector> delta =
                    try_via_centers[i].difference_by(current_shove_via->get_center());
                std::vector<Item*> ignore_items;
                if (delta != nullptr &&
                    MoveDrillItemAlgo::check(current_shove_via, *delta, p_max_recursion_depth,
                                             p_max_via_recursion_depth - 1, ignore_items, board_,
                                             p_time_limit))
                {
                    shove_via_ok = true;
                    break;
                }
            }
        }
        if (!shove_via_ok)
        {
            return false;
        }
    }

    if (trace_piece_count == 0)
    {
        return true;
    }
    if (p_max_recursion_depth <= 0)
    {
        board_->set_shove_failing_obstacle(entries.get_found_obstacle());
        return false;
    }

    const bool is_orthogonal_mode = p_trace_shape.is_IntBox();
    for (;;)
    {
        std::unique_ptr<PolylineTrace> current_substitute_trace(entries.next_substitute_trace_piece());
        if (current_substitute_trace == nullptr)
        {
            break;
        }
        if (p_max_spring_over_recursion_depth > 0)
        {
            SpringOverResult spring_over_result =
                spring_over(current_substitute_trace->polyline(),
                            current_substitute_trace->get_compensated_half_width(*search_tree), p_layer,
                            current_substitute_trace->get_net_nos(),
                            current_substitute_trace->get_clearance_class(), false,
                            p_max_spring_over_recursion_depth, nullptr);
            if (!spring_over_result.polyline)
            {
                // spring_over did not work
                return false;
            }
            if (spring_over_result.changed)
            {
                // spring_over changed something
                --p_max_spring_over_recursion_depth;
                current_substitute_trace->change(
                    std::make_shared<geometry::planar::Polyline>(*spring_over_result.polyline));
            }
        }
        for (int i = 0; i < current_substitute_trace->tile_shape_count(); ++i)
        {
            const auto& current_lines = current_substitute_trace->polyline().arr;
            if (i + 1 >= static_cast<int>(current_lines.size()))
            {
                continue;
            }
            std::unique_ptr<geometry::planar::Direction> current_direction =
                geometry::planar::Direction::get_instance(current_lines[i + 1].direction());
            const bool is_in_front = p_dir == nullptr || p_dir->equals(current_direction.get());
            if (is_in_front)
            {
                CalcShapeAndFromSide current(*current_substitute_trace, i, is_orthogonal_mode, true);
                if (!check(current.shape(), current.from_side(), current_direction.get(), p_layer,
                           current_substitute_trace->get_net_nos(),
                           current_substitute_trace->get_clearance_class(), p_max_recursion_depth - 1,
                           p_max_via_recursion_depth, p_max_spring_over_recursion_depth, p_time_limit))
                {
                    return false;
                }
            }
        }
    }
    return true;
}

bool ShoveTraceAlgo::insert(const geometry::planar::TileShape& p_trace_shape,
                           const CalcFromSide* p_from_side,
                           int p_layer,
                           const std::vector<int>& p_net_no_arr,
                           int p_cl_type,
                           const std::vector<Item*>& p_ignore_items,
                           int p_max_recursion_depth,
                           int p_max_via_recursion_depth,
                           int p_max_spring_over_recursion_depth)
{
    // Port of Java TraceShover.insert(...) at TraceShover.java:417-590.
    if (p_trace_shape.is_empty())
    {
        logger::FRLogger::instance().warn("ShoveTraceAlgo.insert: shape empty");
        return true;
    }
    if (!p_trace_shape.is_contained_in(board_->get_bounding_box()))
    {
        // Java board.setShoveFailingObstacle(board.getOutline()); the C++
        // BoardOutline is not a board::Item, so no obstacle can be reported.
        board_->set_shove_failing_obstacle(nullptr);
        return false;
    }
    if (!MoveDrillItemAlgo::shove_vias(p_trace_shape, p_from_side, p_layer, p_net_no_arr, p_cl_type,
                                       p_ignore_items, p_max_recursion_depth, p_max_via_recursion_depth,
                                       true, board_))
    {
        return false;
    }
    ShapeTraceEntries entries(p_trace_shape, p_layer, p_net_no_arr, p_cl_type, p_from_side, board_);
    auto* search_tree = board_->get_search_tree_manager()->get_default_tree();
    std::vector<Item*> obstacles_vec = to_item_vector(search_tree, p_trace_shape, p_layer, p_cl_type);
    obstacles_vec = remove_items(obstacles_vec, get_ignore_items_at_tie_pins(p_trace_shape, p_layer, p_net_no_arr));
    const bool obstacles_shovable = entries.store_items(obstacles_vec, false, true);
    if (!entries.shove_via_list.empty())
    {
        board_->set_shove_failing_obstacle(entries.shove_via_list.front());
        return false;
    }
    if (!obstacles_shovable)
    {
        board_->set_shove_failing_obstacle(entries.get_found_obstacle());
        return false;
    }
    if (entries.substitute_trace_count() == 0)
    {
        return true;
    }
    if (p_max_recursion_depth <= 0)
    {
        board_->set_shove_failing_obstacle(entries.get_found_obstacle());
        return false;
    }
    const bool tails_exist_before = board_->contains_trace_tails(obstacles_vec, p_net_no_arr);
    entries.cutout_traces(obstacles_vec);
    const bool is_orthogonal_mode = p_trace_shape.is_IntBox();
    for (;;)
    {
        std::unique_ptr<PolylineTrace> current_substitute_trace(entries.next_substitute_trace_piece());
        if (current_substitute_trace == nullptr)
        {
            break;
        }
        if (current_substitute_trace->first_corner().to_float() ==
            current_substitute_trace->last_corner().to_float())
        {
            continue;
        }
        if (p_max_spring_over_recursion_depth > 0)
        {
            SpringOverResult spring_over_result =
                spring_over(current_substitute_trace->polyline(),
                            current_substitute_trace->get_compensated_half_width(*search_tree), p_layer,
                            current_substitute_trace->get_net_nos(),
                            current_substitute_trace->get_clearance_class(), false,
                            p_max_spring_over_recursion_depth, nullptr);
            if (!spring_over_result.polyline)
            {
                // spring_over did not work
                return false;
            }
            if (spring_over_result.changed)
            {
                // spring_over changed something
                --p_max_spring_over_recursion_depth;
                current_substitute_trace->change(
                    std::make_shared<geometry::planar::Polyline>(*spring_over_result.polyline));
            }
        }
        const std::vector<int> current_net_nos = current_substitute_trace->get_net_nos();
        for (int i = 0; i < current_substitute_trace->tile_shape_count(); ++i)
        {
            CalcShapeAndFromSide current(*current_substitute_trace, i, is_orthogonal_mode, false);
            if (!insert(current.shape(), current.from_side(), p_layer, current_net_nos,
                        current_substitute_trace->get_clearance_class(), p_ignore_items,
                        p_max_recursion_depth - 1, p_max_via_recursion_depth,
                        p_max_spring_over_recursion_depth))
            {
                return false;
            }
        }
        for (int i = 0; i < current_substitute_trace->corner_count(); ++i)
        {
            board_->join_changed_area(current_substitute_trace->polyline().corner_approx(i), p_layer);
        }
        PolylineTrace* inserted_trace =
            board_->insert_trace_without_cleaning(std::make_shared<geometry::planar::Polyline>(
                                                     current_substitute_trace->polyline()),
                                                 p_layer, current_substitute_trace->get_half_width(),
                                                 current_net_nos, current_substitute_trace->get_clearance_class(),
                                                 FixedState::NOT_FIXED);
        if (inserted_trace != nullptr)
        {
            if (const ChangedArea* changed_area = board_->changed_area())
            {
                geometry::planar::IntOctagon changed_area_layer = changed_area->get_area(p_layer);
                inserted_trace->normalize(&changed_area_layer);
            }
        }
        if (!tails_exist_before)
        {
            // Java Point[] endCorners = { firstCorner, lastCorner }
            const geometry::planar::Point* end_corners[2] = {&current_substitute_trace->first_corner(),
                                                            &current_substitute_trace->last_corner()};
            for (int i = 0; i < 2; ++i)
            {
                Trace* tail = board_->get_trace_tail(*end_corners[i], p_layer, current_net_nos);
                if (tail != nullptr)
                {
                    remove_items_from_board(
                        board_, tail->get_connection_items(Item::StopConnectionOption::VIA));
                    for (int current_net_no : current_net_nos)
                    {
                        board_->combine_traces(current_net_no);
                    }
                }
            }
        }
    }
    return true;
}

std::vector<Item*> ShoveTraceAlgo::to_item_vector(ShapeSearchTree* p_search_tree,
                                                 const geometry::planar::TileShape& p_shape,
                                                 int p_layer,
                                                 int p_cl_type)
{
    if (p_search_tree == nullptr) return {};
    const std::set<Item*, TreeStorableLess> obstacles =
        p_search_tree->overlapping_items_with_clearance(p_shape, p_layer, {}, p_cl_type);
    return std::vector<Item*>(obstacles.begin(), obstacles.end());
}

std::vector<Item*> ShoveTraceAlgo::remove_items(const std::vector<Item*>& p_item_arr,
                                                const std::vector<Item*>& p_removed_arr)
{
    std::vector<Item*> result = p_item_arr;
    for (Item* removed : p_removed_arr)
    {
        result.erase(std::remove(result.begin(), result.end(), removed), result.end());
    }
    return result;
}

std::vector<Item*> ShoveTraceAlgo::get_ignore_items_at_tie_pins(const geometry::planar::TileShape& p_trace_shape,
                                                                 int p_layer,
                                                                 const std::vector<int>& p_net_no_arr)
{
    // Port of Java TraceShover.getIgnoreItemsAtTiePins().
    std::vector<Item*> result;
    if (!board_) return result;
    std::set<SearchTreeObject*, TreeStorableLess> overlapping =
        board_->overlapping_objects(p_trace_shape, p_layer);
    for (auto* obj : overlapping)
    {
        auto* item = dynamic_cast<Item*>(obj);
        auto* pin = dynamic_cast<Pin*>(item);
        if (pin != nullptr && pin->shares_net_no(p_net_no_arr))
        {
            for (Item* contact : pin->get_all_contacts(p_layer))
            {
                result.push_back(contact);
            }
        }
    }
    return result;
}

ShoveTraceAlgo::SpringOverResult ShoveTraceAlgo::spring_over(const geometry::planar::Polyline& p_polyline,
                                                            int p_half_width,
                                                            int p_layer,
                                                            const std::vector<int>& p_net_no_arr,
                                                            int p_cl_type,
                                                            bool p_over_connected_pins,
                                                            int p_recursion_depth,
                                                            const std::set<Pin*>* p_contact_pins)
{
        // Port of Java TraceShover.springOver(): checks for obstacles in the way of
    // p_polyline and tries to wrap the polyline around the first one found in
    // counterclock sense.
    SpringOverResult result;
    Item* found_obstacle = nullptr;
    geometry::planar::IntBox found_obstacle_bounding_box = geometry::planar::IntBox::EMPTY;
    ShapeSearchTree* search_tree = board_->get_search_tree_manager()->get_default_tree();
    std::vector<int> check_net_no_arr;
    if (p_contact_pins == nullptr)
    {
        check_net_no_arr = p_net_no_arr;
    }
    else
    {
        check_net_no_arr.clear();
    }
    for (int i = 0; i < static_cast<int>(p_polyline.arr.size()) - 2; i++)
    {
        std::unique_ptr<geometry::planar::TileShape> current_shape = p_polyline.offset_shape(p_half_width, i);
        if (current_shape == nullptr)
        {
            continue;
        }
        std::set<Item*, TreeStorableLess> obstacles =
            search_tree->overlapping_items_with_clearance(*current_shape, p_layer, check_net_no_arr,
                                                          p_cl_type);
        for (Item* current_item : obstacles)
        {
            bool is_obstacle;
            if (current_item->shares_net_no(p_net_no_arr))
            {
                // to avoid acid traps
                const auto* current_pin = dynamic_cast<const Pin*>(current_item);
                is_obstacle = current_pin != nullptr && p_contact_pins != nullptr &&
                              p_contact_pins->count(const_cast<Pin*>(current_pin)) == 0;
            }
            else if (const auto* current_area = dynamic_cast<const ConductionArea*>(current_item))
            {
                is_obstacle = current_area->get_is_obstacle();
            }
            else if (dynamic_cast<const ViaObstacleArea*>(current_item) != nullptr ||
                     dynamic_cast<const ComponentObstacleArea*>(current_item) != nullptr)
            {
                is_obstacle = false;
            }
            else if (dynamic_cast<const PolylineTrace*>(current_item) != nullptr)
            {
                if (current_item->is_shove_fixed())
                {
                    is_obstacle = true;
                    // check for a shove fixed trace exit stub, which has to be
                    // ignored at a tie pin.
                    for (Item* current_contact : current_item->get_normal_contacts())
                    {
                        if (current_contact->shares_net_no(p_net_no_arr))
                        {
                            is_obstacle = false;
                        }
                    }
                }
                else
                {
                    // an unfixed trace can be pushed aside eventually
                    is_obstacle = false;
                }
            }
            else
            {
                // an unfixed via can be pushed aside eventually
                is_obstacle = !current_item->is_routable();
            }

            if (is_obstacle)
            {
                if (found_obstacle == nullptr)
                {
                    found_obstacle = current_item;
                    found_obstacle_bounding_box = current_item->get_bounding_box();
                }
                else if (found_obstacle != current_item)
                {
                    // check, if 1 obstacle is contained in the other obstacle and
                    // take the bigger obstacle in this case. That may happen in
                    // case of fixed vias inside of pins.
                    const geometry::planar::IntBox current_item_bounding_box = current_item->get_bounding_box();
                    if (found_obstacle_bounding_box.intersects(current_item_bounding_box))
                    {
                        if (current_item_bounding_box.contains(found_obstacle_bounding_box))
                        {
                            found_obstacle = current_item;
                            found_obstacle_bounding_box = current_item_bounding_box;
                        }
                        else if (!found_obstacle_bounding_box.contains(current_item_bounding_box))
                        {
                            // 2 intersecting obstacles which do not contain
                            // each other: spring over is not possible
                            return result;
                        }
                    }
                }
            }
        }
        if (found_obstacle != nullptr)
        {
            break;
        }
    }
    if (found_obstacle == nullptr)
    {
        // no obstacle in the way, nothing to do
        result.polyline = p_polyline;
        result.changed = false;
        return result;
    }

    // Java additionally rejects the board outline here. The C++ BoardOutline is
    // not a board::Item and therefore can never be the found obstacle.
    if (p_recursion_depth <= 0 ||
        (dynamic_cast<const Trace*>(found_obstacle) != nullptr && !found_obstacle->is_shove_fixed()))
    {
        board_->set_shove_failing_obstacle(found_obstacle);
        return result;
    }

    bool try_spring_over = true;
    if (!p_over_connected_pins)
    {
        // check if the obstacle has a trace contact on p_layer
        for (Item* current_contact : found_obstacle->get_all_contacts(p_layer))
        {
            if (dynamic_cast<const Trace*>(current_contact) != nullptr)
            {
                try_spring_over = false;
                break;
            }
        }
    }
    const geometry::planar::TileShape* obstacle_shape = nullptr;
    if (try_spring_over)
    {
        if (dynamic_cast<const ObstacleArea*>(found_obstacle) != nullptr ||
            dynamic_cast<const Trace*>(found_obstacle) != nullptr)
        {
            if (found_obstacle->tree_shape_count(*search_tree) == 1)
            {
                obstacle_shape = found_obstacle->get_tree_shape(*search_tree, 0);
            }
            else
            {
                try_spring_over = false;
            }
        }
        else if (const auto* found_drill_item = dynamic_cast<const DrillItem*>(found_obstacle))
        {
            obstacle_shape = found_drill_item->get_tree_shape_on_layer(search_tree, p_layer);
        }
    }
    if (!try_spring_over || obstacle_shape == nullptr)
    {
        board_->set_shove_failing_obstacle(found_obstacle);
        return result;
    }

    std::unique_ptr<geometry::planar::TileShape> offset_shape;
    if (search_tree->is_clearance_compensation_used())
    {
        offset_shape = obstacle_shape->offset(p_half_width + 1);
    }
    else
    {
        // enlarge the shape in 2 steps for symmetry reasons
        const int offset = p_half_width + 1;
        const double half_cl_offset =
            0.5 * board_->clearance_value(found_obstacle->get_clearance_class(), p_cl_type, p_layer);
        offset_shape = obstacle_shape->offset(offset + half_cl_offset);
        if (offset_shape != nullptr)
        {
            offset_shape = offset_shape->offset(half_cl_offset);
        }
    }
    if (offset_shape == nullptr)
    {
        board_->set_shove_failing_obstacle(found_obstacle);
        return result;
    }
    const rules::AngleRestriction angle_restriction = board_->get_rules().get_trace_angle_restriction();
    if (angle_restriction == rules::AngleRestriction::RIGHT)
    {
        offset_shape = std::make_unique<geometry::planar::IntBox>(offset_shape->bounding_box());
    }
    else if (angle_restriction == rules::AngleRestriction::FORTY_FIVE)
    {
        offset_shape = std::make_unique<geometry::planar::IntOctagon>(offset_shape->bounding_octagon());
    }

    if (offset_shape->contains_inside(p_polyline.first_corner()) ||
        offset_shape->contains_inside(p_polyline.last_corner()))
    {
        // can happen with clearance compensation off because of asymmetry in
        // calculations with the offset shapes
        board_->set_shove_failing_obstacle(found_obstacle);
        return result;
    }

    std::vector<std::pair<int, int>> entries = offset_shape->entrance_points(p_polyline);
    if (entries.empty())
    {
        result.polyline = p_polyline; // no obstacle
        result.changed = false;
        return result;
    }
    if (entries.size() < 2)
    {
        board_->set_shove_failing_obstacle(found_obstacle);
        return result;
    }

    const int first_intersection_side_no = entries.front().second;
    const int last_intersection_side_no = entries.back().second;
    const int first_intersection_line_no = entries.front().first;
    const int last_intersection_line_no = entries.back().first;
    int side_diff = last_intersection_side_no - first_intersection_side_no;
    if (side_diff < 0)
    {
        side_diff += offset_shape->border_line_count();
    }
    else if (side_diff == 0)
    {
        geometry::planar::FloatPoint compare_corner = offset_shape->corner_approx(first_intersection_side_no);
        geometry::planar::FloatPoint first_intersection = p_polyline.arr[first_intersection_line_no].intersection_approx(
            offset_shape->border_line(first_intersection_side_no));
        geometry::planar::FloatPoint second_intersection =
            p_polyline.arr[last_intersection_line_no].intersection_approx(
                offset_shape->border_line(last_intersection_side_no));
        if (compare_corner.distance(second_intersection) < compare_corner.distance(first_intersection))
        {
            side_diff += offset_shape->border_line_count();
        }
    }

    // geometry::planar::Line has no default constructor, so the substitute
    // lines are collected instead of preallocated like Java's Line[] array.
    std::vector<geometry::planar::Line> substitute_lines;
    substitute_lines.reserve(static_cast<size_t>(side_diff) + 3);
    substitute_lines.push_back(p_polyline.arr[first_intersection_line_no]);
    int current_edge_line_no = first_intersection_side_no;
    for (int i = 1; i <= side_diff + 1; i++)
    {
        substitute_lines.push_back(offset_shape->border_line(current_edge_line_no));
        if (current_edge_line_no == offset_shape->border_line_count() - 1)
        {
            current_edge_line_no = 0;
        }
        else
        {
            current_edge_line_no++;
        }
    }
    substitute_lines.push_back(p_polyline.arr[last_intersection_line_no]);
    geometry::planar::Polyline substitute_polyline(substitute_lines);
    // build a circuit around the offset shape in counter clock sense from the
    // first intersection point to the second intersection point
    std::vector<geometry::planar::Polyline> pieces = offset_shape->cutout(p_polyline);
    geometry::planar::Polyline result_polyline = substitute_polyline;

    if (!pieces.empty())
    {
        result_polyline = *pieces[0].combine(substitute_polyline);
    }
    if (pieces.size() > 1)
    {
        result_polyline = *result_polyline.combine(pieces[1]);
    }
    SpringOverResult recursion_result =
        spring_over(result_polyline, p_half_width, p_layer, p_net_no_arr, p_cl_type,
                    p_over_connected_pins, p_recursion_depth - 1, p_contact_pins);
    // Java returns the recursion result unchanged. The result object is a new
    // one in any case, because the obstacle has been substituted, so the
    // "changed" flag is set even if the recursive call changed nothing.
    recursion_result.changed = true;
    return recursion_result;
}

std::optional<geometry::planar::Polyline> ShoveTraceAlgo::spring_over_obstacles(
    const geometry::planar::Polyline& p_polyline,
    int p_half_width,
    int p_layer,
    const std::vector<int>& p_net_no_arr,
    int p_cl_type,
    const std::set<Pin*>* p_contact_pins)
{
    // Port of Java TraceShover.springOverObstacles(): this function looks
    // contrary to spring_over for the shortest way around the obstacles.
    static constexpr int MAX_SPRING_OVER_RECURSION_DEPTH = 20;
    SpringOverResult counter_clock_wise_result =
        spring_over(p_polyline, p_half_width, p_layer, p_net_no_arr, p_cl_type, true,
                    MAX_SPRING_OVER_RECURSION_DEPTH, p_contact_pins);
    if (!counter_clock_wise_result.changed)
    {
        return p_polyline; // no obstacle
    }

    geometry::planar::Polyline reversed_polyline = p_polyline.reverse();
    SpringOverResult clock_wise_result =
        spring_over(reversed_polyline, p_half_width, p_layer, p_net_no_arr, p_cl_type, true,
                    MAX_SPRING_OVER_RECURSION_DEPTH, p_contact_pins);

    if (clock_wise_result.polyline.has_value() && counter_clock_wise_result.polyline.has_value())
    {
        if (clock_wise_result.polyline->length_approx() <=
            counter_clock_wise_result.polyline->length_approx())
        {
            return clock_wise_result.polyline->reverse();
        }
        return counter_clock_wise_result.polyline;
    }
    if (clock_wise_result.polyline.has_value())
    {
        return clock_wise_result.polyline->reverse();
    }
    if (counter_clock_wise_result.polyline.has_value())
    {
        return counter_clock_wise_result.polyline;
    }
    return std::nullopt;
}

} // namespace freerouting::board
