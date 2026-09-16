#pragma once

#include <memory>

namespace freerouting {
namespace core { class RoutingJob; }
namespace board { class RoutingBoard; }
namespace designforms { namespace specctra { struct Result; } }

namespace board {

class BoardLoader
{
public:
    static bool load_board_if_needed(core::RoutingJob& job);
    static std::unique_ptr<RoutingBoard> load_board(const std::string& filepath);

    /**
     * Materializes the parsed wires (traces) and via placements into the board
     * as board items. Returns the number of inserted items. Mirrors the wiring
     * insertion part of Java Wiring.read_scope().
     */
    static int materialize_wiring(RoutingBoard& p_board,
                                  const designforms::specctra::Result& p_parsed);
};

} // namespace board
} // namespace freerouting