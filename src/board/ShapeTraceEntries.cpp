#include "board/ShapeTraceEntries.h"

#include "board/CalcFromSide.h"
#include "board/PolylineTrace.h"
#include "board/Via.h"
#include "board/RoutingBoard.h"
#include "board/SearchTreeManager.h"
#include "board/ShapeSearchTree.h"
#include "board/Item.h"
#include "board/Pin.h"
#include "board/ConductionArea.h"
#include "board/ComponentObstacleArea.h"
#include "board/ViaObstacleArea.h"
#include "geometry/planar/LineSegment.h"
#include "geometry/planar/TileShape.h"
#include "logger/FRLogger.h"

#include <algorithm>
#include <cmath>

namespace freerouting::board
{

ShapeTraceEntries::ShapeTraceEntries(const geometry::planar::TileShape& p_shape,
                                     int p_layer,
                                     const std::vector<int>& p_own_net_nos,
                                     int p_cl_type,
                                     const CalcFromSide* p_from_side,
                                     RoutingBoard* p_board)
    : layer_(p_layer), own_net_nos_(p_own_net_nos), cl_class_(p_cl_type), board_(p_board)
{
    shape_owned_ = p_shape.clone();
    shape_ = shape_owned_.get();
    if (p_from_side)
    {
        from_side_ = *p_from_side;
    }
}

bool ShapeTraceEntries::net_nos_equal(const std::vector<int>& a, const std::vector<int>& b)
{
    if (a.size() != b.size()) return false;
    for (int x : a)
    {
        bool found = false;
        for (int y : b) if (x == y) { found = true; break; }
        if (!found) return false;
    }
    return true;
}

bool ShapeTraceEntries::polyline_equals(const geometry::planar::Polyline& p_first,
                                      const geometry::planar::Polyline& p_second)
{
    // Java compares the cutout pieces with the input polyline by object
    // identity; the C++ TileShape::cutout(Polyline) returns a copy of the
    // input if nothing was cut off, so the pieces are compared geometrically.
    if (p_first.arr.size() != p_second.arr.size()) return false;
    for (size_t i = 0; i < p_first.arr.size(); ++i)
    {
        if (!p_first.arr[i].fast_equals(p_second.arr[i])) return false;
    }
    return true;
}

void ShapeTraceEntries::cutout_trace(PolylineTrace* p_trace,
                                     const geometry::planar::TileShape& p_shape,
                                     int p_cl_class)
{
    // Port of Java ShapeTraceEntries.cutoutTrace()
    // (ShapeTraceEntries.java:66-107).
    if (p_trace == nullptr) return;
    if (!p_trace->is_on_board())
    {
        logger::FRLogger::instance().warn("ShapeTraceEntries.cutout_trace: trace is deleted");
        return;
    }
    auto* board = dynamic_cast<RoutingBoard*>(p_trace->get_board());
    if (board == nullptr) return;
    auto* search_tree = board->get_search_tree_manager()->get_default_tree();
    std::unique_ptr<geometry::planar::TileShape> offset_shape;
    if (search_tree && search_tree->is_clearance_compensation_used())
    {
        double current_offset = p_trace->get_compensated_half_width(*search_tree) + c_offset_add;
        offset_shape = p_shape.offset(current_offset);
    }
    else
    {
        // enlarge the shape in 2 steps for symmetry reasons
        int cl_offset = board->clearance_value(p_trace->get_clearance_class(), p_cl_class,
                                               p_trace->get_layer())
                        + c_offset_add;
        offset_shape = p_shape.offset(p_trace->get_half_width());
        if (offset_shape) offset_shape = offset_shape->offset(cl_offset);
    }
    if (!offset_shape) return;
    const auto& trace_lines = p_trace->polyline();
    std::vector<geometry::planar::Polyline> pieces = offset_shape->cutout(trace_lines);
    if (pieces.size() == 1 && polyline_equals(pieces[0], trace_lines))
    {
        // nothing cut off
        return;
    }
    if (pieces.size() == 2 && offset_shape->is_outside(pieces[0].first_corner()) &&
        offset_shape->is_outside(pieces[1].last_corner()))
    {
        fast_cutout_trace(p_trace, pieces[0], pieces[1]);
    }
    else
    {
        // Java board.removeItem(trace) also evicts the search tree entries
        remove_trace_from_board(board, p_trace);
        for (const auto& piece : pieces)
        {
            board->insert_trace_without_cleaning(
                std::make_shared<geometry::planar::Polyline>(piece), p_trace->get_layer(),
                p_trace->get_half_width(), p_trace->get_net_nos(), p_trace->get_clearance_class(),
                FixedState::NOT_FIXED);
        }
    }
}

void ShapeTraceEntries::fast_cutout_trace(PolylineTrace* p_trace,
                                          const geometry::planar::Polyline& p_start_piece,
                                          const geometry::planar::Polyline& p_end_piece)
{
    // Port of Java ShapeTraceEntries.fastCutoutTrace()
    // (ShapeTraceEntries.java:110-151).
    auto* board = dynamic_cast<RoutingBoard*>(p_trace->get_board());
    if (board == nullptr) return;
    board->insert_trace_without_cleaning(
        std::make_shared<geometry::planar::Polyline>(p_start_piece), p_trace->get_layer(),
        p_trace->get_half_width(), p_trace->get_net_nos(), p_trace->get_clearance_class(),
        FixedState::NOT_FIXED);
    board->insert_trace_without_cleaning(
        std::make_shared<geometry::planar::Polyline>(p_end_piece), p_trace->get_layer(),
        p_trace->get_half_width(), p_trace->get_net_nos(), p_trace->get_clearance_class(),
        FixedState::NOT_FIXED);
    // Java board.searchTreeManager.reuseEntriesAfterCutout(trace, start, end)
    // transfers the precalculated tree entries of the cut-out trace to the two
    // pieces. That is a pure allocation optimisation - the resulting tree
    // contents are the same as with freshly inserted entries, which is what
    // insert_trace_without_cleaning() has done above.
    remove_trace_from_board(board, p_trace);
}

void ShapeTraceEntries::remove_trace_from_board(RoutingBoard* p_board, PolylineTrace* p_trace)
{
    if (p_board == nullptr || p_trace == nullptr) return;
    p_board->remove_item(p_trace);
    p_trace->set_on_board(false);
}

bool ShapeTraceEntries::store_items(const std::vector<Item*>& p_item_list,
                                    bool p_is_pad_check,
                                    bool p_copper_sharing_allowed)
{
    for (Item* curr_item : p_item_list)
    {
        if (!curr_item) continue;
        if (!p_is_pad_check &&
            (dynamic_cast<ViaObstacleArea*>(curr_item) || dynamic_cast<ComponentObstacleArea*>(curr_item)))
        {
            continue;
        }
        bool contains_own_net = curr_item->shares_net_no(own_net_nos_);
        if (auto* area = dynamic_cast<ConductionArea*>(curr_item))
        {
            if (contains_own_net || !area->get_is_obstacle()) continue;
        }
        if (curr_item->is_shove_fixed() && !contains_own_net)
        {
            found_obstacle_ = curr_item;
            return false;
        }
        if (auto* via = dynamic_cast<Via*>(curr_item))
        {
            if (p_is_pad_check || !contains_own_net)
            {
                shove_via_list.push_back(via);
            }
        }
        else if (auto* trace = dynamic_cast<PolylineTrace*>(curr_item))
        {
            if (!store_trace(trace)) return false;
        }
        else
        {
            if (contains_own_net)
            {
                if (!p_copper_sharing_allowed)
                {
                    found_obstacle_ = curr_item;
                    return false;
                }
                if (p_is_pad_check)
                {
                    // Pin::drill_allowed not yet ported – treat any Pin as non-drillable for pad check (conservative headless)
                    if (dynamic_cast<Pin*>(curr_item))
                    {
                        found_obstacle_ = curr_item;
                        return false;
                    }
                }
            }
            else
            {
                found_obstacle_ = curr_item;
                return false;
            }
        }
    }
    search_from_side();
    resort();
    return calculate_stack_levels();
}

bool ShapeTraceEntries::store_trace(PolylineTrace* p_trace)
{
    if (!board_ || !shape_) return false;
    auto* search_tree = board_->get_search_tree_manager()->get_default_tree();
    std::unique_ptr<geometry::planar::TileShape> offset_shape;
    if (search_tree && search_tree->is_clearance_compensation_used())
    {
        double off = p_trace->get_compensated_half_width(*search_tree) + c_offset_add;
        offset_shape = shape_->offset(off);
    }
    else
    {
        // enlarge in 2 steps for symmetry
        // clearance_value fallback – use BoardRules matrix if available
        int cl_offset = 0;
        if (board_)
        {
            const auto& cm = board_->get_rules().get_clearance_matrix();
            // get_value needs p_layer; use stored layer_ for trace vs own
            cl_offset = cm.get_value(p_trace->get_clearance_class(), cl_class_, p_trace->get_layer());
        }
        auto step1 = shape_->offset(p_trace->get_half_width());
        offset_shape = step1->offset(cl_offset + c_offset_add);
    }
    if (!offset_shape) return false;

    auto entries = offset_shape->entrance_points(p_trace->polyline());
    for (auto& e : entries)
    {
        int trace_line_no = e.first;
        int edge_no = e.second;
        auto is = p_trace->polyline().arr[trace_line_no].intersection_approx(offset_shape->border_line(edge_no));
        insert_entry_point(p_trace, trace_line_no, edge_no, is);
    }

    // Look, if an end point of the trace lies in the interior of the shape.
    // This may be the case, if a via touches the shape. Port of Java
    // ShapeTraceEntries.storeTrace (the loop assigns endCorner at its *end*,
    // so iteration 0 inspects firstCorner() and iteration 1 lastCorner()).
    if (!p_trace->shares_net_no(own_net_nos_))
    {
        if (!p_trace->nets_normal())
        {
            return false;
        }
        geometry::planar::IntPoint end_corner = p_trace->first_corner().to_float().round();
        for (int i = 0; i < 2; ++i)
        {
            if (offset_shape->contains(end_corner))
            {
                ItemSet contact_list = (i == 0) ? p_trace->get_start_contacts()
                                                        : p_trace->get_end_contacts();
                int contact_count = 0;
                bool store_end_corner = true;

                for (Item* contact_item : contact_list)
                {
                    if (contact_item == nullptr)
                    {
                        continue;
                    }
                    if (!contact_item->is_routable())
                    {
                        found_obstacle_ = contact_item;
                        return false;
                    }
                    if (const auto* contact_trace = dynamic_cast<const Trace*>(contact_item))
                    {
                        // Java compares the contact with itself for half width and
                        // clearance class, so only is_shove_fixed() can trigger here.
                        if (contact_item->is_shove_fixed()
                            || contact_trace->get_half_width() != p_trace->get_half_width()
                            || contact_item->get_clearance_class() != contact_trace->get_clearance_class())
                        {
                            if (offset_shape->contains_inside(end_corner))
                            {
                                found_obstacle_ = contact_item;
                                return false;
                            }
                        }
                    }
                    else if (const auto* contact_via = dynamic_cast<const Via*>(contact_item))
                    {
                        const geometry::planar::TileShape* via_shape =
                            contact_via->get_tree_shape_on_layer(search_tree, layer_);
                        double via_trace_diff = 0.0;
                        if (via_shape != nullptr && !search_tree->is_clearance_compensation_used())
                        {
                            const int via_clearance =
                                board_->clearance_value(contact_item->get_clearance_class(), cl_class_, layer_);
                            const int trace_clearance =
                                board_->clearance_value(p_trace->get_clearance_class(), cl_class_, layer_);
                            via_trace_diff = via_shape->smallest_radius()
                                             - static_cast<double>(p_trace->get_compensated_half_width(*search_tree));
                            if (trace_clearance > via_clearance)
                            {
                                via_trace_diff += via_clearance - trace_clearance;
                            }
                        }
                        if (via_shape == nullptr)
                        {
                            // the via does not reach this layer - not an obstacle here
                        }
                        else if (via_trace_diff < 0)
                        {
                            // the via is smaller than the trace
                            found_obstacle_ = contact_item;
                            return false;
                        }
                        else if (via_trace_diff == 0 && !offset_shape->contains_inside(end_corner))
                        {
                            // the via need not to be shoved
                            store_end_corner = false;
                        }
                    }
                    ++contact_count;
                }
                if (contact_count == 1 && store_end_corner)
                {
                    if (auto projection = offset_shape->nearest_border_point(end_corner))
                    {
                        const int projection_side = offset_shape->contains_on_border_line_no(*projection);
                        // the following may not be correct because the trace may not contain a
                        // suitable line for the construction of the end line of the substitute trace.
                        const int trace_line_segment_no =
                            (i == 0) ? 0 : static_cast<int>(p_trace->polyline().arr.size()) - 1;
                        if (projection_side >= 0)
                        {
                            insert_entry_point(p_trace, trace_line_segment_no, projection_side,
                                              projection->to_float());
                        }
                    }
                }
                else if (contact_count == 0 && offset_shape->contains_inside(end_corner))
                {
                    shape_contains_trace_tails_ = true;
                }
            }
            end_corner = p_trace->last_corner().to_float().round();
        }
    }
    found_obstacle_ = p_trace;
    return true;
}

void ShapeTraceEntries::search_from_side()
{
    // Port of Java ShapeTraceEntries.searchFromSide()
    // (ShapeTraceEntries.java:444-460). Java keeps the entry approximation
    // null when no own net entry is found, which resort() branches on.
    if (from_side_.has_value() && from_side_->no >= 0) return;
    EntryPoint* cur = list_anchor_;
    int cur_no = 0;
    std::optional<geometry::planar::FloatPoint> cur_approx;
    while (cur != nullptr)
    {
        if (cur->trace != nullptr && cur->trace->shares_net_no(own_net_nos_))
        {
            cur_no = cur->edge_no;
            cur_approx = cur->entry_approx;
            break;
        }
        cur = cur->next;
    }
    from_side_ = CalcFromSide::values(cur_no, cur_approx);
}

void ShapeTraceEntries::resort()
{
    // Port of Java ShapeTraceEntries.resort()
    // (ShapeTraceEntries.java:462-573).
    if (!shape_ || !from_side_.has_value()) return;
    const int edge_count = shape_->border_line_count();
    if (from_side_->no < 0 || from_side_->no >= edge_count)
    {
        logger::FRLogger::instance().warn("ShapeTraceEntries.resort: from side not calculated");
        return;
    }
    // Resort the intersection points, so that they start in the middle of the
    // from side.
    const geometry::planar::FloatPoint compare_corner1 = shape_->corner_approx(from_side_->no);
    const geometry::planar::FloatPoint compare_corner2 =
        (from_side_->no == edge_count - 1) ? shape_->corner_approx(0)
                                            : shape_->corner_approx(from_side_->no + 1);
    double from_point_dist = 0.0;
    std::optional<geometry::planar::FloatPoint> from_point_projection;
    if (from_side_->border_intersection.has_value())
    {
        from_point_projection =
            from_side_->border_intersection->projection_approx(shape_->border_line(from_side_->no));
        from_point_dist = from_point_projection->distance_square(compare_corner1);
        if (from_point_dist >= compare_corner1.distance_square(compare_corner2))
        {
            from_side_ = CalcFromSide::values(from_side_->no, std::nullopt);
        }
    }
    // Search the first intersection point between the side middle and
    // compare_corner2.
    EntryPoint* current = list_anchor_;
    EntryPoint* prev = nullptr;
    while (current != nullptr)
    {
        if (current->edge_no > from_side_->no) break;
        if (current->edge_no == from_side_->no)
        {
            if (from_side_->border_intersection.has_value())
            {
                const geometry::planar::FloatPoint current_projection =
                    current->entry_approx.projection_approx(shape_->border_line(from_side_->no));
                if (current_projection.distance_square(compare_corner1) >= from_point_dist
                    && current_projection.distance_square(*from_point_projection)
                           <= current_projection.distance_square(compare_corner1))
                {
                    break;
                }
            }
            else
            {
                if (current->entry_approx.distance_square(compare_corner2)
                    <= current->entry_approx.distance_square(compare_corner1))
                {
                    break;
                }
            }
        }
        prev = current;
        current = prev->next;
    }
    if (current != nullptr && current != list_anchor_)
    {
        rotate_entry_list_around_anchor(current, edge_count);
    }
    // Remove intersections between two other intersections of the same
    // connected set, so that only the first and the last intersection is kept.
    if (list_anchor_ == nullptr) return;
    prev = list_anchor_;
    std::vector<int> prev_net_nos = prev->trace ? prev->trace->get_net_nos() : std::vector<int>{};

    current = list_anchor_->next;
    std::vector<int> current_net_nos;
    EntryPoint* next = nullptr;
    if (current != nullptr)
    {
        current_net_nos = current->trace ? current->trace->get_net_nos() : std::vector<int>{};
        next = current->next;
    }
    else
    {
        current_net_nos = std::vector<int>{};
    }
    EntryPoint* before_prev = nullptr;
    while (next != nullptr)
    {
        const std::vector<int> next_net_nos =
            next->trace ? next->trace->get_net_nos() : std::vector<int>{};
        if (net_nos_equal(prev_net_nos, current_net_nos)
            && net_nos_equal(current_net_nos, next_net_nos))
        {
            prev->next = next;
        }
        else
        {
            before_prev = prev;
            prev = current;
            prev_net_nos = current_net_nos;
        }
        current_net_nos = next_net_nos;
        current = next;
        next = current->next;
    }

    // Remove nodes of the own net at the start and at the end of the list.
    if (current != nullptr && net_nos_equal(current_net_nos, own_net_nos_))
    {
        prev->next = nullptr;
        if (net_nos_equal(prev_net_nos, own_net_nos_))
        {
            if (before_prev != nullptr)
            {
                before_prev->next = nullptr;
            }
            else
            {
                list_anchor_ = nullptr;
            }
        }
    }

    if (list_anchor_ != nullptr && list_anchor_->trace != nullptr
        && net_nos_equal(list_anchor_->trace->get_net_nos(), own_net_nos_))
    {
        list_anchor_ = list_anchor_->next;
        if (list_anchor_ != nullptr && list_anchor_->trace != nullptr
            && net_nos_equal(list_anchor_->trace->get_net_nos(), own_net_nos_))
        {
            list_anchor_ = list_anchor_->next;
        }
    }
}

void ShapeTraceEntries::rotate_entry_list_around_anchor(EntryPoint* p_new_anchor, int p_edge_count)
{
    // Port of Java ShapeTraceEntries.rotateEntryListAroundAnchor()
    // (ShapeTraceEntries.java:765-783).
    EntryPoint* current = p_new_anchor;
    EntryPoint* prev = nullptr;
    while (current != nullptr)
    {
        prev = current;
        current = prev->next;
    }
    prev->next = list_anchor_;
    current = list_anchor_;
    while (current != p_new_anchor)
    {
        // Add p_edge_count to the edge numbers to differentiate points before
        // and after the middle of the from side.
        current->edge_no += p_edge_count;
        prev = current;
        current = prev->next;
    }
    prev->next = nullptr;
    list_anchor_ = p_new_anchor;
}

bool ShapeTraceEntries::calculate_stack_levels()
{
    if (!list_anchor_) return true;
    EntryPoint* cur = list_anchor_;
    std::vector<int> cur_nets = cur->trace ? cur->trace->get_net_nos() : std::vector<int>{};
    int cur_level = net_nos_equal(cur_nets, own_net_nos_) ? 0 : 1;
    while (cur)
    {
        if (cur->stack_level < 0)
        {
            ++trace_piece_count_;
            cur->stack_level = cur_level;
            if (cur_level > max_stack_level_) max_stack_level_ = cur_level;
        }
        // Find next foreign/own occurrence to determine level change
        EntryPoint* check = cur->next;
        int next_foreign_idx = 0, last_own_idx = 0;
        EntryPoint* last_own = nullptr;
        EntryPoint* first_foreign = nullptr;
        int n = 0;
        while (check)
        {
            ++n;
            std::vector<int> cn = check->trace ? check->trace->get_net_nos() : std::vector<int>{};
            if (net_nos_equal(cn, cur_nets))
            {
                last_own_idx = n;
                last_own = check;
                check->stack_level = cur->stack_level;
            }
            else if (next_foreign_idx == 0)
            {
                next_foreign_idx = n;
                first_foreign = check;
            }
            check = check->next;
        }
        EntryPoint* next = nullptr;
        if (n != 0)
        {
            if (next_foreign_idx != 0 && next_foreign_idx < last_own_idx)
            {
                next = first_foreign;
                if (next->stack_level >= 0) return false;
                ++cur_level;
            }
            else
            {
                if (last_own_idx != 0) next = last_own;
                else
                {
                    next = first_foreign;
                    if (next && next->stack_level >= 0)
                    {
                        --cur_level;
                        if (next->stack_level != cur_level) return false;
                    }
                }
            }
            // splice out irrelevant middle entries
            EntryPoint* it = cur->next;
            while (it != next) it = it->next;
            cur->next = next;
            cur = next;
            if (cur) cur_nets = cur->trace ? cur->trace->get_net_nos() : std::vector<int>{};
        }
        else break;
    }
    return cur_level == 1 || list_anchor_ == nullptr;
}

PolylineTrace* ShapeTraceEntries::next_substitute_trace_piece()
{
    // Port of Java ShapeTraceEntries.nextSubstituteTracePiece()
    // (ShapeTraceEntries.java:226-282). pop_piece_internal() returns the
    // head of the isolated entry chain; Java's popPiece() returns the pair
    // (first, last) of the piece, and the last entry is the tail of that
    // chain, so it is recovered here by walking the next pointers.
    EntryPoint* first = pop_piece_internal();
    if (!first) return nullptr;
    EntryPoint* last = first;
    while (last->next) last = last->next;

    PolylineTrace* cur_trace = first->trace;
    if (!cur_trace || !shape_ || !board_) return nullptr;
    auto* search_tree = board_->get_search_tree_manager()->get_default_tree();
    std::unique_ptr<geometry::planar::TileShape> offset_shape;
    if (search_tree && search_tree->is_clearance_compensation_used())
    {
        double off = cur_trace->get_compensated_half_width(*search_tree) + c_offset_add;
        offset_shape = shape_->offset(off);
    }
    else
    {
        // enlarge the shape in 2 steps for symmetry reasons
        offset_shape = shape_->offset(cur_trace->get_half_width());
        int cl_off = board_->clearance_value(cur_trace->get_clearance_class(), cl_class_, layer_)
                     + c_offset_add;
        offset_shape = offset_shape->offset(cl_off);
    }
    if (!offset_shape) return nullptr;

    const int edge_count = shape_->border_line_count();
    const int edge_diff = last->edge_no - first->edge_no;
    if (edge_count <= 0 || edge_diff < 0 || first->edge_no < 0 || first->edge_no >= edge_count)
    {
        // Java would use a negative array index here; the C++ port bails out
        // instead of reading out of bounds.
        logger::FRLogger::instance().warn("ShapeTraceEntries.next_substitute_trace_piece: inconsistent edge numbers");
        return nullptr;
    }

    // calculate the polyline of the substitute trace
    std::vector<geometry::planar::Line> piece_lines;
    piece_lines.reserve(static_cast<size_t>(edge_diff) + 3);
    // start with the intersecting line of the trace at the start entry
    const auto& start_polyline = first->trace->polyline();
    const auto& end_polyline = last->trace->polyline();
    if (first->trace_line_no >= static_cast<int>(start_polyline.arr.size()) ||
        last->trace_line_no >= static_cast<int>(end_polyline.arr.size()))
    {
        return nullptr;
    }
    piece_lines.push_back(start_polyline.arr[first->trace_line_no]);
    // fill the interior lines with the appropriate edge lines of the offset shape
    int current_edge_no = first->edge_no % edge_count;
    for (int i = 1; i < edge_diff + 2; i++)
    {
        piece_lines.push_back(offset_shape->border_line(current_edge_no));
        if (current_edge_no == edge_count - 1)
        {
            current_edge_no = 0;
        }
        else
        {
            ++current_edge_no;
        }
    }
    // end with the intersecting line of the trace at the end entry
    piece_lines.push_back(end_polyline.arr[last->trace_line_no]);

    auto poly = std::make_shared<geometry::planar::Polyline>(piece_lines);
    if (poly->is_empty()) return next_substitute_trace_piece();

    // Create PolylineTrace - Java allocates it with id 0 and hands it to the
    // board only after the recursion loop of TraceShover.insert() has
    // finished, so the piece stays off the board while its segments are
    // inserted (otherwise it would obstruct its own segments).
    auto* result = new PolylineTrace(poly, layer_, cur_trace->get_half_width(),
                                     cur_trace->get_net_nos(), cur_trace->get_clearance_class(), 0,
                                     0, FixedState::NOT_FIXED);
    result->set_board(board_);
    return result;
}

ShapeTraceEntries::EntryPoint* ShapeTraceEntries::pop_piece_internal()
{
    if (!list_anchor_)
    {
        return nullptr;
    }
    EntryPoint* first = list_anchor_;
    EntryPoint* prev_first = nullptr;
    while (first && first->stack_level != max_stack_level_)
    {
        prev_first = first;
        first = first->next;
    }
    if (!first) return nullptr;
    EntryPoint* last = first;
    EntryPoint* after_last = first->next;
    while (after_last && after_last->stack_level == max_stack_level_ &&
           after_last->trace && first->trace &&
           net_nos_equal(after_last->trace->get_net_nos(), first->trace->get_net_nos()))
    {
        last = after_last;
        after_last = last->next;
    }
    if (prev_first) prev_first->next = after_last;
    else list_anchor_ = after_last;

    // recalc max
    max_stack_level_ = 0;
    for (EntryPoint* c = list_anchor_; c; c = c->next)
        if (c->stack_level > max_stack_level_) max_stack_level_ = c->stack_level;
    --trace_piece_count_;
    if (first->trace && net_nos_equal(first->trace->get_net_nos(), own_net_nos_))
    {
        return pop_piece_internal();
    }
    // isolate piece
    last->next = nullptr;
    return first;
}

void ShapeTraceEntries::insert_entry_point(PolylineTrace* p_trace, int p_trace_line_no,
                                           int p_edge_no,
                                           const geometry::planar::FloatPoint& p_entry_approx)
{
    auto ep = std::make_unique<EntryPoint>();
    ep->trace = p_trace;
    ep->trace_line_no = p_trace_line_no;
    ep->edge_no = p_edge_no;
    ep->entry_approx = p_entry_approx;
    EntryPoint* raw = ep.get();
    owned_entries_.push_back(std::move(ep));

    EntryPoint* prev = nullptr;
    EntryPoint* cur = list_anchor_;
    while (cur)
    {
        if (cur->edge_no > raw->edge_no) break;
        if (cur->edge_no == raw->edge_no)
        {
            auto prev_corner = shape_->corner_approx(p_edge_no);
            auto next_corner = shape_->corner_approx((p_edge_no + 1) % shape_->border_line_count());
            double cur_proj = prev_corner.scalar_product(cur->entry_approx, next_corner);
            double new_proj = prev_corner.scalar_product(p_entry_approx, next_corner);
            if (new_proj <= cur_proj) break;
        }
        prev = cur;
        cur = cur->next;
    }
    raw->next = cur;
    if (prev) prev->next = raw;
    else list_anchor_ = raw;
}

void ShapeTraceEntries::cutout_traces(const std::vector<Item*>& p_item_list)
{
    for (Item* it : p_item_list)
    {
        if (auto* tr = dynamic_cast<PolylineTrace*>(it))
        {
            if (!tr->shares_net_no(own_net_nos_))
            {
                cutout_trace(tr, *shape_, cl_class_);
            }
        }
    }
}

} // namespace freerouting::board
