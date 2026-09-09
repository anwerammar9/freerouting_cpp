#include "board/BoardLoader.h"
#include "board/Layer.h"
#include "board/RoutingBoard.h"
#include "board/Pin.h"
#include "board/BoardOutline.h"
#include "rules/BoardRules.h"
#include "rules/Net.h"
#include "designforms/specctra/Scanner.h"
#include "core/RoutingJob.h"
#include "logger/FRLogger.h"

#include <fstream>

namespace freerouting {
namespace board {

bool BoardLoader::load_board_if_needed(core::RoutingJob& job)
{
    if (job.board != nullptr)
    {
        return true;
    }
    if (job.get_input_file().empty())
    {
        logger::FRLogger::instance().error("BoardLoader: no input file set on the routing job");
        return false;
    }
    auto board = load_board(job.get_input_file());
    if (!board)
    {
        return false;
    }
    // The job keeps the board; ownership is transferred so it lives for the
    // whole routing run (leaked intentionally until job teardown is wired up).
    job.board = board.release();
    return true;
}

std::unique_ptr<RoutingBoard> BoardLoader::load_board(const std::string& filepath)
{
    auto parsed = designforms::specctra::read_dsn(filepath);
    if (!parsed.success)
    {
        logger::FRLogger::instance().error(
            "BoardLoader: failed to parse '" + filepath + "': " + parsed.error_message);
        return nullptr;
    }

    // --- Layer structure -------------------------------------------------
    std::vector<std::shared_ptr<Layer>> layers;
    for (const auto& layer_def : parsed.layers)
    {
        layers.push_back(std::make_shared<Layer>(layer_def.name, layer_def.is_signal));
    }
    if (layers.empty())
    {
        // Fallback: a single signal layer so the board is still constructible.
        layers.push_back(std::make_shared<Layer>("F.Cu", true));
    }
    auto layer_structure = std::make_shared<LayerStructure>(layers);

    // --- Design rules -----------------------------------------------------
    // DSN default clearance is in internal units; use a sane default for now.
    auto rules = std::make_unique<::freerouting::rules::BoardRules>(*layer_structure, 10);

    // Register the nets from the network section (1-based net numbers match
    // the pin net_no references in the parsed data).
    for (const auto& net_def : parsed.nets)
    {
        rules->get_nets().add(net_def.name, 1, false);
    }

    // --- Board bounding box from the boundary path ------------------------
    geometry::planar::IntBox bounding_box(
        parsed.board_min_x, parsed.board_min_y,
        parsed.board_min_x + parsed.board_width,
        parsed.board_min_y + parsed.board_height);
    if (parsed.board_width <= 0 || parsed.board_height <= 0)
    {
        bounding_box = geometry::planar::IntBox(0, 0, 10000, 10000);
    }

    auto board = std::make_unique<RoutingBoard>(bounding_box, *layer_structure, std::move(rules));

    // Hand the parsed data to the board and materialize pin items.
    board->set_parsed_result(parsed);
    board->create_pins_from_parsed();

    logger::FRLogger::instance().info("Board loaded: " + parsed.board_name +
        " (" + std::to_string(parsed.layers.size()) + " layers, " +
        std::to_string(parsed.nets.size()) + " nets, " +
        std::to_string(parsed.pins.size()) + " pins, " +
        std::to_string(parsed.components.size()) + " components)");

    return board;
}

} // namespace board
} // namespace freerouting