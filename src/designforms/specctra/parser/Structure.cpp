#include "designforms/specctra/parser/Structure.h"

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
            else
            {
                // keepout / via_keepout / via list / snap_angle / control:
                // T108, autoroute_settings and create_board: T109
                ScopeKeyword::skip_scope(p_par.scanner);
            }
        }
        if (!read_ok)
        {
            return false;
        }
    }
    // Java creates the board from the construction info and inserts the
    // keepouts and planes here; that part of read_scope follows with createBoard (T109).
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

} // namespace freerouting::designforms::specctra::parser