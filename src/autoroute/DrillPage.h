#pragma once

#include <memory>
#include <vector>

#include "autoroute/ExpandableObject.h"
#include "autoroute/ExpansionDrill.h"
#include "autoroute/MazeSearchElement.h"
#include "geometry/planar/IntBox.h"

namespace freerouting::board {
class RoutingBoard;
}

namespace freerouting::autoroute {

class AutorouteEngine;

/**
 * A rectangular page of ExpansionDrills used in the maze search algorithm.
 * Mirrors Java app.freerouting.autoroute.drill.DrillPage.
 *
 * The drills of a page are the places where the search algorithm can change the
 * layer. They are calculated as the convex pieces of the page shape with the
 * obstacle shapes (vias, pins, traces of other nets) cut out. If attach_smd is
 * set, the center of a drillable pin is used as the drill location where one is
 * covered by a drill shape.
 */
class DrillPage : public ExpandableObject
{
public:
    DrillPage(const geometry::planar::IntBox& p_shape, board::RoutingBoard* p_board);

    /** The shape of the page. */
    const geometry::planar::IntBox shape;

    // ---- ExpandableObject interface ----

    geometry::planar::IntBox get_shape() const override { return shape; }
    int get_dimension() const override { return 2; }
    CompleteExpansionRoom* other_room(CompleteExpansionRoom* p_room) override { return nullptr; }
    int maze_search_element_count() const override;
    MazeSearchElement* get_maze_search_element(int p_no) override;
    void reset() override;

    /**
     * Returns the drills on this page. If p_attach_smd, drilling to smd pins is
     * allowed. The drills are recalculated when the autoroute net changes or the
     * page was invalidated. Mirrors Java DrillPage.getDrills().
     */
    std::vector<ExpansionDrill*> get_drills(AutorouteEngine* p_autoroute_engine, bool p_attach_smd);

    /**
     * Invalidates the drills of this page so that they are recalculated at the
     * next call of get_drills(). Mirrors Java DrillPage.invalidate().
     */
    void invalidate() { drills_.clear(); }

private:
    board::RoutingBoard* board_ = nullptr;
    std::vector<std::unique_ptr<ExpansionDrill>> drills_;
    std::vector<MazeSearchElement> maze_search_elements_;
    int net_no_ = -1;
};

} // namespace freerouting::autoroute