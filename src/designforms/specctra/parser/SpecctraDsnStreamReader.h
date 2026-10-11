#pragma once

#include <istream>
#include <string>

#include "designforms/specctra/parser/IJFlexScanner.h"

namespace freerouting::designforms::specctra::parser {

/**
 * The DSN lexer. Port of the JFlex-generated Java SpecctraDsnStreamReader
 * (spec: SpecctraFileDescription.flex). The rules are implemented by hand as
 * a longest-match scanner over the UTF-8 decoded input; the keyword table and
 * the character classes are taken from the generated Java tables (the 2008
 * flex file is outdated, e.g. "generated_by_freeroute" -> GENERATED_BY_FREEROUTING).
 */
class SpecctraDsnStreamReader : public IJFlexScanner
{
public:
    // Lexical states (values of the generated Java class).
    static constexpr int YYINITIAL = 0;
    static constexpr int STRING1 = 1;
    static constexpr int STRING2 = 2;
    static constexpr int NAME = 3;
    static constexpr int LAYER_NAME = 4;
    static constexpr int COMPONENT_NAME = 5;
    static constexpr int SPEC_CHAR = 6;
    static constexpr int IGNORE_QUOTE = 7;

    /** Reads the whole stream (Java wraps the InputStream in a default-charset reader, i.e. UTF-8). */
    explicit SpecctraDsnStreamReader(std::istream& p_input);
    explicit SpecctraDsnStreamReader(const std::string& p_text, bool p_is_text);

    const std::string& get_scope_identifier() const override { return scope_identifier_; }
    void set_scope_identifier(const std::string& p_identifier) override { scope_identifier_ = p_identifier; }

    Token next_token() override;
    std::string next_string() override { return next_string(false, ' '); }
    std::string next_string(bool p_ignore_newline) override { return next_string(p_ignore_newline, ' '); }
    std::string next_string(bool p_ignore_newline, char32_t p_leading);
    std::vector<std::string> next_string_list() override { return next_string_list(' '); }
    std::vector<std::string> next_string_list(char p_separator) override;
    std::optional<double> next_double() override;
    bool next_closing_bracket() override;
    void yybegin(int p_new_state) override { lexical_state_ = p_new_state; }
    int yystate() const { return lexical_state_; }

private:
    char32_t char_at(size_t p_pos) const { return p_pos < buffer_.size() ? buffer_[p_pos] : U'\0'; }
    std::string text(size_t p_start, size_t p_end) const;
    Token read_quoted_string(char32_t p_quote, size_t p_start);
    void skip_illegal_character();

    std::u32string buffer_;
    size_t pos_ = 0; // Java zzMarkedPos
    int lexical_state_ = YYINITIAL;
    std::string scope_identifier_;
};

} // namespace freerouting::designforms::specctra::parser
