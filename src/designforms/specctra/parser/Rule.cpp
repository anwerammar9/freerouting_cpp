#include "designforms/specctra/parser/Rule.h"

#include "designforms/specctra/parser/DsnFile.h"
#include "designforms/specctra/parser/Keyword.h"
#include "designforms/specctra/parser/ScopeKeyword.h"
#include "designforms/specctra/parser/SpecctraDsnStreamReader.h"
#include "logger/FRLogger.h"

namespace freerouting::designforms::specctra::parser {

namespace {
void warn(const std::string& p_message) { logger::FRLogger::instance().warn(p_message); }

double next_double_or_zero(IJFlexScanner& p_scanner, const char* p_where)
{
    const std::optional<double> value = p_scanner.next_double();
    if (!value)
    {
        // Java unboxes a null Double here (NullPointerException).
        warn(std::string(p_where) + ": number expected at '" + p_scanner.get_scope_identifier() + "'");
        return 0;
    }
    return *value;
}
} // namespace

std::optional<std::vector<std::shared_ptr<Rule>>> Rule::read_scope(IJFlexScanner& p_scanner)
{
    std::vector<std::shared_ptr<Rule>> result;
    Token curr_token;
    for (;;)
    {
        const Token prev_token = curr_token;
        curr_token = p_scanner.next_token();
        if (curr_token.is_eof())
        {
            warn("Rule.read_scope: unexpected end of file at '" + p_scanner.get_scope_identifier() + "'");
            return std::nullopt;
        }
        if (curr_token.is(Keyword::CLOSED_BRACKET))
        {
            // end of scope
            break;
        }
        if (prev_token.is(Keyword::OPEN_BRACKET))
        {
            // every rule starts with a "("
            std::shared_ptr<Rule> curr_rule;
            if (curr_token.is(Keyword::WIDTH))
            {
                curr_rule = WidthRule::read(p_scanner);
            }
            else if (curr_token.is(Keyword::CLEARANCE))
            {
                curr_rule = ClearanceRule::read(p_scanner);
            }
            else
            {
                ScopeKeyword::skip_scope(p_scanner);
            }
            if (curr_rule != nullptr)
            {
                result.push_back(std::move(curr_rule));
            }
        }
    }
    return result;
}

std::optional<Rule::LayerRule> Rule::read_layer_rule_scope(IJFlexScanner& p_scanner)
{
    LayerRule result;
    for (;;)
    {
        p_scanner.yybegin(SpecctraDsnStreamReader::LAYER_NAME);
        const Token next_token = p_scanner.next_token();
        if (next_token.is(Keyword::OPEN_BRACKET))
        {
            break;
        }
        if (!next_token.is_string())
        {
            warn("Rule.read_layer_rule_scope: string expected at '" + p_scanner.get_scope_identifier() + "'");
            return std::nullopt;
        }
        result.layer_names.push_back(next_token.string_value);
    }
    for (;;)
    {
        const Token next_token = p_scanner.next_token();
        if (next_token.is(Keyword::CLOSED_BRACKET))
        {
            break;
        }
        if (!next_token.is(Keyword::RULE))
        {
            warn("Rule.read_layer_rule_scope: rule expected at '" + p_scanner.get_scope_identifier() + "'");
            return std::nullopt;
        }
        // Java ruleList.addAll(readScope(scanner)) throws on a null result.
        std::optional<std::vector<std::shared_ptr<Rule>>> rules = read_scope(p_scanner);
        if (!rules)
        {
            return std::nullopt;
        }
        result.rules.insert(result.rules.end(), rules->begin(), rules->end());
    }
    return result;
}

std::shared_ptr<WidthRule> WidthRule::read(IJFlexScanner& p_scanner)
{
    const double value = next_double_or_zero(p_scanner, "Rule.read_width_rule");
    if (!p_scanner.next_closing_bracket())
    {
        return nullptr;
    }
    return std::make_shared<WidthRule>(value);
}

std::shared_ptr<ClearanceRule> ClearanceRule::read(IJFlexScanner& p_scanner)
{
    const double value = next_double_or_zero(p_scanner, "Rule.read_clearance_rule");
    std::vector<std::string> class_pairs;
    Token next_token = p_scanner.next_token();
    if (!next_token.is(Keyword::CLOSED_BRACKET))
    {
        // look for "(type"
        if (!next_token.is(Keyword::OPEN_BRACKET))
        {
            warn("Rule.read_clearance_rule: ( expected at '" + p_scanner.get_scope_identifier() + "'");
            return nullptr;
        }
        next_token = p_scanner.next_token();
        if (!next_token.is(Keyword::TYPE))
        {
            warn("Rule.read_clearance_rule: type expected at '" + p_scanner.get_scope_identifier() + "'");
            return nullptr;
        }
        class_pairs = p_scanner.next_string_list(DsnFile::kClassClearanceSeparator);
        // check the closing ")" of "(type" and of "(clear"
        if (!p_scanner.next_closing_bracket() || !p_scanner.next_closing_bracket())
        {
            warn("Rule.read_clearance_rule: closing bracket expected at '" + p_scanner.get_scope_identifier() + "'");
            return nullptr;
        }
    }
    return std::make_shared<ClearanceRule>(value, std::move(class_pairs));
}

} // namespace freerouting::designforms::specctra::parser
