#pragma once

#include "autoroute/MazeSearchElement.h"

namespace freerouting::board {
class Item;
class ShapeSearchTree;
class Trace;
} // namespace freerouting::board

namespace freerouting::autoroute {

class MazeSearchAlgo;

/**
 * Resolves whether the maze expansion may rip up an obstacle room and
 * calculates the cost of doing so. Port of Java
 * app.freerouting.autoroute.maze.MazeRipupResolver.
 */
class MazeRipupResolver
{
public:
    static constexpr double FANOUT_COST_CONSTANT = 20000;

    explicit MazeRipupResolver(MazeSearchAlgo* p_search);

    /**
     * Returns the cost of ripping up p_obstacle_item when the expansion
     * continues from p_list_element, or -1 if the room may not be ripped.
     * Port of Java MazeRipupResolver.checkRipup().
     */
    int check_ripup(const MazeListElement& p_list_element, board::Item* p_obstacle_item, bool p_door_is_small);

    /**
     * Returns true, if a ripped item may be left through a small door.
     * Port of Java MazeRipupResolver.checkLeavingRippedItem().
     */
    bool check_leaving_ripped_item(const MazeListElement& p_list_element);

    /**
     * Returns the cost factor protecting a fanout via ending at p_trace.
     * Port of Java MazeRipupResolver.calcFanoutViaRipupCostFactor().
     */
    static double calc_fanout_via_ripup_cost_factor(const board::Trace* p_trace);

private:
    /**
     * Returns true, if the door of p_list_element can be entered while
     * ignoring p_ignore_item and the items directly connected to it.
     * Port of Java MazeRipupResolver.enterThroughSmallDoor().
     */
    bool enter_through_small_door(const MazeListElement& p_list_element, const board::Item* p_ignore_item);

    /** The maze search this resolver belongs to. */
    MazeSearchAlgo* search_;
};

} // namespace freerouting::autoroute
