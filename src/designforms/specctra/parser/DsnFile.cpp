#include "designforms/specctra/parser/DsnFile.h"

#include "designforms/specctra/parser/Keyword.h"
#include "designforms/specctra/parser/ScopeKeyword.h"
#include "logger/FRLogger.h"

namespace freerouting::designforms::specctra::parser {

bool DsnFile::read_on_off_scope(IJFlexScanner& p_scanner)
{
    const Token next_token = p_scanner.next_token();
    bool result = false;
    if (next_token.is(Keyword::ON))
    {
        result = true;
    }
    else if (!next_token.is(Keyword::OFF))
    {
        logger::FRLogger::instance().warn(
            "DsnFile.read_boolean: Keyword.OFF expected at '" + p_scanner.get_scope_identifier() + "'");
    }
    ScopeKeyword::skip_scope(p_scanner);
    return result;
}

int DsnFile::read_integer_scope(IJFlexScanner& p_scanner)
{
    Token next_token = p_scanner.next_token();
    if (!next_token.is_int())
    {
        logger::FRLogger::instance().warn(
            "DsnFile.read_integer_scope: number expected at '" + p_scanner.get_scope_identifier() + "'");
        return 0;
    }
    const int value = next_token.int_value;
    next_token = p_scanner.next_token();
    if (!next_token.is(Keyword::CLOSED_BRACKET))
    {
        logger::FRLogger::instance().warn(
            "DsnFile.read_integer_scope: closing bracket expected at '" + p_scanner.get_scope_identifier() + "'");
        return 0;
    }
    return value;
}

double DsnFile::read_float_scope(IJFlexScanner& p_scanner)
{
    Token next_token = p_scanner.next_token();
    if (!next_token.is_number())
    {
        logger::FRLogger::instance().warn(
            "DsnFile.read_float_scope: number expected at '" + p_scanner.get_scope_identifier() + "'");
        return 0;
    }
    const double value = next_token.number();
    next_token = p_scanner.next_token();
    if (!next_token.is(Keyword::CLOSED_BRACKET))
    {
        logger::FRLogger::instance().warn(
            "DsnFile.read_float_scope: closing bracket expected at '" + p_scanner.get_scope_identifier() + "'");
        return 0;
    }
    return value;
}

std::optional<std::string> DsnFile::read_string_scope(IJFlexScanner& p_scanner)
{
    std::string result = p_scanner.next_string(true);
    Token next_token = p_scanner.next_token();
    if (!next_token.is(Keyword::CLOSED_BRACKET))
    {
        logger::FRLogger::instance().warn("DsnFile.read_string_scope: closing bracket expected at '"
            + p_scanner.get_scope_identifier() + "', got: " + next_token.to_string());
        while (!next_token.is_eof() && !next_token.is(Keyword::CLOSED_BRACKET))
        {
            next_token = p_scanner.next_token();
        }
    }
    return result;
}

std::optional<std::vector<std::string>> DsnFile::read_string_list_scope(IJFlexScanner& p_scanner)
{
    std::vector<std::string> result = p_scanner.next_string_list();
    if (!p_scanner.next_closing_bracket())
    {
        return std::nullopt;
    }
    return result;
}

} // namespace freerouting::designforms::specctra::parser
