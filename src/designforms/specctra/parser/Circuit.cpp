#include "designforms/specctra/parser/Circuit.h"

#include <utility>

#include "designforms/specctra/parser/DsnFile.h"
#include "designforms/specctra/parser/Keyword.h"
#include "designforms/specctra/parser/ScopeKeyword.h"
#include "designforms/specctra/parser/Structure.h"
#include "logger/FRLogger.h"

namespace freerouting::designforms::specctra::parser {

std::optional<std::pair<double, double>> Circuit::read_length_scope(IJFlexScanner& p_scanner)
{
    auto& log = logger::FRLogger::instance();
    double length_arr[2] = { 0, 0 };
    Token next_token;
    for (int i = 0; i < 2; ++i)
    {
        next_token = p_scanner.next_token();
        if (next_token.is_number())
        {
            length_arr[i] = next_token.number();
        }
        else
        {
            log.warn("Circuit.read_length_scope: number expected at '" + p_scanner.get_scope_identifier() + "'");
            return std::nullopt;
        }
    }
    for (;;)
    {
        const Token prev_token = next_token;
        next_token = p_scanner.next_token();
        if (next_token.is_eof())
        {
            log.warn("Circuit.read_length_scope: unexpected end of file at '"
                + p_scanner.get_scope_identifier() + "'");
            return std::nullopt;
        }
        if (next_token.is(Keyword::CLOSED_BRACKET))
        {
            // end of scope
            break;
        }
        if (prev_token.is(Keyword::OPEN_BRACKET))
        {
            ScopeKeyword::skip_scope(p_scanner);
        }
    }
    // Java: new LengthMatchingRule(lengthArr[0], lengthArr[1]) with ctor (maxLength, minLength).
    return std::pair<double, double>{ length_arr[0], length_arr[1] };
}

std::optional<Circuit::ReadScopeResult> Circuit::read_scope(IJFlexScanner& p_scanner)
{
    auto& log = logger::FRLogger::instance();
    double min_trace_length = 0;
    double max_trace_length = 0;
    std::vector<std::string> use_via;
    std::vector<std::string> use_layer;
    Token next_token;
    for (;;)
    {
        const Token prev_token = next_token;
        next_token = p_scanner.next_token();
        if (next_token.is_eof())
        {
            log.warn("Circuit.read_scope: unexpected end of file at '" + p_scanner.get_scope_identifier() + "'");
            return std::nullopt;
        }
        if (next_token.is(Keyword::CLOSED_BRACKET))
        {
            // end of scope
            break;
        }
        if (prev_token.is(Keyword::OPEN_BRACKET))
        {
            if (next_token.is(Keyword::LENGTH))
            {
                std::optional<std::pair<double, double>> length_rule = read_length_scope(p_scanner);
                if (length_rule)
                {
                    // Java parity: lengthArr[0] is the max length, lengthArr[1] the min length.
                    min_trace_length = length_rule->second;
                    max_trace_length = length_rule->first;
                }
            }
            else if (next_token.is(Keyword::USE_VIA))
            {
                // Java: useVia.addAll(readViaPadstacks(scanner)) -- addAll(null) is a NPE in Java;
                // C++ skips a failed read instead.
                std::optional<std::vector<std::string>> via_padstacks = Structure::read_via_padstacks(p_scanner);
                if (via_padstacks)
                {
                    use_via.insert(use_via.end(), via_padstacks->begin(), via_padstacks->end());
                }
            }
            else if (next_token.is(Keyword::USE_LAYER))
            {
                // Java: useLayer.addAll(Arrays.stream(readStringListScope(scanner)).toList()).
                std::optional<std::vector<std::string>> string_list = DsnFile::read_string_list_scope(p_scanner);
                if (string_list)
                {
                    use_layer.insert(use_layer.end(), string_list->begin(), string_list->end());
                }
            }
            else
            {
                ScopeKeyword::skip_scope(p_scanner);
            }
        }
    }
    return ReadScopeResult{ max_trace_length, min_trace_length, std::move(use_via), std::move(use_layer) };
}

} // namespace freerouting::designforms::specctra::parser