#include "board/PolylineTrace.h"

#include "board/BasicBoard.h"
#include "board/ConductionArea.h"
#include "board/Pin.h"
#include "board/PullTightAlgo.h"
#include "board/RoutingBoard.h"
#include "board/ShapeSearchTree.h"
#include "board/Via.h"
#include "geometry/planar/Direction.h"
#include "geometry/planar/IntOctagon.h"
#include "geometry/planar/LineSegment.h"
#include "logger/FRLogger.h"
#include "rules/Net.h"
#include "rules/NetClass.h"

#include <algorithm>

namespace freerouting {
namespace board {

PolylineTrace::PolylineTrace(const std::shared_ptr<geometry::planar::Polyline>& p_polyline,
                             int p_layer, int p_half_width,
                             const std::vector<int>& p_net_no_arr,
                             int p_clearance_type, int p_id_no, int p_group_no,
                             FixedState p_fixed_state)
    : Trace(p_layer, p_half_width, p_net_no_arr, p_clearance_type, p_id_no, p_group_no, p_fixed_state)
    , polyline_(p_polyline)
{
    if (p_polyline == nullptr || p_polyline->arr.size() < 3)
    {
        logger::FRLogger::instance().warn("PolylineTrace: polyline with >= 3 lines expected");
    }
    if (p_polyline != nullptr)
    {
        set_bounding_box(p_polyline->bounding_box().offset(get_half_width()));
    }
}

void PolylineTrace::ensure_corners() const
{
    if (!corner_points_.empty() || polyline_ == nullptr)
    {
        return;
    }
    const std::vector<geometry::planar::IntPoint> corners = polyline_->corner_arr();
    corner_points_.reserve(corners.size());
    for (const geometry::planar::IntPoint& curr_corner : corners)
    {
        corner_points_.push_back(std::make_shared<geometry::planar::IntPoint>(curr_corner));
    }
}

const geometry::planar::Point& PolylineTrace::first_corner() const
{
    static const geometry::planar::IntPoint origin(0, 0);
    ensure_corners();
    if (corner_points_.empty())
    {
        return origin;
    }
    return *corner_points_.front();
}

geometry::planar::IntPoint PolylineTrace::own_corner(int p_index) const
{
    ensure_corners();
    if (corner_points_.empty())
    {
        return geometry::planar::IntPoint(0, 0);
    }
    const geometry::planar::Point& corner = (p_index < 0)
                                               ? *corner_points_.back()
                                               : *corner_points_[static_cast<size_t>(p_index)];
    // corner_points_ caches IntPoint instances, see ensure_corners().
    const auto* int_corner = dynamic_cast<const geometry::planar::IntPoint*>(&corner);
    return (int_corner != nullptr) ? *int_corner : geometry::planar::IntPoint(0, 0);
}

const geometry::planar::Point& PolylineTrace::last_corner() const
{
    static const geometry::planar::IntPoint origin(0, 0);
    ensure_corners();
    if (corner_points_.empty())
    {
        return origin;
    }
    return *corner_points_.back();
}

const std::vector<std::shared_ptr<geometry::planar::Point>>& PolylineTrace::corners() const
{
    ensure_corners();
    return corner_points_;
}

int PolylineTrace::corner_count() const
{
    if (polyline_ == nullptr)
    {
        return 0;
    }
    return static_cast<int>(std::max<int64_t>(polyline_->arr.size() - 1, 0));
}

const geometry::planar::Point& PolylineTrace::corner(int p_index) const
{
    static const geometry::planar::IntPoint origin(0, 0);
    ensure_corners();
    if (corner_points_.empty())
    {
        return origin;
    }
    const size_t index = static_cast<size_t>(std::clamp(p_index, 0, static_cast<int>(corner_points_.size()) - 1));
    return *corner_points_[index];
}

double PolylineTrace::get_length() const
{
    if (polyline_ == nullptr)
    {
        return 0.0;
    }
    return polyline_->length_approx();
}

int PolylineTrace::tile_shape_count() const
{
    if (polyline_ == nullptr)
    {
        return 0;
    }
    return static_cast<int>(std::max<int64_t>(polyline_->arr.size() - 2, 0));
}

geometry::planar::IntOctagon PolylineTrace::get_tile_shape(int p_index) const
{
    const int shape_count = tile_shape_count();
    if (polyline_ == nullptr || p_index < 0 || p_index >= shape_count)
    {
        logger::FRLogger::instance().warn("PolylineTrace.get_tile_shape: p_index out of range");
        return geometry::planar::IntOctagon();
    }
    // Java PolylineTrace.get_trace_connection_shape builds the line segment
    // p_index + 1 and simplifies it. The C++ port approximates that with the
    // segment bounding octagon enlarged by the half width.
    const geometry::planar::LineSegment segment(*polyline_, p_index + 1);
    return segment.bounding_octagon().enlarge(get_half_width());
}

void PolylineTrace::clear_derived_data()
{
    corner_points_.clear();
    Item::clear_derived_data();
}

void PolylineTrace::change(const std::shared_ptr<geometry::planar::Polyline>& p_new_polyline)
{
    polyline_ = p_new_polyline;
    corner_points_.clear();
    if (polyline_ != nullptr)
    {
        set_bounding_box(polyline_->bounding_box().offset(get_half_width()));
    }
}

bool PolylineTrace::pull_tight(bool p_own_net_only, int p_pull_tight_accuracy,
                               datastructures::Stoppable* p_stoppable)
{
    auto* routing_board = dynamic_cast<RoutingBoard*>(get_board());
    if (routing_board == nullptr)
    {
        return false;
    }
    std::vector<int> opt_net_no_arr;
    if (p_own_net_only)
    {
        opt_net_no_arr = get_net_nos();
    }
    std::unique_ptr<PullTightAlgo> pull_tight_algo = PullTightAlgo::get_instance(
        routing_board, opt_net_no_arr, nullptr, p_pull_tight_accuracy, p_stoppable, -1, nullptr,
        -1);
    return pull_tight(pull_tight_algo.get());
}

bool PolylineTrace::pull_tight(PullTightAlgo* p_pull_tight_algo)
{
    if (p_pull_tight_algo == nullptr || polyline_ == nullptr)
    {
        return false;
    }
    if (!is_on_board())
    {
        // This trace may have been deleted in a trace split for example
        return false;
    }
    if (is_shove_fixed())
    {
        return false;
    }
    if (!nets_normal())
    {
        return false;
    }
    const std::vector<int>& only_nets = p_pull_tight_algo->only_net_nos();
    if (!only_nets.empty() && get_net_nos() != only_nets)
    {
        return false;
    }
    const std::vector<int>& net_nos = get_net_nos();
    if (!net_nos.empty())
    {
        const rules::Net* net = get_board()->get_rules().get_nets().get(net_nos[0]);
        if (net == nullptr || net->get_net_class() == nullptr
            || !net->get_net_class()->is_pull_tight())
        {
            return false;
        }
    }
    geometry::planar::Polyline new_lines = p_pull_tight_algo->pull_tight(
        *polyline_, get_layer(), get_half_width(), net_nos, get_clearance_class(),
        touching_pins_at_end_corners());
    bool changed = new_lines.arr.size() != polyline_->arr.size();
    if (!changed)
    {
        for (size_t i = 0; i < new_lines.arr.size(); ++i)
        {
            if (!new_lines.arr[i].equals(polyline_->arr[i]))
            {
                changed = true;
                break;
            }
        }
    }
    if (changed)
    {
        change(std::make_shared<geometry::planar::Polyline>(new_lines));
        return true;
    }
    // TODO: swap_connection_to_pin(true/false) once ported (Java lines 707-727).
    return false;
}

namespace {

geometry::planar::IntPoint to_int_point(const geometry::planar::Point& p_point)
{
    if (const auto* int_point = dynamic_cast<const geometry::planar::IntPoint*>(&p_point))
    {
        return *int_point;
    }
    return p_point.to_float().round();
}

std::optional<geometry::planar::Line> line_through_point(
    const geometry::planar::IntPoint& p_point,
    const geometry::planar::Direction& p_dir)
{
    if (const auto* int_dir = dynamic_cast<const geometry::planar::IntDirection*>(&p_dir))
    {
        return geometry::planar::Line::get_instance(p_point, *int_dir);
    }
    return std::nullopt;
}

} // namespace

std::vector<Trace*> PolylineTrace::split(const geometry::planar::Point& p_point)
{
    std::vector<Trace*> result;
    if (polyline_ == nullptr)
    {
        return result;
    }
    const geometry::planar::IntPoint probe = to_int_point(p_point);
    for (int i = 0; i + 2 < static_cast<int>(polyline_->arr.size()); ++i)
    {
        geometry::planar::LineSegment curr_segment(*polyline_, i + 1);
        if (curr_segment.contains(probe))
        {
            std::unique_ptr<geometry::planar::Direction> split_dir =
                geometry::planar::Direction::get_instance(curr_segment.get_line().direction());
            std::unique_ptr<geometry::planar::Direction> turned = split_dir->turn_45_degree(2);
            std::optional<geometry::planar::Line> split_line =
                line_through_point(probe, *turned);
            if (!split_line.has_value())
            {
                continue;
            }
            std::optional<std::array<PolylineTrace*, 2>> pieces =
                split_at_line(i + 1, *split_line);
            if (pieces.has_value())
            {
                for (PolylineTrace* piece : *pieces)
                {
                    if (piece != nullptr)
                    {
                        result.push_back(piece);
                    }
                }
                return result;
            }
        }
    }
    return result;
}

std::optional<std::array<PolylineTrace*, 2>> PolylineTrace::split_at_line(
    int p_line_no, const geometry::planar::Line& p_new_end_line)
{
    if (!is_on_board() || polyline_ == nullptr)
    {
        return std::nullopt;
    }
    std::vector<std::unique_ptr<geometry::planar::Polyline>> split_polylines =
        polyline_->split(p_line_no, p_new_end_line);
    if (split_polylines.size() != 2)
    {
        return std::nullopt;
    }
    if (split_inside_drill_pad_prohibited(p_line_no, p_new_end_line))
    {
        return std::nullopt;
    }
    auto* routing_board = dynamic_cast<RoutingBoard*>(get_board());
    if (routing_board == nullptr)
    {
        return std::nullopt;
    }
    routing_board->remove_item(this);
    // remove_item only unlinks the item list; evict the stale tree entries too
    // so re-queries don't re-split the removed trace (infinite loop). TODO:
    // unify list/tree bookkeeping in remove_item/add_item directly.
    get_board()->get_search_tree_manager()->remove(this);
    std::array<PolylineTrace*, 2> result{nullptr, nullptr};
    for (int k = 0; k < 2; ++k)
    {
        result[k] = get_board()->insert_trace_without_cleaning(
            std::make_shared<geometry::planar::Polyline>(*split_polylines[k]), get_layer(),
            get_half_width(), get_net_nos(), get_clearance_class(), get_fixed_state());
    }
    return result;
}

std::vector<PolylineTrace*> PolylineTrace::split(
    const geometry::planar::IntOctagon* p_clip_shape)
{
    std::vector<PolylineTrace*> result;
    if (!nets_normal())
    {
        // only normal nets are split
        result.push_back(this);
        return result;
    }
    BasicBoard* board = get_board();
    if (board == nullptr || polyline_ == nullptr)
    {
        result.push_back(this);
        return result;
    }
    auto* routing_board = dynamic_cast<RoutingBoard*>(board);
    bool own_trace_split = false;
    ShapeSearchTree* default_tree = board->get_search_tree_manager()->get_default_tree();
    const int line_count = static_cast<int>(polyline_->arr.size());
    for (int i = 0; i < line_count - 2 && !own_trace_split; ++i)
    {
        if (p_clip_shape != nullptr)
        {
            geometry::planar::LineSegment curr_segment(*polyline_, i + 1);
            if (!p_clip_shape->intersects(curr_segment.bounding_box()))
            {
                continue;
            }
        }
        const geometry::planar::TileShape* curr_shape = get_tree_shape(*default_tree, i);
        if (curr_shape == nullptr)
        {
            continue;
        }
        geometry::planar::LineSegment curr_line_segment(*polyline_, i + 1);
        // look for intersecting traces with the i-th line segment
        std::vector<ShapeSearchTree::TreeEntry> overlapping_entries;
        default_tree->overlapping_tree_entries(*curr_shape, get_layer(), overlapping_entries);
        for (size_t entry_idx = 0; entry_idx < overlapping_entries.size(); ++entry_idx)
        {
            if (!is_on_board())
            {
                // this trace has been deleted in a cleanup operation
                return result;
            }
            ShapeSearchTree::TreeEntry found_entry = overlapping_entries[entry_idx];
            auto* found_item = dynamic_cast<Item*>(found_entry.object);
            if (found_item == nullptr)
            {
                continue;
            }
            if (found_item == this)
            {
                if (found_entry.shape_index_in_object >= i - 1
                    && found_entry.shape_index_in_object <= i + 1)
                {
                    // don't split own trace at this line or at neighbour lines
                    continue;
                }
                // try to handle intermediate segments of length 0 by comparing end corners
                if (i < found_entry.shape_index_in_object)
                {
                    if (polyline_->corner(i + 1)
                        == polyline_->corner(found_entry.shape_index_in_object))
                    {
                        continue;
                    }
                }
                else
                {
                    if (polyline_->corner(found_entry.shape_index_in_object + 1)
                        == polyline_->corner(i))
                    {
                        continue;
                    }
                }
            }
            if (!found_item->shares_net(this))
            {
                continue;
            }
            if (auto* found_trace = dynamic_cast<PolylineTrace*>(found_item))
            {
                geometry::planar::LineSegment found_line_segment(*found_trace->polyline_,
                                               found_entry.shape_index_in_object + 1);
                std::vector<geometry::planar::Line> intersecting_lines =
                    found_line_segment.intersection(curr_line_segment);
                std::vector<PolylineTrace*> split_pieces;
                bool found_trace_split = false;

                // try splitting the found trace first
                if (found_trace != this)
                {
                    for (geometry::planar::Line& intersecting_line : intersecting_lines)
                    {
                        const int line_no = found_entry.shape_index_in_object + 1;
                        std::optional<std::array<PolylineTrace*, 2>> curr_split_pieces =
                            found_trace->split_at_line(line_no, intersecting_line);
                        if (curr_split_pieces.has_value())
                        {
                            for (PolylineTrace* piece : *curr_split_pieces)
                            {
                                if (piece != nullptr)
                                {
                                    found_trace_split = true;
                                    split_pieces.push_back(piece);
                                }
                            }
                            if (found_trace_split)
                            {
                                // reread the overlapping tree entries because
                                // the board has changed
                                overlapping_entries.clear();
                                default_tree->overlapping_tree_entries(
                                    *curr_shape, get_layer(), overlapping_entries);
                                entry_idx = static_cast<size_t>(-1);
                                break;
                            }
                        }
                    }
                    if (!found_trace_split)
                    {
                        split_pieces.push_back(found_trace);
                    }
                }
                // now try splitting the own trace
                std::vector<geometry::planar::Line> own_intersecting_lines =
                    curr_line_segment.intersection(found_line_segment);
                for (geometry::planar::Line& intersecting_line : own_intersecting_lines)
                {
                    std::optional<std::array<PolylineTrace*, 2>> curr_split_pieces =
                        split_at_line(i + 1, intersecting_line);
                    if (curr_split_pieces.has_value())
                    {
                        own_trace_split = true;
                        // this trace was split itself into 2.
                        if ((*curr_split_pieces)[0] != nullptr)
                        {
                            std::vector<PolylineTrace*> sub =
                                (*curr_split_pieces)[0]->split(p_clip_shape);
                            result.insert(result.end(), sub.begin(), sub.end());
                        }
                        if ((*curr_split_pieces)[1] != nullptr)
                        {
                            std::vector<PolylineTrace*> sub =
                                (*curr_split_pieces)[1]->split(p_clip_shape);
                            result.insert(result.end(), sub.begin(), sub.end());
                        }
                        break;
                    }
                }
                if (found_trace_split || own_trace_split)
                {
                    // something was split,
                    // remove cycles containing a split piece
                    for (PolylineTrace* curr_piece : split_pieces)
                    {
                        board->remove_if_cycle(curr_piece);
                    }
                    // remove cycles in the own split pieces last
                    // to preserve them, if possible
                    for (PolylineTrace* curr_piece : result)
                    {
                        board->remove_if_cycle(curr_piece);
                    }
                }
            }
            else if (auto* curr_drill_item = dynamic_cast<DrillItem*>(found_item))
            {
                const geometry::planar::IntPoint split_point = curr_drill_item->get_center();
                if (curr_line_segment.contains(split_point))
                {
                    std::unique_ptr<geometry::planar::Direction> split_dir =
                        geometry::planar::Direction::get_instance(
                            curr_line_segment.get_line().direction());
                    std::unique_ptr<geometry::planar::Direction> turned =
                        split_dir->turn_45_degree(2);
                    std::optional<geometry::planar::Line> split_line =
                        line_through_point(split_point, *turned);
                    if (split_line.has_value())
                    {
                        split_at_line(i + 1, *split_line);
                    }
                }
            }
            else if (!is_user_fixed())
            {
                if (auto* conduction = dynamic_cast<ConductionArea*>(found_item))
                {
                    (void)conduction;
                    bool ignore_areas = false;
                    const std::vector<int>& net_nos = get_net_nos();
                    if (!net_nos.empty())
                    {
                        const rules::Net* curr_net =
                            board->get_rules().get_nets().get(net_nos[0]);
                        if (curr_net != nullptr && curr_net->get_net_class() != nullptr)
                        {
                            ignore_areas = curr_net->get_net_class()
                                               ->is_ignore_cycles_with_areas();
                        }
                    }
                    std::set<Item*> start_contacts = get_start_contacts();
                    std::set<Item*> end_contacts = get_end_contacts();
                    if (!ignore_areas && start_contacts.count(found_item) > 0
                        && end_contacts.count(found_item) > 0)
                    {
                        // this trace can be removed because of cycle with conduction area
                        if (routing_board != nullptr)
                        {
                            routing_board->remove_item(this);
                            get_board()->get_search_tree_manager()->remove(this);
                        }
                        return result;
                    }
                }
            }
        }
    }
    if (!own_trace_split)
    {
        result.push_back(this);
    }
    if (result.size() > 1 && routing_board != nullptr)
    {
        for (Item* curr_item : result)
        {
            routing_board->additional_update_after_change(curr_item);
        }
    }
    return result;
}

bool PolylineTrace::combine()
{
    // Port of Java PolylineTrace.combine(). Java iterates instead of
    // recursing, because a long chain of combinable traces grew the call stack
    // by one frame per combined trace and crashed with a StackOverflowError.
    bool something_changed = false;
    while (is_on_board() && polyline_ != nullptr
           && (combine_at_start(true) || combine_at_end(true)))
    {
        something_changed = true;
        // Java notifies the board observers here; the C++ headless port has no
        // observer bus yet.
        if (auto* routing_board = dynamic_cast<RoutingBoard*>(get_board()))
        {
            routing_board->additional_update_after_change(this);
        }
    }
    return something_changed;
}

bool PolylineTrace::combine_at_start(bool p_ignore_areas)
{
    if (polyline_ == nullptr)
    {
        return false;
    }
    // Java copies the corner (Point startCorner = firstCorner()). first_corner()
    // returns a reference into corner_points_, which clear_derived_data() below
    // empties, so keep the corner alive with a copy of the cache entry.
    const geometry::planar::IntPoint start_corner = own_corner(0);
    std::set<Item*> contacts = get_normal_contacts(start_corner, false);
    if (p_ignore_areas)
    {
        for (auto it = contacts.begin(); it != contacts.end();)
        {
            if (dynamic_cast<ConductionArea*>(*it) != nullptr)
            {
                it = contacts.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }
    if (contacts.size() != 1)
    {
        return false;
    }
    PolylineTrace* other_trace = nullptr;
    bool trace_found = false;
    bool reverse_order = false;
    for (Item* curr_ob : contacts)
    {
        auto* trace = dynamic_cast<PolylineTrace*>(curr_ob);
        if (trace == nullptr || trace->polyline_ == nullptr)
        {
            continue;
        }
        if (trace->get_layer() == get_layer() && trace->nets_equal(this)
            && trace->get_half_width() == get_half_width()
            && trace->get_fixed_state() == get_fixed_state())
        {
            if (start_corner.to_float() == trace->last_corner().to_float())
            {
                other_trace = trace;
                trace_found = true;
                break;
            }
            if (start_corner.to_float() == trace->first_corner().to_float())
            {
                other_trace = trace;
                reverse_order = true;
                trace_found = true;
                break;
            }
        }
    }
    if (!trace_found || other_trace == nullptr)
    {
        return false;
    }

    // Java: board.item_list.save_for_undo(this) - no undo in the C++ port.
    const std::vector<geometry::planar::Line>& this_lines = polyline_->arr;
    const std::vector<geometry::planar::Line>& other_arr = other_trace->polyline_->arr;
    if (this_lines.size() < 3 || other_arr.size() < 3)
    {
        return false;
    }
    std::vector<geometry::planar::Line> other_lines;
    if (reverse_order)
    {
        other_lines.reserve(other_arr.size());
        for (int i = static_cast<int>(other_arr.size()) - 1; i >= 0; --i)
        {
            other_lines.push_back(other_arr[i].opposite());
        }
    }
    else
    {
        other_lines = other_arr;
    }
    const bool skip_line =
        other_lines[other_lines.size() - 2].is_equal_or_opposite(this_lines[1]);
    std::vector<geometry::planar::Line> new_lines;
    new_lines.reserve(this_lines.size() + other_lines.size());
    for (size_t i = 0; i + 1 < other_lines.size(); ++i)
    {
        new_lines.push_back(other_lines[i]);
    }
    if (skip_line)
    {
        // junction line duplicated: drop it once
        new_lines.pop_back();
    }
    for (size_t i = 1; i < this_lines.size(); ++i)
    {
        new_lines.push_back(this_lines[i]);
    }
    geometry::planar::Polyline joined_polyline(new_lines);
    BasicBoard* board = get_board();
    if (board == nullptr)
    {
        return false;
    }
    SearchTreeManager* manager = board->get_search_tree_manager();
    // Always use the slow path (remove/reassign/reinsert); the fast
    // merge_entries_in_front optimization is not ported.
    if (manager != nullptr)
    {
        manager->remove(this);
    }
    polyline_ = std::make_shared<geometry::planar::Polyline>(joined_polyline);
    clear_derived_data();
    if (manager != nullptr)
    {
        manager->insert(this);
    }
    auto* routing_board = dynamic_cast<RoutingBoard*>(board);
    if (polyline_->arr.size() < 3)
    {
        if (routing_board != nullptr)
        {
            routing_board->remove_item(this);
        }
        if (manager != nullptr)
        {
            manager->remove(this);
        }
    }
    if (routing_board != nullptr)
    {
        routing_board->remove_item(other_trace);
    }
    if (manager != nullptr)
    {
        manager->remove(other_trace);
    }
    if (routing_board != nullptr)
    {
        routing_board->join_changed_area(start_corner.to_float(), get_layer());
    }
    return true;
}

bool PolylineTrace::combine_at_end(bool p_ignore_areas)
{
    if (polyline_ == nullptr)
    {
        return false;
    }
    // Java copies the corner (Point endCorner = lastCorner()), see combine_at_start().
    const geometry::planar::IntPoint end_corner = own_corner(-1);
    std::set<Item*> contacts = get_normal_contacts(end_corner, false);
    if (p_ignore_areas)
    {
        for (auto it = contacts.begin(); it != contacts.end();)
        {
            if (dynamic_cast<ConductionArea*>(*it) != nullptr)
            {
                it = contacts.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }
    if (contacts.size() != 1)
    {
        return false;
    }
    PolylineTrace* other_trace = nullptr;
    bool trace_found = false;
    bool reverse_order = false;
    for (Item* curr_ob : contacts)
    {
        auto* trace = dynamic_cast<PolylineTrace*>(curr_ob);
        if (trace == nullptr || trace->polyline_ == nullptr)
        {
            continue;
        }
        if (trace->get_layer() == get_layer() && trace->nets_equal(this)
            && trace->get_half_width() == get_half_width()
            && trace->get_fixed_state() == get_fixed_state())
        {
            if (end_corner.to_float() == trace->first_corner().to_float())
            {
                other_trace = trace;
                trace_found = true;
                break;
            }
            if (end_corner.to_float() == trace->last_corner().to_float())
            {
                other_trace = trace;
                reverse_order = true;
                trace_found = true;
                break;
            }
        }
    }
    if (!trace_found || other_trace == nullptr)
    {
        return false;
    }

    const std::vector<geometry::planar::Line>& this_lines = polyline_->arr;
    const std::vector<geometry::planar::Line>& other_arr = other_trace->polyline_->arr;
    if (this_lines.size() < 3 || other_arr.size() < 3)
    {
        return false;
    }
    std::vector<geometry::planar::Line> other_lines;
    if (reverse_order)
    {
        other_lines.reserve(other_arr.size());
        for (int i = static_cast<int>(other_arr.size()) - 1; i >= 0; --i)
        {
            other_lines.push_back(other_arr[i].opposite());
        }
    }
    else
    {
        other_lines = other_arr;
    }
    const bool skip_line =
        this_lines[this_lines.size() - 2].is_equal_or_opposite(other_lines[1]);
    std::vector<geometry::planar::Line> new_lines;
    new_lines.reserve(this_lines.size() + other_lines.size());
    for (size_t i = 0; i + 1 < this_lines.size(); ++i)
    {
        new_lines.push_back(this_lines[i]);
    }
    size_t other_start = 1;
    if (skip_line)
    {
        // junction line duplicated: drop it once (overwrites the last pushed
        // line, mirroring the Java arraycopy overlap)
        new_lines.pop_back();
    }
    for (size_t i = other_start; i < other_lines.size(); ++i)
    {
        new_lines.push_back(other_lines[i]);
    }
    geometry::planar::Polyline joined_polyline(new_lines);
    BasicBoard* board = get_board();
    if (board == nullptr)
    {
        return false;
    }
    SearchTreeManager* manager = board->get_search_tree_manager();
    if (manager != nullptr)
    {
        manager->remove(this);
    }
    polyline_ = std::make_shared<geometry::planar::Polyline>(joined_polyline);
    clear_derived_data();
    if (manager != nullptr)
    {
        manager->insert(this);
    }
    auto* routing_board = dynamic_cast<RoutingBoard*>(board);
    if (polyline_->arr.size() < 3)
    {
        if (routing_board != nullptr)
        {
            routing_board->remove_item(this);
        }
        if (manager != nullptr)
        {
            manager->remove(this);
        }
    }
    if (routing_board != nullptr)
    {
        routing_board->remove_item(other_trace);
    }
    if (manager != nullptr)
    {
        manager->remove(other_trace);
    }
    if (routing_board != nullptr)
    {
        routing_board->join_changed_area(end_corner.to_float(), get_layer());
    }
    return true;
}

bool PolylineTrace::normalize(const geometry::planar::IntOctagon* p_clip_shape)
{
    return normalize_impl(p_clip_shape, 0);
}

bool PolylineTrace::normalize_impl(const geometry::planar::IntOctagon* p_clip_shape,
                                   int p_normalization_depth)
{
    static constexpr int MAX_NORMALIZATION_DEPTH = 16;
    if (p_normalization_depth > MAX_NORMALIZATION_DEPTH)
    {
        logger::FRLogger::instance().error(
            "Max normalization depth reached with trace '" + std::to_string(get_id()) + "'");
        return false;
    }

    // Early exit if trace is very simple (only 2 points)
    if (corner_count() <= 2)
    {
        return false;
    }

    // Java notifies observers here; skipped in the C++ headless port.
    std::vector<PolylineTrace*> split_pieces = split(p_clip_shape);
    bool result = split_pieces.size() != 1;
    for (PolylineTrace* curr_split_trace : split_pieces)
    {
        if (curr_split_trace != nullptr && curr_split_trace->is_on_board())
        {
            bool trace_combined = curr_split_trace->combine();
            if (curr_split_trace->corner_count() == 2
                && curr_split_trace->first_corner().to_float()
                    == curr_split_trace->last_corner().to_float())
            {
                // remove trace with only 1 corner
                if (auto* routing_board =
                        dynamic_cast<RoutingBoard*>(curr_split_trace->get_board()))
                {
                    routing_board->remove_item(curr_split_trace);
                    curr_split_trace->get_board()->get_search_tree_manager()->remove(
                        curr_split_trace);
                }
                result = true;
            }
            else if (trace_combined)
            {
                curr_split_trace->normalize_impl(p_clip_shape, p_normalization_depth + 1);
                result = true;
            }
        }
    }
    return result;
}

bool PolylineTrace::split_inside_drill_pad_prohibited(
    int p_line_no, const geometry::planar::Line& p_line) const
{
    if (board_ == nullptr || polyline_ == nullptr)
    {
        return false;
    }
    std::unique_ptr<geometry::planar::Point> intersection =
        polyline_->arr[p_line_no].intersection(p_line);
    if (intersection == nullptr)
    {
        return false;
    }
    std::set<Item*> overlap_items = board_->pick_items(*intersection, get_layer(), nullptr);
    bool pad_found = false;
    for (Item* curr_item : overlap_items)
    {
        if (!curr_item->shares_net(this))
        {
            continue;
        }
        if (auto* curr_pin = dynamic_cast<Pin*>(curr_item))
        {
            if (curr_pin->get_center().equals(intersection.get()))
            {
                return false; // split always at the center of a drill item.
            }
            pad_found = true;
        }
        else if (auto* curr_trace = dynamic_cast<Trace*>(curr_item))
        {
            if ((curr_trace != this
                 && curr_trace->first_corner().equals(intersection.get()))
                || curr_trace->last_corner().equals(intersection.get()))
            {
                return false;
            }
        }
    }
    return pad_found;
}

} // namespace board
} // namespace freerouting