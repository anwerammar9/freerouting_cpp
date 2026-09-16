#include "interactive/NetIncompletes.h"

#include <cmath>
#include <set>
#include <utility>

#include "board/ConductionArea.h"
#include "board/Item.h"
#include "board/RoutingBoard.h"
#include "board/Trace.h"
#include "board/Via.h"
#include "datastructures/PlanarDelaunayTriangulation.h"
#include "datastructures/Signum.h"
#include "rules/BoardRules.h"
#include "rules/Net.h"
#include "rules/NetClass.h"

namespace freerouting {
namespace interactive {

namespace {

/**
 * Returns the cumulative trace length of all traces on the board belonging to
 * p_net_no. Mirrors Java Net.get_trace_length().
 */
double get_trace_length(board::RoutingBoard* p_board, int p_net_no)
{
    double cumulative_trace_length = 0;
    for (board::Item* curr_item : p_board->get_connectable_items(p_net_no))
    {
        if (auto* trace = dynamic_cast<board::Trace*>(curr_item))
        {
            cumulative_trace_length += trace->get_length();
        }
    }
    return cumulative_trace_length;
}

} // namespace

struct NetIncompletes::NetItem : datastructures::PlanarDelaunayTriangulation::Storable
{
    board::Item* item;
    std::shared_ptr<std::set<board::Item*>> connected_set;

    NetItem(board::Item* p_item, std::shared_ptr<std::set<board::Item*>> p_connected_set)
        : item(p_item), connected_set(std::move(p_connected_set))
    {
    }

    std::vector<std::shared_ptr<geometry::planar::Point>> get_triangulation_corners() const override
    {
        return item->get_ratsnest_corners();
    }
};

struct NetIncompletes::Edge
{
    const NetItem* from_item;
    geometry::planar::FloatPoint from_corner;
    const NetItem* to_item;
    geometry::planar::FloatPoint to_corner;
    double length_square;

    Edge(const NetItem* p_from_item, const geometry::planar::FloatPoint& p_from_corner,
         const NetItem* p_to_item, const geometry::planar::FloatPoint& p_to_corner)
        : from_item(p_from_item),
          from_corner(p_from_corner),
          to_item(p_to_item),
          to_corner(p_to_corner),
          length_square(p_to_corner.distance_square(p_from_corner))
    {
    }

