#include "autoroute/MazeRipupResolver.h"

#include "autoroute/AutorouteControl.h"
#include "autoroute/AutorouteEngine.h"
#include "autoroute/Connection.h"
#include "autoroute/ExpansionRoom.h"
#include "autoroute/MazeSearchAlgo.h"
#include "autoroute/MazeSearchElement.h"
#include "board/FixedState.h"
#include "board/Item.h"
#include "board/PolylineTrace.h"
#include "board/Pin.h"
#include "board/SearchTreeObject.h"
#include "board/ShapeSearchTree.h"
#include "board/Trace.h"
#include "board/Via.h"
#include "geometry/planar/Direction.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/Point.h"
#include "geometry/planar/Polyline.h"
#include "geometry/planar/TileShape.h"
#include "geometry/planar/Vector.h"
#include "logger/FRLogger.h"
#include "settings/RouterSettings.h"

#include <algorithm>
#include <cmath>
#include <climits>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace freerouting::autoroute {

using geometry::planar::FloatPoint;
using geometry::planar::IntBox;
using geometry::planar::IntDirection;
using geometry::planar::IntPoint;
using geometry::planar::IntVector;
using geometry::planar::Line;
using geometry::planar::Polyline;
using geometry::planar::TileShape;
using logger::FRLogger;

MazeRipupResolver::MazeRipupResolver(MazeSearchAlgo* p_search)
    : search_(p_search)
{
}

double MazeRipupResolver::calc_fanout_via_ripup_cost_factor(const board::Trace* p_trace)
{
    if (p_trace == nullptr)
    {
        return 1;
    }

    for (int i = 0; i < 2; i++)
    {
        board::ItemSet current_end_contacts;
        if (i == 0)
        {
            current_end_contacts = p_trace->get_start_contacts();
        }
        else
        {
            current_end_contacts = p_trace->get_end_contacts();
        }
        if (current_end_contacts.size() != 1)
        {
            continue;
        }
        board::Item* current_trace_contact = *current_end_contacts.begin();
        bool protect_fanout_via = false;
        if (dynamic_cast<board::Pin*>(current_trace_contact) != nullptr
            && current_trace_contact->first_layer() == current_trace_contact->last_layer())
        {
            protect_fanout_via = true;
        }
        else if (auto* contact_trace = dynamic_cast<board::PolylineTrace*>(current_trace_contact))
        {
            if (current_trace_contact->get_fixed_state() == board::FixedState::SHOVE_FIXED
                && contact_trace->corner_count() == 2)
            {
                protect_fanout_via = true;
            }
        }

        if (protect_fanout_via)
        {
            double fanout_via_cost_factor = static_cast<double>(p_trace->get_half_width())
                                            / p_trace->get_length();
            fanout_via_cost_factor *= fanout_via_cost_factor;
            fanout_via_cost_factor *= FANOUT_COST_CONSTANT;
            return std::max(fanout_via_cost_factor, 1.0);
        }
    }
    return 1;
}

