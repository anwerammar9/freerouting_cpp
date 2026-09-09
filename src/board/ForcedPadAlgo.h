#pragma once

#include <vector>
#include <memory>
#include <set>
#include <optional>

#include "geometry/planar/TileShape.h"
#include "geometry/planar/Point.h"
#include "geometry/planar/Vector.h"
#include "geometry/planar/Polyline.h"
#include "geometry/planar/IntOctagon.h"
#include "geometry/planar/IntBox.h"
#include "board/CalcFromSide.h"
#include "board/Item.h"
#include "board/RoutingBoard.h"
#include "datastructures/TimeLimit.h"

namespace freerouting::board {

class ShapeTraceEntries;
class ShoveTraceAlgo;
class PolylineTrace;
class CalcShapeAndFromSide;
class Via;
class Pin;
class SearchTreeManager;
class ShapeSearchTree;

/**
 * Class with functions for checking and inserting pads with eventually shoving aside obstacle traces.
 * Mirrors Java ForcedPadAlgo.java (352 lines).
 */
class ForcedPadAlgo
{
public:
    enum class CheckDrillResult
    {
        DRILLABLE,
        DRILLABLE_WITH_ATTACH_SMD,
        NOT_DRILLABLE
    };

    explicit ForcedPadAlgo(RoutingBoard* p_board);

    CheckDrillResult check_forced_pad(const geometry::planar::TileShape& p_pad_shape,
                                      const CalcFromSide* p_from_side,
                                      int p_layer,
                                      const std::vector<int>& p_net_no_arr,
                                      int p_cl_type,
                                      bool p_copper_sharing_allowed,
                                      const std::vector<Item*>& p_ignore_items,
                                      int p_max_recursion_depth,
                                      int p_max_via_recursion_depth,
                                      bool p_check_only_front,
                                      const datastructures::TimeLimit* p_time_limit);

    bool forced_pad(const geometry::planar::TileShape& p_pad_shape,
                    const CalcFromSide* p_from_side,
                    int p_layer,
                    const std::vector<int>& p_net_no_arr,
                    int p_cl_type,
                    bool p_copper_sharing_allowed,
                    const std::vector<Item*>& p_ignore_items,
                    int p_max_recursion_depth,
                    int p_max_via_recursion_depth);

    CalcFromSide calc_from_side(const geometry::planar::TileShape& p_shape,
                                const geometry::planar::Point& p_shape_center,
                                int p_layer,
                                int p_offset,
                                int p_cl_class);

private:
    static geometry::planar::IntOctagon calc_check_shape_for_from_side(const geometry::planar::TileShape& p_shape,
                                                                       const geometry::planar::Point& p_shape_center,
                                                                       const geometry::planar::Line& p_border_line);

    static bool in_front_of_pad(const geometry::planar::Line& p_line,
                                const geometry::planar::TileShape& p_pad_shape,
                                int p_from_side,
                                int p_width,
                                bool p_with_sides);

    RoutingBoard* board_ = nullptr;
};

} // namespace freerouting::board