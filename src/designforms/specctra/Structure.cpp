#include "designforms/specctra/Structure.h"

#include "logger/FRLogger.h"

namespace freerouting::designforms::specctra {

bool Structure::skip_scope(Scanner& p_scanner)
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

bool Structure::read_boundary_scope(Scanner& p_scanner, Result& p_result)
{
    int64_t x1 = 0, y1 = 0, x2 = 0, y2 = 0;
    bool found_coords = false;
    bool first_coord = true;
    bool read_x = true;
    int64_t cur_x = 0, cur_y = 0;

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
            if (keyword == "rectangle")
            {
                found_coords = true;
            }
            else if (keyword == "path")
            {
                found_coords = true;
                p_scanner.read_string();
                p_scanner.read_int();
            }
            else if (keyword == "type")
            {
            }
            else if (keyword == "clearance_class")
            {
                p_scanner.read_string();
            }
        }
        else if (token.type == Scanner::TokenType::INTEGER || token.type == Scanner::TokenType::FLOAT)
        {
            if (found_coords)
            {
                double coord = (token.type == Scanner::TokenType::INTEGER) 
                    ? static_cast<double>(std::get<int64_t>(token.value))
                    : std::get<double>(token.value);

                int64_t c = static_cast<int64_t>(coord);

                if (read_x)
                {
                    cur_x = c;
                    read_x = false;
                }
                else
                {
                    cur_y = c;
                    read_x = true;

                    if (first_coord)
                    {
                        x1 = x2 = cur_x;
                        y1 = y2 = cur_y;
                        first_coord = false;
                    }
                    else
                    {
                        if (cur_x < x1) x1 = cur_x;
                        if (cur_y < y1) y1 = cur_y;
                        if (cur_x > x2) x2 = cur_x;
                        if (cur_y > y2) y2 = cur_y;
                    }
                }
            }
        }
    }

    if (!first_coord)
    {
        p_result.board_min_x = x1;
        p_result.board_min_y = y1;
        p_result.board_width = x2 - x1;
        p_result.board_height = y2 - y1;
    }

    return true;
}

bool Structure::read_layer_scope(Scanner& p_scanner, Result& p_result)
{
    LayerInfo layer_info;
    layer_info.is_signal = true;
    layer_info.index = static_cast<int>(p_result.layers.size());

    layer_info.name = p_scanner.read_string();

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

                if (keyword == "type")
                {
                    Scanner::Token type_token = p_scanner.next_token();
                    if (type_token.type == Scanner::TokenType::STRING)
                    {
                        std::string type_str = std::get<std::string>(type_token.value);
                        layer_info.is_signal = (type_str == "signal");
                    }
                    while (!p_scanner.eof())
                    {
                        Scanner::Token end_token = p_scanner.next_token();
                        if (end_token.type == Scanner::TokenType::CLOSED_PAREN)
                            break;
                    }
                }
                else if (keyword == "property")
                {
                    skip_scope(p_scanner);
                }
                else if (keyword == "use_net")
                {
                    while (!p_scanner.eof())
                    {
                        Scanner::Token net_token = p_scanner.next_token();
                        if (net_token.type == Scanner::TokenType::CLOSED_PAREN)
                        {
                            break;
                        }
                        if (net_token.type == Scanner::TokenType::STRING)
                        {
                            layer_info.net_names.push_back(std::get<std::string>(net_token.value));
                        }
                    }
                }
                else if (keyword == "rule")
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

    p_result.layers.push_back({layer_info.index, layer_info.name, layer_info.is_signal});
    return true;
}

bool Structure::read_via_padstack_scope(Scanner& p_scanner, Result& p_result)
{
    ViaDef via_def;

    via_def.name = p_scanner.read_string();

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

            if (keyword == "hole")
            {
                via_def.drill = static_cast<int>(p_scanner.read_int());
            }
            else if (keyword == "shape")
            {
                skip_scope(p_scanner);
            }
        }
        else if (token.type == Scanner::TokenType::INTEGER)
        {
            via_def.layer_indices.push_back(static_cast<int>(std::get<int64_t>(token.value)));
        }
    }

    p_result.vias.push_back(via_def);
    return true;
}

bool Structure::read_scope(Scanner& p_scanner, Result& p_result)
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

                if (keyword == "layer")
                {
                    read_layer_scope(p_scanner, p_result);
                }
                else if (keyword == "boundary")
                {
                    read_boundary_scope(p_scanner, p_result);
                }
                else if (keyword == "via")
                {
                    read_via_padstack_scope(p_scanner, p_result);
                }
                else if (keyword == "control")
                {
                    skip_scope(p_scanner);
                }
                else if (keyword == "rule")
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