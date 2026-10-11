#pragma once

#include <ostream>
#include <string>

namespace freerouting {

namespace board {
class RoutingBoard;
}

namespace geometry::planar {
class FloatPoint;
}

namespace designforms {
namespace specctra {

/**
 * Writes a Specctra session (.ses) file from a board. Port of Java io.specctra.SesWriter:
 * session header, base design, placement, was_is and the routes (library_out with the via
 * padstacks, network_out with wires, vias and conduction areas).
 */
class SessionWriter {
public:
    /** Kept for the CLI: writes the session of p_board to the file given in the constructor. */
    explicit SessionWriter(const std::string& p_filename);

    /** Writes the session file; false if the file cannot be opened. */
    bool write_session(const board::RoutingBoard& p_board, const std::string& p_board_name);

    /**
     * Java SesWriter.write: serialises the routing result of p_board to p_out. p_design_name is the
     * pcb name written as base_design; the session name replaces ".dsn" with ".ses". The stream is
     * flushed, not closed.
     */
    static void write(const board::RoutingBoard& p_board, std::ostream& p_out, const std::string& p_design_name);

    /**
     * Java SesWriter.formatPlacementRotation: whole degrees as integers ("0", "339"), fractional
     * degrees with at most 3 decimals and no trailing zeros ("338.5").
     */
    static std::string format_placement_rotation(double p_degrees);

private:
    std::string filename_;
};

} // namespace specctra
} // namespace designforms
} // namespace freerouting
