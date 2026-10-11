#include "designforms/specctra/parser/Parser.h"

#include <array>
#include <string>
#include <vector>

#include "datastructures/IdentifierType.h"
#include "datastructures/IndentFileWriter.h"
#include "designforms/specctra/parser/DsnFile.h"
#include "designforms/specctra/parser/Keyword.h"
#include "designforms/specctra/parser/ReadScopeParameter.h"
#include "designforms/specctra/parser/SpecctraDsnStreamReader.h"
#include "logger/FRLogger.h"

namespace freerouting::designforms::specctra::parser {

std::optional<board::Communication::WriteResolution> Parser::read_write_solution(
    ReadScopeParameter& p_par)
{
    auto& log = logger::FRLogger::instance();
    Token next_token = p_par.scanner.next_token();
    if (!next_token.is_string())
    {
        log.warn("Parser.read_write_solution: string expected at '"
            + p_par.scanner.get_scope_identifier() + "'");
        return std::nullopt;
    }
    const std::string resolution_string = next_token.string_value;
    next_token = p_par.scanner.next_token();
    if (!next_token.is_int())
    {
        log.warn("Parser.read_write_solution: integer expected expected at '"
            + p_par.scanner.get_scope_identifier() + "'");
        return std::nullopt;
    }
    const int resolution_value = next_token.int_value;
    next_token = p_par.scanner.next_token();
    if (!next_token.is(Keyword::CLOSED_BRACKET))
    {
        log.warn("Parser.read_write_solution: closing_bracket expected at '"
            + p_par.scanner.get_scope_identifier() + "'");
        return std::nullopt;
    }
    return board::Communication::WriteResolution{ resolution_string, resolution_value };
}

std::optional<std::array<std::string, 2>> Parser::read_constant(ReadScopeParameter& p_par)
{
    auto& log = logger::FRLogger::instance();
    std::array<std::string, 2> result;
    p_par.scanner.yybegin(SpecctraDsnStreamReader::NAME);
    Token next_token = p_par.scanner.next_token();
    if (!next_token.is_string())
    {
        log.warn("Parser.read_constant: string expected at '"
            + p_par.scanner.get_scope_identifier() + "'");
        return std::nullopt;
    }
    result[0] = next_token.string_value;
    p_par.scanner.yybegin(SpecctraDsnStreamReader::NAME);
    next_token = p_par.scanner.next_token();
    if (!next_token.is_string())
    {
        log.warn("Parser.read_constant: string expected at '"
            + p_par.scanner.get_scope_identifier() + "'");
        return std::nullopt;
    }
    result[1] = next_token.string_value;
    next_token = p_par.scanner.next_token();
    if (!next_token.is(Keyword::CLOSED_BRACKET))
    {
        log.warn("Parser.read_constant: closing_bracket expected at '"
            + p_par.scanner.get_scope_identifier() + "'");
        return std::nullopt;
    }
    return result;
}

void Parser::write_scope(datastructures::IndentFileWriter& p_file,
    const board::Communication::SpecctraParserInfo& p_parser_info,
    const datastructures::IdentifierType& p_identifier_type, bool p_reduced)
{
    p_file.start_scope();
    p_file << "parser";
    if (!p_reduced)
    {
        p_file.new_line();
        p_file << "(string_quote ";
        p_file << p_parser_info.string_quote;
        p_file << ")";
        p_file.new_line();
        p_file << "(space_in_quoted_tokens on)";
    }
    if (p_parser_info.host_cad.has_value())
    {
        p_file.new_line();
        p_file << "(host_cad ";
        p_identifier_type.write(*p_parser_info.host_cad, p_file.stream());
        p_file << ")";
    }
    if (p_parser_info.host_version.has_value())
    {
        p_file.new_line();
        p_file << "(host_version ";
        p_identifier_type.write(*p_parser_info.host_version, p_file.stream());
        p_file << ")";
    }
    for (const std::array<std::string, 2>& current_constant : p_parser_info.constants)
    {
        p_file.new_line();
        p_file << "(constant ";
        for (const std::string& value : current_constant)
        {
            p_identifier_type.write(value, p_file.stream());
            p_file << " ";
        }
        p_file << ")";
    }
    if (p_parser_info.write_resolution)
    {
        p_file.new_line();
        p_file << "(write_resolution ";
        p_file << p_parser_info.write_resolution->char_name.substr(0, 1);
        p_file << " ";
        p_file << std::to_string(p_parser_info.write_resolution->positive_int);
        p_file << ")";
    }
    if (!p_reduced)
    {
        p_file.new_line();
        p_file << "(generated_by_freerouting)";
    }
    p_file.end_scope();
}

std::optional<std::string> Parser::read_quote_char(IJFlexScanner& p_scanner)
{
    auto& log = logger::FRLogger::instance();
    Token next_token = p_scanner.next_token();
    if (!next_token.is_string())
    {
        log.warn("Parser.read_quote_char: string expected at '" + p_scanner.get_scope_identifier() + "'");
        return std::nullopt;
    }
    const std::string result = next_token.string_value;
    next_token = p_scanner.next_token();
    if (!next_token.is(Keyword::CLOSED_BRACKET))
    {
        log.warn("Parser.read_quote_char: closing bracket expected at '"
            + p_scanner.get_scope_identifier() + "'");
        return std::nullopt;
    }
    return result;
}

bool Parser::read_scope(ReadScopeParameter& p_par) const
{
    auto& log = logger::FRLogger::instance();
    Token next_token;
    for (;;)
    {
        const Token prev_token = next_token;
        next_token = p_par.scanner.next_token();
        if (next_token.is_eof())
        {
            log.warn("Parser.read_scope: unexpected end of file at '"
                + p_par.scanner.get_scope_identifier() + "'");
            return false;
        }
        if (next_token.is(Keyword::CLOSED_BRACKET))
        {
            // end of scope
            break;
        }
        bool read_ok = true;
        if (prev_token.is(Keyword::OPEN_BRACKET))
        {
            if (next_token.is(Keyword::STRING_QUOTE))
            {
                std::optional<std::string> quote_char = read_quote_char(p_par.scanner);
                if (!quote_char)
                {
                    return false;
                }
                p_par.string_quote = *quote_char;
            }
            else if (next_token.is(Keyword::HOST_CAD))
            {
                p_par.host_cad = DsnFile::read_string_scope(p_par.scanner);
            }
            else if (next_token.is(Keyword::HOST_VERSION))
            {
                p_par.host_version = DsnFile::read_string_scope(p_par.scanner);
            }
            else if (next_token.is(Keyword::CONSTANT))
            {
                std::optional<std::array<std::string, 2>> current_constant = read_constant(p_par);
                if (current_constant)
                {
                    p_par.constants.push_back(*current_constant);
                }
            }
            else if (next_token.is(Keyword::WRITE_RESOLUTION))
            {
                p_par.write_resolution = read_write_solution(p_par);
            }
            else if (next_token.is(Keyword::GENERATED_BY_FREEROUTING))
            {
                p_par.dsn_file_generated_by_host = false;
                // skip the closing bracket
                skip_scope(p_par.scanner);
            }
            else
            {
                skip_scope(p_par.scanner);
            }
        }
        if (!read_ok)
        {
            return false;
        }
    }
    return true;
}

} // namespace freerouting::designforms::specctra::parser