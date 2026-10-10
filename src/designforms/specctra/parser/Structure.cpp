#include "designforms/specctra/parser/Structure.h"

#include <cmath>

#include <cctype>

#include <algorithm>

#include "designforms/specctra/parser/AutorouteSettings.h"
#include "designforms/specctra/parser/Component.h"
#include "designforms/specctra/parser/DsnFile.h"
#include "designforms/specctra/parser/Keyword.h"
#include "designforms/specctra/parser/Path.h"
#include "designforms/specctra/parser/ReadScopeParameter.h"
#include "designforms/specctra/parser/SpecctraDsnStreamReader.h"
#include "logger/FRLogger.h"
#include "rules/Net.h"
#include "geometry/planar/PolylineArea.h"
#include "geometry/planar/PolygonShape.h"
#include "geometry/planar/Limits.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/ConvexTileShape.h"
#include "designforms/specctra/parser/Rectangle.h"
#include "board/RoutingBoard.h"
#include "board/ConductionArea.h"
#include "rules/DefaultItemClearanceClasses.h"
#include "rules/NetClass.h"

namespace freerouting::designforms::specctra::parser {

namespace {

void warn(const std::string& p_message) { logger::FRLogger::instance().warn(p_message); }

/** Java (int) Math.round(double): floor(x + 0.5). */
int java_round(double p_value) { return static_cast<int>(std::floor(p_value + 0.5)); }

/** Java String.split(p_separator, 2) for a literal separator: trailing empty strings are kept. */
std::vector<std::string> split_once(const std::string& p_value, const std::string& p_separator)
{
    const size_t pos = p_value.find(p_separator);
    if (pos == std::string::npos || p_separator.empty())
    {
        return {p_value};
    }
    return {p_value.substr(0, pos), p_value.substr(pos + p_separator.size())};
}

bool starts_with(const std::string& p_value, const std::string& p_prefix)
{
    return p_value.compare(0, p_prefix.size(), p_prefix) == 0;
}

bool ends_with(const std::string& p_value, const std::string& p_suffix)
{
    return p_value.size() >= p_suffix.size()
        && p_value.compare(p_value.size() - p_suffix.size(), p_suffix.size(), p_suffix) == 0;
}

/** Java equalsIgnoreCase for ASCII names. */
bool equals_ignore_case(const std::string& p_a, const std::string& p_b)
{
    if (p_a.size() != p_b.size())
    {
        return false;
    }
    for (size_t i = 0; i < p_a.size(); ++i)
    {
        if (std::tolower(static_cast<unsigned char>(p_a[i])) != std::tolower(static_cast<unsigned char>(p_b[i])))
        {
            return false;
        }
    }
    return true;
}

std::string remove_quotes(std::string p_value)
{
    p_value.erase(std::remove(p_value.begin(), p_value.end(), '"'), p_value.end());
    return p_value;
}

} // namespace

void Structure::update_board_rules(
    const ReadScopeParameter& p_par, const BoardConstructionInfo& p_info, rules::BoardRules& p_board_rules)
{
    const CoordinateTransform& transform = *p_par.coordinate_transform;
    bool smd_to_turn_gap_found = false;
    // update the clearance matrix
    for (const std::shared_ptr<Rule>& current_object : p_info.default_rules)
    {
        if (const auto* current_rule = dynamic_cast<const ClearanceRule*>(current_object.get()))
        {
            if (set_clearance_rule(*current_rule, -1, transform, p_board_rules, p_par.string_quote))
            {
                smd_to_turn_gap_found = true;
            }
        }
    }
    // update width rules
    for (const std::shared_ptr<Rule>& current_object : p_info.default_rules)
    {
        if (const auto* rule = dynamic_cast<const WidthRule*>(current_object.get()))
        {
            const int trace_halfwidth = java_round(transform.dsn_to_board(rule->value) / 2);
            p_board_rules.set_default_trace_half_widths(trace_halfwidth);
        }
    }
    for (const LayerRule& layer_rule : p_info.layer_dependent_rules)
    {
        const int layer_index = p_par.layer_structure->get_no(layer_rule.layer_name);
        if (layer_index < 0)
        {
            continue;
        }
        for (const std::shared_ptr<Rule>& current_object : layer_rule.rules)
        {
            if (const auto* rule = dynamic_cast<const WidthRule*>(current_object.get()))
            {
                const int trace_halfwidth = java_round(transform.dsn_to_board(rule->value) / 2);
                p_board_rules.set_default_trace_half_width(layer_index, trace_halfwidth);
            }
            else if (const auto* current_rule = dynamic_cast<const ClearanceRule*>(current_object.get()))
            {
                set_clearance_rule(*current_rule, layer_index, transform, p_board_rules, p_par.string_quote);
            }
        }
    }
    if (!smd_to_turn_gap_found)
    {
        p_board_rules.set_pin_edge_to_turn_dist(p_board_rules.get_min_trace_half_width());
    }
}

bool Structure::set_clearance_rule(const ClearanceRule& p_rule, int p_layer, const CoordinateTransform& p_transform,
    rules::BoardRules& p_board_rules, const std::string& p_string_quote)
{
    bool result = false;
    rules::ClearanceMatrix& matrix = p_board_rules.get_clearance_matrix();
    const int current_clearance = java_round(p_transform.dsn_to_board(p_rule.value));
    if (p_rule.clearance_class_pairs.empty())
    {
        if (p_layer < 0)
        {
            matrix.set_default_value(current_clearance);
        }
        else
        {
            matrix.set_default_value(p_layer, current_clearance);
        }
        return result;
    }
    if (contains_wire_clearance_pair(p_rule.clearance_class_pairs))
    {
        create_default_clearance_classes(p_board_rules);
    }

    for (std::string current_string : p_rule.clearance_class_pairs)
    {
        if (equals_ignore_case(current_string, "smd_to_turn_gap"))
        {
            p_board_rules.set_pin_edge_to_turn_dist(current_clearance);
            result = true;
            continue;
        }
        std::vector<std::string> current_pair(2);
        if (p_rule.clearance_class_pairs.size() == 2)
        {
            current_pair[0] = p_rule.clearance_class_pairs[0];
            current_pair[1] = p_rule.clearance_class_pairs[1];
            for (int i = 0; i < 2; ++i)
            {
                current_pair[i] = remove_quotes(current_pair[i]);
                // Java parity: the leading '_' of the second name is checked inside this loop,
                // i.e. twice (once before and once after its quotes are removed).
                if (starts_with(current_pair[1], "_"))
                {
                    current_pair[1] = current_pair[1].substr(1);
                }
            }
        }
        else if (!p_string_quote.empty() && starts_with(current_string, p_string_quote))
        {
            // split at the second occurrence of the string quote
            current_string = current_string.substr(p_string_quote.size());
            current_pair = split_once(current_string, p_string_quote);
            if (current_pair.size() != 2 || !starts_with(current_pair[1], "_"))
            {
                warn("Structure.set_clearance_rule: '_' expected at '" + current_string + "'");
                warn("You probably get this error because your clearance rule name has spaces or "
                     "special characters in its name. Please change them first, and try again.");
                continue;
            }
            current_pair[1] = current_pair[1].substr(1);
        }
        else
        {
            current_pair = split_once(current_string, "_");
            if (current_pair.size() != 2)
            {
                // pairs with more than 1 underline like smd_via_same_net are not implemented
                continue;
            }
        }

        int first_class_no = current_pair[0] == "wire" ? 1 : matrix.get_no(current_pair[0]); // 1 = default class
        if (first_class_no < 0)
        {
            first_class_no = append_clearance_class(p_board_rules, current_pair[0]);
        }
        int second_class_no = current_pair[1] == "wire" ? 1 : matrix.get_no(current_pair[1]);
        if (second_class_no < 0)
        {
            second_class_no = append_clearance_class(p_board_rules, current_pair[1]);
        }
        if (p_layer < 0)
        {
            matrix.set_value(first_class_no, second_class_no, current_clearance);
            matrix.set_value(second_class_no, first_class_no, current_clearance);
        }
        else
        {
            matrix.set_value(first_class_no, second_class_no, p_layer, current_clearance);
            matrix.set_value(second_class_no, first_class_no, p_layer, current_clearance);
        }
    }
    return result;
}

bool Structure::contains_wire_clearance_pair(const std::vector<std::string>& p_clearance_pairs)
{
    for (const std::string& current_pair : p_clearance_pairs)
    {
        if (starts_with(current_pair, "wire_") || ends_with(current_pair, "_wire"))
        {
            return true;
        }
    }
    return false;
}

void Structure::create_default_clearance_classes(rules::BoardRules& p_board_rules)
{
    append_clearance_class(p_board_rules, "via");
    append_clearance_class(p_board_rules, "smd");
    append_clearance_class(p_board_rules, "pin");
    append_clearance_class(p_board_rules, "area");
}

int Structure::append_clearance_class(rules::BoardRules& p_board_rules, const std::string& p_name)
{
    rules::ClearanceMatrix& matrix = p_board_rules.get_clearance_matrix();
    matrix.append_class(p_name);
    const int result = matrix.get_no(p_name);
    rules::DefaultItemClearanceClasses& defaults =
        p_board_rules.get_default_net_class()->get_default_item_clearance_classes();
    if (p_name == "via")
    {
        defaults.set(rules::ItemClass::VIA, result);
    }
    else if (p_name == "pin")
    {
        defaults.set(rules::ItemClass::PIN, result);
    }
    else if (p_name == "smd")
    {
        defaults.set(rules::ItemClass::SMD, result);
    }
    else if (p_name == "area")
    {
        defaults.set(rules::ItemClass::AREA, result);
    }
    // other names: unsupported item classes are ignored (Java switch default)
    return result;
}

bool Structure::clearance_equals(const rules::ClearanceMatrix& p_matrix, int p_layer_1, int p_layer_2)
{
    if (p_layer_1 == p_layer_2)
    {
        return true;
    }
    for (int i = 1; i < p_matrix.get_class_count(); ++i)
    {
        for (int j = i; j < p_matrix.get_class_count(); ++j)
        {
            if (p_matrix.get_value(i, j, p_layer_1, false) != p_matrix.get_value(i, j, p_layer_2, false))
            {
                return false;
            }
        }
    }
    return true;
}

bool Structure::read_scope(ReadScopeParameter& p_par) const
{
    auto& log = logger::FRLogger::instance();
    BoardConstructionInfo info;
    // If true, components on the back side are rotated before mirroring (Java flipStyleRotateFirst).
    bool flip_style_rotate_first = false;
    Token next_token;
    for (;;)
    {
        const Token prev_token = next_token;
        next_token = p_par.scanner.next_token();
        if (next_token.is_eof())
        {
            log.warn("Structure.read_scope: unexpected end of file at '" + p_par.scanner.get_scope_identifier() + "'");
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
            // a new scope inside the structure scope is expected
            if (next_token.is(Keyword::BOUNDARY))
            {
                read_boundary_scope(p_par.scanner, info);
            }
            else if (next_token.is(Keyword::LAYER))
            {
                read_ok = read_layer_scope(p_par.scanner, info, p_par.string_quote);
                if (p_par.layer_structure != nullptr)
                {
                    // correct the layer structure because another layer was read
                    p_par.layer_structure = std::make_shared<LayerStructure>(info.layer_info);
                }
            }
            else if (next_token.is(Keyword::PLANE_SCOPE))
            {
                if (p_par.layer_structure == nullptr)
                {
                    p_par.layer_structure = std::make_shared<LayerStructure>(info.layer_info);
                }
                Keyword::PLANE_SCOPE->read_scope(p_par);
            }
            else if (next_token.is(Keyword::RULE))
            {
                std::optional<std::vector<std::shared_ptr<Rule>>> rules = Rule::read_scope(p_par.scanner);
                if (rules)
                {
                    info.default_rules.insert(info.default_rules.end(), rules->begin(), rules->end());
                }
            }
            else if (next_token.is(Keyword::VIA))
            {
                p_par.via_padstack_names = read_via_padstacks(p_par.scanner);
            }
            else if (next_token.is(Keyword::KEEPOUT))
            {
                if (p_par.layer_structure == nullptr)
                {
                    p_par.layer_structure = std::make_shared<LayerStructure>(info.layer_info);
                }
                std::optional<Shape::ReadAreaScopeResult> area =
                    Shape::read_area_scope(p_par.scanner, p_par.layer_structure.get(), false);
                if (area)
                {
                    // Java adds null areas to the list and crashes on insertion; skip them instead.
                    p_par.keepout_list.push_back(std::move(*area));
                }
            }
            else if (next_token.is(Keyword::VIA_KEEPOUT))
            {
                if (p_par.layer_structure == nullptr)
                {
                    p_par.layer_structure = std::make_shared<LayerStructure>(info.layer_info);
                }
                std::optional<Shape::ReadAreaScopeResult> area =
                    Shape::read_area_scope(p_par.scanner, p_par.layer_structure.get(), false);
                if (area)
                {
                    // See keepout branch above.
                    p_par.via_keepout_list.push_back(std::move(*area));
                }
            }
            else if (next_token.is(Keyword::PLACE_KEEPOUT))
            {
                if (p_par.layer_structure == nullptr)
                {
                    p_par.layer_structure = std::make_shared<LayerStructure>(info.layer_info);
                }
                std::optional<Shape::ReadAreaScopeResult> area =
                    Shape::read_area_scope(p_par.scanner, p_par.layer_structure.get(), false);
                if (area)
                {
                    // See keepout branch above.
                    p_par.place_keepout_list.push_back(std::move(*area));
                }
            }
            else if (next_token.is(Keyword::CONTROL))
            {
                read_ok = read_control_scope(p_par);
            }
            else if (next_token.is(Keyword::SNAP_ANGLE))
            {
                std::optional<board::AngleRestriction> snap_angle = read_snap_angle(p_par.scanner);
                if (snap_angle)
                {
                    p_par.snap_angle = *snap_angle;
                }
            }
            else if (next_token.is(Keyword::AUTOROUTE_SETTINGS))
            {
                // Java parity: the scope is only read while no layer structure exists yet
                // (i.e. before the first keepout/plane scope); afterwards it is not consumed here.
                if (p_par.layer_structure == nullptr)
                {
                    p_par.layer_structure = std::make_shared<LayerStructure>(info.layer_info);
                    p_par.autoroute_settings =
                        AutorouteSettings::read_scope(p_par.scanner, *p_par.layer_structure);
                }
            }
            else if (next_token.is(Keyword::FLIP_STYLE))
            {
                // The correct location is the place_control scope, but Electra writes it here (Java).
                flip_style_rotate_first = PlaceControl::read_flip_style_rotate_first(p_par.scanner);
            }
            else
            {
                ScopeKeyword::skip_scope(p_par.scanner);
            }
        }
        if (!read_ok)
        {
            return false;
        }
    }
    // Create the board from the data read (Java: "TODO: move this method somewhere outside").
    bool result = true;
    if (p_par.get_board() == nullptr)
    {
        result = create_board(p_par, info);
    }
    board::RoutingBoard* board = p_par.get_board();
    if (board == nullptr)
    {
        return false;
    }
    if (flip_style_rotate_first)
    {
        board->set_flip_style_rotate_first(true);
    }

    // insert the keepouts
    for (const Shape::ReadAreaScopeResult& current_area : p_par.keepout_list)
    {
        if (!insert_keepout(current_area, p_par, KeepoutType::keepout, board::FixedState::SYSTEM_FIXED))
        {
            return false;
        }
    }
    for (const Shape::ReadAreaScopeResult& current_area : p_par.via_keepout_list)
    {
        if (!insert_keepout(current_area, p_par, KeepoutType::via_keepout, board::FixedState::SYSTEM_FIXED))
        {
            return false;
        }
    }
    for (const Shape::ReadAreaScopeResult& current_area : p_par.place_keepout_list)
    {
        if (!insert_keepout(current_area, p_par, KeepoutType::place_keepout, board::FixedState::SYSTEM_FIXED))
        {
            return false;
        }
    }

    // insert the planes
    for (const ReadScopeParameter::PlaneInfo& plane_info : p_par.plane_list)
    {
        const Net::Id net_id{plane_info.net_name, 1};
        if (!p_par.netlist.contains(net_id))
        {
            if (Net* new_net = p_par.netlist.add_net(net_id))
            {
                board->mutable_rules().get_nets().add(new_net->id.name, new_net->id.subnet_number, true);
            }
        }
        rules::Net* current_net = board->mutable_rules().get_nets().get(plane_info.net_name, 1);
        if (current_net == nullptr)
        {
            log.warn("Plane.read_scope: net not found at '" + p_par.scanner.get_scope_identifier() + "'");
            continue;
        }
        std::shared_ptr<geometry::planar::Area> plane_area =
            to_board_area(Shape::transform_area_to_board(plane_info.area.shape_list, *p_par.coordinate_transform));
        const std::shared_ptr<Layer>& current_layer = plane_info.area.shape_list.front()->layer;
        if (current_layer->no < 0)
        {
            log.warn("Plane.read_scope: unexpected layer name at '" + p_par.scanner.get_scope_identifier() + "'");
            return false;
        }
        int clearance_class_index;
        if (!plane_info.area.clearance_class_name.empty())
        {
            clearance_class_index = board->get_rules().get_clearance_matrix().get_no(plane_info.area.clearance_class_name);
            if (clearance_class_index < 0)
            {
                log.warn("Structure.read_scope: clearance class not found at '" + p_par.scanner.get_scope_identifier()
                    + "'");
                clearance_class_index = rules::BoardRules::clearance_class_none();
            }
        }
        else
        {
            clearance_class_index =
                current_net->get_net_class()->get_default_item_clearance_classes().get(rules::ItemClass::AREA);
        }
        board->insert_conduction_area(plane_area, current_layer->no, {current_net->get_net_no()}, clearance_class_index,
            false, board::FixedState::SYSTEM_FIXED);
    }
    insert_missing_power_planes(info.layer_info, p_par.netlist, *board);

    // Java applies the DSN autoroute settings to the current routing job here. Without a job
    // (MinimalBoardManager) Java skips it; the job-backed path comes with HeadlessBoardManager (T124a).
    return result;
}

bool Structure::read_layer_scope(
    IJFlexScanner& p_scanner, BoardConstructionInfo& p_info, const std::string& p_string_quote)
{
    (void)p_string_quote;
    auto& log = logger::FRLogger::instance();
    bool layer_ok = true;
    bool is_signal = true;

    const std::string layer_string = p_scanner.next_string();

    std::vector<std::string> net_names;
    Token next_token = p_scanner.next_token();
    while (!next_token.is(Keyword::CLOSED_BRACKET))
    {
        if (!next_token.is(Keyword::OPEN_BRACKET))
        {
            log.warn("Structure.read_layer_scope: ( expected at '" + p_scanner.get_scope_identifier() + "'");
            return false;
        }
        next_token = p_scanner.next_token();
        if (next_token.is(Keyword::TYPE))
        {
            next_token = p_scanner.next_token();
            if (next_token.is(Keyword::POWER))
            {
                is_signal = false;
            }
            else if (!next_token.is(Keyword::SIGNAL)
                && next_token.to_string() != Keyword::JUMPER->get_name())
            {
                if (next_token.is_string())
                {
                    log.error("Structure.read_layer_scope: the layer '" + layer_string
                        + "' has an unknown layer type '" + next_token.to_string() + "'");
                }
                else
                {
                    log.warn("Structure.read_layer_scope: the layer '" + layer_string
                        + "' has an unknown layer type at '" + p_scanner.get_scope_identifier() + "'");
                }
                layer_ok = false;
            }
            next_token = p_scanner.next_token();
            if (!next_token.is(Keyword::CLOSED_BRACKET))
            {
                log.warn("Structure.read_layer_scope: ) expected at '" + p_scanner.get_scope_identifier() + "'");
                return false;
            }
        }
        else if (next_token.is(Keyword::RULE))
        {
            std::optional<std::vector<std::shared_ptr<Rule>>> current_rules = Rule::read_scope(p_scanner);
            if (current_rules)
            {
                p_info.layer_dependent_rules.push_back(LayerRule{ layer_string, *current_rules });
            }
        }
        else if (next_token.is(Keyword::USE_NET))
        {
            for (;;)
            {
                p_scanner.yybegin(SpecctraDsnStreamReader::NAME);
                next_token = p_scanner.next_token();
                if (next_token.is(Keyword::CLOSED_BRACKET))
                {
                    break;
                }
                if (next_token.is_string())
                {
                    net_names.push_back(next_token.string_value);
                }
                else
                {
                    log.warn("Structure.read_layer_scope: string expected at '" + p_scanner.get_scope_identifier() + "'");
                }
            }
        }
        else
        {
            ScopeKeyword::skip_scope(p_scanner);
        }
        next_token = p_scanner.next_token();
    }
    if (layer_ok)
    {
        p_info.layer_info.push_back(std::make_shared<Layer>(
            layer_string, p_info.found_layer_count, is_signal, std::move(net_names)));
        ++p_info.found_layer_count;
    }
    return true;
}

bool Structure::read_boundary_scope(IJFlexScanner& p_scanner, BoardConstructionInfo& p_info)
{
    auto& log = logger::FRLogger::instance();
    std::unique_ptr<Shape> current_shape = Shape::read_scope(p_scanner, nullptr);
    Token prev_token;
    for (;;)
    {
        const Token next_token = p_scanner.next_token();
        if (next_token.is_eof())
        {
            // Java would loop forever on null tokens; stop at the end of the file instead.
            return false;
        }
        if (next_token.is(Keyword::CLOSED_BRACKET))
        {
            break;
        }
        if (prev_token.is(Keyword::OPEN_BRACKET))
        {
            if (next_token.is(Keyword::CLEARANCE_CLASS))
            {
                std::optional<std::string> clearance_name = DsnFile::read_string_scope(p_scanner);
                if (clearance_name)
                {
                    p_info.outline_clearance_class_name = *clearance_name;
                }
            }
            else
            {
                add_boundary_shape(p_info, Shape::read_scope_from_keyword(p_scanner, next_token, nullptr));
            }
        }
        prev_token = next_token;
    }
    if (current_shape == nullptr)
    {
        log.warn("Structure.read_boundary_scope: shape is null at '" + p_scanner.get_scope_identifier() + "'");
        return true;
    }
    add_boundary_shape(p_info, std::move(current_shape));
    return true;
}

void Structure::add_boundary_shape(BoardConstructionInfo& p_info, std::unique_ptr<Shape> p_shape)
{
    if (p_shape == nullptr)
    {
        return;
    }
    if (dynamic_cast<PolylinePath*>(p_shape.get()) || dynamic_cast<PolygonPath*>(p_shape.get()))
    {
        p_info.outline_shapes.push_back(std::move(p_shape));
        return;
    }
    if (p_shape->layer == Layer::pcb())
    {
        if (p_info.bounding_shape == nullptr)
        {
            p_info.bounding_shape = std::move(p_shape);
        }
        else
        {
            p_info.outline_shapes.push_back(std::move(p_shape));
        }
    }
    else if (p_shape->layer == Layer::signal())
    {
        p_info.outline_shapes.push_back(std::move(p_shape));
    }
    else
    {
        warn("Structure.add_boundary_shape: unexpected layer at boundary");
    }
}

std::optional<std::vector<std::string>> Structure::read_via_padstacks(IJFlexScanner& p_scanner)
{
    auto& log = logger::FRLogger::instance();
    std::vector<std::string> normal_vias;
    std::vector<std::string> spare_vias;
    for (;;)
    {
        const Token next_token = p_scanner.next_token();
        if (next_token.is_eof())
        {
            // Java catches the scanner IOException and returns null.
            return std::nullopt;
        }
        if (next_token.is(Keyword::CLOSED_BRACKET))
        {
            break;
        }
        if (next_token.is(Keyword::OPEN_BRACKET))
        {
            const Token after_open = p_scanner.next_token();
            if (after_open.is(Keyword::SPARE))
            {
                std::optional<std::vector<std::string>> spares = read_via_padstacks(p_scanner);
                if (!spares)
                {
                    return std::nullopt;
                }
                spare_vias = std::move(*spares);
            }
            else
            {
                ScopeKeyword::skip_scope(p_scanner);
            }
        }
        else if (next_token.is_string())
        {
            normal_vias.push_back(next_token.string_value);
        }
        else
        {
            log.warn("Structure.read_via_padstack: String expected at '" + p_scanner.get_scope_identifier() + "'");
            return std::nullopt;
        }
    }
    // add the spare vias to the end of the list
    normal_vias.insert(normal_vias.end(), spare_vias.begin(), spare_vias.end());
    return normal_vias;
}

std::optional<board::AngleRestriction> Structure::read_snap_angle(IJFlexScanner& p_scanner)
{
    auto& log = logger::FRLogger::instance();
    board::AngleRestriction snap_angle;
    const Token next_token = p_scanner.next_token();
    if (next_token.is(Keyword::NINETY_DEGREE))
    {
        snap_angle = board::AngleRestriction::NINETY_DEGREE;
    }
    else if (next_token.is(Keyword::FORTYFIVE_DEGREE))
    {
        snap_angle = board::AngleRestriction::FORTYFIVE_DEGREE;
    }
    else if (next_token.is(Keyword::NONE))
    {
        snap_angle = board::AngleRestriction::NONE;
    }
    else
    {
        log.warn("Structure.read_snap_angle_scope: unexpected token at '" + p_scanner.get_scope_identifier() + "'");
        return std::nullopt;
    }
    const Token closing = p_scanner.next_token();
    if (!closing.is(Keyword::CLOSED_BRACKET))
    {
        log.warn("Structure.read_snap_angle_scope: closing bracket expected at '" + p_scanner.get_scope_identifier() + "'");
        return std::nullopt;
    }
    return snap_angle;
}

bool Structure::read_control_scope(ReadScopeParameter& p_par)
{
    auto& log = logger::FRLogger::instance();
    Token next_token;
    for (;;)
    {
        const Token prev_token = next_token;
        next_token = p_par.scanner.next_token();
        if (next_token.is_eof())
        {
            log.warn("Structure.read_control_scope: unexpected end of file at '" + p_par.scanner.get_scope_identifier() + "'");
            return false;
        }
        if (next_token.is(Keyword::CLOSED_BRACKET))
        {
            // end of scope
            break;
        }
        if (prev_token.is(Keyword::OPEN_BRACKET))
        {
            if (next_token.is(Keyword::VIA_AT_SMD))
            {
                p_par.via_at_smd_allowed = DsnFile::read_on_off_scope(p_par.scanner);
            }
            else
            {
                ScopeKeyword::skip_scope(p_par.scanner);
            }
        }
    }
    return true;
}

namespace {

rules::AngleRestriction to_rules_angle_restriction(board::AngleRestriction p_angle)
{
    switch (p_angle)
    {
    case board::AngleRestriction::NINETY_DEGREE:
        return rules::AngleRestriction::RIGHT;
    case board::AngleRestriction::FORTYFIVE_DEGREE:
        return rules::AngleRestriction::FORTY_FIVE;
    default:
        return rules::AngleRestriction::NONE;
    }
}

/** Java Structure.OutlineShape: an outline shape with its bounding box and convex pieces. */
struct OutlineShape
{
    std::shared_ptr<geometry::planar::PolylineShape> shape;
    geometry::planar::IntBox bounding_box;
    bool is_hole = false;

