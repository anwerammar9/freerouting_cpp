#include "designforms/specctra/parser/Package.h"

#include <stdexcept>

#include "designforms/specctra/parser/IJFlexScanner.h"
#include "designforms/specctra/parser/Keyword.h"
#include "designforms/specctra/parser/Layer.h"
#include "designforms/specctra/parser/ScopeKeyword.h"
#include "designforms/specctra/parser/SpecctraDsnStreamReader.h"
#include "logger/FRLogger.h"

namespace freerouting::designforms::specctra::parser {

namespace {

void warn(const std::string& p_message) { logger::FRLogger::instance().warn(p_message); }

/** Java nextToken.toString() for a String or Integer token. */
std::string token_text(const Token& p_token)
{
    return p_token.is_int() ? std::to_string(p_token.int_value) : p_token.string_value;
}

} // namespace

std::optional<Package> Package::read_scope(IJFlexScanner& p_scanner, const LayerStructure& p_layer_structure)
{
    Package result;
    Token next_token = p_scanner.next_token();
    if (!next_token.is_string())
    {
        warn("Package.read_scope: String expected at '" + p_scanner.get_scope_identifier() + "'");
        return std::nullopt;
    }
    result.name = next_token.string_value;
    p_scanner.set_scope_identifier(result.name);
    for (;;)
    {
        const Token prev_token = next_token;
        next_token = p_scanner.next_token();
        if (next_token.is_eof())
        {
            warn("Package.read_scope: unexpected end of file at '" + p_scanner.get_scope_identifier() + "'");
            return std::nullopt;
        }
        if (next_token.is(Keyword::CLOSED_BRACKET))
        {
            break; // end of scope
        }
        if (!prev_token.is(Keyword::OPEN_BRACKET))
        {
            continue;
        }
        if (next_token.is(Keyword::PIN))
        {
            std::optional<PinInfo> next_pin = read_pin_info(p_scanner);
            if (!next_pin.has_value())
            {
                return std::nullopt;
            }
            result.pin_info_arr.push_back(std::move(*next_pin));
        }
        else if (next_token.is(Keyword::SIDE))
        {
            result.is_front = read_placement_side(p_scanner);
        }
        else if (next_token.is(Keyword::OUTLINE))
        {
            std::unique_ptr<Shape> current_shape = Shape::read_scope(p_scanner, &p_layer_structure);
            if (current_shape != nullptr)
            {
                result.outline.push_back(std::move(current_shape));
            }
            // overread the closing bracket
            next_token = p_scanner.next_token();
            if (!next_token.is(Keyword::CLOSED_BRACKET))
            {
                warn("Package.read_scope: closed bracket expected at '" + p_scanner.get_scope_identifier() + "'");
                return std::nullopt;
            }
        }
        else if (next_token.is(Keyword::KEEPOUT))
        {
            std::optional<Shape::ReadAreaScopeResult> keepout_area =
                Shape::read_area_scope(p_scanner, &p_layer_structure, false);
            if (keepout_area.has_value())
            {
                result.keepouts.push_back(std::move(*keepout_area));
            }
            else
            {
                logger::FRLogger::instance().error(
                    "Package.read_scope: could not read keepout area of package '" + result.name + "'");
            }
        }
        else if (next_token.is(Keyword::VIA_KEEPOUT))
        {
            std::optional<Shape::ReadAreaScopeResult> keepout_area =
                Shape::read_area_scope(p_scanner, &p_layer_structure, false);
            if (keepout_area.has_value())
            {
                result.via_keepouts.push_back(std::move(*keepout_area));
            }
        }
        else if (next_token.is(Keyword::PLACE_KEEPOUT))
        {
            std::optional<Shape::ReadAreaScopeResult> keepout_area =
                Shape::read_area_scope(p_scanner, &p_layer_structure, false);
            if (keepout_area.has_value())
            {
                result.place_keepouts.push_back(std::move(*keepout_area));
            }
        }
        else
        {
            ScopeKeyword::skip_scope(p_scanner);
        }
    }
    return result;
}

std::optional<Package::PinInfo> Package::read_pin_info(IJFlexScanner& p_scanner)
{
    PinInfo result;
    // Read the padstack name.
    p_scanner.yybegin(SpecctraDsnStreamReader::NAME);
    Token next_token = p_scanner.next_token();
    if (!next_token.is_string() && !next_token.is_int())
    {
        warn("Package.read_pin_info: String or Integer expected at '" + p_scanner.get_scope_identifier() + "'");
        return std::nullopt;
    }
    result.padstack_name = token_text(next_token);

    p_scanner.yybegin(SpecctraDsnStreamReader::NAME); // to handle pin names starting with a digit
    next_token = p_scanner.next_token();
    if (next_token.is(Keyword::OPEN_BRACKET))
    {
        // read the padstack rotation
        next_token = p_scanner.next_token();
        if (next_token.is(Keyword::ROTATE))
        {
            result.rotation = read_rotation(p_scanner);
        }
        else
        {
            ScopeKeyword::skip_scope(p_scanner);
        }
        p_scanner.yybegin(SpecctraDsnStreamReader::NAME);
        next_token = p_scanner.next_token();
    }
    // Read the pin name.
    if (!next_token.is_string() && !next_token.is_int())
    {
        warn("Package.read_pin_info: String or Integer expected at '" + p_scanner.get_scope_identifier() + "'");
        return std::nullopt;
    }
    result.pin_name = token_text(next_token);

    for (int i = 0; i < 2; ++i)
    {
        next_token = p_scanner.next_token();
        if (!next_token.is_number())
        {
            warn("Package.read_pin_info: number expected at '" + p_scanner.get_scope_identifier() + "'");
            return std::nullopt;
        }
        result.rel_coor[i] = next_token.number();
    }
    // Handle scopes at the end of the pin scope.
    for (;;)
    {
        const Token prev_token = next_token;
        next_token = p_scanner.next_token();
        if (next_token.is_eof())
        {
            warn("Package.read_pin_info: unexpected end of file at '" + p_scanner.get_scope_identifier() + "'");
            return std::nullopt;
        }
        if (next_token.is(Keyword::CLOSED_BRACKET))
        {
            break; // end of scope
        }
        if (prev_token.is(Keyword::OPEN_BRACKET))
        {
            if (next_token.is(Keyword::ROTATE))
            {
                result.rotation = read_rotation(p_scanner);
            }
            else
            {
                ScopeKeyword::skip_scope(p_scanner);
            }
        }
    }
    return result;
}

double Package::read_rotation(IJFlexScanner& p_scanner)
{
    double result = 0;
    const std::string next_string = p_scanner.next_string();
    try
    {
        result = std::stod(next_string); // Java Double.parseDouble
    }
    catch (const std::exception&)
    {
        warn("Package.read_rotation: number expected at '" + p_scanner.get_scope_identifier() + "'");
    }
    // overread the closing bracket
    const Token next_token = p_scanner.next_token();
    if (!next_token.is(Keyword::CLOSED_BRACKET))
    {
        warn("Package.read_rotation: closing bracket expected at '" + p_scanner.get_scope_identifier() + "'");
    }
    return result;
}

bool Package::read_placement_side(IJFlexScanner& p_scanner)
{
    Token next_token = p_scanner.next_token();
    const bool result = !next_token.is(Keyword::BACK);
    next_token = p_scanner.next_token();
    if (!next_token.is(Keyword::CLOSED_BRACKET))
    {
        warn("Package.read_placement_side: closing bracket expected at '" + p_scanner.get_scope_identifier() + "'");
    }
    return result;
}

} // namespace freerouting::designforms::specctra::parser
