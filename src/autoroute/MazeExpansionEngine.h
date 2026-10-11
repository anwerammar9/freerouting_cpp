#pragma once

#include <map>
#include <memory>
#include <vector>

#include "board/ForcedPadAlgo.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/Point.h"

namespace freerouting::board {
class Item;
class RoutingBoard;
class Padstack;
class ShapeSearchTree;
class Via;
} // namespace freerouting::board

namespace freerouting::autoroute {

class AutorouteControl;
class AutorouteEngine;
class DrillPage;
class ExpansionDrill;
class MazeListElement;
class MazeSearchAlgo;
class ObstacleExpansionRoom;

/**
 * Expands drill pages, existing vias, and candidate via layers for a maze
 * search. Port of Java
 * app.freerouting.autoroute.maze.MazeExpansionEngine.
 */
class MazeExpansionEngine
{
public:
    explicit MazeExpansionEngine(MazeSearchAlgo* p_search) : search_(p_search) {}

    /**
     * Expands a drill to a new maze search element with p_add_costs added to
     * its expansion value. Port of Java
     * MazeExpansionEngine.expandToDrill().
     */
    void expand_to_drill(ExpansionDrill* p_drill, const MazeListElement& p_from_element, int p_add_costs);

    /**
     * Inserts a drill page between a room and its candidate drills. Port of
     * Java MazeExpansionEngine.expandToDrillPage().
     */
    void expand_to_drill_page(DrillPage* p_drill_page, const MazeListElement& p_from_element);

    /**
     * Expands all drills of the drill page of the door of p_from_element. Port
     * of Java MazeExpansionEngine.expandToDrillsOfPage().
     */
    void expand_to_drills_of_page(const MazeListElement& p_from_element);

    /**
     * Returns the expansion drill of p_via, so that the maze search can change
     * the layer at the location of an existing via. The drill is created once
     * per via and cached, its rooms are the obstacle expansion rooms of the via.
     * Port of Java Via.getAutorouteDrillInfo().
     *
     * Divergence: Java caches the drill on the Via itself and drops it in
     * Via.clearDerivedData(). ExpansionDrill is an autoroute type, so a Via of
     * the board module may not hold one. The C++ cache is therefore owned by
     * this engine and keyed by via, and lives as long as the maze search that
     * created it. Only the rooms of the drill are affected by a reset of the
     * derived data of the via, and they are filled again below.
     */
    ExpansionDrill* get_autoroute_drill_info(board::Via* p_via, board::ShapeSearchTree* p_autoroute_tree);

    /**
     * Expands the layers of the drill of p_list_element, to which a via may be
     * inserted without a clearance violation. Port of Java
     * MazeExpansionEngine.expandToOtherLayers().
     */
    void expand_to_other_layers(const MazeListElement& p_list_element);

private:
    /**
     * Returns true, if a via of the via rule of the current control may be
     * drilled at the location of p_drill on p_layer into the room p_room_shape
     * of the drill. DRILLABLE_WITH_ATTACH_SMD is only returned, if no via of the
     * via rule is drillable without attaching an SMD pad. Port of Java
     * MazeExpansionEngine.checkLayerWithAnyMatchingVia().
     */
    board::ForcedPadAlgo::CheckDrillResult check_layer_with_any_matching_via(
        const ExpansionDrill& p_drill, int p_layer, const geometry::planar::IntBox& p_room_shape,
        const std::vector<int>& p_net_nos);

    /** The maze search this engine expands. Mirrors Java MazeExpansionEngine.search. */
    MazeSearchAlgo* search_ = nullptr;

    /** The cached expansion drills of the existing vias, keyed by via. */
    std::map<board::Via*, std::unique_ptr<ExpansionDrill>> via_drill_cache_;
};

} // namespace freerouting::autoroute