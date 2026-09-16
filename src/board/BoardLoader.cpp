#include "board/BoardLoader.h"
#include "board/Layer.h"
#include "board/RoutingBoard.h"
#include "board/Pin.h"
#include "board/Padstack.h"
#include "board/BoardOutline.h"
#include "rules/BoardRules.h"
#include "rules/Net.h"
#include "designforms/specctra/Scanner.h"
#include "core/RoutingJob.h"
#include "geometry/planar/Polyline.h"
#include "logger/FRLogger.h"

#include <fstream>
#include <map>
#include <set>

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

    const int inserted = materialize_wiring(*board, parsed);

    logger::FRLogger::instance().info("Board loaded: " + parsed.board_name +
        " (" + std::to_string(parsed.layers.size()) + " layers, " +
        std::to_string(parsed.nets.size()) + " nets, " +
        std::to_string(parsed.pins.size()) + " pins, " +
        std::to_string(parsed.components.size()) + " components, " +
        std::to_string(inserted) + " wiring items)");

    return board;
}

namespace {

// Circle shapes are stored center-relative in the padstack (around (0,0)).
std::unique_ptr<geometry::planar::Shape> make_pad_shape(int64_t p_radius)
{
    return std::make_unique<geometry::planar::Circle>(
        geometry::planar::IntPoint(0, 0), std::max<int64_t>(1, p_radius));
}

} // namespace

int BoardLoader::materialize_wiring(RoutingBoard& p_board,
                                    const designforms::specctra::Result& p_parsed)
{
    // --- Build via padstacks from the structure "via" scopes --------------
    std::map<std::string, board::Padstack*> padstack_map;
    for (const auto& via_def : p_parsed.vias)
    {
        auto* padstack = new board::Padstack(via_def.name);
        const int layer_count = p_board.get_layer_count();
        const int from_layer = via_def.layer_indices.empty() ? 0 : via_def.layer_indices.front();
        const int to_layer =
            via_def.layer_indices.empty() ? layer_count - 1 : via_def.layer_indices.back();
        padstack->set_from_layer(from_layer);
        padstack->set_to_layer(to_layer);
        padstack->set_drill(via_def.drill);

        // Pad radius: prefer the library padstack width, else fall back on the drill.
        int64_t radius = via_def.drill;
        for (const auto& padstack_def : p_parsed.padstacks)
        {
            if (padstack_def.name == via_def.name)
            {
                radius = static_cast<int64_t>(std::llround(padstack_def.width / 2.0));
                break;
            }
        }
        for (int layer = from_layer; layer <= to_layer; ++layer)
        {
            padstack->add_shape(layer, make_pad_shape(radius).release());
        }
        padstack_map[via_def.name] = padstack;
    }

    // --- Traces -----------------------------------------------------------
    int inserted_traces = 0;
    for (const auto& wire_def : p_parsed.wires)
    {
        if (wire_def.is_conduction_area)
        {
            // Plane materialization not ported yet.
            logger::FRLogger::instance().warn(
                "BoardLoader: conduction area wire on layer " +
                std::to_string(wire_def.layer_no) + " skipped (planes not supported yet)");
            continue;
        }
        if (wire_def.coords.size() < 4)
        {
            continue;
        }

        std::vector<geometry::planar::IntPoint> corners;
        corners.reserve(wire_def.coords.size() / 2);
        for (std::size_t i = 0; i + 1 < wire_def.coords.size(); i += 2)
        {
            const geometry::planar::IntPoint corner(
                static_cast<int64_t>(std::llround(wire_def.coords[i])),
                static_cast<int64_t>(std::llround(wire_def.coords[i + 1])));
            if (corners.empty() || corners.back() != corner)
            {
                corners.push_back(corner);
            }
        }
        if (corners.size() < 2)
        {
            continue;
        }

        auto polyline = std::make_shared<geometry::planar::Polyline>(
            geometry::planar::Polyline::from_corners(corners));
        p_board.insert_trace_without_cleaning(
            polyline,
            wire_def.layer_no,
            std::max<int>(1, static_cast<int>(std::llround(wire_def.width / 2.0))),
            wire_def.net_nos,
            1,
            wire_def.fixed ? board::FixedState::SHOVE_FIXED : board::FixedState::FROM_DESIGN);
        ++inserted_traces;
    }

    // --- Vias -------------------------------------------------------------
    int inserted_vias = 0;
    for (const auto& via_def : p_parsed.via_placements)
    {
        auto it = padstack_map.find(via_def.padstack_name);
        if (it == padstack_map.end())
        {
            logger::FRLogger::instance().warn(
                "BoardLoader: via padstack '" + via_def.padstack_name + "' not found");
            continue;
        }
        if (via_def.location.size() < 2)
        {
            continue;
        }
        const geometry::planar::IntPoint center(
            static_cast<int64_t>(std::llround(via_def.location[0])),
            static_cast<int64_t>(std::llround(via_def.location[1])));
        p_board.insert_via(
            it->second,
            center,
            via_def.net_nos,
            1,
            via_def.fixed ? board::FixedState::SHOVE_FIXED : board::FixedState::FROM_DESIGN,
            true);
        ++inserted_vias;
    }

    // --- Normalize traces per net (Java Wiring.read_scope tail) -----------
    for (int net_no = 1; net_no <= static_cast<int>(p_parsed.nets.size()); ++net_no)
    {
        p_board.normalize_traces(net_no);
    }

    return inserted_traces + inserted_vias;
}

} // namespace board
} // namespace freerouting