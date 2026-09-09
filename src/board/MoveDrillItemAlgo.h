#pragma once

#include <vector>
#include <memory>

#include "geometry/planar/TileShape.h"
#include "geometry/planar/Point.h"
#include "geometry/planar/Vector.h"
#include "board/Item.h"
#include "board/RoutingBoard.h"
#include "board/CalcFromSide.h"
#include "datastructures/TimeLimit.h"

namespace freerouting::board {

class DrillItem;
class Via;
class ShapeTraceEntries;
class ShapeSearchTree;

/**
 * Contains internal auxiliary functions of class RoutingBoard for shoving vias and pins.
 * Mirrors Java MoveDrillItemAlgo.java (248 lines).
 */
class MoveDrillItemAlgo
{
public:
    /**
     * Checks, if p_drill_item can be translated by p_vector by shoving obstacle traces and vias aside,
     * so that no clearance violations occur.
     */
    static bool check(DrillItem* p_drill_item,
                      const geometry::planar::Vector& p_vector,
                      int p_max_recursion_depth,
                      int p_max_via_recursion_depth,
                      const std::vector<Item*>& p_ignore_items,
                      RoutingBoard* p_board,
                      const datastructures::TimeLimit* p_time_limit);

    /**
     * Translates p_drill_item by p_vector by shoving obstacle traces and vias aside,
     * so that no clearance violations occur.
     */
    static bool insert(DrillItem* p_drill_item,
                       const geometry::planar::Vector& p_vector,
                       int p_max_recursion_depth,
                       int p_max_via_recursion_depth,
                       const geometry::planar::IntOctagon* p_tidy_region,
                       RoutingBoard* p_board);

    /**
     * Shoves vias out of p_obstacle_shape. Returns false, if the database is damaged,
     * so that an undo is necessary afterwards.
     */
    static bool shove_vias(const geometry::planar::TileShape& p_obstacle_shape,
                           const CalcFromSide* p_from_side,
                           int p_layer,
                           const std::vector<int>& p_net_no_arr,
                           int p_cl_type,
                           const std::vector<Item*>& p_ignore_items,
                           int p_max_recursion_depth,
                           int p_max_via_recursion_depth,
                           bool p_copper_sharing_allowed,
                           RoutingBoard* p_board);

    /**
     * Calculates possible new location for a via to shove outside p_obstacle_shape.
     * if p_extended_check is true, more than 1 possible new locations are calculated.
     * The function is used here and in ShoveTraceAlgo.check.
     */
    static std::vector<geometry::planar::IntPoint> try_shove_via_points(
        const geometry::planar::TileShape& p_obstacle_shape,
        int p_layer,
        Via* p_via,
        int p_cl_class_no,
        bool p_extended_check,
        RoutingBoard* p_board);

private:
    MoveDrillItemAlgo() = delete;
};

} // namespace freerouting::board