    explicit OutlineShape(std::shared_ptr<geometry::planar::PolylineShape> p_shape)
        : shape(std::move(p_shape)), bounding_box(shape->bounding_box())
    {
    }

    /** True, if this shape contains all corners of p_other. Java containsAllCorners. */
    bool contains_all_corners(const OutlineShape& p_other) const
    {
        const auto& convex_shapes = shape->split_to_convex();
        if (convex_shapes.empty())
        {
            return false; // calculation of the convex shapes failed
        }
        const int corner_count = p_other.shape->border_line_count();
        for (int i = 0; i < corner_count; ++i)
        {
            const geometry::planar::IntPoint current_corner = p_other.shape->corner(i);
            bool is_contained = false;
            for (const auto& convex_shape : convex_shapes)
            {
                if (convex_shape->contains(current_corner))
                {
                    is_contained = true;
                    break;
                }
            }
            if (!is_contained)
            {
                return false;
            }
        }
        return true;
    }
};

void insert_keepout_on_layer(board::RoutingBoard& p_board, const std::shared_ptr<geometry::planar::Area>& p_area,
    int p_layer, const std::string& p_clearance_class_name, Structure::KeepoutType p_keepout_type,
    board::FixedState p_fixed_state)
{
    int clearance_class_index;
    if (p_clearance_class_name.empty())
    {
        clearance_class_index =
            p_board.mutable_rules().get_default_net_class()->get_default_item_clearance_classes().get(rules::ItemClass::AREA);
    }
    else
    {
        clearance_class_index = p_board.get_rules().get_clearance_matrix().get_no(p_clearance_class_name);
        if (clearance_class_index < 0)
        {
            warn("Keepout.insert_keepout: clearance class not found at '" + p_clearance_class_name + "'");
            clearance_class_index = rules::BoardRules::clearance_class_none();
        }
    }
    if (p_keepout_type == Structure::KeepoutType::via_keepout)
    {
        p_board.insert_via_obstacle(p_area, p_layer, clearance_class_index, p_fixed_state);
    }
    else if (p_keepout_type == Structure::KeepoutType::place_keepout)
    {
        p_board.insert_component_obstacle(p_area, p_layer, clearance_class_index, p_fixed_state);
    }
    else
    {
        p_board.insert_obstacle(p_area, p_layer, clearance_class_index, p_fixed_state);
    }
}

} // namespace

std::vector<std::shared_ptr<geometry::planar::PolylineShape>> Structure::separate_holes(
    std::vector<std::shared_ptr<geometry::planar::PolylineShape>>& p_outline_shapes)
{
    std::vector<OutlineShape> shapes;
    shapes.reserve(p_outline_shapes.size());
    for (const auto& outline_shape : p_outline_shapes)
    {
        shapes.emplace_back(outline_shape);
    }
    for (size_t i = 0; i < shapes.size(); ++i)
    {
        OutlineShape& current_shape = shapes[i];
        for (size_t j = 0; j < shapes.size(); ++j)
        {
            // check if shapes[j] may contain shapes[i]
            const OutlineShape& other_shape = shapes[j];
            if (i == j || other_shape.is_hole)
            {
                continue;
            }
            if (!other_shape.bounding_box.contains(current_shape.bounding_box))
            {
                continue;
            }
            current_shape.is_hole = other_shape.contains_all_corners(current_shape);
        }
    }
    std::vector<std::shared_ptr<geometry::planar::PolylineShape>> hole_list;
    for (const OutlineShape& shape : shapes)
    {
        if (shape.is_hole)
        {
            auto it = std::find(p_outline_shapes.begin(), p_outline_shapes.end(), shape.shape);
            if (it != p_outline_shapes.end())
            {
                p_outline_shapes.erase(it);
            }
            hole_list.push_back(shape.shape);
        }
    }
    return hole_list;
}

bool Structure::create_board(ReadScopeParameter& p_par, BoardConstructionInfo& p_info)
{
    const int layer_count = static_cast<int>(p_info.layer_info.size());
    if (layer_count == 0)
    {
        warn("Structure.create_board: layers missing in structure scope at '" + p_par.scanner.get_scope_identifier()
            + "'");
        return false;
    }
    if (p_info.bounding_shape == nullptr)
    {
        // happens if the boundary shape with layer pcb is missing
        if (p_info.outline_shapes.empty())
        {
            warn("Structure.create_board: outline missing at '" + p_par.scanner.get_scope_identifier() + "'");
            p_par.board_outline_ok = false;
            return false;
        }
        std::unique_ptr<Rectangle> bounding_box = p_info.outline_shapes.front()->bounding_box();
        for (size_t i = 1; i < p_info.outline_shapes.size(); ++i)
        {
            bounding_box = std::make_unique<Rectangle>(bounding_box->union_with(*p_info.outline_shapes[i]->bounding_box()));
        }
        p_info.bounding_shape = std::move(bounding_box);
    }
    const std::unique_ptr<Rectangle> bounding_box = p_info.bounding_shape->bounding_box();

    std::vector<std::shared_ptr<board::Layer>> board_layer_arr;
    for (const std::shared_ptr<Layer>& current_layer : p_info.layer_info)
    {
        if (current_layer->no < 0 || current_layer->no >= layer_count)
        {
            warn("Structure.create_board: illegal layer number at '" + p_par.scanner.get_scope_identifier() + "'");
            return false;
        }
        board_layer_arr.push_back(std::make_shared<board::Layer>(current_layer->name, current_layer->is_signal));
    }
    const board::LayerStructure board_layer_structure(board_layer_arr);
    p_par.layer_structure = std::make_shared<LayerStructure>(p_info.layer_info);

    // Approximate scaling between dsn and board coordinates.
    int scale_factor = std::max(p_par.resolution, 1);
    double max_coor = 0;
    for (int i = 0; i < 4; ++i)
    {
        max_coor = std::max(max_coor, std::abs(bounding_box->coor[i] * p_par.resolution));
    }
    if (max_coor == 0)
    {
        p_par.board_outline_ok = false;
        return false;
    }
    // make the scale factor smaller, if there is a danger of integer overflow
    while (5 * max_coor >= geometry::planar::Limits::CRIT_INT)
    {
        scale_factor /= 10;
        max_coor /= 10;
    }
    p_par.coordinate_transform = CoordinateTransform(scale_factor, 0, 0);

    std::unique_ptr<geometry::planar::Shape> board_bounding_box = bounding_box->transform_to_board(*p_par.coordinate_transform);
    const auto* bounding_tile = dynamic_cast<const geometry::planar::ConvexTileShape*>(board_bounding_box.get());
    const auto* bounds_box = bounding_tile != nullptr
        ? dynamic_cast<const geometry::planar::IntBox*>(&bounding_tile->tile())
        : nullptr;
    if (bounds_box == nullptr)
    {
        warn("Structure.create_board: IntBox expected");
        return false;
    }
    const geometry::planar::IntBox bounds = bounds_box->offset(1000);

    std::vector<std::shared_ptr<geometry::planar::PolylineShape>> board_outline_shapes;
    for (const std::shared_ptr<Shape>& outline_shape : p_info.outline_shapes)
    {
        std::shared_ptr<Shape> current_shape = outline_shape;
        if (const auto* current_path = dynamic_cast<const PolygonPath*>(current_shape.get()))
        {
            if (current_path->width != 0)
            {
                // set the width to 0, because the offset function used in transform_to_board is not
                // implemented for shapes which are not convex (Java comment)
                current_shape = std::make_shared<PolygonPath>(current_path->layer, 0, current_path->coordinate_arr);
            }
        }
        std::shared_ptr<geometry::planar::PolylineShape> current_board_shape =
            to_polyline_shape(current_shape->transform_to_board(*p_par.coordinate_transform));
        if (current_board_shape != nullptr && current_board_shape->dimension() > 0)
        {
            board_outline_shapes.push_back(current_board_shape);
        }
    }
    if (board_outline_shapes.empty())
    {
        // construct an outline from the bounding shape, if the outline is missing
        board_outline_shapes.push_back(
            to_polyline_shape(p_info.bounding_shape->transform_to_board(*p_par.coordinate_transform)));
    }
    const std::vector<std::shared_ptr<geometry::planar::PolylineShape>> hole_shapes =
        separate_holes(board_outline_shapes);

    auto board_rules = std::make_unique<rules::BoardRules>(
        board_layer_structure, rules::ClearanceMatrix::get_default_instance(board_layer_structure, 0));
    board::Communication::SpecctraParserInfo parser_info;
    parser_info.string_quote = p_par.string_quote;
    parser_info.host_cad = p_par.host_cad;
    parser_info.host_version = p_par.host_version;
    parser_info.constants = p_par.constants;
    parser_info.write_resolution = p_par.write_resolution;
    parser_info.dsn_file_generated_by_host = p_par.dsn_file_generated_by_host;
    auto board_communication = std::make_unique<board::Communication>(
        p_par.unit, p_par.resolution, std::move(parser_info), scale_factor, 0, 0);
    if (board_communication->host_is_old_kicad())
    {
        warn("Structure.create_board: The DSN file was exported from an old KiCad version "
             "that has known compatibility issues. Please update KiCad to version 6 or newer.");
    }

    update_board_rules(p_par, p_info, *board_rules);
    board_rules->set_trace_angle_restriction(to_rules_angle_restriction(p_par.snap_angle));
    p_par.board_handling->create_board(bounds, board_layer_structure, board_outline_shapes,
        p_info.outline_clearance_class_name, std::move(board_rules), std::move(board_communication));

    board::RoutingBoard* board = p_par.get_board();
    // Insert the holes of the board outline as obstacles.
    for (const auto& outline_hole : hole_shapes)
    {
        for (int i = 0; i < board_layer_structure.layer_count(); ++i)
        {
            board->insert_obstacle(
                std::make_shared<geometry::planar::PolylineArea>(outline_hole), i, 0, board::FixedState::SYSTEM_FIXED);
        }
    }
    return true;
}

bool Structure::insert_keepout(const Shape::ReadAreaScopeResult& p_area, ReadScopeParameter& p_par,
    KeepoutType p_keepout_type, board::FixedState p_fixed_state)
{
    std::shared_ptr<geometry::planar::Area> keepout_area =
        to_board_area(Shape::transform_area_to_board(p_area.shape_list, *p_par.coordinate_transform));
    if (keepout_area == nullptr || keepout_area->dimension() < 2)
    {
        // A degenerate keepout cannot be enforced; the board stays valid without it (Java).
        warn("Keepout zone '" + p_area.area_name
            + "' was skipped because its geometry is degenerate "
              "(e.g. zero-area polygon). This is likely a DSN export issue in your EDA tool. "
              "The board will be routed without this keepout constraint.");
        return true;
    }
    board::RoutingBoard* board = p_par.get_board();
    if (board == nullptr)
    {
        warn("Structure.insert_keepout: board not initialized");
        return false;
    }
    const std::shared_ptr<Layer>& current_layer = p_area.shape_list.front()->layer;
    if (current_layer == Layer::signal())
    {
        for (int i = 0; i < board->get_layer_count(); ++i)
        {
            if (p_par.layer_structure->layers[i]->is_signal)
            {
                insert_keepout_on_layer(
                    *board, keepout_area, i, p_area.clearance_class_name, p_keepout_type, p_fixed_state);
            }
        }
    }
    else if (current_layer->no >= 0)
    {
        insert_keepout_on_layer(
            *board, keepout_area, current_layer->no, p_area.clearance_class_name, p_keepout_type, p_fixed_state);
    }
    else
    {
        warn("Structure.insert_keepout: unknown layer name at '" + p_par.scanner.get_scope_identifier() + "'");
        return false;
    }
    return true;
}

void Structure::insert_missing_power_planes(
    const std::vector<std::shared_ptr<Layer>>& p_layer_info, NetList& p_netlist, board::RoutingBoard& p_board)
{
    std::vector<const board::ConductionArea*> conduction_areas;
    for (const board::Item* item : p_board.get_items())
    {
        if (const auto* area = dynamic_cast<const board::ConductionArea*>(item))
        {
            conduction_areas.push_back(area);
        }
    }
    for (const std::shared_ptr<Layer>& current_layer : p_layer_info)
    {
        if (current_layer->is_signal)
        {
            continue;
        }
        bool conduction_area_found = false;
        for (const board::ConductionArea* area : conduction_areas)
        {
            if (area->get_layer() == current_layer->no)
            {
                conduction_area_found = true;
                break;
            }
        }
        if (conduction_area_found || current_layer->net_names.empty())
        {
            continue;
        }
        const Net::Id current_net_id{current_layer->net_names.front(), 1};
        if (!p_netlist.contains(current_net_id))
        {
            if (Net* new_net = p_netlist.add_net(current_net_id))
            {
                p_board.mutable_rules().get_nets().add(new_net->id.name, new_net->id.subnet_number, true);
            }
        }
        rules::Net* current_net = p_board.mutable_rules().get_nets().get(current_net_id.name, current_net_id.subnet_number);
        if (current_net == nullptr)
        {
            warn("Structure.insert_missing_power_planes: net not found at '" + current_net_id.name + "'");
            continue;
        }
        const geometry::planar::IntBox box = p_board.get_bounding_box();
        p_board.insert_conduction_area(to_board_area(as_board_shape(std::make_unique<geometry::planar::IntBox>(box))),
            current_layer->no, {current_net->get_net_no()}, rules::BoardRules::clearance_class_none(), false,
            board::FixedState::SYSTEM_FIXED);
    }
}

} // namespace freerouting::designforms::specctra::parser