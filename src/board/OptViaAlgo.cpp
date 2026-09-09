#include "board/OptViaAlgo.h"

#include <cmath>
#include <limits>
#include <set>

#include "autoroute/ExpansionCostFactor.h"
#include "board/ConductionArea.h"
#include "board/ItemSelectionFilter.h"
#include "board/MoveDrillItemAlgo.h"
#include "board/PolylineTrace.h"
#include "board/RoutingBoard.h"
#include "board/Via.h"
#include "geometry/planar/FloatLine.h"
#include "geometry/planar/Point.h"
#include "geometry/planar/Polyline.h"
#include "geometry/planar/Vector.h"
#include "logger/FRLogger.h"
#include "rules/BoardRules.h"

namespace freerouting::board {

namespace {

constexpr double NO_CONFLICT_LENGTH = static_cast<double>(std::numeric_limits<int>::max());

bool is_orthogonal_vec(const geometry::planar::Vector& p_vector)
{
    if (const auto* int_vec = dynamic_cast<const geometry::planar::IntVector*>(&p_vector))
    {
        return int_vec->is_orthogonal();
    }
    const geometry::planar::FloatPoint f = p_vector.to_float();
    return f.x == 0.0 || f.y == 0.0;
}

bool is_multiple_of_45_vec(const geometry::planar::Vector& p_vector)
{
    if (const auto* int_vec = dynamic_cast<const geometry::planar::IntVector*>(&p_vector))
    {
        return int_vec->is_multiple_of_45_degree();
    }
    const geometry::planar::FloatPoint f = p_vector.to_float();
    return f.x == 0.0 || f.y == 0.0 || std::abs(std::abs(f.x) - std::abs(f.y)) < 1e-9;
}

const std::vector<Item*> EMPTY_IGNORE;

} // namespace

bool OptViaAlgo::opt_via_location(RoutingBoard* p_board,
                                  Via* p_via,
                                  const std::vector<autoroute::ExpansionCostFactor>* p_trace_cost_arr,
                                  int p_trace_pull_tight_accuracy,
                                  int p_max_recursion_depth)
{
    if (p_board == nullptr || p_via == nullptr)
    {
        return false;
    }
    if (p_via->is_shove_fixed())
    {
        return false;
    }
    if (p_max_recursion_depth <= 0)
    {
        logger::FRLogger::instance().debug("OptViaAlgo.opt_via_location: probably endless loop");
        return false;
    }
    std::vector<Item*> contacts = p_via->get_normal_contacts();
    bool is_plane_or_fanout_via = contacts.size() == 1;
    PolylineTrace* first_trace = nullptr;
    PolylineTrace* second_trace = nullptr;
    if (!is_plane_or_fanout_via)
    {
        if (contacts.size() != 2)
        {
            return false;
        }
        Item* curr_item = contacts[0];
        if (curr_item->is_shove_fixed() || dynamic_cast<PolylineTrace*>(curr_item) == nullptr)
        {
            if (dynamic_cast<ConductionArea*>(curr_item) != nullptr)
            {
                is_plane_or_fanout_via = true;
            }
            else
            {
                return false;
            }
        }
        else
        {
            first_trace = static_cast<PolylineTrace*>(curr_item);
        }
        curr_item = contacts[1];
        if (curr_item->is_shove_fixed() || dynamic_cast<PolylineTrace*>(curr_item) == nullptr)
        {
            if (dynamic_cast<ConductionArea*>(curr_item) != nullptr)
            {
                is_plane_or_fanout_via = true;
            }
            else
            {
                return false;
            }
        }
        else
        {
            second_trace = static_cast<PolylineTrace*>(curr_item);
        }
    }
    if (is_plane_or_fanout_via)
    {
        return opt_plane_or_fanout_via(p_board, p_via, p_trace_pull_tight_accuracy,
                                       p_max_recursion_depth);
    }
    if (first_trace == nullptr || second_trace == nullptr)
    {
        return false;
    }
    const geometry::planar::IntPoint via_center = p_via->get_center();
    const int first_layer = first_trace->get_layer();
    const int second_layer = second_trace->get_layer();

    // calculate first_trace_from_corner and second_trace_from_corner
    // Use tolerance-based comparison to match connectivity detection logic
    const int tolerance = static_cast<int>(p_via->min_width() / 2) + 1;

    geometry::planar::IntPoint first_trace_from_corner;
    bool first_found = false;
    if (is_within_tolerance(first_trace->first_corner(), via_center, tolerance))
    {
        first_trace_from_corner = first_trace->polyline().corner(1);
        first_found = true;
    }
    else if (is_within_tolerance(first_trace->last_corner(), via_center, tolerance))
    {
        first_trace_from_corner =
            first_trace->polyline().corner(first_trace->polyline().corner_count() - 2);
        first_found = true;
    }
    if (!first_found)
    {
        // Via is not connected at trace endpoints - skip optimization
        return false;
    }

    geometry::planar::IntPoint second_trace_from_corner;
    bool second_found = false;
    if (is_within_tolerance(second_trace->first_corner(), via_center, tolerance))
    {
        second_trace_from_corner = second_trace->polyline().corner(1);
        second_found = true;
    }
    else if (is_within_tolerance(second_trace->last_corner(), via_center, tolerance))
    {
        second_trace_from_corner =
            second_trace->polyline().corner(second_trace->polyline().corner_count() - 2);
        second_found = true;
    }
    if (!second_found)
    {
        // Via is not connected at trace endpoints - skip optimization
        return false;
    }

    autoroute::ExpansionCostFactor first_layer_trace_costs(1, 1);
    autoroute::ExpansionCostFactor second_layer_trace_costs(1, 1);
    if (p_trace_cost_arr != nullptr)
    {
        if (first_layer >= 0 && first_layer < static_cast<int>(p_trace_cost_arr->size()))
        {
            first_layer_trace_costs = (*p_trace_cost_arr)[first_layer];
        }
        if (second_layer >= 0 && second_layer < static_cast<int>(p_trace_cost_arr->size()))
        {
            second_layer_trace_costs = (*p_trace_cost_arr)[second_layer];
        }
        else
        {
            second_layer_trace_costs = first_layer_trace_costs;
        }
    }
    else
    {
        second_layer_trace_costs = first_layer_trace_costs;
    }

    std::optional<geometry::planar::IntPoint> new_location =
        reposition_via(p_board, p_via, first_trace->get_half_width(),
                       first_trace->get_clearance_class(), first_trace->get_layer(),
                       first_layer_trace_costs, first_trace_from_corner,
                       second_trace->get_half_width(), second_trace->get_clearance_class(),
                       second_trace->get_layer(), second_layer_trace_costs,
                       second_trace_from_corner);
    if (!new_location.has_value() || *new_location == via_center)
    {
        return false;
    }
    std::unique_ptr<geometry::planar::Vector> delta = new_location->difference_by(via_center);
    if (!MoveDrillItemAlgo::insert(p_via, *delta, 9, 9, nullptr, p_board))
    {
        logger::FRLogger::instance().warn("OptViaAlgo.opt_via_location: move via failed");
        return false;
    }
    ItemSelectionFilter trace_filter(ItemSelectionFilter::SelectableChoices::TRACES);
    std::set<Item*> picked_items = p_board->pick_items(*new_location, first_trace->get_layer(),
                                                       &trace_filter);
    for (Item* curr_item : picked_items)
    {
        if (auto* trace = dynamic_cast<PolylineTrace*>(curr_item))
        {
            trace->pull_tight(true, p_trace_pull_tight_accuracy, nullptr);
        }
    }
    picked_items = p_board->pick_items(*new_location, second_trace->get_layer(), &trace_filter);
    for (Item* curr_item : picked_items)
    {
        if (auto* trace = dynamic_cast<PolylineTrace*>(curr_item))
        {
            trace->pull_tight(true, p_trace_pull_tight_accuracy, nullptr);
        }
    }
    ItemSelectionFilter via_filter(ItemSelectionFilter::SelectableChoices::VIAS);
    picked_items = p_board->pick_items(*new_location, first_trace->get_layer(), &via_filter);
    for (Item* curr_item : picked_items)
    {
        if (auto* curr_via = dynamic_cast<Via*>(curr_item))
        {
            opt_via_location(p_board, curr_via, p_trace_cost_arr, p_trace_pull_tight_accuracy,
                             p_max_recursion_depth - 1);
        }
        break;
    }
    return true;
}

bool OptViaAlgo::opt_plane_or_fanout_via(RoutingBoard* p_board,
                                         Via* p_via,
                                         int p_trace_pull_tight_accuracy,
                                         int p_max_recursion_depth)
{
    if (p_max_recursion_depth <= 0)
    {
        logger::FRLogger::instance().debug(
            "OptViaAlgo.opt_plane_or_fanout_via: probably endless loop");
        return false;
    }
    std::vector<Item*> contact_list = p_via->get_normal_contacts();
    if (contact_list.empty())
    {
        return false;
    }
    ConductionArea* contact_plane = nullptr;
    PolylineTrace* contact_trace = nullptr;
    for (Item* curr_contact : contact_list)
    {
        if (auto* area = dynamic_cast<ConductionArea*>(curr_contact))
        {
            if (contact_plane != nullptr)
            {
                return false;
            }
            contact_plane = area;
        }
        else if (auto* trace = dynamic_cast<PolylineTrace*>(curr_contact))
        {
            if (curr_contact->is_shove_fixed() || contact_trace != nullptr)
            {
                return false;
            }
            contact_trace = trace;
        }
        else
        {
            return false;
        }
    }
    if (contact_trace == nullptr)
    {
        return false;
    }
    const geometry::planar::IntPoint via_center = p_via->get_center();

    // Use tolerance based on via size, matching the logic in opt_via_location
    const int tolerance = static_cast<int>(p_via->min_width() / 2) + 1;

    bool at_first_corner = false;
    if (is_within_tolerance(contact_trace->first_corner(), via_center, tolerance))
    {
        at_first_corner = true;
    }
    else if (is_within_tolerance(contact_trace->last_corner(), via_center, tolerance))
    {
        at_first_corner = false;
    }
    else
    {
        // Via is not connected at trace endpoints - skip optimization
        return false;
    }
    const geometry::planar::Polyline& trace_polyline = contact_trace->polyline();
    geometry::planar::IntPoint check_corner;
    if (at_first_corner)
    {
        check_corner = trace_polyline.corner(1);
    }
    else
    {
        check_corner = trace_polyline.corner(trace_polyline.corner_count() - 2);
    }
    const int trace_half_width = contact_trace->get_half_width();
    const int trace_layer = contact_trace->get_layer();
    const int trace_cl_class_no = contact_trace->get_clearance_class();
    std::optional<geometry::planar::IntPoint> new_via_location =
        reposition_via(p_board, p_via, check_corner, trace_half_width, trace_layer,
                       trace_cl_class_no);
    if (!new_via_location.has_value() && trace_polyline.corner_count() >= 3)
    {
        // try to project the via to the previous line
        geometry::planar::IntPoint prev_corner;
        if (at_first_corner)
        {
            prev_corner = trace_polyline.corner(2);
        }
        else
        {
            prev_corner = trace_polyline.corner(trace_polyline.corner_count() - 3);
        }
        const geometry::planar::FloatPoint float_check_corner = check_corner.to_float();
        const geometry::planar::FloatPoint float_via_center = via_center.to_float();
        const geometry::planar::FloatPoint float_prev_corner = prev_corner.to_float();
        if (float_check_corner.scalar_product(float_via_center, float_prev_corner) != 0.0)
        {
            geometry::planar::FloatLine curr_line(float_check_corner, float_prev_corner);
            geometry::planar::IntPoint projection =
                curr_line.perpendicular_projection(float_via_center).round();
            std::unique_ptr<geometry::planar::Vector> diff_vector =
                projection.difference_by(via_center);
            bool projection_ok = true;
            rules::AngleRestriction angle_restriction =
                p_board->get_rules().get_trace_angle_restriction();
            if (projection == via_center
                || (angle_restriction == rules::AngleRestriction::RIGHT
                    && !is_orthogonal_vec(*diff_vector))
                || (angle_restriction == rules::AngleRestriction::FORTY_FIVE
                    && !is_multiple_of_45_vec(*diff_vector)))
            {
                projection_ok = false;
            }
            if (projection_ok
                && MoveDrillItemAlgo::check(p_via, *diff_vector, 0, 0, EMPTY_IGNORE, p_board,
                                            nullptr))
            {
                const double ok_length = p_board->check_trace_segment(
                    via_center, projection, trace_layer, p_via->get_net_nos(), trace_half_width,
                    trace_cl_class_no, false);
                if (ok_length >= NO_CONFLICT_LENGTH)
                {
                    new_via_location = projection;
                }
            }
        }
    }
    if (!new_via_location.has_value())
    {
        return false;
    }
    if (contact_plane != nullptr)
    {
        // check, that the new location is inside the contact plane
        ItemSelectionFilter conduction_filter(
            ItemSelectionFilter::SelectableChoices::CONDUCTION);
        std::set<Item*> picked_items =
            p_board->pick_items(*new_via_location, contact_plane->get_layer(), &conduction_filter);
        bool contact_ok = false;
        for (Item* curr_item : picked_items)
        {
            if (curr_item == contact_plane)
            {
                contact_ok = true;
                break;
            }
        }
        if (!contact_ok)
        {
            return false;
        }
    }
    std::unique_ptr<geometry::planar::Vector> diff_vector =
        new_via_location->difference_by(via_center);
    if (!MoveDrillItemAlgo::insert(p_via, *diff_vector, 9, 9, nullptr, p_board))
    {
        logger::FRLogger::instance().warn("OptViaAlgo.opt_plane_or_fanout_via: move via failed");
        return false;
    }
    ItemSelectionFilter trace_filter(ItemSelectionFilter::SelectableChoices::TRACES);
    std::set<Item*> picked_items =
        p_board->pick_items(*new_via_location, contact_trace->get_layer(), &trace_filter);
    for (Item* curr_item : picked_items)
    {
        if (auto* trace = dynamic_cast<PolylineTrace*>(curr_item))
        {
            trace->pull_tight(true, p_trace_pull_tight_accuracy, nullptr);
        }
    }
    if (*new_via_location == check_corner)
    {
        opt_plane_or_fanout_via(p_board, p_via, p_trace_pull_tight_accuracy,
                                p_max_recursion_depth - 1);
    }
    return true;
}

std::optional<geometry::planar::IntPoint> OptViaAlgo::reposition_via(
    RoutingBoard* p_board,
    Via* p_via,
    const geometry::planar::IntPoint& p_to_location,
    int p_trace_half_width,
    int p_trace_layer,
    int p_trace_cl_class)
{
    const geometry::planar::IntPoint from_location = p_via->get_center();
    if (from_location == p_to_location)
    {
        return std::nullopt;
    }
    const double ok_length = p_board->check_trace_segment(
        from_location, p_to_location, p_trace_layer, p_via->get_net_nos(), p_trace_half_width,
        p_trace_cl_class, false);
    if (ok_length <= 0.0)
    {
        return std::nullopt;
    }
    const geometry::planar::FloatPoint float_from_location = from_location.to_float();
    const geometry::planar::FloatPoint float_to_location = p_to_location.to_float();
    geometry::planar::FloatPoint new_float_to_location;
    if (ok_length >= NO_CONFLICT_LENGTH)
    {
        new_float_to_location = float_to_location;
    }
    else
    {
        new_float_to_location = float_from_location.change_length(float_to_location, ok_length);
    }
    const geometry::planar::IntPoint new_to_location = new_float_to_location.round();
    std::unique_ptr<geometry::planar::Vector> delta = new_to_location.difference_by(from_location);
    if (MoveDrillItemAlgo::check(p_via, *delta, 0, 0, EMPTY_IGNORE, p_board, nullptr))
    {
        return new_to_location;
    }

    const double c_min_length = 0.3 * p_trace_half_width + 1.0;
    double remaining_length = std::min(ok_length, float_from_location.distance(float_to_location));
    double curr_length = remaining_length / 2.0;
    double confirmed_length = 0.0;
    std::optional<geometry::planar::IntPoint> result;
    while (curr_length >= c_min_length)
    {
        geometry::planar::IntPoint check_point =
            float_from_location.change_length(float_to_location, confirmed_length + curr_length)
                .round();
        std::unique_ptr<geometry::planar::Vector> check_delta =
            check_point.difference_by(from_location);
        if (MoveDrillItemAlgo::check(p_via, *check_delta, 0, 0, EMPTY_IGNORE, p_board, nullptr))
        {
            confirmed_length += curr_length;
            result = check_point;
        }
        curr_length /= 2.0;
    }
    return result;
}

bool OptViaAlgo::reposition_via(RoutingBoard* p_board,
                                Via* p_via,
                                const geometry::planar::IntPoint& p_to_location,
                                int p_trace_half_width_1,
                                int p_trace_layer_1,
                                int p_trace_cl_class_1,
                                const geometry::planar::IntPoint& p_connect_location,
                                int p_trace_half_width_2,
                                int p_trace_layer_2,
                                int p_trace_cl_class_2)
{
    const geometry::planar::IntPoint from_location = p_via->get_center();
    if (from_location == p_to_location)
    {
        logger::FRLogger::instance().trace("OptViaAlgo.reposition_via: from_location equal p_to_location");
        return false;
    }
    std::unique_ptr<geometry::planar::Vector> delta = p_to_location.difference_by(from_location);
    if (p_board->get_rules().get_trace_angle_restriction() == rules::AngleRestriction::NONE
        && delta->length_approx() <= 1.5)
    {
        // PullTightAlgoAnyAngle.reduce_corners may not be able to remove the new
        // generated overlap because of numerical stability problems. That would
        // result in an endless loop with removing the generated acute angle in
        // reposition_via.
        return false;
    }
    const std::vector<int>& net_no_arr = p_via->get_net_nos();
    if (p_board->check_trace_segment(from_location, p_to_location, p_trace_layer_1, net_no_arr,
                                     p_trace_half_width_1, p_trace_cl_class_1, false)
        < NO_CONFLICT_LENGTH)
    {
        return false;
    }
    if (p_board->check_trace_segment(p_to_location, p_connect_location, p_trace_layer_2,
                                     net_no_arr, p_trace_half_width_2, p_trace_cl_class_2, false)
        < NO_CONFLICT_LENGTH)
    {
        return false;
    }
    return MoveDrillItemAlgo::check(p_via, *delta, 0, 0, EMPTY_IGNORE, p_board, nullptr);
}

std::optional<geometry::planar::IntPoint> OptViaAlgo::reposition_via(
    RoutingBoard* p_board,
    Via* p_via,
    int p_first_trace_half_width,
    int p_first_trace_cl_class,
    int p_first_trace_layer,
    const autoroute::ExpansionCostFactor& p_first_trace_costs,
    const geometry::planar::IntPoint& p_first_trace_from_corner,
    int p_second_trace_half_width,
    int p_second_trace_cl_class,
    int p_second_trace_layer,
    const autoroute::ExpansionCostFactor& p_second_trace_costs,
    const geometry::planar::IntPoint& p_second_trace_from_corner)
{
    const geometry::planar::IntPoint via_location = p_via->get_center();

    std::unique_ptr<geometry::planar::Vector> first_delta =
        p_first_trace_from_corner.difference_by(via_location);
    std::unique_ptr<geometry::planar::Vector> second_delta =
        p_second_trace_from_corner.difference_by(via_location);
    const double scalar_product = first_delta->scalar_product(*second_delta);

    const geometry::planar::FloatPoint float_via_location = via_location.to_float();
    const geometry::planar::FloatPoint float_first_trace_from_corner =
        p_first_trace_from_corner.to_float();
    const geometry::planar::FloatPoint float_second_trace_from_corner =
        p_second_trace_from_corner.to_float();
    const double first_trace_from_corner_distance =
        float_via_location.distance(float_first_trace_from_corner);
    const double second_trace_from_corner_distance =
        float_via_location.distance(float_second_trace_from_corner);

    // handle case of overlapping lines first
    if (via_location.side_of(p_first_trace_from_corner, p_second_trace_from_corner)
            == geometry::planar::Side::COLLINEAR
        && scalar_product > 0.0)
    {
        if (second_trace_from_corner_distance < first_trace_from_corner_distance)
        {
            return reposition_via(p_board, p_via, p_second_trace_from_corner,
                                  p_first_trace_half_width, p_first_trace_layer,
                                  p_first_trace_cl_class);
        }
        return reposition_via(p_board, p_via, p_first_trace_from_corner,
                              p_second_trace_half_width, p_second_trace_layer,
                              p_second_trace_cl_class);
    }
    std::optional<geometry::planar::IntPoint> result;

    double curr_weighted_distance_1 = float_via_location.weighted_distance(
        float_first_trace_from_corner, p_first_trace_costs.horizontal,
        p_first_trace_costs.vertical);
    double curr_weighted_distance_2 = float_via_location.weighted_distance(
        float_first_trace_from_corner, p_second_trace_costs.horizontal,
        p_second_trace_costs.vertical);

    if (curr_weighted_distance_1 > curr_weighted_distance_2)
    {
        // try to move the via in direction of p_first_trace_from_corner
        result = reposition_via(p_board, p_via, p_first_trace_from_corner,
                                p_second_trace_half_width, p_second_trace_layer,
                                p_second_trace_cl_class);
        if (result.has_value())
        {
            return result;
        }
    }

    curr_weighted_distance_1 = float_via_location.weighted_distance(
        float_second_trace_from_corner, p_second_trace_costs.horizontal,
        p_second_trace_costs.vertical);
    curr_weighted_distance_2 = float_via_location.weighted_distance(
        float_second_trace_from_corner, p_first_trace_costs.horizontal,
        p_first_trace_costs.vertical);

    if (curr_weighted_distance_1 > curr_weighted_distance_2)
    {
        // try to move the via in direction of p_second_trace_from_corner
        result = reposition_via(p_board, p_via, p_second_trace_from_corner,
                                p_first_trace_half_width, p_first_trace_layer,
                                p_first_trace_cl_class);
        if (result.has_value())
        {
            return result;
        }
    }
    if (scalar_product > 0.0
        && p_board->get_rules().get_trace_angle_restriction()
            != rules::AngleRestriction::RIGHT)
    {
        // acute angle
        geometry::planar::IntPoint to_point_1;
        geometry::planar::IntPoint to_point_2;
        geometry::planar::FloatPoint float_to_point_1;
        geometry::planar::FloatPoint float_to_point_2;
        if (first_trace_from_corner_distance < second_trace_from_corner_distance)
        {
            to_point_1 = p_first_trace_from_corner;
            float_to_point_1 = float_first_trace_from_corner;
            float_to_point_2 = float_via_location.change_length(float_second_trace_from_corner,
                                                                first_trace_from_corner_distance);
            to_point_2 = float_to_point_2.round();
        }
        else
        {
            float_to_point_1 = float_via_location.change_length(float_first_trace_from_corner,
                                                                second_trace_from_corner_distance);
            to_point_1 = float_to_point_1.round();
            to_point_2 = p_second_trace_from_corner;
            float_to_point_2 = float_second_trace_from_corner;
        }
        curr_weighted_distance_1 = float_to_point_1.weighted_distance(
            float_to_point_2, p_first_trace_costs.horizontal, p_first_trace_costs.vertical);
        curr_weighted_distance_2 = float_to_point_1.weighted_distance(
            float_to_point_2, p_second_trace_costs.horizontal, p_second_trace_costs.vertical);

        if (curr_weighted_distance_1 > curr_weighted_distance_2)
        {
            // try moving the via first into the direction of to_point_1
            result = reposition_via(p_board, p_via, to_point_1, p_second_trace_half_width,
                                    p_second_trace_layer, p_second_trace_cl_class);
            if (!result.has_value())
            {
                result = reposition_via(p_board, p_via, to_point_2, p_first_trace_half_width,
                                        p_first_trace_layer, p_first_trace_cl_class);
            }
        }
        else
        {
            // try moving the via first into the direction of to_point_2
            result = reposition_via(p_board, p_via, to_point_2, p_first_trace_half_width,
                                    p_first_trace_layer, p_first_trace_cl_class);
            if (!result.has_value())
            {
                result = reposition_via(p_board, p_via, to_point_1, p_second_trace_half_width,
                                        p_second_trace_layer, p_second_trace_cl_class);
            }
        }
        if (result.has_value())
        {
            return result;
        }
    }

    // try decomposition in axisparallel parts
    if (!is_orthogonal_vec(*first_delta))
    {
        geometry::planar::FloatPoint float_check_location(float_via_location.x,
                                                          float_first_trace_from_corner.y);

        curr_weighted_distance_1 = float_via_location.weighted_distance(
            float_first_trace_from_corner, p_first_trace_costs.horizontal,
            p_first_trace_costs.vertical);
        curr_weighted_distance_2 = float_via_location.weighted_distance(
            float_check_location, p_second_trace_costs.horizontal,
            p_second_trace_costs.vertical);
        double curr_weighted_distance_3 = float_check_location.weighted_distance(
            float_first_trace_from_corner, p_first_trace_costs.horizontal,
            p_first_trace_costs.vertical);

        if (curr_weighted_distance_1 > curr_weighted_distance_2 + curr_weighted_distance_3)
        {
            geometry::planar::IntPoint check_location = float_check_location.round();
            if (reposition_via(p_board, p_via, check_location, p_second_trace_half_width,
                               p_second_trace_layer, p_second_trace_cl_class,
                               p_first_trace_from_corner, p_first_trace_half_width,
                               p_first_trace_layer, p_first_trace_cl_class))
            {
                return check_location;
            }
        }

        float_check_location = geometry::planar::FloatPoint(float_first_trace_from_corner.x,
                                                            float_via_location.y);

        curr_weighted_distance_2 = float_via_location.weighted_distance(
            float_check_location, p_second_trace_costs.horizontal,
            p_second_trace_costs.vertical);
        curr_weighted_distance_3 = float_check_location.weighted_distance(
            float_first_trace_from_corner, p_first_trace_costs.horizontal,
            p_first_trace_costs.vertical);

        if (curr_weighted_distance_1 > curr_weighted_distance_2 + curr_weighted_distance_3)
        {
            geometry::planar::IntPoint check_location = float_check_location.round();
            if (reposition_via(p_board, p_via, check_location, p_second_trace_half_width,
                               p_second_trace_layer, p_second_trace_cl_class,
                               p_first_trace_from_corner, p_first_trace_half_width,
                               p_first_trace_layer, p_first_trace_cl_class))
            {
                return check_location;
            }
        }
    }

    if (!is_orthogonal_vec(*second_delta))
    {
        geometry::planar::FloatPoint float_check_location(float_via_location.x,
                                                          float_second_trace_from_corner.y);

        curr_weighted_distance_1 = float_via_location.weighted_distance(
            float_second_trace_from_corner, p_second_trace_costs.horizontal,
            p_second_trace_costs.vertical);
        curr_weighted_distance_2 = float_via_location.weighted_distance(
            float_check_location, p_first_trace_costs.horizontal,
            p_first_trace_costs.vertical);
        double curr_weighted_distance_3 = float_check_location.weighted_distance(
            float_second_trace_from_corner, p_second_trace_costs.horizontal,
            p_second_trace_costs.vertical);

        if (curr_weighted_distance_1 > curr_weighted_distance_2 + curr_weighted_distance_3)
        {
            geometry::planar::IntPoint check_location = float_check_location.round();
            if (reposition_via(p_board, p_via, check_location, p_first_trace_half_width,
                               p_first_trace_layer, p_first_trace_cl_class,
                               p_second_trace_from_corner, p_second_trace_half_width,
                               p_second_trace_layer, p_second_trace_cl_class))
            {
                return check_location;
            }
        }

        float_check_location = geometry::planar::FloatPoint(float_second_trace_from_corner.x,
                                                            float_via_location.y);

        curr_weighted_distance_2 = float_via_location.weighted_distance(
            float_check_location, p_first_trace_costs.horizontal,
            p_first_trace_costs.vertical);
        curr_weighted_distance_3 = float_check_location.weighted_distance(
            float_second_trace_from_corner, p_second_trace_costs.horizontal,
            p_second_trace_costs.vertical);

        if (curr_weighted_distance_1 > curr_weighted_distance_2 + curr_weighted_distance_3)
        {
            geometry::planar::IntPoint check_location = float_check_location.round();
            if (reposition_via(p_board, p_via, check_location, p_first_trace_half_width,
                               p_first_trace_layer, p_first_trace_cl_class,
                               p_second_trace_from_corner, p_second_trace_half_width,
                               p_second_trace_layer, p_second_trace_cl_class))
            {
                return check_location;
            }
        }
    }
    return std::nullopt;
}

bool OptViaAlgo::is_within_tolerance(const geometry::planar::Point& p_1,
                                     const geometry::planar::Point& p_2,
                                     int p_tolerance)
{
    // Convert to FloatPoint for distance calculation, use Manhattan distance
    // (|x1-x2| + |y1-y2|), matching DrillItem.get_normal_contacts().
    const geometry::planar::FloatPoint fp_1 = p_1.to_float();
    const geometry::planar::FloatPoint fp_2 = p_2.to_float();
    const double dx = std::abs(fp_1.x - fp_2.x);
    const double dy = std::abs(fp_1.y - fp_2.y);
    return (dx + dy) <= static_cast<double>(p_tolerance);
}

} // namespace freerouting::board
