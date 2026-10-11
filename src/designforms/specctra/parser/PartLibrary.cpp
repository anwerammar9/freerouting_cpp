#include "designforms/specctra/parser/PartLibrary.h"

#include <optional>

#include "designforms/specctra/parser/IJFlexScanner.h"
#include "designforms/specctra/parser/Keyword.h"
#include "designforms/specctra/parser/ReadScopeParameter.h"
#include "designforms/specctra/parser/SpecctraDsnStreamReader.h"
#include "logger/FRLogger.h"

namespace freerouting::designforms::specctra::parser {

namespace {

void warn(const std::string& p_message) { logger::FRLogger::instance().warn(p_message); }

} // namespace

bool PartLibrary::read_scope(ReadScopeParameter& p_par) const
{
    Token next_token;
    for (;;)
    {
        const Token prev_token = next_token;
        next_token = p_par.scanner.next_token();
        if (next_token.is_eof())
        {
            warn("PartLibrary.read_scope: unexpected end of file at '" + p_par.scanner.get_scope_identifier() + "'");
            return false;
        }
        if (next_token.is(Keyword::CLOSED_BRACKET))
        {
            break; // end of scope
        }
        if (!prev_token.is(Keyword::OPEN_BRACKET))
        {
            continue;
        }
        if (next_token.is(Keyword::LOGICAL_PART_MAPPING))
        {
            std::optional<LogicalPartMapping> next_mapping = read_logical_part_mapping(p_par.scanner);
            if (!next_mapping.has_value())
            {
                return false;
            }
            p_par.logical_part_mappings.push_back(std::move(*next_mapping));
        }
        else if (next_token.is(Keyword::LOGICAL_PART))
        {
            std::optional<LogicalPart> next_part = read_logical_part(p_par.scanner);
            if (!next_part.has_value())
            {
                return false;
            }
            p_par.logical_parts.push_back(std::move(*next_part));
        }
        else
        {
            skip_scope(p_par.scanner);
        }
    }
    return true;
}

std::optional<PartLibrary::LogicalPartMapping> PartLibrary::read_logical_part_mapping(IJFlexScanner& p_scanner)
{
    Token next_token = p_scanner.next_token();
    if (!next_token.is_string())
    {
        warn("PartLibrary.read_logical_part_mapping: string expected at '" + p_scanner.get_scope_identifier() + "'");
        return std::nullopt;
    }
    LogicalPartMapping result;
    result.name = next_token.string_value;
    next_token = p_scanner.next_token();
    if (!next_token.is(Keyword::OPEN_BRACKET))
    {
        warn("PartLibrary.read_logical_part_mapping: open bracket expected at '" + p_scanner.get_scope_identifier()
            + "'");
        return std::nullopt;
    }
    next_token = p_scanner.next_token();
    if (!next_token.is(Keyword::COMPONENT_SCOPE))
    {
        warn("PartLibrary.read_logical_part_mapping: Keyword.COMPONENT_SCOPE expected at '"
            + p_scanner.get_scope_identifier() + "'");
        return std::nullopt;
    }
    for (;;)
    {
        p_scanner.yybegin(SpecctraDsnStreamReader::NAME);
        next_token = p_scanner.next_token();
        if (next_token.is(Keyword::CLOSED_BRACKET))
        {
            break;
        }
        if (!next_token.is_string())
        {
            warn("PartLibrary.read_logical_part_mapping: string expected at '" + p_scanner.get_scope_identifier()
                + "'");
            return std::nullopt;
        }
        result.components.insert(next_token.string_value);
    }
    next_token = p_scanner.next_token();
    if (!next_token.is(Keyword::CLOSED_BRACKET))
    {
        warn("PartLibrary.read_logical_part_mapping: closing bracket expected at '"
            + p_scanner.get_scope_identifier() + "'");
        return std::nullopt;
    }
    return result;
}

std::optional<PartLibrary::LogicalPart> PartLibrary::read_logical_part(IJFlexScanner& p_scanner)
{
    Token next_token = p_scanner.next_token();
    if (!next_token.is_string())
    {
        warn("PartLibrary.read_logical_part: string expected at '" + p_scanner.get_scope_identifier() + "'");
        return std::nullopt;
    }
    LogicalPart result;
    result.name = next_token.string_value;
    p_scanner.set_scope_identifier(result.name);
    for (;;)
    {
        const Token prev_token = next_token;
        next_token = p_scanner.next_token();
        if (next_token.is_eof())
        {
            warn("PartLibrary.read_logical_part: unexpected end of file at '" + p_scanner.get_scope_identifier() + "'");
            return std::nullopt;
        }
        if (next_token.is(Keyword::CLOSED_BRACKET))
        {
            break; // end of scope
        }
        if (prev_token.is(Keyword::OPEN_BRACKET))
        {
            if (next_token.is(Keyword::PIN))
            {
                std::optional<PartPin> current_part_pin = read_part_pin(p_scanner);
                if (!current_part_pin.has_value())
                {
                    return std::nullopt;
                }
                result.part_pins.push_back(std::move(*current_part_pin));
            }
            else
            {
                skip_scope(p_scanner);
            }
        }
    }
    return result;
}

std::optional<PartLibrary::PartPin> PartLibrary::read_part_pin(IJFlexScanner& p_scanner)
{
    auto expect_string = [&](Token p_token, std::string& p_out) {
        if (!p_token.is_string())
        {
            warn("PartLibrary.read_part_pin: string expected at '" + p_scanner.get_scope_identifier() + "'");
            return false;
        }
        p_out = p_token.string_value;
        p_scanner.set_scope_identifier(p_out);
        return true;
    };
    auto expect_int = [&](Token p_token, int* p_out) {
        if (!p_token.is_int())
        {
            warn("PartLibrary.read_part_pin: integer expected at '" + p_scanner.get_scope_identifier() + "'");
            return false;
        }
        if (p_out != nullptr)
        {
            *p_out = p_token.int_value;
        }
        return true;
    };
    PartPin result;
    p_scanner.yybegin(SpecctraDsnStreamReader::NAME);
    if (!expect_string(p_scanner.next_token(), result.pin_name))
    {
        return std::nullopt;
    }
    // Java parity: the integer after the pin name is read but not stored.
    if (!expect_int(p_scanner.next_token(), nullptr))
    {
        return std::nullopt;
    }
    p_scanner.yybegin(SpecctraDsnStreamReader::NAME);
    if (!expect_string(p_scanner.next_token(), result.gate_name))
    {
        return std::nullopt;
    }
    if (!expect_int(p_scanner.next_token(), &result.gate_swap_code))
    {
        return std::nullopt;
    }
    p_scanner.yybegin(SpecctraDsnStreamReader::NAME);
    if (!expect_string(p_scanner.next_token(), result.gate_pin_name))
    {
        return std::nullopt;
    }
    if (!expect_int(p_scanner.next_token(), &result.gate_pin_swap_code))
    {
        return std::nullopt;
    }
    // overread subgates
    Token next_token;
    do
    {
        next_token = p_scanner.next_token();
    } while (!next_token.is(Keyword::CLOSED_BRACKET) && !next_token.is_eof());
    return result;
}

} // namespace freerouting::designforms::specctra::parser
