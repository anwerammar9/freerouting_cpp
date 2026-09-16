#include "designforms/specctra/Placement.h"

#include "logger/FRLogger.h"

#include <iostream>

namespace freerouting::designforms::specctra {

bool Placement::skip_scope(Scanner& p_scanner)
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

bool Placement::read_place_scope(Scanner& p_scanner, Result& p_result, const std::string& p_footprint)
{
    ComponentDef comp_def;
    comp_def.placed = true;
    comp_def.footprint = p_footprint;
    comp_def.package = p_footprint;
    comp_def.rotation = 0;

    // (place <refdes> <x> <y> <side> <rotation> (PN <image>))
    comp_def.name = p_scanner.read_string();
    comp_def.x = static_cast<int64_t>(p_scanner.read_float());
    comp_def.y = static_cast<int64_t>(p_scanner.read_float());

    bool done = false;
    bool rotation_seen = false;
    while (!p_scanner.eof() && !done)
    {
        Scanner::Token token = p_scanner.next_token();

        switch (token.type)
        {
            case Scanner::TokenType::OPEN_PAREN:
                // Sub-scope inside the place entry. Fully consume it so its own
                // closing paren does not terminate the (place ...) entry.
                {
                    std::string keyword = to_lower(p_scanner.read_string());
                    if (keyword == "pn")
                    {
                        comp_def.package = p_scanner.read_string();
                    }
                    else
                    {
                        skip_scope(p_scanner);
                        break;
                    }
                    // 'PN' value consumed; drain the rest of the sub-scope.
                    while (!p_scanner.eof())
                    {
                        Scanner::Token t = p_scanner.next_token();
                        if (t.type == Scanner::TokenType::CLOSED_PAREN)
                        {
                            break;
                        }
                    }
                }
                break;

            case Scanner::TokenType::CLOSED_PAREN:
                done = true;
                break;

            case Scanner::TokenType::INTEGER:
                if (!rotation_seen)
                {
                    // Side was consumed as a string; the first number is the rotation.
                    comp_def.rotation = static_cast<int>(std::get<int64_t>(token.value));
                    rotation_seen = true;
                }
                break;

            case Scanner::TokenType::FLOAT:
                if (!rotation_seen)
                {
                    comp_def.rotation = static_cast<int>(std::get<double>(token.value));
                    rotation_seen = true;
                }
                break;

            case Scanner::TokenType::STRING:
            default:
                break;
        }
    }

    if (!comp_def.name.empty())
    {
        std::cerr << "DBG place refdes='" << comp_def.name << "' x=" << comp_def.x << " y=" << comp_def.y
                  << " rot=" << comp_def.rotation << " pkg='" << comp_def.package << "'\n";
        p_result.components.push_back(comp_def);
    }

    return true;
}

bool Placement::read_component_scope(Scanner& p_scanner, Result& p_result)
{
    // (component <footprint> (place ...) ...)
    std::string footprint = p_scanner.read_string();
    std::cerr << "DBG component footprint='" << footprint << "'\n";

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
            if (keyword_token.type == Scanner::TokenType::STRING)
            {
                std::string keyword = to_lower(std::get<std::string>(keyword_token.value));

                if (keyword == "place")
                {
                    read_place_scope(p_scanner, p_result, footprint);
                }
                else
                {
                    skip_scope(p_scanner);
                }
            }
        }
    }

    return true;
}

bool Placement::read_scope(Scanner& p_scanner, Result& p_result)
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
            if (keyword_token.type == Scanner::TokenType::STRING)
            {
                std::string keyword = to_lower(std::get<std::string>(keyword_token.value));

                if (keyword == "component" || keyword == "package")
                {
                    read_component_scope(p_scanner, p_result);
                }
                else if (keyword == "place_control")
                {
                    skip_scope(p_scanner);
                }
                else
                {
                    skip_scope(p_scanner);
                }
            }
        }
    }

    return true;
}

} // namespace freerouting::designforms::specctra