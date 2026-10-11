#pragma once

#include <optional>
#include <string>

#include "board/Communication.h"
#include "designforms/specctra/parser/ScopeKeyword.h"

namespace freerouting::datastructures {
class IdentifierType;
class IndentFileWriter;
} // namespace freerouting::datastructures

namespace freerouting::designforms::specctra::parser {

class IJFlexScanner;
class ReadScopeParameter;

/**
 * Reads and writes (parser ...) scopes from dsn files. Port of Java
 * io.specctra.parser.Parser.
 */
class Parser : public ScopeKeyword
{
public:
    Parser() : ScopeKeyword("parser") {}

    bool read_scope(ReadScopeParameter& p_par) const override;

    /** Reads a (write_resolution <char> <int>) scope. Java Parser.readWriteSolution. */
    static std::optional<board::Communication::WriteResolution> read_write_solution(
        ReadScopeParameter& p_par);

    /** Reads a (constant <name> <value>) scope. Java Parser.readConstant. */
    static std::optional<std::array<std::string, 2>> read_constant(ReadScopeParameter& p_par);

    /**
     * Writes parser settings, optionally using the reduced session-file
     * representation. Java Parser.writeScope.
     */
    static void write_scope(datastructures::IndentFileWriter& p_file,
        const board::Communication::SpecctraParserInfo& p_parser_info,
        const datastructures::IdentifierType& p_identifier_type, bool p_reduced);

    /** Reads the single-quote-character string and the closing bracket. Java Parser.readQuoteChar. */
    static std::optional<std::string> read_quote_char(IJFlexScanner& p_scanner);
};

} // namespace freerouting::designforms::specctra::parser