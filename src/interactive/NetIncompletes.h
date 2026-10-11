#pragma once

#include "board/ItemSet.h"
#include <list>
#include <memory>
#include <set>
#include <vector>

#include "geometry/planar/Point.h"
#include "interactive/RatsNest.h"

namespace freerouting {
namespace rules {
class Net;
} // namespace rules

namespace board {
class Item;
class RoutingBoard;
} // namespace board

namespace datastructures {
class PlanarDelaunayTriangulation;
} // namespace datastructures

namespace interactive {

/**
 * Creates the Incompletes (Ratsnest) of one net to display them on the screen.
 * Mirrors Java NetIncompletes.java.
 */
class NetIncompletes
{
public:
    NetIncompletes(int p_net_no, const std::vector<board::Item*>& p_net_items,
                   board::RoutingBoard* p_board);

    /**
     * Returns the number of incompletes/airlines of this net.
     */
    int count() const { return static_cast<int>(incompletes_.size()); }
    bool empty() const { return incompletes_.empty(); }

    /**
     * Recalculates the length violations. Returns false, if the length
     * violation has not changed.
     */
    bool calc_length_violation();

    /**
     * Returns the length of the violation of the length restriction of the
     * net, > 0, if the cumulative trace length is too big, < 0, if the trace
     * length is too small, 0, if the trace length is ok or the net has no
     * length restrictions.
     */
    double get_length_violation() const { return length_violation_; }

    rules::Net* get_net() const { return net_; }
    double get_draw_marker_radius() const { return draw_marker_radius_; }

    /**
     * Returns the number of connected groups used to compute the airlines.
     * Mirrors Java NetIncompletes.get_connected_group_count().
     */
    int get_connected_group_count() const { return connected_group_count_; }

    const std::list<std::shared_ptr<RatsNest::AirLine>>& incompletes() const
    {
        return incompletes_;
    }

private:
    struct NetItem;
    struct Edge;

    /**
     * Calculates a vector of Item-connected_set pairs for the items of this
     * net. Pairs belonging to the same connected set are located next to each
     * other. Mirrors Java NetIncompletes.calculate_net_items().
     */
    std::vector<std::shared_ptr<NetItem>> calculate_net_items(
        const std::vector<board::Item*>& p_item_list, int p_net_no) const;

    /**
     * Joins p_from_connected_set to p_to_connected_set and updates the
     * connected sets of the items in p_net_items. The set arguments are taken
     * by value, because they alias NetItem members which this method
     * reassigns. Mirrors Java NetIncompletes.join_connected_sets().
     */
    void join_connected_sets(const std::vector<std::shared_ptr<NetItem>>& p_net_items,
                             std::shared_ptr<board::ItemSet> p_from_connected_set,
                             std::shared_ptr<board::ItemSet> p_to_connected_set) const;

    board::RoutingBoard* board_;
    rules::Net* net_;
    double draw_marker_radius_ = 0;
    double length_violation_ = 0;
    int connected_group_count_ = 0;
    std::list<std::shared_ptr<RatsNest::AirLine>> incompletes_;
};

} // namespace interactive
} // namespace freerouting