int MazeRipupResolver::check_ripup(const MazeListElement& p_list_element,
                                   board::Item* p_obstacle_item,
                                   bool p_door_is_small)
{
    if (search_ == nullptr || p_obstacle_item == nullptr)
    {
        return -1;
    }
    const AutorouteControl& ctrl = search_->get_control();

    if (!p_obstacle_item->is_routable())
    {
        return -1;
    }
    if (p_door_is_small && !enter_through_small_door(p_list_element, p_obstacle_item))
    {
        return -1;
    }
    CompleteExpansionRoom* previous_room = nullptr;
    if (p_list_element.door != nullptr)
    {
        previous_room = p_list_element.door->other_room(p_list_element.next_room);
    }
    bool room_was_shoved = p_list_element.adjustment != MazeSearchElement::Adjustment::NONE;
    board::Item* previous_item = nullptr;
    if (auto* room = dynamic_cast<ObstacleExpansionRoom*>(previous_room))
    {
        previous_item = room->get_item();
    }
    if (room_was_shoved)
    {
        if (previous_item != nullptr && previous_item != p_obstacle_item
            && previous_item->shares_net(p_obstacle_item))
        {
            return -1;
        }
    }
    else if (previous_item == p_obstacle_item)
    {
        return MazeSearchAlgo::ALREADY_RIPPED_COSTS;
    }

    int start_ripup_costs = 1;
    if (ctrl.settings != nullptr)
    {
        start_ripup_costs = ctrl.settings->get_start_ripup_costs();
    }

    double fanout_via_cost_factor = 1.0;
    double cost_factor = 1;
    bool preserve_fanout_protection = !ctrl.remove_unconnected_vias
                                      && ctrl.ripup_costs <= (start_ripup_costs * 2);
    if (auto* obstacle_trace = dynamic_cast<board::Trace*>(p_obstacle_item))
    {
        cost_factor = obstacle_trace->get_half_width();
        if (preserve_fanout_protection)
        {
            fanout_via_cost_factor = calc_fanout_via_ripup_cost_factor(obstacle_trace);
        }
    }
    else if (dynamic_cast<board::Via*>(p_obstacle_item) != nullptr)
    {
        bool look_if_fanout_via = preserve_fanout_protection;
        std::vector<board::Item*> contact_list = p_obstacle_item->get_normal_contacts();
        int contact_count = 0;
        for (board::Item* current_contact : contact_list)
        {
            auto* obstacle_trace = dynamic_cast<board::Trace*>(current_contact);
            if (obstacle_trace == nullptr || current_contact->is_user_fixed())
            {
                return -1;
            }
            ++contact_count;
            cost_factor = std::max(cost_factor, static_cast<double>(obstacle_trace->get_half_width()));
            if (look_if_fanout_via && !ctrl.is_fanout)
            {
                double current_fanout_via_cost_factor = calc_fanout_via_ripup_cost_factor(obstacle_trace);
                if (current_fanout_via_cost_factor > 1)
                {
                    fanout_via_cost_factor = current_fanout_via_cost_factor;
                    look_if_fanout_via = false;
                }
            }
        }
        if (fanout_via_cost_factor <= 1)
        {
            cost_factor *= 0.5 * std::max(contact_count - 1, 0);
        }
    }

    double ripup_cost = ctrl.ripup_costs * cost_factor;
    double detour = 1;
    double trace_length = 0;
    double min_trace_length = 0;
    int item_count = 0;
    std::string connection_item_ids = "[]";
    if (fanout_via_cost_factor <= 1 && !ctrl.is_fanout)
    {
        // Divergence: Java Connection.get() crawls the contacts of the item to
        // collect the whole connection. The C++ Connection::get() is a stub
        // which returns the precalculated connection or nullptr, so a
        // connection which was never precalculated contributes no detour
        // (detour stays 1) here.
        Connection* obstacle_connection = Connection::get(p_obstacle_item);
        if (obstacle_connection != nullptr)
        {
            detour = obstacle_connection->get_detour();
            trace_length = obstacle_connection->trace_length();
            item_count = static_cast<int>(obstacle_connection->item_list.size());
            if (obstacle_connection->start_point && obstacle_connection->end_point)
            {
                const FloatPoint start_float = obstacle_connection->start_point->to_float();
                const FloatPoint end_float = obstacle_connection->end_point->to_float();
                min_trace_length = start_float.distance(end_float);
            }
            std::string ids = "[";
            bool first = true;
            for (board::Item* connection_item : obstacle_connection->item_list)
            {
                if (!first)
                {
                    ids += ",";
                }
                first = false;
                ids += std::to_string(connection_item != nullptr ? connection_item->get_id() : -1);
            }
            ids += "]";
            connection_item_ids = ids;
        }
    }
    bool randomize = ctrl.ripup_pass_no >= 4 && ctrl.ripup_pass_no % 3 != 0;
    if (randomize)
    {
        double random_number = search_->random_generator().next_double();
        double random_factor = 0.5 + random_number * random_number;
        detour *= random_factor;
    }
    ripup_cost /= detour;
    ripup_cost *= fanout_via_cost_factor;
    int result = std::max(static_cast<int>(ripup_cost), 1);
    constexpr int max_ripup_costs = INT_MAX / 100;
    result = std::min(result, max_ripup_costs);

    std::string nets = "[";
    const std::vector<int>& net_nos = p_obstacle_item->get_net_nos();
    for (size_t i = 0; i < net_nos.size(); i++)
    {
        if (i > 0)
        {
            nets += ",";
        }
        nets += std::to_string(net_nos[i]);
    }
    nets += "]";

    FRLogger::instance().trace("CHECK_RIPUP net=" + std::to_string(ctrl.net_no)
                               + ", obstacle_id=" + std::to_string(p_obstacle_item->get_id())
                               + ", obstacle_nets=" + nets
                               + ", connectionItems=" + connection_item_ids
                               + ", halfWidth=" + std::to_string(cost_factor)
                               + ", ripupCosts=" + std::to_string(ctrl.ripup_costs)
                               + ", traceLength=" + std::to_string(trace_length)
                               + ", minTraceLength=" + std::to_string(min_trace_length)
                               + ", itemCount=" + std::to_string(item_count)
                               + ", detour=" + std::to_string(detour) + ", result="
                               + std::to_string(result));
    return result;
}

