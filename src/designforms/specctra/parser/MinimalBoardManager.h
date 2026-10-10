#pragma once

#include <memory>

#include "board/RoutingBoard.h"
#include "designforms/specctra/parser/BoardParserCallback.h"

namespace freerouting::designforms::specctra::parser {

/**
 * Board shim used while parsing without a HeadlessBoardManager. Port of the private Java class
 * ReadScopeParameter.MinimalBoardManager.
 */
class MinimalBoardManager : public BoardParserCallback
{
public:
    board::RoutingBoard* get_routing_board() override { return board_.get(); }

    void create_board(const geometry::planar::IntBox& p_bounding_box, const board::LayerStructure& p_layer_structure,
        std::vector<std::shared_ptr<geometry::planar::PolylineShape>> p_outline_shapes,
        const std::string& p_outline_clearance_class_name, std::unique_ptr<rules::BoardRules> p_rules,
        std::unique_ptr<board::Communication> p_board_communication) override;

    core::RoutingJob* get_current_routing_job() override { return nullptr; }

    /** Hands the board to the caller (DsnReader). */
    std::unique_ptr<board::RoutingBoard> release_board() { return std::move(board_); }

private:
    std::unique_ptr<board::RoutingBoard> board_;
};

} // namespace freerouting::designforms::specctra::parser
