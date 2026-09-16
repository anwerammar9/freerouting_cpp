#include "designforms/specctra/Library.h"

#include "logger/FRLogger.h"

#include <algorithm>
#include <cmath>

namespace freerouting::designforms::specctra {

bool Library::skip_scope(Scanner& p_scanner)
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

bool Library::read_padstack_scope(Scanner& p_scanner, Result& p_result)
{
    PadstackDef padstack_def;

    // Read padstack name
    padstack_def.name = p_scanner.read_string();

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

                if (keyword == "shape")
                {
                    if (p_scanner.next_token().type != Scanner::TokenType::OPEN_PAREN)
                    {
                        continue;
                    }
                    const std::string shape_type = p_scanner.read_string();
                    p_scanner.read_string(); // layer name
                    if (shape_type == "circle")
                    {
                        const double radius = p_scanner.read_float();
                        padstack_def.width = std::max(padstack_def.width, 2.0 * radius);
                        padstack_def.height = std::max(padstack_def.height, 2.0 * radius);
                        p_scanner.read_float();
                        p_scanner.read_float();
                    }
                    else if (shape_type == "rect")
                    {
                        const double x1 = p_scanner.read_float();
                        const double y1 = p_scanner.read_float();
                        const double x2 = p_scanner.read_float();
                        const double y2 = p_scanner.read_float();
                        padstack_def.width = std::max(padstack_def.width, std::abs(x2 - x1));
                        padstack_def.height = std::max(padstack_def.height, std::abs(y2 - y1));
                    }
                    else
                    {
                        skip_scope(p_scanner);
                    }
                    p_scanner.next_token(); // close the nested shape
                }
                else if (keyword == "attach")
                {
                    // Skip attach
                    p_scanner.read_string();
                }
                else if (keyword == "absolute")
                {
                    // Skip absolute
                    p_scanner.read_string();
                }
                else if (keyword == "layer")
                {
                    // Skip layer specification
                    skip_scope(p_scanner);
                }
                else
                {
                    // Skip unknown scope
                    skip_scope(p_scanner);
                }
            }
        }
    }

    bool found = false;
    for (const auto& existing : p_result.padstacks)
    {
        if (existing.name == padstack_def.name)
        {
            found = true;
            break;
        }
    }
    if (!found)
    {
        p_result.padstacks.push_back(padstack_def);
    }

    return true;
}

bool Library::read_image_scope(Scanner& p_scanner, Result& p_result)
{
    ImageDef image;
    image.name = p_scanner.read_string();

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

                if (keyword == "pin")
                {
                    ImagePinDef pin_def;
                    pin_def.padstack = p_scanner.read_string();

                    Scanner::Token name_token = p_scanner.next_token();
                    while (name_token.type == Scanner::TokenType::OPEN_PAREN)
                    {
                        skip_scope(p_scanner);
                        name_token = p_scanner.next_token();
                    }
                    if (name_token.type == Scanner::TokenType::STRING)
                    {
                        pin_def.name = std::get<std::string>(name_token.value);
                    }
                    else if (name_token.type == Scanner::TokenType::INTEGER)
                    {
                        pin_def.name = std::to_string(std::get<int64_t>(name_token.value));
                    }

                    pin_def.x = p_scanner.read_float();
                    pin_def.y = p_scanner.read_float();

                    image.pins.push_back(pin_def);
                }
                else
                {
                    skip_scope(p_scanner);
                }
            }
        }
    }

    // Avoid duplicates (e.g. MountingHole ... ::1 variants)
    bool found = false;
    for (const auto& existing : p_result.images)
    {
        if (existing.name == image.name)
        {
            found = true;
            break;
        }
    }
    if (!found)
    {
        p_result.images.push_back(image);
    }

    return true;
}

bool Library::read_scope(Scanner& p_scanner, Result& p_result)
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

                if (keyword == "padstack")
                {
                    read_padstack_scope(p_scanner, p_result);
                }
                else if (keyword == "image")
                {
                    read_image_scope(p_scanner, p_result);
                }
                else if (keyword == "package")
                {
                    // Treat packages as images as well.
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