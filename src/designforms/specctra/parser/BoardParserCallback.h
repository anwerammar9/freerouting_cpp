#pragma once

#include <memory>
#include <string>
#include <vector>

#include "board/Communication.h"
#include "board/Layer.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/PolylineShape.h"
#include "rules/BoardRules.h"

namespace freerouting::board {
class RoutingBoard;
} // namespace freerouting::board

namespace freerouting::core {
class RoutingJob;
} // namespace freerouting::core

namespace freerouting::designforms::specctra::parser {

/**
 * Narrow callback used by the DSN parser to create the board. Port of Java parser.BoardParserCallback;
 * the only production implementation is MinimalBoardManager (HeadlessBoardManager follows in T124a).
 */
class BoardParserCallback
{
public:
    virtual ~BoardParserCallback() = default;

    /** The created board, or nullptr before the structure scope created it. */
    virtual board::RoutingBoard* get_routing_board() = 0;

    /**
     * Creates the board from the structure scope data. An empty p_outline_clearance_class_name stands
     * for Java null.
     */
    virtual void create_board(const geometry::planar::IntBox& p_bounding_box,
        const board::LayerStructure& p_layer_structure,
        std::vector<std::shared_ptr<geometry::planar::PolylineShape>> p_outline_shapes,
        const std::string& p_outline_clearance_class_name, std::unique_ptr<rules::BoardRules> p_rules,
        std::unique_ptr<board::Communication> p_board_communication)
        = 0;

    /** The routing job of this parse, or nullptr when reading without a job. */
    virtual core::RoutingJob* get_current_routing_job() = 0;
};

} // namespace freerouting::designforms::specctra::parser
