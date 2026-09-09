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
        from_side_ = std::make_unique<CalcFromSide>(*p_from_side);
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

void ShapeTraceEntries::cutout_trace(PolylineTrace* /*p_trace*/,
                                     const geometry::planar::TileShape& /*p_shape*/,
                                     int /*p_cl_class*/)
{
    logger::FRLogger::instance().warn("ShapeTraceEntries.cutout_trace: headless stub – board.item_list not yet ported");
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

    if (!p_trace->shares_net_no(own_net_nos_))
    {
        // Check endpoints inside shape – simplified headless version:
        // If trace has single DRC contact, we treat tail-in-shape as obstacle
        // For headless we conservatively allow endpoints, but flag tails
        const auto& first = p_trace->first_corner();
        const auto& last = p_trace->last_corner();
        if (offset_shape->contains(first.to_float()) || offset_shape->contains(last.to_float()))
        {
            // Check contacts – if trace is normal (not fixed), mark tail
            // Simplified: if no shove-fixed contact, mark tails
            shape_contains_trace_tails_ = true;
        }
    }
    found_obstacle_ = p_trace;
    return true;
}

void ShapeTraceEntries::search_from_side()
{
    if (from_side_ && from_side_->no >= 0) return;
    EntryPoint* cur = list_anchor_;
    int cur_no = 0;
    geometry::planar::FloatPoint cur_approx(0,0);
    bool found = false;
    while (cur)
    {
        if (cur->trace && cur->trace->shares_net_no(own_net_nos_))
        {
            cur_no = cur->edge_no;
            cur_approx = cur->entry_approx;
            found = true;
            break;
        }
        cur = cur->next;
    }
    if (!found) cur_approx = geometry::planar::FloatPoint(0,0);
    from_side_ = std::make_unique<CalcFromSide>(cur_no, cur_approx);
}

