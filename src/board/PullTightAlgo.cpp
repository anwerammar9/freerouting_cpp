#include "board/PullTightAlgo.h"

#include <algorithm>
#include <cmath>

#include "autoroute/ExpansionCostFactor.h"
#include "board/ItemSelectionFilter.h"
#include "board/OptViaAlgo.h"
#include "board/PolylineTrace.h"
#include "board/PullTightAlgo45.h"
#include "board/PullTightAlgo90.h"
#include "board/PullTightAlgoAnyAngle.h"
#include "board/RoutingBoard.h"
#include "board/Trace.h"
#include "board/Via.h"
#include "datastructures/Signum.h"
#include "datastructures/Stoppable.h"
#include "datastructures/TimeLimit.h"
#include "geometry/planar/Direction.h"
#include "geometry/planar/LineSegment.h"
#include "logger/FRLogger.h"
#include "rules/BoardRules.h"
#include "rules/Net.h"

namespace freerouting::board {

namespace {

/**
 * Converts an abstract point to an geometry::planar::IntPoint, rounding float locations.
 * Java geometry::planar::TileShape.is_outside(geometry::planar::Point) accepts any point; the C++ port only
 * provides the geometry::planar::IntPoint overload.
 */
geometry::planar::IntPoint to_int_point(const geometry::planar::Point& p_point)
{
    if (const auto* int_point = dynamic_cast<const geometry::planar::IntPoint*>(&p_point))
    {
        return *int_point;
    }
    return p_point.to_float().round();
}

} // namespace

PullTightAlgo::~PullTightAlgo() = default;

std::optional<geometry::planar::Line> PullTightAlgo::line_through_point(
    const geometry::planar::IntPoint& p_point,
    const geometry::planar::Direction& p_dir)
{
    if (const auto* int_dir = dynamic_cast<const geometry::planar::IntDirection*>(&p_dir))
    {
        return geometry::planar::Line::get_instance(p_point, *int_dir);
    }
    std::unique_ptr<geometry::planar::Vector> vec = p_dir.get_vector();
    std::unique_ptr<geometry::planar::Point> second = vec->add_to(p_point);
    if (const auto* second_int = dynamic_cast<const geometry::planar::IntPoint*>(second.get()))
    {
        if (*second_int == p_point)
        {
            return std::nullopt;
        }
        return geometry::planar::Line(p_point, *second_int);
    }
    return std::nullopt;
}

geometry::planar::IntPoint PullTightAlgo::to_int_point(const geometry::planar::Point& p_point)
{
    if (const auto* int_point = dynamic_cast<const geometry::planar::IntPoint*>(&p_point))
    {
        return *int_point;
    }
    return p_point.to_float().round();
}

bool PullTightAlgo::polylines_equal(const geometry::planar::Polyline& p_first,
                                    const geometry::planar::Polyline& p_second)
{
    if (p_first.arr.size() != p_second.arr.size())
    {
        return false;
    }
    for (size_t i = 0; i < p_first.arr.size(); ++i)
    {
        if (!p_first.arr[i].equals(p_second.arr[i]))
        {
            return false;
        }
    }
    return true;
}

PullTightAlgo::PullTightAlgo(RoutingBoard* p_board,
                             const std::vector<int>& p_only_net_no_arr,
                             datastructures::Stoppable* p_stoppable_thread,
                             int p_time_limit,
                             const geometry::planar::Point* p_keep_point,
                             int p_keep_point_layer)
    : board_(p_board)
    , only_net_no_arr_(p_only_net_no_arr)
    , stoppable_thread_(p_stoppable_thread)
    , keep_point_layer_(p_keep_point_layer)
{
    if (p_time_limit > 0)
    {
        time_limit_ = std::make_unique<datastructures::TimeLimit>(p_time_limit);
    }
    if (p_keep_point != nullptr)
    {
        keep_point_ = p_keep_point->to_float();
    }
}

std::unique_ptr<PullTightAlgo> PullTightAlgo::get_instance(
    RoutingBoard* p_board,
    const std::vector<int>& p_only_net_no_arr,
    const geometry::planar::IntOctagon* p_clip_shape,
    int p_min_translate_dist,
    datastructures::Stoppable* p_stoppable_thread,
    int p_time_limit,
    const geometry::planar::Point* p_keep_point,
    int p_keep_point_layer)
{
    std::unique_ptr<PullTightAlgo> result;
    const rules::AngleRestriction angle_restriction =
        p_board->get_rules().get_trace_angle_restriction();
    if (angle_restriction == rules::AngleRestriction::RIGHT)
    {
        result = std::make_unique<PullTightAlgo90>(p_board, p_only_net_no_arr, p_stoppable_thread,
                                                   p_time_limit, p_keep_point, p_keep_point_layer);
    }
    else if (angle_restriction == rules::AngleRestriction::FORTY_FIVE)
    {
        result = std::make_unique<PullTightAlgo45>(p_board, p_only_net_no_arr, p_stoppable_thread,
                                                   p_time_limit, p_keep_point, p_keep_point_layer);
    }
    else
    {
        result = std::make_unique<PullTightAlgoAnyAngle>(p_board, p_only_net_no_arr,
                                                         p_stoppable_thread, p_time_limit,
                                                         p_keep_point, p_keep_point_layer);
    }
    if (p_clip_shape != nullptr)
    {
        result->curr_clip_shape_ = std::make_unique<geometry::planar::IntOctagon>(*p_clip_shape);
    }
    result->min_translate_dist_ = std::max(p_min_translate_dist, 100);
    return result;
}

void PullTightAlgo::opt_changed_area(
    const std::vector<autoroute::ExpansionCostFactor>* p_trace_cost_arr)
{
    ChangedArea* changed_area = board_->changed_area();
    if (changed_area == nullptr)
    {
        return;
    }
    bool something_changed = true;
    // starting with curr_min_translate_dist big is a try to
    // avoid fine approximation at the beginning to avoid
    // problems with dog ears
    while (something_changed)
    {
        something_changed = false;
        for (int i = 0; i < board_->get_layer_count(); ++i)
        {
            geometry::planar::IntOctagon changed_region = changed_area->get_area(i);
            if (changed_region.is_empty())
            {
                continue;
            }
            changed_area->set_empty(i);
            board_->join_graphics_update_box(changed_region.bounding_box());
            const double changed_area_offset =
                1.5
                * (board_->get_rules().get_clearance_matrix().get_max_value_on_layer(i)
                   + 2 * board_->get_rules().get_max_trace_half_width());
            const geometry::planar::IntOctagon enlarged_region =
                changed_region.enlarge(changed_area_offset);
            // search in the ShapeSearchTree for all overlapping traces
            // with clip_shape on layer i
            std::set<SearchTreeObject*, TreeStorableLess> items =
                board_->overlapping_objects(enlarged_region, i);
            for (SearchTreeObject* curr_ob : items)
            {
                if (is_stop_requested())
                {
                    return;
                }
                if (auto* curr_trace = dynamic_cast<PolylineTrace*>(curr_ob))
                {
                    if (curr_trace->pull_tight(this))
                    {
                        something_changed = true;
                        if (split_traces_at_keep_point())
                        {
                            break;
                        }
                    }
                    else if (smoothen_end_corners_at_trace_1(curr_trace))
                    {
                        something_changed = true;
                        break; // because items may be removed
                    }
                }
                else if (auto* via = dynamic_cast<Via*>(curr_ob))
                {
                    if (p_trace_cost_arr != nullptr
                        && OptViaAlgo::opt_via_location(board_, via, p_trace_cost_arr,
                                                        min_translate_dist_, 10))
                    {
                        something_changed = true;
                    }
                }
            }
        }
    }
}

geometry::planar::Polyline PullTightAlgo::pull_tight(
    const geometry::planar::Polyline& p_polyline,
    int p_layer,
    int p_half_width,
    const std::vector<int>& p_net_no_arr,
    int p_cl_type,
    const std::set<Pin*>& p_contact_pins)
{
    curr_layer_ = p_layer;
    ShapeSearchTree* search_tree = board_->search_tree_manager().get_default_tree();
    curr_half_width_ = p_half_width + search_tree->clearance_compensation_value(p_cl_type, p_layer);
    curr_net_no_arr_ = p_net_no_arr;
    curr_cl_type_ = p_cl_type;
    contact_pins_ = p_contact_pins;
    return pull_tight(p_polyline);
}

bool PullTightAlgo::is_stop_requested() const
{
    if (stoppable_thread_ != nullptr && stoppable_thread_->is_stop_requested())
    {
        return true;
    }
    if (time_limit_ == nullptr)
    {
        return false;
    }
    const bool time_limit_exceeded = time_limit_->limit_exceeded();
    if (time_limit_exceeded)
    {
        if (board_ == nullptr)
        {
            logger::FRLogger::instance().error("PullTightAlgo.is_stop_requested: board is null");
        }
        logger::FRLogger::instance().debug("PullTightAlgo.is_stop_requested: time limit exceeded");
    }
    return time_limit_exceeded;
}

geometry::planar::Polyline PullTightAlgo::reposition_lines(
    const geometry::planar::Polyline& p_polyline)
{
    if (p_polyline.arr.size() < 5)
    {
        return p_polyline;
    }
    for (int i = 2; i < static_cast<int>(p_polyline.arr.size()) - 2; ++i)
    {
        std::optional<geometry::planar::Line> new_line = reposition_line(p_polyline.arr, i);
        if (new_line.has_value())
        {
            std::vector<geometry::planar::Line> line_arr = p_polyline.arr;
            line_arr[i] = *new_line;
            return skip_segments_of_length_0(geometry::planar::Polyline(line_arr));
        }
    }
    return p_polyline;
}

std::optional<geometry::planar::Line> PullTightAlgo::reposition_line(
    const std::vector<geometry::planar::Line>& p_line_arr, int p_no)
{
    if (p_no + 3 > static_cast<int>(p_line_arr.size()))
    {
        return std::nullopt;
    }
    if (curr_clip_shape_ != nullptr)
    // check, that the corners of the line to translate are inside
    // the clip shape
    {
        for (int i = -1; i < 1; ++i)
        {
            std::unique_ptr<geometry::planar::Point> curr_corner =
                p_line_arr[p_no + i].intersection(p_line_arr[p_no + i + 1]);
            if (curr_corner == nullptr
                || curr_clip_shape_->is_outside(to_int_point(*curr_corner)))
            {
                return std::nullopt;
            }
        }
    }
    const geometry::planar::Line& translate_line = p_line_arr[p_no];
    std::unique_ptr<geometry::planar::Point> prev_corner_ptr =
        p_line_arr[p_no - 2].intersection(p_line_arr[p_no - 1]);
    std::unique_ptr<geometry::planar::Point> next_corner_ptr =
        p_line_arr[p_no + 1].intersection(p_line_arr[p_no + 2]);
    if (prev_corner_ptr == nullptr || next_corner_ptr == nullptr)
    {
        return std::nullopt;
    }
    const double prev_dist = translate_line.signed_distance(prev_corner_ptr->to_float());
    const double next_dist = translate_line.signed_distance(next_corner_ptr->to_float());
    if (datastructures::Signum::of(prev_dist) != datastructures::Signum::of(next_dist))
    {
        // the 2 corners are at different sides of translate_line
        return std::nullopt;
    }
    const geometry::planar::Point* nearest_point = nullptr;
    double max_translate_dist = 0.0;
    if (std::abs(prev_dist) < std::abs(next_dist))
    {
        nearest_point = prev_corner_ptr.get();
        max_translate_dist = prev_dist;
    }
    else
    {
        nearest_point = next_corner_ptr.get();
        max_translate_dist = next_dist;
    }
    double translate_dist = max_translate_dist;
    double delta_dist = max_translate_dist;
    const geometry::planar::Side side_of_nearest_point = translate_line.side_of(*nearest_point);
    const int sign = datastructures::Signum::as_int(max_translate_dist);
    std::optional<geometry::planar::Line> new_line;
    const geometry::planar::Line& first_check = p_line_arr[p_no - 1];
    const geometry::planar::Line& third_check = p_line_arr[p_no + 1];
    bool first_time = true;
    while (first_time || std::abs(delta_dist) > min_translate_dist_)
    {
        bool check_ok = false;
        geometry::planar::Line middle_check = translate_line;
        bool have_middle = false;

        if (first_time)
        {
            if (const auto* nearest_int = dynamic_cast<const geometry::planar::IntPoint*>(nearest_point))
            {
                // geometry::planar::Line through nearest_point parallel to translate_line.
                std::unique_ptr<geometry::planar::Point> dir_vec =
                    translate_line.direction().add_to(*nearest_int);
                if (const auto* second_int = dynamic_cast<const geometry::planar::IntPoint*>(dir_vec.get()))
                {
                    middle_check = geometry::planar::Line(*nearest_int, *second_int);
                    have_middle = true;
                }
            }
        }
        if (!have_middle)
        {
            middle_check = translate_line.translate(-translate_dist);
            have_middle = true;
        }
        if (middle_check.equals(translate_line))
        {
            // may happen at first time if nearest_point is not an geometry::planar::IntPoint
            return std::nullopt;
        }
        const geometry::planar::Side new_line_side_of_nearest_point = middle_check.side_of(*nearest_point);
        if (new_line_side_of_nearest_point != side_of_nearest_point
            && new_line_side_of_nearest_point != geometry::planar::Side::COLLINEAR)
        {
            // moved a little bit to far at the first time
            // because of numerical inaccuracy;
            // may happen if nearest_point is not an geometry::planar::IntPoint
            const double shorten_value = sign * 0.5;
            max_translate_dist -= shorten_value;
            translate_dist -= shorten_value;
            delta_dist -= shorten_value;
            continue;
        }
        const geometry::planar::Polyline tmp({first_check, middle_check, third_check});

        if (tmp.arr.size() == 3)
        {
            std::unique_ptr<geometry::planar::TileShape> shape_to_check = tmp.offset_shape(curr_half_width_, 0);
            if (shape_to_check != nullptr)
            {
                check_ok = board_->check_trace_shape(*shape_to_check, curr_layer_,
                                                     curr_net_no_arr_, curr_cl_type_, contact_pin_set());
            }
        }
        delta_dist /= 2.0;
        if (check_ok)
        {
            new_line = middle_check;
            if (first_time)
            {
                // biggest possible change
                break;
            }
            translate_dist += delta_dist;
        }
        else
        {
            translate_dist -= delta_dist;
        }
        first_time = false;
    }
    if (new_line.has_value() && board_->changed_area() != nullptr)
    {
        // mark the changed area
        board_->join_changed_area(first_check.intersection_approx(*new_line), curr_layer_);
        board_->join_changed_area(third_check.intersection_approx(*new_line), curr_layer_);
        board_->join_changed_area(p_line_arr[p_no - 1].intersection_approx(p_line_arr[p_no]),
                                  curr_layer_);
        board_->join_changed_area(
            p_line_arr[p_no].intersection_approx(p_line_arr[p_no + 1]), curr_layer_);
    }
    return new_line;
}

geometry::planar::Polyline PullTightAlgo::skip_segments_of_length_0(
    const geometry::planar::Polyline& p_polyline)
{
    bool polyline_changed = false;
    geometry::planar::Polyline curr_polyline = p_polyline;
    for (int i = 1; i < static_cast<int>(curr_polyline.arr.size()) - 1; ++i)
    {
        bool try_skip = false;
        if (i == 1 || i == static_cast<int>(curr_polyline.arr.size()) - 2)
        // the position of the first corner and the last corner
        // must be retained exactly
        {
            const geometry::planar::IntPoint prev_corner = curr_polyline.corner(i - 1);
            const geometry::planar::IntPoint curr_corner = curr_polyline.corner(i);
            try_skip = curr_corner == prev_corner;
        }
        else
        {
            const geometry::planar::FloatPoint prev_corner = curr_polyline.corner_approx(i - 1);
            const geometry::planar::FloatPoint curr_corner = curr_polyline.corner_approx(i);
            try_skip = curr_corner.distance_square(prev_corner) < c_min_corner_dist_square;
        }

        if (try_skip)
        {
            // check, if skipping the line of length 0 does not
            // result in a clearance violation
            std::vector<geometry::planar::Line> curr_lines;
            curr_lines.reserve(curr_polyline.arr.size() - 1);
            for (int k = 0; k < i; ++k)
            {
                curr_lines.push_back(curr_polyline.arr[k]);
            }
            for (size_t k = static_cast<size_t>(i) + 1; k < curr_polyline.arr.size(); ++k)
            {
                curr_lines.push_back(curr_polyline.arr[k]);
            }
            geometry::planar::Polyline tmp(curr_lines);
            bool check_ok = tmp.arr.size() == curr_lines.size();
            if (check_ok && !curr_polyline.arr[i].is_multiple_of_45_degree())
            {
                // no check necessary for skipping 45 degree lines, because the check is
                // performance critical and the line shapes
                // are intersected with the bounding octagon anyway.
                if (i > 1)
                {
                    std::unique_ptr<geometry::planar::TileShape> shape_to_check =
                        tmp.offset_shape(curr_half_width_, i - 2);
                    if (shape_to_check != nullptr)
                    {
                        check_ok = board_->check_trace_shape(*shape_to_check, curr_layer_,
                                                             curr_net_no_arr_, curr_cl_type_,
                                                             contact_pin_set());
                    }
                    else
                    {
                        check_ok = false;
                    }
                }
                if (check_ok && (i < static_cast<int>(curr_polyline.arr.size()) - 2))
                {
                    std::unique_ptr<geometry::planar::TileShape> shape_to_check =
                        tmp.offset_shape(curr_half_width_, i - 1);
                    if (shape_to_check != nullptr)
                    {
                        check_ok = board_->check_trace_shape(*shape_to_check, curr_layer_,
                                                             curr_net_no_arr_, curr_cl_type_,
                                                             contact_pin_set());
                    }
                    else
                    {
                        check_ok = false;
                    }
                }
            }
            if (check_ok)
            {
                polyline_changed = true;
                curr_polyline = tmp;
                --i;
            }
        }
    }
    if (!polyline_changed)
    {
        return p_polyline;
    }
    return curr_polyline;
}

bool PullTightAlgo::smoothen_end_corners_at_trace(PolylineTrace* p_trace)
{
    if (p_trace == nullptr)
    {
        return false;
    }
    curr_layer_ = p_trace->get_layer();
    curr_half_width_ = p_trace->get_half_width();
    curr_net_no_arr_ = p_trace->get_net_nos();
    curr_cl_type_ = p_trace->get_clearance_class();
    return smoothen_end_corners_at_trace_1(p_trace);
}

bool PullTightAlgo::smoothen_end_corners_at_trace_1(PolylineTrace* p_trace)
{
    // try to improve the connection to other traces
    if (p_trace->is_shove_fixed())
    {
        return false;
    }
    std::set<Pin*> saved_contact_pins = contact_pins_;
    // to allow the trace to slide to the end point of a contact trace, if the contact trace ends at
    // a pin.
    contact_pins_.clear();
    bool result = false;
    bool connection_to_trace_improved = true;
    PolylineTrace* curr_trace = p_trace;
    while (connection_to_trace_improved)
    {
        connection_to_trace_improved = false;
        std::unique_ptr<geometry::planar::Polyline> adjusted_polyline =
            smoothen_end_corners_at_trace_2(curr_trace);
        if (adjusted_polyline != nullptr)
        {
            const int trace_layer = curr_trace->get_layer();
            const int curr_cl_class = curr_trace->get_clearance_class();
            const FixedState curr_fixed_state = curr_trace->get_fixed_state();
            board_->remove_item(curr_trace);
            PolylineTrace* adj_ins_trace = board_->insert_trace_without_cleaning(
                std::make_shared<geometry::planar::Polyline>(*adjusted_polyline), trace_layer,
                curr_half_width_, curr_trace->get_net_nos(), curr_cl_class, curr_fixed_state);
            if (adj_ins_trace != nullptr)
            {
                result = true;
                connection_to_trace_improved = true;
                board_->remove_item(curr_trace);
                curr_trace = adj_ins_trace;
                for (int curr_net_no : curr_trace->get_net_nos())
                {
                    board_->split_traces(adjusted_polyline->first_corner(), trace_layer,
                                         curr_net_no);
                    board_->split_traces(adjusted_polyline->last_corner(), trace_layer,
                                         curr_net_no);
                    board_->normalize_traces(curr_net_no);
                    if (split_traces_at_keep_point())
                    {
                        return true;
                    }
                }
            }
        }
    }
    contact_pins_ = saved_contact_pins;
    return result;
}

bool PullTightAlgo::split_traces_at_keep_point()
{
    if (!keep_point_.has_value())
    {
        return false;
    }
    ItemSelectionFilter filter(ItemSelectionFilter::SelectableChoices::TRACES);
    std::set<Item*> picked_items =
        board_->pick_items(*keep_point_, keep_point_layer_, &filter);
    for (Item* curr_item : picked_items)
    {
        if (auto* trace = dynamic_cast<Trace*>(curr_item))
        {
            std::vector<Trace*> split_pieces = trace->split(*keep_point_);
            if (!split_pieces.empty())
            {
                return true;
            }
        }
    }
    return false;
}

std::unique_ptr<geometry::planar::Polyline> PullTightAlgo::smoothen_end_corners_at_trace_2(
    PolylineTrace* p_trace)
{
    if (p_trace == nullptr || !p_trace->is_on_board())
    {
        return nullptr;
    }
    std::unique_ptr<geometry::planar::Polyline> result = smoothen_start_corner_at_trace(p_trace);
    if (result == nullptr)
    {
        result = smoothen_end_corner_at_trace(p_trace);
        if (result != nullptr && board_->changed_area() != nullptr)
        {
            // mark the changed area
            board_->join_changed_area(
                result->corner_approx(result->corner_count() - 1), curr_layer_);
        }
    }
    else if (board_->changed_area() != nullptr)
    {
        // mark the changed area
        board_->join_changed_area(result->corner_approx(0), curr_layer_);
    }
    if (result != nullptr)
    {
        contact_pins_ = p_trace->touching_pins_at_end_corners();
        result = std::make_unique<geometry::planar::Polyline>(
            skip_segments_of_length_0(*result));
    }
    return result;
}

geometry::planar::Polyline PullTightAlgo::avoid_acid_traps(
    const geometry::planar::Polyline& p_polyline)
{
    // Disabled in Java (returns input); the ShoveTraceAlgo.spring_over_obstacles
    // path below is dead code there and stays unported here.
    return p_polyline;
}

} // namespace freerouting::board
