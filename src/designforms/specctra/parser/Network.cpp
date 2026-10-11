#include "designforms/specctra/parser/Network.h"

#include <algorithm>
#include <cctype>
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
#include "designforms/specctra/parser/Structure.h"
#include "logger/FRLogger.h"
#include "rules/BoardRules.h"
#include "rules/ClearanceMatrix.h"
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

/** Java Math.round for non-negative values used here. */
int java_round(double p_value) { return static_cast<int>(std::floor(p_value + 0.5)); }

/** Java String.equalsIgnoreCase. */
bool equals_ignore_case(const std::string& p_left, const std::string& p_right)
{
    if (p_left.size() != p_right.size())
    {
        return false;
    }
    for (std::size_t i = 0; i < p_left.size(); ++i)
    {
        if (std::tolower(static_cast<unsigned char>(p_left[i])) != std::tolower(static_cast<unsigned char>(p_right[i])))
        {
            return false;
        }
    }
    return true;
}

/** Java KiCadNetClassNames.isKiCadDefaultNetClassName. */
bool is_kicad_default_net_class_name(const std::string& p_name)
{
    if (p_name.empty())
    {
        return false;
    }
    return equals_ignore_case(p_name, "default") || equals_ignore_case(p_name, "kicad_default");
}

/** Java KiCadNetClassNames.resolveNetClass. */
rules::NetClass* resolve_net_class(rules::BoardRules& p_rules, const std::string& p_name)
{
    if (is_kicad_default_net_class_name(p_name))
    {
        return p_rules.get_default_net_class();
    }
    return p_rules.get_net_classes().get(p_name);
}

