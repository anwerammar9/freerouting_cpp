#include "designforms/specctra/parser/Network.h"

#include <cmath>
#include <memory>
#include <regex>
#include <utility>

#include "board/BoardLibrary.h"
#include "board/Padstack.h"
#include "board/Padstacks.h"
#include "board/RoutingBoard.h"
#include "designforms/specctra/parser/DsnFile.h"
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
#include "rules/ViaInfo.h"
#include "rules/ViaRule.h"

namespace freerouting::designforms::specctra::parser {

namespace {

/** Java currentPadstackName.replaceAll("\\.\\d+", ""). */
std::string clean_padstack_name(const std::string& p_name)
{
    static const std::regex kSuffix("\\.\\d+");
    return std::regex_replace(p_name, kSuffix, "");
}

} // namespace

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

std::shared_ptr<rules::ViaInfo> Network::read_via_info(IJFlexScanner& p_scanner, board::RoutingBoard& p_board)
{
    auto& log = logger::FRLogger::instance();
    auto& scanner = static_cast<SpecctraDsnStreamReader&>(p_scanner);

    scanner.yybegin(SpecctraDsnStreamReader::NAME);
    Token next_token = scanner.next_token();
    if (!next_token.is_string())
    {
        log.warn("Network.read_via_info: string expected at '" + scanner.get_scope_identifier() + "'");
        return nullptr;
    }
    const std::string name = next_token.string_value;

    scanner.yybegin(SpecctraDsnStreamReader::NAME);
    next_token = scanner.next_token();
    if (!next_token.is_string())
    {
        log.warn("Network.read_via_info: string expected at '" + scanner.get_scope_identifier() + "'");
        return nullptr;
    }
    const std::string padstack_name = next_token.string_value;
    scanner.set_scope_identifier(padstack_name);
    board::Padstack* via_padstack = p_board.library().get_via_padstack(padstack_name);
    if (via_padstack == nullptr)
    {
        // The padstack may not yet be inserted into the list of via padstacks
        via_padstack = p_board.library().padstacks().get(padstack_name);
        if (via_padstack == nullptr)
        {
            log.warn("Network.read_via_info: padstack not found at '" + scanner.get_scope_identifier() + "'");
            return nullptr;
        }
        p_board.library().add_via_padstack(via_padstack);
    }

    scanner.yybegin(SpecctraDsnStreamReader::NAME);
    next_token = scanner.next_token();
    if (!next_token.is_string())
    {
        log.warn("Network.read_via_info: string expected at '" + scanner.get_scope_identifier() + "'");
        return nullptr;
    }
    int clearance_class = p_board.mutable_rules().get_clearance_matrix().get_no(next_token.string_value);
    if (clearance_class < 0)
    {
        // Clearance class not stored, because it is identical to the default clearance netClass.
        clearance_class = rules::BoardRules::default_clearance_class();
    }

    bool attach_allowed = false;
    next_token = scanner.next_token();
    if (!next_token.is(Keyword::CLOSED_BRACKET))
    {
        if (!next_token.is(Keyword::ATTACH))
        {
            log.warn("Network.read_via_info: Keyword.ATTACH expected at '" + scanner.get_scope_identifier() + "'");
            return nullptr;
        }
        attach_allowed = true;
        next_token = scanner.next_token();
        if (!next_token.is(Keyword::CLOSED_BRACKET))
        {
            log.warn("Network.read_via_info: closing bracket expected at '" + scanner.get_scope_identifier() + "'");
            return nullptr;
        }
    }
    return std::make_shared<rules::ViaInfo>(
        name, via_padstack, clearance_class, attach_allowed, &p_board.mutable_rules());
}

std::optional<std::vector<std::string>> Network::read_via_rule(IJFlexScanner& p_scanner, board::RoutingBoard&)
{
    auto& log = logger::FRLogger::instance();
    auto& scanner = static_cast<SpecctraDsnStreamReader&>(p_scanner);
    std::vector<std::string> result;
    for (;;)
    {
        scanner.yybegin(SpecctraDsnStreamReader::NAME);
        const Token next_token = scanner.next_token();
        if (next_token.is(Keyword::CLOSED_BRACKET))
        {
            break;
        }
        if (!next_token.is_string())
        {
            log.warn("Network.read_via_rule: string expected at '" + scanner.get_scope_identifier() + "'");
            return std::nullopt;
        }
        result.push_back(next_token.string_value);
    }
    return result;
}

void Network::insert_via_infos(const std::vector<std::shared_ptr<rules::ViaInfo>>& p_via_infos,
    board::RoutingBoard& p_board, bool p_attach_allowed)
{
    if (!p_via_infos.empty())
    {
        for (const std::shared_ptr<rules::ViaInfo>& current_info : p_via_infos)
        {
            p_board.mutable_rules().get_via_infos().add(current_info);
        }
    }
    else
    {
        // No via infos found; create default via infos from the via padstacks.
        create_default_via_infos(p_board, p_board.mutable_rules().get_default_net_class(), p_attach_allowed);
    }
}

void Network::create_default_via_infos(board::RoutingBoard& p_board, rules::NetClass* p_net_class, bool p_attach_allowed)
{
    const int clearance_class_index =
        p_net_class->get_default_item_clearance_classes().get(rules::ItemClass::VIA);
    const bool is_default_class = (p_net_class == p_board.mutable_rules().get_default_net_class());
    for (int i = 0; i < p_board.library().via_padstack_count(); ++i)
    {
        board::Padstack* current_padstack = p_board.library().get_via_padstack(i);
        const bool via_attach_allowed = p_attach_allowed && current_padstack->is_attach_allowed();
        std::string via_name;
        if (is_default_class)
        {
            via_name = current_padstack->get_name();
        }
        else
        {
            via_name = current_padstack->get_name() + DsnFile::kClassClearanceSeparator + p_net_class->get_name();
        }
        auto found_via_info = std::make_shared<rules::ViaInfo>(
            via_name, current_padstack, clearance_class_index, via_attach_allowed, &p_board.mutable_rules());
        p_board.mutable_rules().get_via_infos().add(found_via_info);
    }
}

void Network::insert_via_rules(const std::vector<std::vector<std::string>>& p_via_rules, board::RoutingBoard& p_board)
{
    bool rule_found = false;
    for (const std::vector<std::string>& current_list : p_via_rules)
    {
        if (current_list.size() < 2)
        {
            continue;
        }
        if (add_via_rule(current_list, p_board))
        {
            rule_found = true;
        }
    }
    rules::BoardRules& board_rules = p_board.mutable_rules();
    if (!rule_found)
    {
        board_rules.create_default_via_rule(board_rules.get_default_net_class(), "default");
    }
    for (int i = 0; i < board_rules.get_net_classes().count(); ++i)
    {
        board_rules.get_net_classes().get(i)->set_via_rule(board_rules.get_default_via_rule());
    }
}

bool Network::add_via_rule(const std::vector<std::string>& p_name_list, board::RoutingBoard& p_board)
{
    auto& log = logger::FRLogger::instance();
    auto it = p_name_list.begin();
    const std::string rule_name = *it; // Java it.next(); the caller guarantees at least one element.
    rules::BoardRules& board_rules = p_board.mutable_rules();
    rules::ViaRule* existing_rule = board_rules.get_via_rule(rule_name);
    auto current_rule = std::make_unique<rules::ViaRule>(rule_name);
    bool rule_ok = true;
    for (++it; it != p_name_list.end(); ++it)
    {
        const std::shared_ptr<rules::ViaInfo> current_via = board_rules.get_via_infos().get(*it);
        if (current_via != nullptr)
        {
            current_rule->append_via(current_via);
        }
        else
        {
            log.warn("Network.insert_via_rules: viaInfo not found");
            rule_ok = false;
        }
    }
    if (rule_ok)
    {
        if (existing_rule != nullptr)
        {
            // Replace already existing rule.
            board_rules.remove_via_rule(existing_rule);
        }
        board_rules.add_via_rule(std::move(current_rule));
    }
    return rule_ok;
}

bool Network::read_scope(ReadScopeParameter& p_par) const
{
    auto& log = logger::FRLogger::instance();
    // T114b: NetClass and NetClass.ClassClass collections (the class branches below are stubs).
    std::vector<std::shared_ptr<rules::ViaInfo>> via_infos;
    std::vector<std::vector<std::string>> via_rules;
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
                board::RoutingBoard* board = p_par.get_board();
                if (board == nullptr)
                {
                    // Java would throw a NullPointerException here.
                    log.warn("Network.read_scope: via info without board at '" + p_par.scanner.get_scope_identifier()
                        + "'");
                    return false;
                }
                const std::shared_ptr<rules::ViaInfo> current_via_info = read_via_info(p_par.scanner, *board);
                if (current_via_info == nullptr)
                {
                    return false;
                }
                via_infos.push_back(current_via_info);
            }
            else if (next_token.is(Keyword::VIA_RULE))
            {
                board::RoutingBoard* board = p_par.get_board();
                if (board == nullptr)
                {
                    // Java would throw a NullPointerException here.
                    log.warn("Network.read_scope: via rule without board at '" + p_par.scanner.get_scope_identifier()
                        + "'");
                    return false;
                }
                std::optional<std::vector<std::string>> current_via_rule = read_via_rule(p_par.scanner, *board);
                if (!current_via_rule.has_value())
                {
                    return false;
                }
                via_rules.push_back(std::move(*current_via_rule));
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

    board::RoutingBoard* board = p_par.get_board();
    if (board == nullptr)
    {
        // Java would throw a NullPointerException when touching board.library below.
        log.warn("Network.read_scope: network scope without board at '" + p_par.scanner.get_scope_identifier() + "'");
        return false;
    }

    // T114b: append the use_via names of each net class to via_padstack_names.

    // Set the via padstacks after network parsing, so that named vias from both structure and
    // network DSN sections are properly instantiated.
    if (p_par.via_padstack_names.has_value())
    {
        std::vector<board::Padstack*> via_padstacks;
        via_padstacks.reserve(p_par.via_padstack_names->size());
        for (const std::string& current_padstack_name : *p_par.via_padstack_names)
        {
            const std::string cleaned_name = clean_padstack_name(current_padstack_name);
            board::Padstack* current_padstack = board->library().padstacks().get(cleaned_name);
            if (current_padstack != nullptr)
            {
                via_padstacks.push_back(current_padstack);
            }
            else
            {
                log.warn("Library.read_scope: via padstack with name '" + current_padstack_name + " not found at '"
                    + p_par.scanner.get_scope_identifier() + "'");
            }
        }
        board->library().set_via_padstacks(std::move(via_padstacks));
    }

    insert_via_infos(via_infos, *board, p_par.via_at_smd_allowed);
    insert_via_rules(via_rules, *board);
    // T114b: insertNetClasses and insertClassPairs.
    // T115: insertComponents and insertLogicalParts.
    return true;
}

} // namespace freerouting::designforms::specctra::parser
