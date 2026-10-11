#pragma once

#include <istream>
#include <string>

#include "designforms/specctra/BoardReadResult.h"

namespace freerouting::designforms::specctra {

/**
 * Reads a Specctra DSN file and returns a fully constructed board wrapped in a typed
 * BoardReadResult. Port of Java io.specctra.DsnReader; the board is built by the
 * MinimalBoardManager shim of the parser ReadScopeParameter.
 */
class DsnReader
{
public:
    /**
     * Reads a DSN stream. p_design_name (a file name, the path is stripped) is only used in log
     * messages; when empty the pcb name of the DSN header is used. Java DsnReader.readBoard.
     */
    static BoardReadResult read_board(std::istream& p_input, const std::string& p_design_name = {});

    /**
     * Reads only the parser, resolution and structure scopes and stops after the structure scope.
     * Returns Success with the metadata (its board may be nullptr without a valid outline).
     * Java DsnReader.readMetadata.
     */
    static BoardReadResult read_metadata(std::istream& p_input);
};

} // namespace freerouting::designforms::specctra
