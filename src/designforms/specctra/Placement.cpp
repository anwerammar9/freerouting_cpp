#include "designforms/specctra/Placement.h"

#include "logger/FRLogger.h"

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

bool Placement::read_package_placement(Scanner& p_scanner, Result& p_result)
{
    ComponentDef comp_def;
    comp_def.placed = true;

    // Read component name
    comp_def.name = p_scanner.read_string();

    while (!p_scanner.eof())
    {
        Scanner::Token token = p_scanner.next_token();

        if (token.type == Scanner::TokenType::CLOSED_PAREN)
        {
            break;
        }

        if (token.type == Scanner::TokenType::STRING)
        {
            std::string keyword = std::get<std::string>(token.value);

            if (keyword == "place")
            {
                // Read x, y, rotation
                comp_def.x = p_scanner.read_int();
                comp_def.y = p_scanner.read_int();
                
                Scanner::Token next = p_scanner.next_token();
                if (next.type == Scanner::TokenType::STRING)
                {
                    std::string rotation_str = std::get<std::string>(next.value);
                    if (rotation_str == "front")
                        comp_def.rotation = 0;
                    else if (rotation_str == "back")
                        comp_def.rotation = 180;
                    else
                        comp_def.rotation = std::stoi(rotation_str);
                }
                else if (next.type == Scanner::TokenType::INTEGER)
                {
                    comp_def.rotation = static_cast<int>(std::get<int64_t>(next.value));
                }
            }
            else if (keyword == "front" || keyword == "back")
            {
                // Side - front or back
            }
        }
        else if (token.type == Scanner::TokenType::INTEGER)
        {
            // Already handled in "place"
        }
    }

    if (!comp_def.name.empty())
    {
        p_result.components.push_back(comp_def);
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
                std::string keyword = std::get<std::string>(keyword_token.value);

                if (keyword == "package")
                {
                    read_package_placement(p_scanner, p_result);
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