#pragma once

#include <memory>
#include <string>
#include <vector>

#include "geometry/planar/Point.h"
#include "interactive/RatsNestItemInfo.h"

namespace freerouting {
namespace rules {
class Net;
} // namespace rules

namespace board {
class Item;
class RoutingBoard;
} // namespace board

namespace interactive {

class NetIncompletes;

/**
 * Creates all incompletes (ratsnest) to display them on the screen.
 * Mirrors Java RatsNest.java.
 */
class RatsNest
{
public:
    explicit RatsNest(board::RoutingBoard* p_board);

    /**
     * Describes a single incomplete connection of the ratsnest.
     * Mirrors Java RatsNest.AirLine.
     */
    class AirLine
    {
    public:
        AirLine(rules::Net* p_net,
                board::Item* p_from_item,
                const geometry::planar::FloatPoint& p_from_corner,
                board::Item* p_to_item,
                const geometry::planar::FloatPoint& p_to_corner);

        const rules::Net* net;
        board::Item* from_item;
        geometry::planar::FloatPoint from_corner;
        board::Item* to_item;
        geometry::planar::FloatPoint to_corner;

        RatsNestItemInfo get_item_info(board::RoutingBoard* p_board, board::Item* p_item) const;
        std::string to_string(board::RoutingBoard* p_board) const;
    };

    /**
     * Returns the number of incomplete connections (airlines) of the
     * ratsnest. This value might be higher than the number of nets, if the
     * nets have multiple unrouted connections.
     */
    int incomplete_count() const;

    int incomplete_count(int p_net_no) const;

    /**
     * Returns the number of nets whose length restriction is violated.
     */
    int length_violation_count() const;

    /**
     * Returns the length of the violation of the length restriction of the
     * net with number p_net_no, > 0, if the cumulative trace length is too
     * big, < 0, if the trace length is too small, 0, if the trace length is
     * ok or the net has no length restrictions.
     */
    double get_length_violation(int p_net_no) const;

    /**
     * Recalculates the incomplete connections for the input net.
     */
    void recalculate(int p_net_no, board::RoutingBoard* p_board);

    /**
     * Recalculates the incomplete connections for the input net with the input
     * item list.
     */
    void recalculate(int p_net_no, const std::vector<board::Item*>& p_item_list,
                     board::RoutingBoard* p_board);

    /**
     * Returns all airlines of the ratsnest.
     */
    std::vector<std::shared_ptr<AirLine>> get_airlines() const;

    /**
     * Recalculates the length matching violations. Returns false, if the
     * length violations have not changed.
     */
    bool recalculate_length_violations();

    bool is_hidden() const { return hidden_; }
    void hide() { hidden_ = true; }
    void show() { hidden_ = false; }

    /**
     * Sets the visibility filter for the incompletes of the input net.
     */
    void set_filter(int p_net_no, bool p_value);

    bool is_filtered(int p_net_no) const;

    int max_connections = 0;

private:
    board::RoutingBoard* board_;
    std::vector<std::shared_ptr<NetIncompletes>> net_incompletes_;
    std::vector<bool> is_filtered_;
    bool hidden_ = false;
};

} // namespace interactive
} // namespace freerouting