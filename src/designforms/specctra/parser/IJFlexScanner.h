#pragma once

#include <optional>
#include <string>
#include <vector>

namespace freerouting::designforms::specctra::parser {

class Keyword;

/**
 * A token of the DSN lexer. Java returns an Object: a Keyword, a String, an
 * Integer, a Double, or null at the end of the file.
 */
struct Token
{
    enum class Kind
    {
        END_OF_FILE,
        KEYWORD,
        STRING,
        INTEGER,
        DOUBLE
    };

    Kind kind = Kind::END_OF_FILE;
    const Keyword* keyword = nullptr;
    std::string string_value;
    int int_value = 0;
    double double_value = 0.0;

    static Token end_of_file() { return Token{}; }
    static Token of_keyword(const Keyword* p_keyword) { return Token{ Kind::KEYWORD, p_keyword, {}, 0, 0.0 }; }
    static Token of_string(std::string p_value) { return Token{ Kind::STRING, nullptr, std::move(p_value), 0, 0.0 }; }
    static Token of_int(int p_value) { return Token{ Kind::INTEGER, nullptr, {}, p_value, 0.0 }; }
    static Token of_double(double p_value) { return Token{ Kind::DOUBLE, nullptr, {}, 0, p_value }; }

    /** Java `token == null`. */
    bool is_eof() const { return kind == Kind::END_OF_FILE; }
    /** Java `token == p_keyword` (keywords are unique instances). */
    bool is(const Keyword* p_keyword) const { return kind == Kind::KEYWORD && keyword == p_keyword; }
    bool is_keyword() const { return kind == Kind::KEYWORD; }
    /** Java `token instanceof String`. */
    bool is_string() const { return kind == Kind::STRING; }
    /** Java `token instanceof Integer`. */
    bool is_int() const { return kind == Kind::INTEGER; }
    /** Java `token instanceof Double`. */
    bool is_double() const { return kind == Kind::DOUBLE; }
    /** Integer or Double. */
    bool is_number() const { return is_int() || is_double(); }
    /** The numeric value of an Integer or Double token. */
    double number() const { return is_int() ? static_cast<double>(int_value) : double_value; }
    /** Text for log messages (Java Object.toString of the token). */
    std::string to_string() const;
};

/** The lexer interface used by the parser. Port of Java IJFlexScanner. */
class IJFlexScanner
{
public:
    virtual ~IJFlexScanner() = default;

    /** Returns the current scope identifier (used in log messages). */
    virtual const std::string& get_scope_identifier() const = 0;
    virtual void set_scope_identifier(const std::string& p_identifier) = 0;

    /** Returns the next token; throws std::runtime_error for an integer out of range (Java NumberFormatException). */
    virtual Token next_token() = 0;
    virtual std::string next_string() = 0;
    virtual std::string next_string(bool p_ignore_newline) = 0;
    virtual std::vector<std::string> next_string_list() = 0;
    virtual std::vector<std::string> next_string_list(char p_separator) = 0;
    /** Java nextDouble: std::nullopt for a value that cannot be parsed. */
    virtual std::optional<double> next_double() = 0;
    virtual bool next_closing_bracket() = 0;
    virtual void yybegin(int p_new_state) = 0;
};

} // namespace freerouting::designforms::specctra::parser
