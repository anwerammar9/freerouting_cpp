#include "designforms/specctra/parser/Component.h"

#include "board/RoutingBoard.h"
#include "designforms/specctra/parser/DsnFile.h"
#include "designforms/specctra/parser/IJFlexScanner.h"
#include "designforms/specctra/parser/Keyword.h"
#include "designforms/specctra/parser/ReadScopeParameter.h"
#include "designforms/specctra/parser/SpecctraDsnStreamReader.h"
#include "logger/FRLogger.h"

namespace freerouting::designforms::specctra::parser {

namespace {

void warn(const std::string& p_message) { logger::FRLogger::instance().warn(p_message); }

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

} // namespace

bool Component::read_scope(ReadScopeParameter& p_par) const
{
    std::optional<ComponentPlacement> component_placement = read_component_scope(p_par.scanner);
    if (!component_placement.has_value())
    {
        return false;
    }
    p_par.placement_list.push_back(std::move(*component_placement));
    return true;
}

std::optional<ComponentPlacement> Component::read_component_scope(IJFlexScanner& p_scanner)
{
    Token next_token = p_scanner.next_token();
    if (!next_token.is_string())
    {
        warn("Component.read_scope: component name expected at '" + p_scanner.get_scope_identifier() + "'");
        return std::nullopt;
    }
    ComponentPlacement component_placement(next_token.string_value);
    Token prev_token = next_token;
    next_token = p_scanner.next_token();
    while (!next_token.is(Keyword::CLOSED_BRACKET) && !next_token.is_eof())
    {
        if (prev_token.is(Keyword::OPEN_BRACKET) && next_token.is(Keyword::PLACE))
        {
            std::optional<ComponentPlacement::ComponentLocation> next_location = read_place_scope(p_scanner);
            if (!next_location.has_value())
            {
                return std::nullopt;
            }
            component_placement.locations.push_back(std::move(*next_location));
        }
        prev_token = next_token;
        next_token = p_scanner.next_token();
    }
    return component_placement;
}

std::optional<ComponentPlacement::ComponentLocation> Component::read_place_scope(IJFlexScanner& p_scanner)
{
    ComponentPlacement::ComponentLocation result;
    result.name = p_scanner.next_string(true);
    std::array<double, 2> location{};
    Token next_token;
    for (int i = 0; i < 2; ++i)
    {
        next_token = p_scanner.next_token();
        if (next_token.is_number())
        {
            location[i] = next_token.number();
        }
        else if (next_token.is(Keyword::CLOSED_BRACKET))
        {
            // component is not yet placed
            return result;
        }
        else
        {
            warn("Component.read_place_scope: Double was expected as the second and third "
                 "parameter of the component/place command at '"
                + p_scanner.get_scope_identifier() + "'");
            return std::nullopt;
        }
    }
    result.coor = location;
    next_token = p_scanner.next_token();
    if (next_token.is(Keyword::BACK))
    {
        result.is_front = false;
    }
    else if (!next_token.is(Keyword::FRONT))
    {
        warn("Component.read_place_scope: Keyword.FRONT expected at '" + p_scanner.get_scope_identifier() + "'");
    }
    next_token = p_scanner.next_token();
    if (!next_token.is_number())
    {
        warn("Component.read_place_scope: number expected at '" + p_scanner.get_scope_identifier() + "'");
        return std::nullopt;
    }
    result.rotation = next_token.number();
    next_token = p_scanner.next_token();
    while (next_token.is(Keyword::OPEN_BRACKET))
    {
        next_token = p_scanner.next_token();
        if (next_token.is(Keyword::LOCK_TYPE))
        {
            result.position_fixed = read_lock_type(p_scanner);
        }
        else if (next_token.is(Keyword::PIN) || next_token.is(Keyword::KEEPOUT) || next_token.is(Keyword::VIA_KEEPOUT)
            || next_token.is(Keyword::PLACE_KEEPOUT))
        {
            std::optional<ComponentPlacement::ItemClearanceInfo> info = read_item_clearance_info(p_scanner);
            if (!info.has_value())
            {
                return std::nullopt;
            }
            auto& target = next_token.is(Keyword::PIN) ? result.pin_infos
                : next_token.is(Keyword::KEEPOUT)      ? result.keepout_infos
                : next_token.is(Keyword::VIA_KEEPOUT)  ? result.via_keepout_infos
                                                       : result.place_keepout_infos;
            target[info->name] = *info; // Java Map.put: the last one wins
        }
        else if (next_token.is(Keyword::PN) || (next_token.is_string() && equals_ignore_case(next_token.string_value, "PN")))
        {
            result.part_number = DsnFile::read_string_scope(p_scanner).value_or("");
        }
        else
        {
            skip_scope(p_scanner);
        }
        next_token = p_scanner.next_token();
    }
    if (!next_token.is(Keyword::CLOSED_BRACKET))
    {
        warn("Component.read_place_scope: ) expected at '" + p_scanner.get_scope_identifier() + "'");
        return std::nullopt;
    }
    return result;
}

std::optional<ComponentPlacement::ItemClearanceInfo> Component::read_item_clearance_info(IJFlexScanner& p_scanner)
{
    p_scanner.yybegin(SpecctraDsnStreamReader::NAME);
    Token next_token = p_scanner.next_token();
    if (!next_token.is_string())
    {
        warn("Component.read_item_clearance_info: String expected at '" + p_scanner.get_scope_identifier() + "'");
        return std::nullopt;
    }
    const std::string name = next_token.string_value;
    std::optional<std::string> cl_class_name;
    next_token = p_scanner.next_token();
    while (next_token.is(Keyword::OPEN_BRACKET))
    {
        next_token = p_scanner.next_token();
        if (next_token.is(Keyword::CLEARANCE_CLASS)
            || (next_token.is_string()
                && (equals_ignore_case(next_token.string_value, "clearance_class")
                    || equals_ignore_case(next_token.string_value, "clearanceClass"))))
        {
            cl_class_name = DsnFile::read_string_scope(p_scanner);
        }
        else
        {
            skip_scope(p_scanner);
        }
        next_token = p_scanner.next_token();
    }
    if (!next_token.is(Keyword::CLOSED_BRACKET))
    {
        warn("Component.read_item_clearance_info: ) expected at '" + p_scanner.get_scope_identifier() + "'");
        return std::nullopt;
    }
    if (!cl_class_name.has_value())
    {
        warn("Component.read_item_clearance_info: clearance class name not found at '"
            + p_scanner.get_scope_identifier() + "'");
        return std::nullopt;
    }
    return ComponentPlacement::ItemClearanceInfo{name, *cl_class_name};
}

bool Component::read_lock_type(IJFlexScanner& p_scanner)
{
    bool result = false;
    for (;;)
    {
        const Token next_token = p_scanner.next_token();
        if (next_token.is(Keyword::CLOSED_BRACKET) || next_token.is_eof()) // Java loops forever at EOF
        {
            break;
        }
        if (next_token.is(Keyword::POSITION))
        {
            result = true;
        }
    }
    return result;
}

bool PlaceControl::read_flip_style_rotate_first(IJFlexScanner& p_scanner)
{
    bool result = false;
    Token next_token = p_scanner.next_token();
    if (next_token.is(Keyword::ROTATE_FIRST))
    {
        result = true;
    }
    next_token = p_scanner.next_token();
    if (!next_token.is(Keyword::CLOSED_BRACKET))
    {
        warn("Structure.read_flip_style: closing bracket expected at '" + p_scanner.get_scope_identifier() + "'");
        return false;
    }
    return result;
}

bool PlaceControl::read_scope(ReadScopeParameter& p_par) const
{
    bool flip_style_rotate_first = false;
    Token next_token;
    for (;;)
    {
        const Token prev_token = next_token;
        next_token = p_par.scanner.next_token();
        if (next_token.is_eof())
        {
            warn("PlaceControl.read_scope: unexpected end of file at '" + p_par.scanner.get_scope_identifier() + "'");
            return false;
        }
        if (next_token.is(Keyword::CLOSED_BRACKET))
        {
            break; // end of scope
        }
        if (prev_token.is(Keyword::OPEN_BRACKET) && next_token.is(Keyword::FLIP_STYLE))
        {
            flip_style_rotate_first = read_flip_style_rotate_first(p_par.scanner);
        }
    }
    if (flip_style_rotate_first)
    {
        // Java: board.components.setFlipStyleRotateFirst(true) (throws if there is no board)
        if (board::RoutingBoard* board = p_par.get_board())
        {
            board->set_flip_style_rotate_first(true);
        }
    }
    return true;
}

} // namespace freerouting::designforms::specctra::parser
