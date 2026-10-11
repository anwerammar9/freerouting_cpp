#include "designforms/specctra/parser/NetClass.h"

#include <utility>

#include "designforms/specctra/parser/Circuit.h"
#include "designforms/specctra/parser/DsnFile.h"
#include "designforms/specctra/parser/Keyword.h"
#include "designforms/specctra/parser/ScopeKeyword.h"
#include "designforms/specctra/parser/SpecctraDsnStreamReader.h"
#include "logger/FRLogger.h"

namespace freerouting::designforms::specctra::parser {

std::optional<NetClass> NetClass::read_scope(IJFlexScanner& p_scanner)
{
    auto& scanner = static_cast<SpecctraDsnStreamReader&>(p_scanner);

    // read the class name
    scanner.yybegin(SpecctraDsnStreamReader::NAME);
    const std::string class_name = scanner.next_string();

    // read the nets belonging to the class
    std::vector<std::string> net_list = scanner.next_string_list();

    std::vector<std::shared_ptr<Rule>> rules;
    std::vector<Rule::LayerRule> layer_rules;
    std::vector<std::string> use_via;
    std::vector<std::string> use_layer;
    std::optional<std::string> via_rule;
    std::optional<std::string> trace_clearance_class;
    bool pull_tight = true;
    bool shove_fixed = false;
    double min_trace_length = 0;
    double max_trace_length = 0;

    Token next_token = scanner.next_token();
    Token prev_token = next_token;
    for (;;)
    {
        next_token = scanner.next_token();
        if (next_token.is_eof())
        {
            logger::FRLogger::instance().warn(
                "NetClass.read_scope: unexpected end of file at '" + scanner.get_scope_identifier() + "'");
            return std::nullopt;
        }
        if (next_token.is(Keyword::CLOSED_BRACKET))
        {
            // end of scope
            break;
        }
        if (prev_token.is(Keyword::OPEN_BRACKET))
        {
            if (next_token.is(Keyword::RULE))
            {
                std::optional<std::vector<std::shared_ptr<Rule>>> read = Rule::read_scope(scanner);
                if (read.has_value())
                {
                    rules.insert(rules.end(), read->begin(), read->end());
                }
            }
            else if (next_token.is(Keyword::LAYER_RULE))
            {
                std::optional<Rule::LayerRule> read = Rule::read_layer_rule_scope(scanner);
                if (read.has_value())
                {
                    layer_rules.push_back(std::move(*read));
                }
            }
            else if (next_token.is(Keyword::VIA_RULE))
            {
                via_rule = DsnFile::read_string_scope(scanner);
            }
            else if (next_token.is(Keyword::CIRCUIT))
            {
                std::optional<Circuit::ReadScopeResult> current_rule = Circuit::read_scope(scanner);
                if (current_rule.has_value())
                {
                    max_trace_length = current_rule->max_length;
                    min_trace_length = current_rule->min_length;
                    use_via.insert(use_via.end(), current_rule->use_via.begin(), current_rule->use_via.end());
                    use_layer.insert(use_layer.end(), current_rule->use_layer.begin(), current_rule->use_layer.end());
                }
            }
            else if (next_token.is(Keyword::CLEARANCE_CLASS))
            {
                trace_clearance_class = DsnFile::read_string_scope(scanner);
                if (!trace_clearance_class.has_value())
                {
                    return std::nullopt;
                }
            }
            else if (next_token.is(Keyword::SHOVE_FIXED))
            {
                shove_fixed = DsnFile::read_on_off_scope(scanner);
            }
            else if (next_token.is(Keyword::PULL_TIGHT))
            {
                pull_tight = DsnFile::read_on_off_scope(scanner);
            }
            else
            {
                ScopeKeyword::skip_scope(scanner);
            }
        }
        prev_token = next_token;
    }

    NetClass result;
    result.name = std::move(class_name);
    result.trace_clearance_class = std::move(trace_clearance_class);
    result.net_list = std::move(net_list);
    result.rules = std::move(rules);
    result.layer_rules = std::move(layer_rules);
    result.use_via = std::move(use_via);
    result.use_layer = std::move(use_layer);
    result.via_rule = std::move(via_rule);
    result.shove_fixed = shove_fixed;
    result.pull_tight = pull_tight;
    result.min_trace_length = min_trace_length;
    result.max_trace_length = max_trace_length;
    return result;
}

std::optional<NetClass::ClassClass> NetClass::read_class_class_scope(IJFlexScanner& p_scanner)
{
    auto& scanner = static_cast<SpecctraDsnStreamReader&>(p_scanner);
    ClassClass result;
    Token prev_token;
    for (;;)
    {
        Token next_token = scanner.next_token();
        if (next_token.is_eof())
        {
            logger::FRLogger::instance().warn(
                "NetClass.read_class_class_scope: unexpected end of file at '" + scanner.get_scope_identifier() + "'");
            return std::nullopt;
        }
        if (next_token.is(Keyword::CLOSED_BRACKET))
        {
            // end of scope
            break;
        }
        if (prev_token.is(Keyword::OPEN_BRACKET))
        {
            if (next_token.is(Keyword::CLASSES))
            {
                std::optional<std::vector<std::string>> names = DsnFile::read_string_list_scope(scanner);
                if (names.has_value())
                {
                    result.class_names.insert(result.class_names.end(), names->begin(), names->end());
                }
            }
            else if (next_token.is(Keyword::RULE))
            {
                std::optional<std::vector<std::shared_ptr<Rule>>> read = Rule::read_scope(scanner);
                if (read.has_value())
                {
                    result.rules.insert(result.rules.end(), read->begin(), read->end());
                }
            }
            else if (next_token.is(Keyword::LAYER_RULE))
            {
                std::optional<Rule::LayerRule> read = Rule::read_layer_rule_scope(scanner);
                if (read.has_value())
                {
                    result.layer_rules.push_back(std::move(*read));
                }
            }
        }
        prev_token = next_token;
    }
    return result;
}

} // namespace freerouting::designforms::specctra::parser