    /**
     * Ordering by length in ascending order with coordinate tie-breakers.
     * Mirrors Java NetIncompletes.Edge.compareTo(); the tie-breakers prevent
     * edges with the same length from being skipped in the set.
     */
    struct Less
    {
        bool operator()(const Edge& p_a, const Edge& p_b) const
        {
            if (p_a.length_square != p_b.length_square)
            {
                return p_a.length_square < p_b.length_square;
            }
            if (p_a.from_corner.x != p_b.from_corner.x)
            {
                return p_a.from_corner.x < p_b.from_corner.x;
            }
            if (p_a.from_corner.y != p_b.from_corner.y)
            {
                return p_a.from_corner.y < p_b.from_corner.y;
            }
            if (p_a.to_corner.x != p_b.to_corner.x)
            {
                return p_a.to_corner.x < p_b.to_corner.x;
            }
            return p_a.to_corner.y < p_b.to_corner.y;
        }
    };
};

NetIncompletes::NetIncompletes(int p_net_no, const std::vector<board::Item*>& p_net_items,
                               board::RoutingBoard* p_board)
    : board_(p_board),
      net_(p_board->get_rules().get_nets().get(p_net_no)),
      draw_marker_radius_(p_board->get_rules().get_min_trace_half_width() * 2)
{
    // Filter out dangling items (vias and tracks with is_tail() == true)
    // AND items with zero contacts (unconnected pins/pads).
    // These are DRC violations, not unrouted connections, and should not be
    // counted as incompletes.
    std::vector<board::Item*> filtered_items;
    for (board::Item* item : p_net_items)
    {
        if (item->is_tail())
        {
            continue;
        }
        // Except for DrillItems (pins/vias) - unrouted pins legitimately have
        // no contacts and SHOULD appear in the ratsnest. Except for
        // ConductionArea which acts as a connection medium.
        if (!dynamic_cast<board::ConductionArea*>(item) && !dynamic_cast<board::DrillItem*>(item)
            && item->get_normal_contacts().empty())
        {
            continue;
        }
        filtered_items.push_back(item);
    }

    // Create an array of Item-connected_set pairs.
    std::vector<std::shared_ptr<NetItem>> net_items = calculate_net_items(filtered_items, p_net_no);
    if (net_items.size() <= 1)
    {
        return;
    }

    // Create a Delaunay Triangulation for the net_items.
    std::vector<std::shared_ptr<datastructures::PlanarDelaunayTriangulation::Storable>>
        triangulation_objects;
    triangulation_objects.reserve(net_items.size());
    for (const auto& net_item : net_items)
    {
        triangulation_objects.push_back(net_item);
    }
    datastructures::PlanarDelaunayTriangulation triangulation(triangulation_objects);

    // Sort the result edges of the triangulation by length in ascending order.
    std::vector<datastructures::PlanarDelaunayTriangulation::ResultEdge> triangulation_lines =
        triangulation.get_edge_lines();
    std::set<Edge, Edge::Less> sorted_edges;
    for (const auto& curr_line : triangulation_lines)
    {
        const NetItem* from_item = static_cast<NetItem*>(curr_line.start_object.get());
        const NetItem* to_item = static_cast<NetItem*>(curr_line.end_object.get());
        sorted_edges.insert(Edge(from_item, curr_line.start_point->to_float(), to_item,
                                 curr_line.end_point->to_float()));
    }

    // Create the Airlines. Skip edges, whose from_item and to_item are already
    // in the same connected set or whose connected sets have already an
    // airline.
    for (const Edge& curr_edge : sorted_edges)
    {
        if (curr_edge.from_item->connected_set == curr_edge.to_item->connected_set)
        {
            continue; // airline exists already
        }
        incompletes_.push_back(std::make_shared<RatsNest::AirLine>(
            net_, curr_edge.from_item->item, curr_edge.from_corner, curr_edge.to_item->item,
            curr_edge.to_corner));
        join_connected_sets(net_items, curr_edge.from_item->connected_set,
                            curr_edge.to_item->connected_set);
    }
    calc_length_violation();
}

bool NetIncompletes::calc_length_violation()
{
    double old_violation = this->length_violation_;
    double max_length = 0;
    double min_length = 0;
    if (net_ != nullptr && net_->get_net_class() != nullptr)
    {
        max_length = net_->get_net_class()->get_maximum_trace_length();
        min_length = net_->get_net_class()->get_minimum_trace_length();
    }
    if (max_length <= 0 && min_length <= 0)
    {
        this->length_violation_ = 0;
        return false;
    }
    double new_violation = 0;
    double trace_length = get_trace_length(board_, net_ != nullptr ? net_->get_net_no() : 0);
    if (max_length > 0 && trace_length > max_length)
    {
        new_violation = trace_length - max_length;
    }
    if (min_length > 0 && trace_length < min_length && this->incompletes_.empty())
    {
        new_violation = trace_length - min_length;
    }
    this->length_violation_ = new_violation;
    return std::abs(new_violation - old_violation) > 0.1;
}

std::vector<std::shared_ptr<NetIncompletes::NetItem>> NetIncompletes::calculate_net_items(
    const std::vector<board::Item*>& p_item_list, int p_net_no) const
{
    std::vector<std::shared_ptr<NetItem>> result;
    std::set<board::Item*> unique_items(p_item_list.begin(), p_item_list.end());

    while (!unique_items.empty())
    {
        board::Item* start_item = *unique_items.begin();
        auto curr_connected_set = std::make_shared<std::set<board::Item*>>(
            start_item->get_connected_set(p_net_no, false));

        // Collect the items of the connected set which are still present to
        // avoid modifying unique_items while iterating.
        std::vector<board::Item*> items_in_component;
        for (board::Item* item_in_set : *curr_connected_set)
        {
            if (unique_items.find(item_in_set) != unique_items.end())
            {
                items_in_component.push_back(item_in_set);
            }
        }

        for (board::Item* curr_item : items_in_component)
        {
            result.push_back(std::make_shared<NetItem>(curr_item, curr_connected_set));
            unique_items.erase(curr_item);
        }
    }
    return result;
}

void NetIncompletes::join_connected_sets(
    const std::vector<std::shared_ptr<NetItem>>& p_net_items,
    const std::shared_ptr<std::set<board::Item*>>& p_from_connected_set,
    const std::shared_ptr<std::set<board::Item*>>& p_to_connected_set) const
{
    for (const auto& curr_net_item : p_net_items)
    {
        if (curr_net_item->connected_set == p_from_connected_set)
        {
            p_to_connected_set->insert(curr_net_item->item);
            curr_net_item->connected_set = p_to_connected_set;
        }
    }
}

} // namespace interactive
} // namespace freerouting