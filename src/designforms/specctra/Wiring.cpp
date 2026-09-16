#include "designforms/specctra/Wiring.h"

#include "designforms/specctra/Scanner.h"
#include "logger/FRLogger.h"

namespace freerouting::designforms::specctra {

namespace {

bool skip_scope(Scanner& p_scanner)
{
    int open_bracket_count = 1;
    while (open_bracket_count > 0)
    {
        Scanner::Token token = p_scanner.next_token();
        if (token.type == Scanner::TokenType::OPEN_PAREN)
        {
            ++open_bracket_count;
        }
        else if (token.type == Scanner::TokenType::CLOSED_PAREN)
        {
            --open_bracket_count;
        }
        else if (p_scanner.eof())
        {
            return false;
        }
    }
    return true;
}

// Consumes the rest of the current scope (all tokens up to and including its
// matching closing paren) after a partial read, so an error mid-scope does not
// desynchronize the enclosing (wiring ...) loop.
void drain_scope(Scanner& p_scanner)
{
    int depth = 0;
    while (!p_scanner.eof())
    {
        Scanner::Token token = p_scanner.next_token();
        if (token.type == Scanner::TokenType::OPEN_PAREN)
        {
            ++depth;
        }
        else if (token.type == Scanner::TokenType::CLOSED_PAREN)
        {
            if (depth == 0)
            {
                return;
            }
            --depth;
        }
    }
}

/**
 * Reads "(net <name> [subnet])" and resolves the net number against the nets
 * already registered in the result. Mirrors Java Wiring.read_net_id() plus the
 * net lookup of Wiring.get_subnets().
 */
std::vector<int> read_net_id(Scanner& p_scanner, const Result& p_result)
{
    const std::string net_name = p_scanner.read_string();

    while (!p_scanner.eof())
    {
        Scanner::Token token = p_scanner.next_token();
        if (token.type == Scanner::TokenType::CLOSED_PAREN)
        {
            break;
        }
    }

    if (net_name.empty())
    {
        return {};
    }

    for (const auto& net_def : p_result.nets)
    {
        if (net_def.name == net_name)
        {
            return {net_def.id};
        }
    }

    logger::FRLogger::instance().warn(
        "Wiring: net with name '" + net_name + "' not found in the network scope");
    return {};
}

/**
 * Maps a DSN fixed-state keyword to the NOT_FIXED/SHOVE_FIXED semantics used
 * during board materialization. Mirrors Java Wiring.calc_fixed().
 */
bool read_fixed_state(Scanner& p_scanner)
{
    const std::string keyword = to_lower(p_scanner.read_string());
    return !keyword.empty() && keyword != "normal";
}

/**
 * Reads a polyline/polygon path header: layer name, width and coordinates,
 * until the closing bracket of the path scope.
 */
bool read_path_scope(Scanner& p_scanner, Result& p_result, WireDef& p_wire)
{
    const std::string layer_name = p_scanner.read_string();
    int layer_no = -1;
    for (int i = 0; i < static_cast<int>(p_result.layers.size()); ++i)
    {
        if (p_result.layers[i].name == layer_name)
        {
            layer_no = i;
            break;
        }
    }
    if (layer_no < 0)
    {
        logger::FRLogger::instance().warn(
            "Wiring.read_wire_scope: layer with name '" + layer_name +
            "' not found in the layer structure");
        return false;
    }
    p_wire.layer_no = layer_no;

    std::vector<double> values;
    while (!p_scanner.eof())
    {
        Scanner::Token token = p_scanner.next_token();
        if (token.type == Scanner::TokenType::CLOSED_PAREN)
        {
            break;
        }
        if (token.type == Scanner::TokenType::INTEGER)
        {
            values.push_back(static_cast<double>(std::get<int64_t>(token.value)));
        }
        else if (token.type == Scanner::TokenType::FLOAT)
        {
            values.push_back(std::get<double>(token.value));
        }
        else
        {
            logger::FRLogger::instance().warn(
                "Wiring.read_wire_scope: number expected inside path scope");
            return false;
        }
    }

    if (values.size() < 3)
    {
        logger::FRLogger::instance().warn(
            "Wiring.read_wire_scope: too few numbers inside path scope");
        return false;
    }

    p_wire.width = values[0];
    p_wire.coords.assign(values.begin() + 1, values.end());
    return true;
}

} // namespace

bool Wiring::read_scope(Scanner& p_scanner, Result& p_result)
{
    while (!p_scanner.eof())
    {
        Scanner::Token token = p_scanner.next_token();

        if (token.type == Scanner::TokenType::CLOSED_PAREN)
        {
            break;
        }

        if (token.type == Scanner::TokenType::OPEN_PAREN)
        {
            Scanner::Token keyword_token = p_scanner.next_token();
            if (keyword_token.type != Scanner::TokenType::STRING)
            {
                continue;
            }
            const std::string keyword = to_lower(std::get<std::string>(keyword_token.value));

            if (keyword == "wire")
            {
                read_wire_scope(p_scanner, p_result);
            }
            else if (keyword == "via")
            {
                read_via_scope(p_scanner, p_result);
            }
            else
            {
                skip_scope(p_scanner);
            }
        }
    }

    return true;
}

bool Wiring::read_wire_scope(Scanner& p_scanner, Result& p_result)
{
    WireDef wire_def;

    while (!p_scanner.eof())
    {
        Scanner::Token token = p_scanner.next_token();

        if (token.type == Scanner::TokenType::CLOSED_PAREN)
        {
            break;
        }

        if (token.type != Scanner::TokenType::OPEN_PAREN)
        {
            continue;
        }

        Scanner::Token keyword_token = p_scanner.next_token();
        if (keyword_token.type != Scanner::TokenType::STRING)
        {
            continue;
        }
        const std::string keyword = to_lower(std::get<std::string>(keyword_token.value));

        if (keyword == "polyline_path")
        {
            if (!read_path_scope(p_scanner, p_result, wire_def))
            {
                return false;
            }
        }
        else if (keyword == "polygon_path")
        {
            if (!read_path_scope(p_scanner, p_result, wire_def))
            {
                return false;
            }
            wire_def.is_polygon_path = true;
        }
        else if (keyword == "rect" || keyword == "rectangle" || keyword == "polygon" || keyword == "circle")
        {
            // Conduction area (plane). Parsed but not materialized yet; the
            // data would need a board ConductionArea/Area to be built.
            skip_scope(p_scanner);
            wire_def.is_conduction_area = true;
        }
        else if (keyword == "window")
        {
            skip_scope(p_scanner);
        }
        else if (keyword == "net")
        {
            wire_def.net_nos = read_net_id(p_scanner, p_result);
        }
        else if (keyword == "clearance_class")
        {
            p_scanner.read_string();
            if (!p_scanner.expect(Scanner::TokenType::CLOSED_PAREN))
            {
                return false;
            }
        }
        else if (keyword == "type")
        {
            wire_def.fixed = read_fixed_state(p_scanner);
            if (!p_scanner.expect(Scanner::TokenType::CLOSED_PAREN))
            {
                return false;
            }
        }
        else
        {
            skip_scope(p_scanner);
        }
    }

    if (wire_def.is_conduction_area)
    {
        // Keep the parsed area wire out of the trace path for now.
        p_result.wires.push_back(wire_def);
        return true;
    }

    if (wire_def.coords.size() < 4)
    {
        logger::FRLogger::instance().warn(
            "Wiring.read_wire_scope: shape missing or too few coordinates");
        // The closing paren of this wire scope was already consumed by the loop.
        return true;
    }

    p_result.wires.push_back(wire_def);
    return true;
}

bool Wiring::read_via_scope(Scanner& p_scanner, Result& p_result)
{
    ViaPlacementDef via_def;

    via_def.padstack_name = p_scanner.read_string();

    // Read the location (x, y).
    for (int i = 0; i < 2; ++i)
    {
        Scanner::Token token = p_scanner.next_token();
        if (token.type == Scanner::TokenType::INTEGER)
        {
            via_def.location.push_back(static_cast<double>(std::get<int64_t>(token.value)));
        }
        else if (token.type == Scanner::TokenType::FLOAT)
        {
            via_def.location.push_back(std::get<double>(token.value));
        }
        else
        {
            logger::FRLogger::instance().warn(
                "Wiring.read_via_scope: number expected for via location");
            drain_scope(p_scanner);
            return false;
        }
    }

    while (!p_scanner.eof())
    {
        Scanner::Token token = p_scanner.next_token();

        if (token.type == Scanner::TokenType::CLOSED_PAREN)
        {
            break;
        }

        if (token.type != Scanner::TokenType::OPEN_PAREN)
        {
            continue;
        }

        Scanner::Token keyword_token = p_scanner.next_token();
        if (keyword_token.type != Scanner::TokenType::STRING)
        {
            continue;
        }
        const std::string keyword = to_lower(std::get<std::string>(keyword_token.value));

        if (keyword == "net")
        {
            via_def.net_nos = read_net_id(p_scanner, p_result);
        }
        else if (keyword == "clearance_class")
        {
            p_scanner.read_string();
            if (!p_scanner.expect(Scanner::TokenType::CLOSED_PAREN))
            {
                drain_scope(p_scanner);
                return false;
            }
        }
        else if (keyword == "type")
        {
            via_def.fixed = read_fixed_state(p_scanner);
            if (!p_scanner.expect(Scanner::TokenType::CLOSED_PAREN))
            {
                drain_scope(p_scanner);
                return false;
            }
        }
        else
        {
            skip_scope(p_scanner);
        }
    }

    // The padstack must be resolvable later (by name) to place the via.
    bool padstack_found = false;
    for (const auto& via_padstack : p_result.vias)
    {
        if (via_padstack.name == via_def.padstack_name)
        {
            padstack_found = true;
            break;
        }
    }
    if (!padstack_found)
    {
        for (const auto& padstack : p_result.padstacks)
        {
            if (padstack.name == via_def.padstack_name)
            {
                padstack_found = true;
                break;
            }
        }
    }
    if (!padstack_found)
    {
        logger::FRLogger::instance().warn(
            "Wiring.read_via_scope: via padstack '" + via_def.padstack_name +
            "' not found in the library or structure scope");
        return false;
    }

    p_result.via_placements.push_back(via_def);
    return true;
}

} // namespace freerouting::designforms::specctra