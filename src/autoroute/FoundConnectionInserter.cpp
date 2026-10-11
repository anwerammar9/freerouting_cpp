#include "autoroute/FoundConnectionInserter.h"
#include "autoroute/FoundConnectionLocator.h"
#include "board/Item.h"
#include "board/ItemSelectionFilter.h"
#include "board/Padstack.h"
#include "board/Pin.h"
#include "board/PolylineTrace.h"
#include "board/RoutingBoard.h"
#include "board/Trace.h"
#include "board/ForcedViaAlgo.h"
#include "geometry/planar/Point.h"
#include "geometry/planar/Polyline.h"
#include "logger/FRLogger.h"
#include "rules/BoardRules.h"
#include "rules/ClearanceMatrix.h"
#include "rules/ViaInfo.h"
#include "rules/ViaRule.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

namespace freerouting::autoroute {

namespace {

std::string format_point(const geometry::planar::Point& p_point)
{
    return p_point.to_string();
}

/**
 * The C++ port has two AngleRestriction enums: the rules one, which the board
 * stores, and the board one, which FoundConnectionLocator works with. Java
 * has a single one.
 */
board::AngleRestriction to_board_angle_restriction(rules::AngleRestriction p_angle)
{
    switch (p_angle)
    {
    case rules::AngleRestriction::RIGHT:
        return board::AngleRestriction::NINETY_DEGREE;
    case rules::AngleRestriction::FORTY_FIVE:
        return board::AngleRestriction::FORTYFIVE_DEGREE;
    case rules::AngleRestriction::NONE:
    default:
        return board::AngleRestriction::NONE;
    }
}

} // namespace

FoundConnectionInserter::FoundConnectionInserter(board::RoutingBoard* p_board,
                                                 const AutorouteControl& p_ctrl)
    : board_(p_board), ctrl_(&p_ctrl)
{
}

int FoundConnectionInserter::trace_half_width(int p_layer) const
{
    // Divergence: Java indexes ctrl.traceHalfWidth[layer] directly and throws
    // for an out-of-range layer; the C++ control provides a bounds-checked
    // accessor instead.
    return ctrl_->get_trace_half_width(p_layer);
}

std::optional<geometry::planar::IntPoint> FoundConnectionInserter::insert_forced_segment(
    const geometry::planar::IntPoint& p_from_corner,
    const geometry::planar::IntPoint& p_to_corner,
    int p_half_width,
    int p_layer) const
{
    // Java repeats this argument list for every insertForcedTraceSegment call;
    // the helper mirrors them once.
    return board_->insert_forced_trace_segment(
        p_from_corner, p_to_corner, p_half_width, p_layer,
        std::vector<int>{ctrl_->net_no}, ctrl_->trace_clearance_class_no,
        ctrl_->max_shove_trace_recursion_depth, ctrl_->max_shove_via_recursion_depth,
        ctrl_->max_spring_over_recursion_depth, std::numeric_limits<int>::max(),
        ctrl_->pull_tight_accuracy, true, nullptr);
}

std::unique_ptr<FoundConnectionInserter> FoundConnectionInserter::get_instance(
    const FoundConnectionLocator* p_connection,
    board::RoutingBoard* p_board,
    const AutorouteControl& p_ctrl)
{
    // Port of Java FoundConnectionInserter.getInstance() at
    // FoundConnectionInserter.java:40-111.
    if (p_connection == nullptr)
    {
        logger::FRLogger::instance().warn("FoundConnectionInserter: no connection to insert");
        return nullptr;
    }
    // Divergence: Java only rejects a null connectionItems collection. An
    // empty collection would make the Java code dereference the null
    // lastCorner in the trailing insertVia call; the C++ port rejects it here.
    if (p_connection->connection_items.empty())
    {
        logger::FRLogger::instance().warn(
            "FoundConnectionInserter: connection without connection items");
        return nullptr;
    }
    if (p_board == nullptr)
    {
        logger::FRLogger::instance().warn("FoundConnectionInserter: no board to insert into");
        return nullptr;
    }

    int current_layer = p_connection->target_layer;
    auto new_instance = std::unique_ptr<FoundConnectionInserter>(
        new FoundConnectionInserter(p_board, p_ctrl));

    for (const auto& current_new_item : p_connection->connection_items)
    {
        // Divergence: Java dereferences corners[0] unconditionally; the C++
        // port rejects an item without corners instead of reading out of range.
        if (current_new_item.corners.empty())
        {
            logger::FRLogger::instance().warn(
                "FoundConnectionInserter: connection item without corners");
            return nullptr;
        }
        logger::FRLogger::instance().trace(
            "FoundConnectionInserter: inserting trace item on layer " +
            std::to_string(current_new_item.layer) + " with " +
            std::to_string(current_new_item.corners.size()) + " corners, start " +
            format_point(current_new_item.corners.front()) + ", end " +
            format_point(current_new_item.corners.back()));
        if (!new_instance->insert_via(current_new_item.corners.front(), current_layer,
                                      current_new_item.layer))
        {
            return nullptr;
        }
        current_layer = current_new_item.layer;
        if (!new_instance->insert_trace(current_new_item))
        {
            return nullptr;
        }
    }
    if (!new_instance->last_corner_.has_value())
    {
        logger::FRLogger::instance().warn(
            "FoundConnectionInserter: no last corner for net #" +
            std::to_string(p_ctrl.net_no));
        return nullptr;
    }
    if (!new_instance->insert_via(*new_instance->last_corner_, current_layer,
                                  p_connection->start_layer))
    {
        return nullptr;
    }
    if (const auto* target_trace = dynamic_cast<const board::PolylineTrace*>(p_connection->target_item))
    {
        if (new_instance->first_corner_.has_value())
        {
            p_board->connect_to_trace(*new_instance->first_corner_,
                                      const_cast<board::PolylineTrace*>(target_trace),
                                      new_instance->trace_half_width(p_connection->start_layer),
                                      p_ctrl.trace_clearance_class_no);
        }
        else
        {
            logger::FRLogger::instance().warn(
                "FoundConnectionInserter: firstCorner is null for net #" +
                std::to_string(p_ctrl.net_no) +
                ", skipping connect_to_trace for target item. "
                "This may indicate a degenerate route segment.");
        }
    }
    if (const auto* start_trace = dynamic_cast<const board::PolylineTrace*>(p_connection->start_item))
    {
        if (new_instance->last_corner_.has_value())
        {
            p_board->connect_to_trace(*new_instance->last_corner_,
                                      const_cast<board::PolylineTrace*>(start_trace),
                                      new_instance->trace_half_width(p_connection->target_layer),
                                      p_ctrl.trace_clearance_class_no);
        }
        else
        {
            logger::FRLogger::instance().warn(
                "FoundConnectionInserter: lastCorner is null for net #" +
                std::to_string(p_ctrl.net_no) +
                ", skipping connect_to_trace for start item. "
                "This may indicate a degenerate route segment.");
        }
    }

    p_board->normalize_traces(p_ctrl.net_no);

    return new_instance;
}

bool FoundConnectionInserter::insert_trace(const FoundConnectionLocator::ResultItem& p_trace)
{
    // Port of Java FoundConnectionInserter.insertTrace() at
    // FoundConnectionInserter.java:127-453.
    const std::vector<geometry::planar::IntPoint>& corners = p_trace.corners;
    if (corners.empty())
    {
        return false;
    }
    if (corners.size() == 1)
    {
        // Single-point trace: the start and end are the same location (already
        // at the target). Set both first_corner and last_corner so that
        // connect_to_trace is not called with null.
        if (!first_corner_.has_value())
        {
            first_corner_ = corners.front();
        }
        last_corner_ = corners.front();
        return true;
    }

    // switch off correcting connection to pin because it may get wrong in
    // inserting the polygon line for line.
    const double saved_edge_to_turn_dist = board_->get_rules().get_pin_edge_to_turn_dist();
    board_->mutable_rules().set_pin_edge_to_turn_dist(-1);

    // Look for pins at the start and the end of trace in case that neckdown is
    // necessary.
    board::Pin* start_pin = nullptr;
    board::Pin* end_pin = nullptr;
    if (ctrl_->with_neckdown)
    {
        board::ItemSelectionFilter pin_filter(board::ItemSelectionFilter::SelectableChoices::PINS);
        geometry::planar::IntPoint current_end_corner = corners.front();
        for (int i = 0; i < 2; ++i)
        {
            board::ItemSet picked_items =
                board_->pick_items(current_end_corner, p_trace.layer, &pin_filter);
            for (board::Item* current_item : picked_items)
            {
                auto* current_pin = dynamic_cast<board::Pin*>(current_item);
                if (current_pin != nullptr && current_pin->contains_net(ctrl_->net_no)
                    && current_pin->get_center().equals(current_end_corner))
                {
                    if (i == 0)
                    {
                        start_pin = current_pin;
                    }
                    else
                    {
                        end_pin = current_pin;
                    }
                }
            }
            current_end_corner = corners.back();
        }
    }
    const std::vector<int> net_nos{ctrl_->net_no};

    int from_corner_no = 0;
    bool result = true;
    for (int i = 1; i < static_cast<int>(corners.size()); ++i)
    {
        std::vector<geometry::planar::IntPoint> current_corner_arr(corners.begin() + from_corner_no,
                                                                   corners.begin() + i + 1);
        // Java new Polyline(Point[]) closes the polyline with two 45 degree
        // stub lines, which Polyline::from_corners mirrors.
        geometry::planar::Polyline insert_polyline =
            geometry::planar::Polyline::from_corners(current_corner_arr);
        std::optional<geometry::planar::IntPoint> ok_point = board_->insert_forced_trace_polyline(
            insert_polyline, trace_half_width(p_trace.layer), p_trace.layer, net_nos,
            ctrl_->trace_clearance_class_no, ctrl_->max_shove_trace_recursion_depth,
            ctrl_->max_shove_via_recursion_depth, ctrl_->max_spring_over_recursion_depth,
            std::numeric_limits<int>::max(), ctrl_->pull_tight_accuracy, true, nullptr);
        // Java compares the returned corner against the polyline corners by
        // reference identity; the C++ port compares the IntPoint values. A
        // Java null return therefore maps to an empty optional.
        const bool reached_last_corner =
            ok_point.has_value() && *ok_point == insert_polyline.last_corner();
        const bool reached_first_corner =
            ok_point.has_value() && *ok_point == insert_polyline.first_corner();
        bool neckdown_inserted = false;
        bool micro_neckdown_inserted = false;
        if (ok_point.has_value() && !reached_last_corner && ctrl_->with_neckdown
            && current_corner_arr.size() == 2)
        {
            neckdown_inserted = insert_neckdown(*ok_point, current_corner_arr[1], p_trace.layer,
                                                start_pin, end_pin);
        }
        if (!neckdown_inserted && ok_point.has_value() && !reached_last_corner && ctrl_->is_fanout
            && current_corner_arr.size() == 2)
        {
            micro_neckdown_inserted = insert_fanout_micro_neckdown(
                ok_point, current_corner_arr[1], p_trace.layer, net_nos, start_pin, end_pin);
        }
        if (reached_last_corner || neckdown_inserted || micro_neckdown_inserted)
        {
            from_corner_no = i;
        }
        else if (reached_first_corner && i != static_cast<int>(corners.size()) - 1)
        {
            // if ok_point == insert_polyline.first_corner() the spring over may
            // have failed. Spring over may correct the situation because an
            // insertion, which is ok with clearance compensation may cause
            // violations without clearance compensation. In this case
            // repeating the insertion with more distant corners may allow the
            // spring over to correct the situation.
            if (from_corner_no > 0)
            {
                // corners[i] may be inside the offset for the substitute trace
                // around a spring_over obstacle (if clearance compensation is
                // off).
                if (current_corner_arr.size() < 3)
                {
                    // first correction
                    --from_corner_no;
                }
            }
            logger::FRLogger::instance().trace(
                "FoundConnectionInserter: violation corrected");
        }
        else
        {
            logger::FRLogger::instance().debug(
                "FoundConnectionInserter: insert trace failed for net #" +
                std::to_string(ctrl_->net_no) + " at corner " + std::to_string(i) + "/"
                + std::to_string(corners.size() - 1) + " on layer "
                + std::to_string(p_trace.layer) + ", trace width: "
                + std::to_string(trace_half_width(p_trace.layer)) + ", from corner: "
                + std::to_string(from_corner_no) + ", okPoint: "
                + (ok_point.has_value() ? format_point(*ok_point) : std::string("null"))
                + ", target: " + insert_polyline.last_corner().to_string());
            trace_fanout_diagnostic(
                "trace_insert_failed",
                "layer=" + std::to_string(p_trace.layer) + ", corner_index=" + std::to_string(i)
                    + ", from_corner_index=" + std::to_string(from_corner_no)
                    + ", traceHalfWidth=" + std::to_string(trace_half_width(p_trace.layer))
                    + ", traceClearanceClass=" + std::to_string(ctrl_->trace_clearance_class_no)
                    + ", start_pin_clearance_class="
                    + std::to_string(start_pin != nullptr ? start_pin->get_clearance_class() : -1)
                    + ", end_pin_clearance_class="
                    + std::to_string(end_pin != nullptr ? end_pin->get_clearance_class() : -1)
                    + ", okPoint=" + (ok_point.has_value() ? format_point(*ok_point) : std::string("null"))
                    + ", target=" + insert_polyline.last_corner().to_string());
            result = false;
            break;
        }
    }

    for (int i = 0; i < static_cast<int>(corners.size()) - 1; ++i)
    {
        board::Trace* trace_stub = board_->get_trace_tail(corners[static_cast<size_t>(i)],
                                                          p_trace.layer, net_nos);
        if (trace_stub != nullptr)
        {
            board_->remove_item(trace_stub);
        }
    }

    board_->mutable_rules().set_pin_edge_to_turn_dist(saved_edge_to_turn_dist);
    if (!first_corner_.has_value())
    {
        first_corner_ = corners.front();
    }
    last_corner_ = corners.back();
    return result;
}

bool FoundConnectionInserter::insert_fanout_micro_neckdown(
    const std::optional<geometry::planar::IntPoint>& p_ok_point,
    const geometry::planar::IntPoint& p_target_point,
    int p_layer,
    const std::vector<int>& p_net_nos,
    board::Pin* p_start_pin,
    board::Pin* p_end_pin)
{
    // Port of Java FoundConnectionInserter.insertFanoutMicroNeckdown() at
    // FoundConnectionInserter.java:455-523.
    const geometry::planar::IntPoint from_point =
        p_ok_point.has_value() ? *p_ok_point : p_target_point;
    if (from_point == p_target_point)
    {
        return false;
    }
    const int base_half_width = trace_half_width(p_layer);
    // LinkedHashSet<Integer> of Java: ordered and deduplicated.
    std::vector<int> candidate_half_widths;
    auto add_candidate = [&candidate_half_widths](int p_value) {
        if (std::find(candidate_half_widths.begin(), candidate_half_widths.end(), p_value)
            == candidate_half_widths.end())
        {
            candidate_half_widths.push_back(p_value);
        }
    };
    if (p_start_pin != nullptr && p_start_pin->is_on_layer(p_layer))
    {
        add_candidate(p_start_pin->get_trace_neckdown_halfwidth(p_layer));
    }
    if (p_end_pin != nullptr && p_end_pin->is_on_layer(p_layer))
    {
        add_candidate(p_end_pin->get_trace_neckdown_halfwidth(p_layer));
    }
    add_candidate(std::max(1, (base_half_width * 3) / 4));
    add_candidate(std::max(1, (base_half_width * 3) / 5));
    add_candidate(std::max(1, base_half_width / 2));

    for (int candidate_half_width : candidate_half_widths)
    {
        if (candidate_half_width <= 0 || candidate_half_width >= base_half_width)
        {
            continue;
        }
        std::optional<geometry::planar::IntPoint> candidate_ok_point =
            board_->insert_forced_trace_segment(
                from_point, p_target_point, candidate_half_width, p_layer, p_net_nos,
                ctrl_->trace_clearance_class_no, ctrl_->max_shove_trace_recursion_depth,
                ctrl_->max_shove_via_recursion_depth, ctrl_->max_spring_over_recursion_depth,
                std::numeric_limits<int>::max(), ctrl_->pull_tight_accuracy, true, nullptr);
        if (candidate_ok_point.has_value() && *candidate_ok_point == p_target_point)
        {
            trace_fanout_diagnostic(
                "trace_insert_micro_neckdown_success",
                "layer=" + std::to_string(p_layer) + ", candidate_half_width="
                    + std::to_string(candidate_half_width) + ", baseHalfWidth="
                    + std::to_string(base_half_width) + ", traceClearanceClass="
                    + std::to_string(ctrl_->trace_clearance_class_no) + ", from="
                    + format_point(from_point) + ", to=" + format_point(p_target_point));
            return true;
        }
    }
    trace_fanout_diagnostic(
        "trace_insert_micro_neckdown_failed",
        "layer=" + std::to_string(p_layer) + ", baseHalfWidth=" + std::to_string(base_half_width)
            + ", traceClearanceClass=" + std::to_string(ctrl_->trace_clearance_class_no) + ", from="
            + format_point(from_point) + ", to=" + format_point(p_target_point));
    return false;
}

bool FoundConnectionInserter::insert_neckdown(const geometry::planar::IntPoint& p_from_corner,
                                             const geometry::planar::IntPoint& p_to_corner,
                                             int p_layer,
                                             board::Pin* p_start_pin,
                                             board::Pin* p_end_pin)
{
    // Port of Java FoundConnectionInserter.insertNeckdown() at
    // FoundConnectionInserter.java:525-537.
    if (p_start_pin != nullptr)
    {
        std::optional<geometry::planar::IntPoint> ok_point =
            try_neck_down(p_to_corner, p_from_corner, p_layer, p_start_pin, true);
        if (ok_point.has_value() && *ok_point == p_from_corner)
        {
            return true;
        }
    }
    if (p_end_pin != nullptr)
    {
        std::optional<geometry::planar::IntPoint> ok_point =
            try_neck_down(p_from_corner, p_to_corner, p_layer, p_end_pin, false);
        return ok_point.has_value() && *ok_point == p_to_corner;
    }
    return false;
}

std::optional<geometry::planar::IntPoint> FoundConnectionInserter::try_neck_down(
    const geometry::planar::IntPoint& p_from_corner,
    const geometry::planar::IntPoint& p_to_corner,
    int p_layer,
    board::Pin* p_pin,
    bool p_at_start)
{
    // Port of Java FoundConnectionInserter.tryNeckDown() at
    // FoundConnectionInserter.java:539-676.
    // Java declares p_at_start but never reads it; kept for signature parity.
    (void)p_at_start;
    if (p_pin == nullptr || !p_pin->is_on_layer(p_layer))
    {
        return std::nullopt;
    }
    const geometry::planar::FloatPoint pin_center = p_pin->get_center().to_float();
    const double current_clearance =
        board_->get_rules().get_clearance_matrix().get_value(ctrl_->trace_clearance_class_no,
                                                             p_pin->get_clearance_class(), p_layer,
                                                             true);
    const double pin_neckdown_distance = 2.0 * (0.5 * p_pin->get_max_width(p_layer) + current_clearance);
    if (pin_center.distance(p_to_corner.to_float()) >= pin_neckdown_distance)
    {
        return std::nullopt;
    }

    const int neckdown_halfwidth = p_pin->get_trace_neckdown_halfwidth(p_layer);
    if (neckdown_halfwidth >= trace_half_width(p_layer))
    {
        return std::nullopt;
    }

    const geometry::planar::FloatPoint float_from_corner = p_from_corner.to_float();
    const geometry::planar::FloatPoint float_to_corner = p_to_corner.to_float();

    static constexpr int kTolerance = 2;

    const std::vector<int> net_nos{ctrl_->net_no};

    double ok_length = board_->check_trace_segment(p_from_corner, p_to_corner, p_layer, net_nos,
                                                   trace_half_width(p_layer),
                                                   ctrl_->trace_clearance_class_no, true);
    if (ok_length >= static_cast<double>(std::numeric_limits<int>::max()))
    {
        return p_from_corner;
    }
    ok_length -= kTolerance;
    geometry::planar::IntPoint neckdown_end_point;
    if (ok_length <= kTolerance)
    {
        neckdown_end_point = p_from_corner;
    }
    else
    {
        const geometry::planar::FloatPoint float_neckdown_end_point =
            float_from_corner.change_length(float_to_corner, ok_length);
        neckdown_end_point = float_neckdown_end_point.round();
        // add a corner in case neckdown_end_point is not exactly on the line
        // from p_from_corner to p_to_corner
        const bool horizontal_first =
            std::abs(float_from_corner.x - float_neckdown_end_point.x)
            >= std::abs(float_from_corner.y - float_neckdown_end_point.y);
        geometry::planar::IntPoint add_corner = FoundConnectionLocator::calculate_additional_corner(
                float_from_corner, float_neckdown_end_point, horizontal_first,
                to_board_angle_restriction(board_->get_rules().get_trace_angle_restriction()))
                                                  .round();
        std::optional<geometry::planar::IntPoint> current_ok_point = insert_forced_segment(
            p_from_corner, add_corner, trace_half_width(p_layer), p_layer);
        if (!current_ok_point.has_value() || *current_ok_point != add_corner)
        {
            return p_from_corner;
        }
        current_ok_point = insert_forced_segment(add_corner, neckdown_end_point,
                                                 trace_half_width(p_layer), p_layer);
        if (!current_ok_point.has_value() || *current_ok_point != neckdown_end_point)
        {
            return p_from_corner;
        }
        add_corner = FoundConnectionLocator::calculate_additional_corner(
                         float_neckdown_end_point, float_to_corner, !horizontal_first,
                         to_board_angle_restriction(board_->get_rules().get_trace_angle_restriction()))
                         .round();
        if (!add_corner.equals(p_to_corner))
        {
            current_ok_point = insert_forced_segment(neckdown_end_point, add_corner,
                                                     trace_half_width(p_layer), p_layer);
            if (!current_ok_point.has_value() || *current_ok_point != add_corner)
            {
                return p_from_corner;
            }
            neckdown_end_point = add_corner;
        }
    }

    return insert_forced_segment(neckdown_end_point, p_to_corner, neckdown_halfwidth, p_layer);
}

bool FoundConnectionInserter::insert_via(const geometry::planar::IntPoint& p_location,
                                         int p_input_from_layer,
                                         int p_input_to_layer)
{
    // Port of Java FoundConnectionInserter.insertVia() at
    // FoundConnectionInserter.java:683-785.
    if (p_input_from_layer == p_input_to_layer)
    {
        return true; // no via necessary
    }
    // sort the input layers
    const int from_layer = std::min(p_input_from_layer, p_input_to_layer);
    const int to_layer = std::max(p_input_from_layer, p_input_to_layer);
    const std::vector<int> net_nos{ctrl_->net_no};
    const rules::ViaRule* via_rule = ctrl_->via_rule;
    if (via_rule == nullptr)
    {
        logger::FRLogger::instance().debug(
            "FoundConnectionInserter: no via rule for net #" + std::to_string(ctrl_->net_no));
        trace_fanout_diagnostic("via_mask_not_found",
                                "fromLayer=" + std::to_string(from_layer) + ", toLayer="
                                    + std::to_string(to_layer) + ", location="
                                    + format_point(p_location) + ", via rule missing");
        return false;
    }
    std::shared_ptr<rules::ViaInfo> via_info;
    bool found_suitable_span = false;
    for (int i = 0; i < via_rule->via_count(); ++i)
    {
        std::shared_ptr<rules::ViaInfo> current_via_info = via_rule->get_via(i);
        if (current_via_info == nullptr)
        {
            continue;
        }
        board::Padstack* current_via_padstack = current_via_info->get_padstack();
        if (current_via_padstack == nullptr)
        {
            continue;
        }
        if (current_via_padstack->get_from_layer() > from_layer
            || current_via_padstack->get_to_layer() < to_layer)
        {
            continue;
        }
        found_suitable_span = true;
        if (board::ForcedViaAlgo::check(current_via_info, p_location, net_nos,
                                        ctrl_->max_shove_trace_recursion_depth,
                                        ctrl_->max_shove_via_recursion_depth, board_,
                                        &ctrl_->trace_half_width,
                                        ctrl_->trace_clearance_class_no))
        {
            via_info = current_via_info;
            break;
        }
    }
    if (via_info == nullptr)
    {
        if (!found_suitable_span)
        {
            logger::FRLogger::instance().debug(
                "FoundConnectionInserter: via mask not found for net #"
                + std::to_string(ctrl_->net_no) + " covering layers " + std::to_string(from_layer)
                + " to " + std::to_string(to_layer));
        }
        else
        {
            logger::FRLogger::instance().debug(
                "FoundConnectionInserter: via placement blocked by clearance/shove limits for net #"
                + std::to_string(ctrl_->net_no));
        }
        trace_fanout_diagnostic(
            "via_mask_not_found",
            "fromLayer=" + std::to_string(from_layer) + ", toLayer=" + std::to_string(to_layer)
                + ", location=" + format_point(p_location) + ", traceClearanceClass="
                + std::to_string(ctrl_->trace_clearance_class_no) + ", viaClearanceClass="
                + std::to_string(ctrl_->via_clearance_class) + ", trace_half_width_from="
                + std::to_string(trace_half_width(from_layer)) + ", trace_half_width_to="
                + std::to_string(trace_half_width(to_layer)));
        return false;
    }
    // insert the via
    if (!board::ForcedViaAlgo::insert(via_info, p_location, net_nos,
                                      ctrl_->trace_clearance_class_no, ctrl_->trace_half_width,
                                      ctrl_->max_shove_trace_recursion_depth,
                                      ctrl_->max_shove_via_recursion_depth, board_))
    {
        logger::FRLogger::instance().debug(
            "FoundConnectionInserter: forced via failed for net #" + std::to_string(ctrl_->net_no));
        trace_fanout_diagnostic(
            "forced_via_insert_failed",
            "fromLayer=" + std::to_string(from_layer) + ", toLayer=" + std::to_string(to_layer)
                + ", location=" + format_point(p_location) + ", selected_via_clearance_class="
                + std::to_string(via_info->get_clearance_class()) + ", traceClearanceClass="
                + std::to_string(ctrl_->trace_clearance_class_no) + ", trace_half_width_from="
                + std::to_string(trace_half_width(from_layer)) + ", trace_half_width_to="
                + std::to_string(trace_half_width(to_layer)));
        return false;
    }
    return true;
}

bool FoundConnectionInserter::should_trace_fanout_diagnostics() const
{
    return ctrl_->is_fanout && !ctrl_->fanout_start_pin_name.empty()
        && ctrl_->fanout_start_pin_name.rfind("U27-", 0) == 0;
}

void FoundConnectionInserter::trace_fanout_diagnostic(const std::string& p_event,
                                                      const std::string& p_message) const
{
    if (!should_trace_fanout_diagnostics())
    {
        return;
    }
    logger::FRLogger::instance().trace("FANOUT_DIG event=" + p_event + ", pin="
                                        + ctrl_->fanout_start_pin_name + ", net="
                                        + std::to_string(ctrl_->net_no) + ", " + p_message);
}

} // namespace freerouting::autoroute
