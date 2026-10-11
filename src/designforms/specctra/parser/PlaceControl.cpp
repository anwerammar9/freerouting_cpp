#include "designforms/specctra/parser/PlaceControl.h"

#include "board/RoutingBoard.h"
#include "designforms/specctra/parser/IJFlexScanner.h"
#include "designforms/specctra/parser/Keyword.h"
#include "designforms/specctra/parser/ReadScopeParameter.h"
#include "logger/FRLogger.h"

namespace freerouting::designforms::specctra::parser {

namespace {

void warn(const std::string& p_message) { logger::FRLogger::instance().warn(p_message); }

} // namespace

bool PlaceControl::read_flip_style_rotate_first(IJFlexScanner& p_scanner)
{
    bool result = false;
    Token next_token = p_scanner.next_token();
    if (next_token.is(Keyword::ROTATE_FIRST))
    {
        result = true;
    }
    next_token = p_scanner.next_token();
    if (!next_token.is(Keyword::CLOSED_BRACKET))
    {
        warn("Structure.read_flip_style: closing bracket expected at '" + p_scanner.get_scope_identifier() + "'");
        return false;
    }
    return result;
}

bool PlaceControl::read_scope(ReadScopeParameter& p_par) const
{
    bool flip_style_rotate_first = false;
    Token next_token;
    for (;;)
    {
        const Token prev_token = next_token;
        next_token = p_par.scanner.next_token();
        if (next_token.is_eof())
        {
            warn("PlaceControl.read_scope: unexpected end of file at '" + p_par.scanner.get_scope_identifier() + "'");
            return false;
        }
        if (next_token.is(Keyword::CLOSED_BRACKET))
        {
            break; // end of scope
        }
        if (prev_token.is(Keyword::OPEN_BRACKET) && next_token.is(Keyword::FLIP_STYLE))
        {
            flip_style_rotate_first = read_flip_style_rotate_first(p_par.scanner);
        }
    }
    if (flip_style_rotate_first)
    {
        // Java: board.components.setFlipStyleRotateFirst(true) (throws if there is no board)
        if (board::RoutingBoard* board = p_par.get_board())
        {
            board->set_flip_style_rotate_first(true);
        }
    }
    return true;
}

} // namespace freerouting::designforms::specctra::parser
