#pragma once

#include <string>

#include "datastructures/IdentifierType.h"
#include "datastructures/IndentFileWriter.h"
#include "designforms/specctra/parser/CoordinateTransform.h"

namespace freerouting::board {
class BasicBoard;
}

namespace freerouting::settings {
class RouterSettings;
}

namespace freerouting::designforms::specctra::parser {

/**
 * Default parameter type used while writing a Specctra dsn-file. Port of Java
 * parser.WriteScopeParameter.
 *
 * C++ deviations: Java holds strong references; the C++ port keeps non-owning
 * pointers to the board and the autoroute settings (both outlive the writer).
 */
class WriteScopeParameter
{
public:
    /**
     * If p_compat_mode is true, only standard specctra dsn scopes are written,
     * so that any host system with a specctra interface can read them.
     */
    WriteScopeParameter(board::BasicBoard* p_board,
        settings::RouterSettings* p_autoroute_settings,
        datastructures::IndentFileWriter& p_file,
        std::string p_string_quote,
        CoordinateTransform p_coordinate_transform,
        bool p_compat_mode);

    board::BasicBoard* board;
    settings::RouterSettings* autoroute_settings;
    datastructures::IndentFileWriter& file;
    CoordinateTransform coordinate_transform;
    bool compat_mode;
    datastructures::IdentifierType identifier_type;
};

} // namespace freerouting::designforms::specctra::parser
