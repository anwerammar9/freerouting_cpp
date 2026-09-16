#include "designforms/specctra/Network.h"

#include "logger/FRLogger.h"

namespace freerouting::designforms::specctra {

bool Network::skip_scope(Scanner& p_scanner)
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

bool Network::read_via_scope(Scanner& p_scanner, Result& p_result)
{
    while (!p_scanner.eof())
    {
        Scanner::Token token = p_scanner.next_token();

        if (token.type == Scanner::TokenType::CLOSED_PAREN)
        {
            break;
        }

        if (token.type == Scanner::TokenType::STRING)
        {
            // Skip until we hit the closing paren
        }
    }
    return true;
}

bool Network::read_class_scope(Scanner& p_scanner, Result& p_result)
{
    std::string class_name = p_scanner.read_string();

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

                if (keyword == "circuit")
                {
                    // Skip circuit scope
                    skip_scope(p_scanner);
                }
                else if (keyword == "rule")
                {
                    // Skip rule scope
                    skip_scope(p_scanner);
                }
                else if (keyword == "clearance")
                {
                    skip_scope(p_scanner);
                }
                else if (keyword == "via_rule")
                {
                    skip_scope(p_scanner);
                }
                else if (keyword == "pull_tight")
                {
                    skip_scope(p_scanner);
                }
                else if (keyword == "shove_fixed")
                {
                    skip_scope(p_scanner);
                }
                else
                {
                    skip_scope(p_scanner);
                }
            }
        }
        else if (token.type == Scanner::TokenType::STRING)
        {
            // This is a net name in the class
        }
    }

    return true;
}

bool Network::read_net_scope(Scanner& p_scanner, Result& p_result)
{
    NetDef net_def;
    net_def.id = static_cast<int>(p_result.nets.size()) + 1;

    // Read net name
    net_def.name = p_scanner.read_string();

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

                if (keyword == "pins")
                {
                    // Read all pin references (space-separated: U101-12 TP102-1)
                    while (!p_scanner.eof())
                    {
                        Scanner::Token pin_token = p_scanner.next_token();
                        if (pin_token.type == Scanner::TokenType::CLOSED_PAREN)
                            break;
                        if (pin_token.type == Scanner::TokenType::STRING)
                        {
                            std::string pin_ref = std::get<std::string>(pin_token.value);
                            size_t dash_pos = pin_ref.find('-');
                            if (dash_pos != std::string::npos)
                            {
                                std::string component = pin_ref.substr(0, dash_pos);
                                std::string pin_name = pin_ref.substr(dash_pos + 1);
                                
                                net_def.pins.push_back({component, pin_name});
                                
                                // Also add to top-level pins
                                PinDef pin_def;
                                pin_def.component = component;
                                pin_def.pin = pin_name;
                                pin_def.net_no = net_def.id;
                                pin_def.x = 0;
                                pin_def.y = 0;
                                p_result.pins.push_back(pin_def);
                            }
                        }
                    }
                }
                else if (keyword == "wire")
                {
                    // Skip wire scope
                    skip_scope(p_scanner);
                }
                else
                {
                    skip_scope(p_scanner);
                }
            }
        }
    }

    if (!net_def.name.empty())
    {
        p_result.nets.push_back(net_def);
    }

    return true;
}

bool Network::read_scope(Scanner& p_scanner, Result& p_result)
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

                if (keyword == "net")
                {
                    read_net_scope(p_scanner, p_result);
                }
                else if (keyword == "class")
                {
                    read_class_scope(p_scanner, p_result);
                }
                else if (keyword == "via")
                {
                    read_via_scope(p_scanner, p_result);
                }
                else if (keyword == "via_rule")
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