#pragma once

#include "autoroute/MazeSearchElement.h"
#include "geometry/planar/FloatLine.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/Point.h"

#include <vector>

namespace freerouting::board {
class Item;
class PolylineTrace;
class RoutingBoard;
} // namespace freerouting::board

namespace freerouting::autoroute {

class AutorouteControl;
class ExpansionDoor;
class ObstacleExpansionRoom;

/**
 * Auxiliary functions used in the maze search to shove a trace room. Port of
 * Java app.freerouting.autoroute.maze.MazeTraceShover.
 */
class MazeTraceShover
{
public:
    MazeTraceShover() = delete;

    /** A door section that was found while shoving a trace room. */
    struct DoorSection
    {
        ExpansionDoor* door = nullptr;
        int section_index = 0;
        geometry::planar::FloatLine section_line;
    };

    /**
     * Returns false, if the algorithm did not succeed and trying to shove from
     * another door section may be more successful.
     *
     * The doors that may be expanded after the shove are appended to
     * p_to_door_list. Port of Java MazeTraceShover.checkShoveTraceLine().
     */
    static bool check_shove_trace_line(const MazeListElement& p_list_element,
                                       ObstacleExpansionRoom* p_obstacle_room,
                                       board::RoutingBoard* p_board,
                                       const AutorouteControl& p_ctrl,
                                       bool p_shove_to_the_left,
                                       std::vector<DoorSection>& p_to_door_list);

    /**
     * Check if the endpoints of trace and from_item are matching, so that the
     * shove can continue through a link door. Port of Java
     * MazeTraceShover.endPointsMatching().
     */
    static bool end_points_matching(const board::PolylineTrace& p_trace, const board::Item* p_from_item);
};

} // namespace freerouting::autoroute
