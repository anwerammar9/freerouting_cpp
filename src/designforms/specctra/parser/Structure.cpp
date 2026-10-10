#include "designforms/specctra/parser/Structure.h"

#include "designforms/specctra/parser/AutorouteSettings.h"
#include "designforms/specctra/parser/DsnFile.h"
#include "designforms/specctra/parser/Keyword.h"
#include "designforms/specctra/parser/Path.h"
#include "designforms/specctra/parser/ReadScopeParameter.h"
#include "designforms/specctra/parser/SpecctraDsnStreamReader.h"
#include "logger/FRLogger.h"

namespace freerouting::designforms::specctra::parser {

namespace {

void warn(const std::string& p_message) { logger::FRLogger::instance().warn(p_message); }

} // namespace

bool Structure::read_scope(ReadScopeParameter& p_par) const
{
    auto& log = logger::FRLogger::instance();
    BoardConstructionInfo info;
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
            else
            {
                // flip_style: T112; Java skips unknown scopes.
                ScopeKeyword::skip_scope(p_par.scanner);
            }
        }
        if (!read_ok)
        {
            return false;
        }
    }
    // Java creates the board from the construction info and inserts the
    // keepouts, planes and missing power planes here; that part of read_scope
    // follows with createBoard (T109).
    return true;
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

} // namespace freerouting::designforms::specctra::parser