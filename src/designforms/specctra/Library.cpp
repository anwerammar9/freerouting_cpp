#include "designforms/specctra/Library.h"

#include "logger/FRLogger.h"

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
    ViaDef via_def;

    // Read padstack name
    via_def.name = p_scanner.read_string();

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

                if (keyword == "shape")
                {
                    // Skip shape scope
                    skip_scope(p_scanner);
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

    // Check if via with same name already exists
    bool found = false;
    for (const auto& existing : p_result.vias)
    {
        if (existing.name == via_def.name)
        {
            found = true;
            break;
        }
    }
    if (!found)
    {
        p_result.vias.push_back(via_def);
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
                std::string keyword = std::get<std::string>(keyword_token.value);

                if (keyword == "padstack")
                {
                    read_padstack_scope(p_scanner, p_result);
                }
                else if (keyword == "package")
                {
                    // Skip package scope for now
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