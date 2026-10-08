#include "designforms/specctra/parser/ScopeKeyword.h"

#include <stdexcept>

#include "designforms/specctra/parser/ReadScopeParameter.h"
#include "designforms/specctra/parser/SpecctraDsnStreamReader.h"
#include "logger/FRLogger.h"

namespace freerouting::designforms::specctra::parser {

bool ScopeKeyword::skip_scope(IJFlexScanner& p_scanner)
{
    int open_bracket_count = 1;
    while (open_bracket_count > 0)
    {
        p_scanner.yybegin(SpecctraDsnStreamReader::NAME);
        Token current_token;
        try
        {
            current_token = p_scanner.next_token();
        }
        catch (const std::exception& e)
        {
            logger::FRLogger::instance().error(std::string("ScopeKeyword.skip_scope: Error while scanning file: ") + e.what());
            return false;
        }
        if (current_token.is_eof())
        {
            return false; // end of file
        }
        if (current_token.is(Keyword::OPEN_BRACKET))
        {
            ++open_bracket_count;
        }
        else if (current_token.is(Keyword::CLOSED_BRACKET))
        {
            --open_bracket_count;
        }
    }
    p_scanner.yybegin(SpecctraDsnStreamReader::YYINITIAL);
    return true;
}

bool ScopeKeyword::read_scope(ReadScopeParameter& p_par) const
{
    Token next_token;
    for (;;)
    {
        const Token prev_token = next_token;
        next_token = p_par.scanner.next_token();
        if (next_token.is_eof())
        {
            // end of file
            return true;
        }
        if (next_token.is(Keyword::CLOSED_BRACKET))
        {
            // end of scope
            break;
        }
        if (prev_token.is(Keyword::OPEN_BRACKET))
        {
            // a new scope is expected
            if (const auto* next_scope = dynamic_cast<const ScopeKeyword*>(next_token.keyword))
            {
                if (!next_scope->read_scope(p_par))
                {
                    return false;
                }
            }
            else
            {
                // skip unknown scope
                skip_scope(p_par.scanner);
            }
        }
    }
    return true;
}

} // namespace freerouting::designforms::specctra::parser
