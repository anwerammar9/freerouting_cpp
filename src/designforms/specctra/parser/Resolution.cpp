#include "designforms/specctra/parser/Resolution.h"

#include "board/Communication.h"
#include "board/Unit.h"
#include "datastructures/IndentFileWriter.h"
#include "designforms/specctra/parser/ReadScopeParameter.h"
#include "logger/FRLogger.h"

namespace freerouting::designforms::specctra::parser {

void Resolution::write_scope(
    datastructures::IndentFileWriter& p_file, const board::Communication& p_board_communication)
{
    p_file.new_line();
    p_file << "(resolution ";
    p_file << board::UnitHelper::to_string(p_board_communication.get_unit());
    p_file << " " << p_board_communication.get_resolution();
    p_file << ")";
}

bool Resolution::read_scope(ReadScopeParameter& p_par) const
{
    auto& log = logger::FRLogger::instance();
    // read the unit
    Token next_token = p_par.scanner.next_token();
    if (!next_token.is_string())
    {
        log.warn("Resolution.read_scope: string expected at '" + p_par.scanner.get_scope_identifier() + "'");
        return false;
    }
    const std::optional<board::Unit> unit = board::UnitHelper::parse(next_token.string_value);
    if (!unit)
    {
        log.warn("Resolution.read_scope: unit mil, inch or mm expected at '" + p_par.scanner.get_scope_identifier() + "'");
        return false;
    }
    p_par.unit = *unit;
    // read the scale factor
    next_token = p_par.scanner.next_token();
    if (!next_token.is_int())
    {
        log.warn("Resolution.read_scope: integer expected at '" + p_par.scanner.get_scope_identifier() + "'");
        return false;
    }
    p_par.resolution = next_token.int_value;
    // overread the closing bracket
    next_token = p_par.scanner.next_token();
    if (!next_token.is(Keyword::CLOSED_BRACKET))
    {
        log.warn("Resolution.read_scope: closing bracket expected at '" + p_par.scanner.get_scope_identifier() + "'");
        return false;
    }
    return true;
}

} // namespace freerouting::designforms::specctra::parser
