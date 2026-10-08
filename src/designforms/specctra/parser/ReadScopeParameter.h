#pragma once

#include <string>
#include <vector>

#include <optional>

#include "board/Unit.h"
#include "designforms/specctra/parser/CoordinateTransform.h"
#include "designforms/specctra/parser/IJFlexScanner.h"

namespace freerouting::designforms::specctra::parser {

/**
 * Structured properties and helper data shared by the scope readers while a
 * DSN file is parsed. Port of Java ReadScopeParameter; the fields are added
 * as the scope readers using them are ported.
 */
class ReadScopeParameter
{
public:
    explicit ReadScopeParameter(IJFlexScanner& p_scanner) : scanner(p_scanner) {}

    IJFlexScanner& scanner;

    /** Warnings collected during parsing. Java ReadScopeParameter.warnings. */
    std::vector<std::string> warnings;

    /** The quote character of the parser scope. Java stringQuote. */
    std::string string_quote = "\"";
    std::string host_cad;
    std::string host_version;
    bool dsn_file_generated_by_host = true;
    /** Set to false by the structure reader when the board outline is absent. */
    bool board_outline_ok = true;

    /** Set when the structure scope is read. Java coordinateTransform. */
    std::optional<CoordinateTransform> coordinate_transform;
    /** From the resolution scope. Java unit / resolution (defaults MIL, 100). */
    board::Unit unit = board::Unit::MIL;
    int resolution = 100;
};

} // namespace freerouting::designforms::specctra::parser
