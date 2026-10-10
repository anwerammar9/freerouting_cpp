#include "designforms/specctra/parser/Network.h"

#include <cmath>
#include <memory>

#include "board/RoutingBoard.h"
#include "designforms/specctra/parser/Keyword.h"
#include "designforms/specctra/parser/Layer.h"
#include "designforms/specctra/parser/NetList.h"
#include "designforms/specctra/parser/ReadScopeParameter.h"
#include "designforms/specctra/parser/Rule.h"
#include "designforms/specctra/parser/SpecctraDsnStreamReader.h"
#include "logger/FRLogger.h"
#include "rules/BoardRules.h"
#include "rules/Net.h"
#include "rules/NetClass.h"

namespace freerouting::designforms::specctra::parser {

std::vector<std::set<Net::Pin>> Network::create_ordered_subnets(const std::vector<Net::Pin>& p_pin_list)
{
    std::vector<std::set<Net::Pin>> result;
    if (p_pin_list.empty())
    {
        return result;
    }

    auto it = p_pin_list.begin();
    Net::Pin prev_pin = *it;
    for (++it; it != p_pin_list.end(); ++it)
    {
        std::set<Net::Pin> current_subnet_pin_list;
        current_subnet_pin_list.insert(prev_pin);
        current_subnet_pin_list.insert(*it);
        result.push_back(std::move(current_subnet_pin_list));
        prev_pin = *it;
    }
    return result;
}

std::optional<std::vector<Net::Pin>> Network::read_net_pins(IJFlexScanner& p_scanner)
{
    auto& log = logger::FRLogger::instance();
    // Java casts the scanner to SpecctraDsnStreamReader to use nextString(boolean, char).
    auto& scanner = static_cast<SpecctraDsnStreamReader&>(p_scanner);
    std::vector<Net::Pin> pin_list;
    std::string component_name;
    while (!(component_name = scanner.next_string(true, U'-')).empty())
    {
        scanner.yybegin(SpecctraDsnStreamReader::SPEC_CHAR);
        scanner.next_token(); // overread the hyphen

        const std::string pin_name = scanner.next_string(true);
        pin_list.push_back(Net::Pin{ component_name, pin_name });
    }

    const Token next_token = scanner.next_token();
    if (next_token.is_eof())
    {
        log.warn("Network.read_net_pins: unexpected end of file at '" + scanner.get_scope_identifier() + "'");
        return std::nullopt;
    }
    if (!next_token.is(Keyword::CLOSED_BRACKET))
    {
        // not end of scope
        log.warn("Network.read_net_pins: expected closed bracket is missing at '" + scanner.get_scope_identifier() + "'");
    }
    return pin_list;
}

bool Network::read_net_scope(IJFlexScanner& p_scanner, NetList& p_net_list, board::RoutingBoard& p_board,
    const CoordinateTransform& p_coordinate_transform, const LayerStructure& p_layer_structure)
{
    auto& log = logger::FRLogger::instance();
    // read the net name
    const std::string net_name = p_scanner.next_string();

    Token next_token = p_scanner.next_token();
    int subnet_number = 1;
    const bool scope_is_empty = next_token.is(Keyword::CLOSED_BRACKET);
    if (next_token.is_int())
    {
        subnet_number = next_token.int_value;
    }
    bool pin_order_found = false;
    std::vector<Net::Pin> pin_list;
    std::vector<std::shared_ptr<Rule>> net_rules;
    std::vector<std::set<Net::Pin>> subnet_pin_lists;
    if (!scope_is_empty)
    {
        for (;;)
        {
            const Token prev_token = next_token;
            next_token = p_scanner.next_token();
            if (next_token.is_eof())
            {
                log.warn("Network.read_net_scope: unexpected end of file at '" + p_scanner.get_scope_identifier() + "'");
                return false;
            }
            if (next_token.is(Keyword::CLOSED_BRACKET))
            {
                // end of scope
                break;
            }
            if (prev_token.is(Keyword::OPEN_BRACKET))
            {
                if (next_token.is(Keyword::PINS))
                {
                    std::optional<std::vector<Net::Pin>> pins = read_net_pins(p_scanner);
                    if (!pins)
                    {
                        return false;
                    }
                    pin_list.insert(pin_list.end(), pins->begin(), pins->end());
                }
                else if (next_token.is(Keyword::ORDER))
                {
                    pin_order_found = true;
                    std::optional<std::vector<Net::Pin>> pins = read_net_pins(p_scanner);
                    if (!pins)
                    {
                        return false;
                    }
                    pin_list.insert(pin_list.end(), pins->begin(), pins->end());
                }
                else if (next_token.is(Keyword::FROMTO))
                {
                    std::optional<std::vector<Net::Pin>> pins = read_net_pins(p_scanner);
                    if (!pins)
                    {
                        return false;
                    }
                    subnet_pin_lists.emplace_back(pins->begin(), pins->end());
                }
                else if (next_token.is(Keyword::RULE))
                {
                    std::optional<std::vector<std::shared_ptr<Rule>>> rules = Rule::read_scope(p_scanner);
                    if (rules)
                    {
                        net_rules.insert(net_rules.end(), rules->begin(), rules->end());
                    }
                }
                else if (next_token.is(Keyword::LAYER_RULE))
                {
                    log.warn("Network.read_net_scope: layer_rule not yet implemented at '"
                        + p_scanner.get_scope_identifier() + "'");
                    ScopeKeyword::skip_scope(p_scanner);
                }
                else
                {
                    ScopeKeyword::skip_scope(p_scanner);
                }
            }
        }
    }
    if (subnet_pin_lists.empty())
    {
        if (pin_order_found)
        {
            subnet_pin_lists = create_ordered_subnets(pin_list);
        }
        else
        {
            subnet_pin_lists.emplace_back(pin_list.begin(), pin_list.end());
        }
    }
    for (const std::set<Net::Pin>& current_pin_list : subnet_pin_lists)
    {
        const Net::Id net_id{ net_name, subnet_number };
        if (!p_net_list.contains(net_id))
        {
            Net* new_net = p_net_list.add_net(net_id);
            const bool contains_plane = p_layer_structure.contains_plane(net_name);
            if (new_net != nullptr)
            {
                p_board.mutable_rules().get_nets().add(new_net->id.name, new_net->id.subnet_number, contains_plane);
            }
        }
        Net* current_subnet = p_net_list.get_net(net_id);
        if (current_subnet == nullptr)
        {
            log.warn("Network.read_net_scope: net not found in netlist at '" + p_scanner.get_scope_identifier() + "'");
            return false;
        }
        current_subnet->set_pins(std::vector<Net::Pin>(current_pin_list.begin(), current_pin_list.end()));
        if (!net_rules.empty())
        {
            // Evaluate the net rules.
            rules::Net* board_net =
                p_board.mutable_rules().get_nets().get(current_subnet->id.name, current_subnet->id.subnet_number);
            if (board_net == nullptr)
            {
                log.warn("Network.read_net_scope: board net not found at '" + p_scanner.get_scope_identifier() + "'");
                return false;
            }
            for (const std::shared_ptr<Rule>& current_object : net_rules)
            {
                if (const auto* rule = dynamic_cast<const WidthRule*>(current_object.get()))
                {
                    const rules::NetClass* default_net_rule = p_board.mutable_rules().get_default_net_class();
                    const double wire_width = rule->value;
                    // Java (int) Math.round(x): floor(x + 0.5), not std::lround.
                    const int trace_halfwidth =
                        static_cast<int>(std::floor(p_coordinate_transform.dsn_to_board(wire_width) / 2 + 0.5));
                    rules::NetClass* net_rule = p_board.mutable_rules().get_net_classes().find(
                        trace_halfwidth, default_net_rule->get_trace_clearance_class(), default_net_rule->get_via_rule());
                    if (net_rule == nullptr)
                    {
                        // create a new net rule
                        net_rule = p_board.mutable_rules().get_new_net_class();
                    }
                    net_rule->set_trace_half_width(trace_halfwidth);
                    board_net->set_net_class(net_rule);
                }
                else
                {
                    log.warn("Network.read_net_scope: Rule not yet implemented at '"
                        + p_scanner.get_scope_identifier() + "'");
                }
            }
        }
        ++subnet_number;
    }
    return true;
}

bool Network::read_scope(ReadScopeParameter& p_par) const
{
    auto& log = logger::FRLogger::instance();
    Token next_token;
    for (;;)
    {
        const Token prev_token = next_token;
        next_token = p_par.scanner.next_token();
        if (next_token.is_eof())
        {
            log.warn("Network.read_scope: unexpected end of file at '" + p_par.scanner.get_scope_identifier() + "'");
            return false;
        }
        if (next_token.is(Keyword::CLOSED_BRACKET))
        {
            // end of scope
            break;
        }
        if (prev_token.is(Keyword::OPEN_BRACKET))
        {
            if (next_token.is(Keyword::NET))
            {
                board::RoutingBoard* board = p_par.get_board();
                if (board == nullptr || !p_par.coordinate_transform.has_value() || p_par.layer_structure == nullptr)
                {
                    // Java would throw a NullPointerException here; a net scope always follows structure.
                    log.warn("Network.read_scope: net scope without structure at '" + p_par.scanner.get_scope_identifier()
                        + "'");
                    return false;
                }
                // Java ignores the return value of readNetScope.
                read_net_scope(
                    p_par.scanner, p_par.netlist, *board, *p_par.coordinate_transform, *p_par.layer_structure);
            }
            else if (next_token.is(Keyword::VIA))
            {
                // T114a: Network.readViaInfo
                ScopeKeyword::skip_scope(p_par.scanner);
            }
            else if (next_token.is(Keyword::VIA_RULE))
            {
                // T114a: Network.readViaRule
                ScopeKeyword::skip_scope(p_par.scanner);
            }
            else if (next_token.is(Keyword::CLASS))
            {
                // T114b: NetClass.readScope
                ScopeKeyword::skip_scope(p_par.scanner);
            }
            else if (next_token.is(Keyword::CLASS_CLASS))
            {
                // T114b: NetClass.readClassClassScope
                ScopeKeyword::skip_scope(p_par.scanner);
            }
            else
            {
                ScopeKeyword::skip_scope(p_par.scanner);
            }
        }
    }

    // T114a: add the net-class via padstacks, insertViaInfos and insertViaRules.
    // T114b: insertNetClasses and insertClassPairs.
    // T115: insertComponents and insertLogicalParts.
    return true;
}

} // namespace freerouting::designforms::specctra::parser