/** Java String.split("_"): split at every '_', dropping trailing empty parts. */
std::vector<std::string> java_split_underscore(const std::string& p_text)
{
    std::vector<std::string> result;
    std::size_t start = 0;
    for (;;)
    {
        const std::size_t pos = p_text.find('_', start);
        if (pos == std::string::npos)
        {
            result.push_back(p_text.substr(start));
            break;
        }
        result.push_back(p_text.substr(start, pos - start));
        start = pos + 1;
    }
    while (!result.empty() && result.back().empty())
    {
        result.pop_back();
    }
    return result;
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

void Network::insert_net_classes(const std::vector<NetClass>& p_net_classes, ReadScopeParameter& p_par)
{
    board::RoutingBoard* routing_board = p_par.get_board();
    for (const NetClass& current_class : p_net_classes)
    {
        insert_net_class(current_class, *p_par.layer_structure, *routing_board, *p_par.coordinate_transform,
            p_par.via_at_smd_allowed);
    }
}

void Network::insert_net_class(const NetClass& p_net_class, const LayerStructure& p_layer_structure,
    board::RoutingBoard& p_board, const CoordinateTransform& p_coordinate_transform, bool p_via_at_smd_allowed)
{
    auto& log = logger::FRLogger::instance();
    rules::BoardRules& board_rules = p_board.mutable_rules();
    rules::NetClass* board_net_class = is_kicad_default_net_class_name(p_net_class.name)
        ? board_rules.get_default_net_class()
        : board_rules.append_net_class(p_net_class.name);
    if (p_net_class.trace_clearance_class.has_value())
    {
        const int trace_clearance_class = board_rules.get_clearance_matrix().get_no(*p_net_class.trace_clearance_class);
        if (trace_clearance_class >= 0)
        {
            board_net_class->set_trace_clearance_class(trace_clearance_class);
        }
        else
        {
            log.warn("Network.insert_net_class: clearance class not found at '" + board_net_class->get_name() + "'");
        }
    }
    if (p_net_class.via_rule.has_value())
    {
        rules::ViaRule* via_rule = board_rules.get_via_rule(*p_net_class.via_rule);
        if (via_rule != nullptr)
        {
            board_net_class->set_via_rule(via_rule);
        }
        else
        {
            log.warn("Network.insert_net_class: via rule not found at '" + board_net_class->get_name() + "'");
        }
    }
    if (p_net_class.max_trace_length > 0)
    {
        board_net_class->set_maximum_trace_length(p_coordinate_transform.dsn_to_board(p_net_class.max_trace_length));
    }
    if (p_net_class.min_trace_length > 0)
    {
        board_net_class->set_minimum_trace_length(p_coordinate_transform.dsn_to_board(p_net_class.min_trace_length));
    }

    for (const std::string& current_net_name : p_net_class.net_list)
    {
        std::vector<rules::Net*> current_net_list = board_rules.get_nets().get_all_by_name(current_net_name);
        for (rules::Net* current_net : current_net_list)
        {
            current_net->set_net_class(board_net_class);
        }
    }

    bool clearance_rule_found = false;
    for (const std::shared_ptr<Rule>& current_rule : p_net_class.rules)
    {
        if (const auto* rule1 = dynamic_cast<const WidthRule*>(current_rule.get()))
        {
            const int trace_halfwidth = java_round(p_coordinate_transform.dsn_to_board(rule1->value / 2));
            board_net_class->set_trace_half_width(trace_halfwidth);
        }
        else if (const auto* rule = dynamic_cast<const ClearanceRule*>(current_rule.get()))
        {
            add_clearance_rule(board_rules.get_clearance_matrix(), board_net_class, *rule, -1, p_coordinate_transform);
            clearance_rule_found = true;
        }
        else
        {
            log.warn("Network.insert_net_class: rule type not yet implemented at '" + board_net_class->get_name() + "'");
        }
    }

    for (const Rule::LayerRule& current_layer_rule : p_net_class.layer_rules)
    {
        for (const std::string& current_layer_name : current_layer_rule.layer_names)
        {
            const int layer_index = p_board.get_layer_structure().get_no(current_layer_name);
            if (layer_index < 0)
            {
                log.warn("Network.insert_net_class: layer not found at '" + board_net_class->get_name() + "'");
                continue;
            }
            for (const std::shared_ptr<Rule>& current_rule : current_layer_rule.rules)
            {
                if (const auto* rule1 = dynamic_cast<const WidthRule*>(current_rule.get()))
                {
                    const int trace_halfwidth = java_round(p_coordinate_transform.dsn_to_board(rule1->value / 2));
                    board_net_class->set_trace_half_width(layer_index, trace_halfwidth);
                }
                else if (const auto* rule = dynamic_cast<const ClearanceRule*>(current_rule.get()))
                {
                    add_clearance_rule(
                        board_rules.get_clearance_matrix(), board_net_class, *rule, layer_index, p_coordinate_transform);
                    clearance_rule_found = true;
                }
                else
                {
                    log.warn("Network.insert_net_class: layer rule type not yet implemented at '"
                        + board_net_class->get_name() + "'");
                }
            }
        }
    }

    board_net_class->set_pull_tight(p_net_class.pull_tight);
    board_net_class->set_shove_fixed(p_net_class.shove_fixed);
    bool via_infos_created = false;
    if (clearance_rule_found && board_net_class != board_rules.get_default_net_class())
    {
        create_default_via_infos(p_board, board_net_class, p_via_at_smd_allowed);
        via_infos_created = true;
    }
    if (!p_net_class.use_via.empty())
    {
        create_via_rule(p_net_class.use_via, board_net_class, p_board, p_via_at_smd_allowed);
    }
    else if (via_infos_created)
    {
        board_rules.create_default_via_rule(board_net_class, board_net_class->get_name());
    }
    if (!p_net_class.use_layer.empty())
    {
        create_active_trace_layers(p_net_class.use_layer, p_layer_structure, board_net_class);
    }
}

void Network::insert_class_pairs(const std::vector<NetClass::ClassClass>& p_class_classes, ReadScopeParameter& p_par)
{
    board::RoutingBoard& board = *p_par.get_board();
    for (const NetClass::ClassClass& current_class_class : p_class_classes)
    {
        const std::vector<std::string>& class_names = current_class_class.class_names;
        for (std::size_t i = 0; i < class_names.size(); ++i)
        {
            rules::NetClass* first_class = resolve_net_class(board.mutable_rules(), class_names[i]);
            if (first_class == nullptr)
            {
                continue;
            }
            for (std::size_t j = i + 1; j < class_names.size(); ++j)
            {
                rules::NetClass* second_class = resolve_net_class(board.mutable_rules(), class_names[j]);
                if (second_class == nullptr)
                {
                    continue;
                }
                insert_class_pair_info(
                    current_class_class, first_class, second_class, board, *p_par.coordinate_transform);
            }
        }
    }
}

void Network::insert_class_pair_info(const NetClass::ClassClass& p_class_class, rules::NetClass* p_first_class,
    rules::NetClass* p_second_class, board::RoutingBoard& p_board, const CoordinateTransform& p_coordinate_transform)
{
    auto& log = logger::FRLogger::instance();
    for (const std::shared_ptr<Rule>& current_rule : p_class_class.rules)
    {
        if (const auto* current_clearance_rule = dynamic_cast<const ClearanceRule*>(current_rule.get()))
        {
            add_mixed_clearance_rule(p_board.mutable_rules().get_clearance_matrix(), p_first_class, p_second_class,
                *current_clearance_rule, -1, p_coordinate_transform);
        }
        else
        {
            log.warn("Network.insert_class_pair_info: unexpected rule");
        }
    }
    for (const Rule::LayerRule& current_layer_rule : p_class_class.layer_rules)
    {
        for (const std::string& current_layer_name : current_layer_rule.layer_names)
        {
            const int layer_index = p_board.get_layer_structure().get_no(current_layer_name);
            if (layer_index < 0)
            {
                log.warn("Network.insert_class_pair_info: layer not found at '" + current_layer_name + "'");
                continue;
            }
            for (const std::shared_ptr<Rule>& current_rule : current_layer_rule.rules)
            {
                if (const auto* rule = dynamic_cast<const ClearanceRule*>(current_rule.get()))
                {
                    add_mixed_clearance_rule(p_board.mutable_rules().get_clearance_matrix(), p_first_class,
                        p_second_class, *rule, layer_index, p_coordinate_transform);
                }
                else
                {
                    log.warn("Network.insert_class_pair_info: unexpected layer rule type");
                }
            }
        }
    }
}

void Network::add_mixed_clearance_rule(rules::ClearanceMatrix& p_clearance_matrix, rules::NetClass* p_first_class,
    rules::NetClass* p_second_class, const ClearanceRule& p_clearance_rule, int p_layer_index,
    const CoordinateTransform& p_coordinate_transform)
{
    const int current_clearance = java_round(p_coordinate_transform.dsn_to_board(p_clearance_rule.value));
    const std::string first_class_name = p_first_class->get_name();
    int first_class_no = p_clearance_matrix.get_no(first_class_name);
    if (first_class_no < 0)
    {
        p_clearance_matrix.append_class(first_class_name);
        first_class_no = p_clearance_matrix.get_no(first_class_name);
    }
    const std::string second_class_name = p_second_class->get_name();
    int second_class_no = p_clearance_matrix.get_no(second_class_name);
    if (second_class_no < 0)
    {
        p_clearance_matrix.append_class(second_class_name);
        second_class_no = p_clearance_matrix.get_no(second_class_name);
    }
    if (p_clearance_rule.clearance_class_pairs.empty())
    {
        if (p_layer_index < 0)
        {
            p_clearance_matrix.set_value(first_class_no, second_class_no, current_clearance);
            p_clearance_matrix.set_value(second_class_no, first_class_no, current_clearance);
        }
        else
        {
            p_clearance_matrix.set_value(first_class_no, second_class_no, p_layer_index, current_clearance);
            p_clearance_matrix.set_value(second_class_no, first_class_no, p_layer_index, current_clearance);
        }
        return;
    }
    for (const std::string& current_string : p_clearance_rule.clearance_class_pairs)
    {
        const std::vector<std::string> current_pair = java_split_underscore(current_string);
        if (current_pair.size() != 2)
        {
            continue;
        }
        for (int i = 0; i < 2; ++i)
        {
            int current_first_class_no = 0;
            int current_second_class_no = 0;
            if (i == 0)
            {
                current_first_class_no = get_clearance_class(p_clearance_matrix, p_first_class, current_pair[0]);
                current_second_class_no = get_clearance_class(p_clearance_matrix, p_second_class, current_pair[1]);
            }
            else
            {
                current_first_class_no = get_clearance_class(p_clearance_matrix, p_second_class, current_pair[0]);
                current_second_class_no = get_clearance_class(p_clearance_matrix, p_first_class, current_pair[1]);
            }
            if (p_layer_index < 0)
            {
                p_clearance_matrix.set_value(current_first_class_no, current_second_class_no, current_clearance);
                p_clearance_matrix.set_value(current_second_class_no, current_first_class_no, current_clearance);
            }
            else
            {
                p_clearance_matrix.set_value(
                    current_first_class_no, current_second_class_no, p_layer_index, current_clearance);
                p_clearance_matrix.set_value(
                    current_second_class_no, current_first_class_no, p_layer_index, current_clearance);
            }
        }
    }
}

void Network::create_default_clearance_classes(rules::NetClass* p_net_class, rules::ClearanceMatrix& p_clearance_matrix)
{
    get_clearance_class(p_clearance_matrix, p_net_class, "via");
    get_clearance_class(p_clearance_matrix, p_net_class, "smd");
    get_clearance_class(p_clearance_matrix, p_net_class, "pin");
    get_clearance_class(p_clearance_matrix, p_net_class, "area");
}

void Network::create_via_rule(const std::vector<std::string>& p_use_via, rules::NetClass* p_net_class,
    board::RoutingBoard& p_board, bool /*p_attach_allowed*/)
{
    auto new_via_rule = std::make_unique<rules::ViaRule>(p_net_class->get_name());
    const int default_via_cl_class = p_net_class->get_default_item_clearance_classes().get(rules::ItemClass::VIA);
    rules::ViaInfos& via_infos = p_board.mutable_rules().get_via_infos();
    for (const std::string& current_via_name : p_use_via)
    {
        for (int i = 0; i < via_infos.count(); ++i)
        {
            std::shared_ptr<rules::ViaInfo> current_via_info = via_infos.get(i);
            if (current_via_info->get_clearance_class() == default_via_cl_class
                && current_via_info->get_padstack()->get_name() == current_via_name)
            {
                new_via_rule->append_via(current_via_info);
            }
        }
    }
    rules::ViaRule* added_rule = p_board.mutable_rules().add_via_rule(std::move(new_via_rule));
    p_net_class->set_via_rule(added_rule);
}

void Network::create_active_trace_layers(const std::vector<std::string>& p_use_layer,
    const LayerStructure& p_layer_structure, rules::NetClass* p_net_class)
{
    for (int i = 0; i < static_cast<int>(p_layer_structure.layers.size()); ++i)
    {
        p_net_class->set_active_routing_layer(i, false);
    }
    for (const std::string& current_layer_name : p_use_layer)
    {
        p_net_class->set_active_routing_layer(p_layer_structure.get_no(current_layer_name), true);
    }
    for (int i = 0; i < static_cast<int>(p_layer_structure.layers.size()); ++i)
    {
        if (!p_net_class->is_active_routing_layer(i))
        {
            p_net_class->set_trace_half_width(i, 0);
        }
    }
}

void Network::add_clearance_rule(rules::ClearanceMatrix& p_clearance_matrix, rules::NetClass* p_net_class,
    const ClearanceRule& p_clearance_rule, int p_layer_index, const CoordinateTransform& p_coordinate_transform)
{
    const int current_clearance = java_round(p_coordinate_transform.dsn_to_board(p_clearance_rule.value));
    const std::string class_name = p_net_class->get_name();
    int class_no = p_clearance_matrix.get_no(class_name);
    if (class_no < 0)
    {
        // class not yet existing, create a new class
        p_clearance_matrix.append_class(class_name);
        class_no = p_clearance_matrix.get_no(class_name);
        // set the clearance values of the new class to the maximum of currentClearance and the
        // existing values.
        for (int i = 1; i < p_clearance_matrix.get_class_count(); ++i)
        {
            for (int j = 0; j < p_clearance_matrix.get_layer_count(); ++j)
            {
                const int current_value = std::max(p_clearance_matrix.get_value(class_no, i, j, false), current_clearance);
                p_clearance_matrix.set_value(class_no, i, j, current_value);
                p_clearance_matrix.set_value(i, class_no, j, current_value);
            }
        }
        p_net_class->get_default_item_clearance_classes().set_all(class_no);
    }
    p_net_class->set_trace_clearance_class(class_no);
    if (p_clearance_rule.clearance_class_pairs.empty())
    {
        if (p_layer_index < 0)
        {
            p_clearance_matrix.set_value(class_no, class_no, current_clearance);
        }
        else
        {
            p_clearance_matrix.set_value(class_no, class_no, p_layer_index, current_clearance);
        }
        return;
    }
    if (Structure::contains_wire_clearance_pair(p_clearance_rule.clearance_class_pairs))
    {
        create_default_clearance_classes(p_net_class, p_clearance_matrix);
    }
    for (const std::string& current_string : p_clearance_rule.clearance_class_pairs)
    {
        const std::vector<std::string> current_pair = java_split_underscore(current_string);
        if (current_pair.size() != 2)
        {
            continue;
        }
        const int first_class_no = get_clearance_class(p_clearance_matrix, p_net_class, current_pair[0]);
        const int second_class_no = get_clearance_class(p_clearance_matrix, p_net_class, current_pair[1]);
        if (p_layer_index < 0)
        {
            p_clearance_matrix.set_value(first_class_no, second_class_no, current_clearance);
            p_clearance_matrix.set_value(second_class_no, first_class_no, current_clearance);
        }
        else
        {
            p_clearance_matrix.set_value(first_class_no, second_class_no, p_layer_index, current_clearance);
            p_clearance_matrix.set_value(second_class_no, first_class_no, p_layer_index, current_clearance);
        }
    }
}

int Network::get_clearance_class(rules::ClearanceMatrix& p_clearance_matrix, rules::NetClass* p_net_class,
    const std::string& p_item_class_name)
{
    const std::string net_class_name = p_net_class->get_name();
    std::string new_class_name = net_class_name;
    if (p_item_class_name != "wire")
    {
        new_class_name = new_class_name + DsnFile::kClassClearanceSeparator + p_item_class_name;
    }
    const int found_class_no = p_clearance_matrix.get_no(new_class_name);
    if (found_class_no >= 0)
    {
        return found_class_no;
    }
    p_clearance_matrix.append_class(new_class_name);
    const int result = p_clearance_matrix.get_no(new_class_name);
    const int net_class_no = p_clearance_matrix.get_no(net_class_name);
    if (net_class_no < 0 || result < 0)
    {
        logger::FRLogger::instance().warn(
            "Network.get_clearance_class: clearance class not found at '" + net_class_name + "'");
        return result;
    }
    // initialize the clearance values of newClassName from netClassName
    for (int i = 1; i < p_clearance_matrix.get_class_count(); ++i)
    {
        for (int j = 0; j < p_clearance_matrix.get_layer_count(); ++j)
        {
            const int current_value = p_clearance_matrix.get_value(net_class_no, i, j, false);
            p_clearance_matrix.set_value(result, i, j, current_value);
            p_clearance_matrix.set_value(i, result, j, current_value);
        }
    }
    if (p_item_class_name == "via")
    {
        p_net_class->get_default_item_clearance_classes().set(rules::ItemClass::VIA, result);
    }
    else if (p_item_class_name == "pin")
    {
        p_net_class->get_default_item_clearance_classes().set(rules::ItemClass::PIN, result);
    }
    else if (p_item_class_name == "smd")
    {
        p_net_class->get_default_item_clearance_classes().set(rules::ItemClass::SMD, result);
    }
    else if (p_item_class_name == "area")
    {
        p_net_class->get_default_item_clearance_classes().set(rules::ItemClass::AREA, result);
    }
    return result;
}

bool Network::read_scope(ReadScopeParameter& p_par) const
{
    auto& log = logger::FRLogger::instance();
    std::vector<NetClass> classes;
    std::vector<NetClass::ClassClass> class_class_list;
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
                std::optional<NetClass> current_class = NetClass::read_scope(p_par.scanner);
                if (!current_class.has_value())
                {
                    return false;
                }
                classes.push_back(std::move(*current_class));
            }
            else if (next_token.is(Keyword::CLASS_CLASS))
            {
                std::optional<NetClass::ClassClass> current_class_class = NetClass::read_class_class_scope(p_par.scanner);
                if (!current_class_class.has_value())
                {
                    return false;
                }
                class_class_list.push_back(std::move(*current_class_class));
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

    // Add any vias defined in the net classes to the list of vias to be instantiated. Java quirk
    // kept: viaPadstackNames aliases the useVia collection of the first class, so the useVia of the
    // following classes is appended to the first class's useVia as well.
    std::vector<std::string>* aliased_use_via = nullptr;
    for (NetClass& current_class : classes)
    {
        if (p_par.via_padstack_names.has_value())
        {
            p_par.via_padstack_names->insert(
                p_par.via_padstack_names->end(), current_class.use_via.begin(), current_class.use_via.end());
            if (aliased_use_via != nullptr)
            {
                aliased_use_via->insert(aliased_use_via->end(), current_class.use_via.begin(), current_class.use_via.end());
            }
        }
        else
        {
            p_par.via_padstack_names = current_class.use_via;
            aliased_use_via = &current_class.use_via;
        }
    }

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
    insert_net_classes(classes, p_par);
    insert_class_pairs(class_class_list, p_par);
    // T115: insertComponents and insertLogicalParts.
    return true;
}

} // namespace freerouting::designforms::specctra::parser