bool MazeRipupResolver::check_leaving_ripped_item(const MazeListElement& p_list_element)
{
    if (search_ == nullptr)
    {
        return false;
    }
    auto* current_door = dynamic_cast<ExpansionDoor*>(p_list_element.door);
    if (current_door == nullptr)
    {
        return false;
    }
    CompleteExpansionRoom* from_room = current_door->other_room(p_list_element.next_room);
    auto* obstacle_room = dynamic_cast<ObstacleExpansionRoom*>(from_room);
    if (obstacle_room == nullptr)
    {
        return false;
    }
    board::Item* current_item = obstacle_room->get_item();
    if (current_item == nullptr || !current_item->is_routable())
    {
        return false;
    }
    return enter_through_small_door(p_list_element, current_item);
}

bool MazeRipupResolver::enter_through_small_door(const MazeListElement& p_list_element,
                                                const board::Item* p_ignore_item)
{
    if (search_ == nullptr || p_list_element.door == nullptr)
    {
        return false;
    }
    if (p_list_element.door->get_dimension() != 1)
    {
        return false;
    }
    // Divergence: Java uses the exact TileShape of the door. The C++ doors
    // carry their bounding box only, so the check polyline is derived from
    // that box.
    const IntBox door_shape = p_list_element.door->get_shape();
    std::optional<Line> door_line;
    FloatPoint prev_corner = door_shape.corner_approx(0);
    int corner_count = door_shape.border_line_count();
    for (int i = 1; i < corner_count; i++)
    {
        FloatPoint next_corner = door_shape.corner_approx(i);
        if (next_corner.distance_square(prev_corner) > 1)
        {
            door_line = door_shape.border_line(i - 1);
            break;
        }
        prev_corner = next_corner;
    }
    if (!door_line)
    {
        return false;
    }

    const IntPoint door_center = door_shape.centre_of_gravity().round();
    int current_layer = 0;
    if (p_list_element.next_room != nullptr)
    {
        current_layer = p_list_element.next_room->get_layer();
    }
    double check_radius = static_cast<double>(
                              search_->compensated_trace_half_width_for_layer(current_layer))
                          + AutorouteEngine::TRACE_WIDTH_TOLERANCE;
    std::vector<Line> lines;
    lines.reserve(3);
    lines.push_back(door_line->translate(check_radius));
    IntVector door_vector = door_line->direction();
    IntDirection door_direction(door_vector.x, door_vector.y);
    lines.push_back(Line::get_instance(door_center, door_direction.turn_45_degree_int(2)));
    lines.push_back(door_line->translate(-check_radius));

    Polyline check_polyline(lines);
    std::unique_ptr<TileShape> check_shape = check_polyline.offset_shape(check_radius, 0);
    if (!check_shape)
    {
        return false;
    }
    const AutorouteControl& ctrl = search_->get_control();
    std::vector<int> ignore_net_nos;
    ignore_net_nos.push_back(ctrl.net_no);

    board::ShapeSearchTree* search_tree = search_->search_tree();
    if (search_tree == nullptr)
    {
        return false;
    }
    std::set<board::SearchTreeObject*, board::TreeStorableLess> overlapping_objects;
    search_tree->overlapping_objects(*check_shape, current_layer, ignore_net_nos, overlapping_objects);

    for (board::SearchTreeObject* current_object : overlapping_objects)
    {
        auto* current_item = dynamic_cast<board::Item*>(current_object);
        if (current_item == nullptr || current_item == p_ignore_item)
        {
            continue;
        }
        if (!current_item->shares_net(p_ignore_item))
        {
            return false;
        }
        const std::vector<board::Item*> normal_contacts = current_item->get_normal_contacts();
        if (std::find(normal_contacts.begin(), normal_contacts.end(), p_ignore_item)
            == normal_contacts.end())
        {
            return false;
        }
    }
    return true;
}

} // namespace freerouting::autoroute