void ShapeTraceEntries::resort()
{
    if (!shape_ || !from_side_) return;
    int edge_count = shape_->border_line_count();
    if (from_side_->no < 0 || from_side_->no >= edge_count)
    {
        logger::FRLogger::instance().warn("ShapeTraceEntries.resort: from side not calculated");
        return;
    }
    geometry::planar::FloatPoint c1 = shape_->corner_approx(from_side_->no);
    geometry::planar::FloatPoint c2;
    if (from_side_->no == edge_count - 1) c2 = shape_->corner_approx(0);
    else c2 = shape_->corner_approx(from_side_->no + 1);

    double from_dist = 0;
    geometry::planar::FloatPoint proj(0,0);
    if (from_side_->border_intersection.x != 0 || from_side_->border_intersection.y != 0)
    {
        // approximate projection – use border intersection directly
        proj = from_side_->border_intersection;
        from_dist = proj.distance_square(c1);
        if (from_dist >= c1.distance_square(c2))
        {
            from_side_ = std::make_unique<CalcFromSide>(from_side_->no, geometry::planar::FloatPoint(0,0));
        }
    }
    // Find first entry after from_side middle
    EntryPoint* cur = list_anchor_;
    EntryPoint* prev = nullptr;
    while (cur)
    {
        if (cur->edge_no > from_side_->no) break;
        if (cur->edge_no == from_side_->no && from_side_->border_intersection.x == 0 && from_side_->border_intersection.y == 0)
        {
            if (cur->entry_approx.distance_square(c2) <= cur->entry_approx.distance_square(c1)) break;
        }
        prev = cur;
        cur = cur->next;
    }
    if (cur && cur != list_anchor_)
    {
        EntryPoint* new_anchor = cur;
        // find tail
        EntryPoint* tail = cur;
        while (tail->next) tail = tail->next;
        tail->next = list_anchor_;
        cur = list_anchor_;
        while (cur != new_anchor)
        {
            cur->edge_no += edge_count;
            cur = cur->next;
        }
        if (prev) prev->next = nullptr;
        list_anchor_ = new_anchor;
    }
    // Remove middle entries of same net triplets (keep first/last)
    if (!list_anchor_) return;
    EntryPoint* a = list_anchor_;
    EntryPoint* b = a->next;
    if (!b) return;
    EntryPoint* c = b->next;
    std::vector<int> a_nets = a->trace ? a->trace->get_net_nos() : std::vector<int>{};
    std::vector<int> b_nets = b->trace ? b->trace->get_net_nos() : std::vector<int>{};
    EntryPoint* before_a = nullptr;
    while (c)
    {
        std::vector<int> c_nets = c->trace ? c->trace->get_net_nos() : std::vector<int>{};
        if (net_nos_equal(a_nets, b_nets) && net_nos_equal(b_nets, c_nets))
        {
            a->next = c;
            b = c;
            b_nets = c_nets;
            c = c->next;
        }
        else
        {
            before_a = a;
            a = b;
            a_nets = b_nets;
            b = c;
            b_nets = c_nets;
            c = c->next;
        }
    }
    // Remove own-net entries at start/end (headless: filter traces sharing own nets)
    while (list_anchor_ && list_anchor_->trace && net_nos_equal(list_anchor_->trace->get_net_nos(), own_net_nos_))
    {
        list_anchor_ = list_anchor_->next;
    }
    if (list_anchor_)
    {
        EntryPoint* cur2 = list_anchor_;
        EntryPoint* prev2 = nullptr;
        while (cur2 && cur2->next) { prev2 = cur2; cur2 = cur2->next; }
        if (cur2 && cur2->trace && net_nos_equal(cur2->trace->get_net_nos(), own_net_nos_))
        {
            if (prev2) prev2->next = nullptr;
            else list_anchor_ = nullptr;
        }
    }
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
        int idx = 0, next_foreign_idx = 0, last_own_idx = 0;
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
    auto* entries = pop_piece_internal();
    if (!entries) return nullptr;
    // entries[0] is first, entries[1] is last of the piece (we return a 2-element array via internal state)
    // Our pop returns first of piece; we need to reconstruct 2 entries via stack level grouping
    // For simplicity, the pop already groups – we reconstruct polyline from first->next up to last
    // Here we use the first entry's trace as template
    PolylineTrace* cur_trace = entries->trace;
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
        auto step1 = shape_->offset(cur_trace->get_half_width());
        int cl_off = 0;
        if (board_)
        {
            const auto& cm = board_->get_rules().get_clearance_matrix();
            cl_off = cm.get_value(cur_trace->get_clearance_class(), cl_class_, layer_);
        }
        offset_shape = step1->offset(cl_off + c_offset_add);
    }
    if (!offset_shape) return nullptr;

    // Re-find the last entry of this piece (all consecutive with same stack level and net)
    EntryPoint* last = entries;
    EntryPoint* after = entries->next;
    // Actually pop_piece_internal already isolated piece – we can treat entries as first, and find last by scanning owned entries with same level
    // Simplified: use entries as start, and find its partner by edge_no ordering
    // For headless minimal, construct a single-segment offset trace around the shape edge
    // If we cannot reconstruct full piece, return a simple trace along offset shape border
    int edge_count = shape_->border_line_count();
    int start_edge = entries->edge_no % edge_count;
    int end_edge = start_edge; // fallback
    // Try to find last via next pointer if still linked (pop removed piece, so next is after)
    // For now create a 3-line polyline: trace line + one offset edge + trace line
    std::vector<geometry::planar::Line> piece_lines;
    piece_lines.reserve(3);
    piece_lines.push_back(cur_trace->polyline().arr[entries->trace_line_no]);
    piece_lines.push_back(offset_shape->border_line(start_edge));
    // duplicate for minimal valid polyline (needs at least 2 lines -> 1 segment, but we need closed piece)
    // Use the same trace line as end to make a valid polyline (will be cleaned by Polyline ctor)
    piece_lines.push_back(cur_trace->polyline().arr[entries->trace_line_no]);

    if (piece_lines.size() < 2) return nullptr;
    auto poly = std::make_shared<geometry::planar::Polyline>(piece_lines);
    if (poly->is_empty()) return next_substitute_trace_piece();

    // Create PolylineTrace – use board to allocate id
    int new_id = board_ ? board_->generate_item_id() : 0;
    auto* result = new PolylineTrace(poly, layer_, cur_trace->get_half_width(),
                                     cur_trace->get_net_nos(), cur_trace->get_clearance_class(),
                                     new_id, 0, cur_trace->get_fixed_state());
    if (board_) result->set_board(board_);
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
