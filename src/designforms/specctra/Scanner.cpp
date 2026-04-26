#include "designforms/specctra/Scanner.h"
#include "designforms/specctra/Structure.h"
#include "designforms/specctra/Library.h"
#include "designforms/specctra/Network.h"
#include "designforms/specctra/Placement.h"

#include <cctype>

namespace freerouting::designforms::specctra {

Scanner::Scanner(const std::string& p_filename)
    : input_stream_(&file_stream_)
    , owns_stream_(true)
{
    file_stream_.open(p_filename, std::ios::in);
    if (file_stream_.is_open())
    {
        read_char();
    }
    else
    {
        at_end_ = true;
    }
}

Scanner::Scanner(std::istream& p_input)
    : input_stream_(&p_input)
    , owns_stream_(false)
{
    read_char();
}

Scanner::~Scanner()
{
    if (owns_stream_ && file_stream_.is_open())
    {
        file_stream_.close();
    }
}

void Scanner::read_char()
{
    if (!input_stream_->eof())
    {
        cur_char_ = input_stream_->get();
        if (input_stream_->eof())
        {
            at_end_ = true;
            cur_char_ = 0;
        }
    }
    else
    {
        at_end_ = true;
        cur_char_ = 0;
    }

    if (cur_char_ == '\n')
    {
        cur_location_.line++;
        cur_location_.column = 1;
    }
    else
    {
        cur_location_.column++;
    }
}

Scanner::Token Scanner::next_token()
{
    while (!at_end_ && std::isspace(cur_char_))
    {
        read_char();
    }

    if (at_end_)
    {
        return {TokenType::STRING, std::string(""), cur_location_};
    }

    Token result;
    result.location = cur_location_;

    switch (cur_char_)
    {
        case '(':
            result.type = TokenType::OPEN_PAREN;
            paren_level_++;
            read_char();
            return result;
        case ')':
            result.type = TokenType::CLOSED_PAREN;
            paren_level_--;
            read_char();
            return result;
        case '"':
            {
                std::string str;
                read_char();
                while (!at_end_ && cur_char_ != '"')
                {
                    str += static_cast<char>(cur_char_);
                    read_char();
                }
                if (!at_end_)
                    read_char();
                result.type = TokenType::STRING;
                result.value = str;
                return result;
            }
        default:
            {
                std::string str;
                while (!at_end_ && !std::isspace(cur_char_) && cur_char_ != '(' && cur_char_ != ')')
                {
                    str += static_cast<char>(cur_char_);
                    read_char();
                }

                if (str.empty())
                {
                    return next_token();
                }

                if (std::isdigit(str[0]) || str[0] == '-' || str[0] == '+')
                {
                    bool is_float = false;
                    for (char c : str)
                    {
                        if (c == '.')
                            is_float = true;
                    }

                    if (is_float)
                    {
                        result.type = TokenType::FLOAT;
                        result.value = std::stod(str);
                    }
                    else
                    {
                        result.type = TokenType::INTEGER;
                        result.value = std::stoll(str);
                    }
                }
                else
                {
                    result.type = TokenType::STRING;
                    result.value = str;
                }
                return result;
            }
    }
}

bool Scanner::expect(TokenType p_type)
{
    Token token = next_token();
    return token.type == p_type;
}

bool Scanner::expect_keyword(const std::string& p_keyword)
{
    Token token = next_token();
    if (token.type != TokenType::STRING)
        return false;
    return std::get<std::string>(token.value) == p_keyword;
}

std::string Scanner::read_string()
{
    Token token = next_token();
    if (token.type == TokenType::STRING)
    {
        return std::get<std::string>(token.value);
    }
    return "";
}

int64_t Scanner::read_int()
{
    Token token = next_token();
    if (token.type == TokenType::INTEGER)
    {
        return std::get<int64_t>(token.value);
    }
    if (token.type == TokenType::FLOAT)
    {
        return static_cast<int64_t>(std::get<double>(token.value));
    }
    return 0;
}

double Scanner::read_float()
{
    Token token = next_token();
    if (token.type == TokenType::FLOAT)
    {
        return std::get<double>(token.value);
    }
    if (token.type == TokenType::INTEGER)
    {
        return static_cast<double>(std::get<int64_t>(token.value));
    }
    return 0.0;
}

bool Scanner::is_open() const
{
    return owns_stream_ ? file_stream_.is_open() : true;
}

Result read_dsn(const std::string& p_filename)
{
    Result result;
    result.success = false;

    Scanner scanner(p_filename);
    if (!scanner.is_open())
    {
        result.error_message = "Could not open file: " + p_filename;
        return result;
    }

    // First token should be OPEN_PAREN
    if (!scanner.expect(Scanner::TokenType::OPEN_PAREN))
    {
        result.error_message = "File does not start with (";
        return result;
    }

    // Next token should be "pcb" keyword
    if (!scanner.expect_keyword("pcb"))
    {
        result.error_message = "File does not start with (pcb";
        return result;
    }

    // Read board name
    result.board_name = scanner.read_string();

    while (!scanner.eof())
    {
        Scanner::Token token = scanner.next_token();

        if (token.type == Scanner::TokenType::OPEN_PAREN)
        {
            Scanner::Token keyword_token = scanner.next_token();
            if (keyword_token.type == Scanner::TokenType::STRING)
            {
                std::string keyword = std::get<std::string>(keyword_token.value);

                if (keyword == "structure")
                {
                    Structure::read_scope(scanner, result);
                }
                else if (keyword == "library")
                {
                    Library::read_scope(scanner, result);
                }
                else if (keyword == "placement")
                {
                    Placement::read_scope(scanner, result);
                }
                else if (keyword == "network")
                {
                    Network::read_scope(scanner, result);
                }
                else if (keyword == "wiring")
                {
                    // Skip wiring section for now
                    while (scanner.current_level() > 0 && !scanner.eof())
                    {
                        Scanner::Token t = scanner.next_token();
                        if (t.type == Scanner::TokenType::CLOSED_PAREN)
                            break;
                    }
                }
                else
                {
                    while (scanner.current_level() > 0 && !scanner.eof())
                    {
                        Scanner::Token t = scanner.next_token();
                        if (t.type == Scanner::TokenType::CLOSED_PAREN)
                            break;
                    }
                }
            }
        }
        else if (token.type == Scanner::TokenType::CLOSED_PAREN)
        {
            if (scanner.current_level() < 0)
                break;
        }
    }

    result.success = true;
    return result;
}

} // namespace freerouting::designforms::specctra