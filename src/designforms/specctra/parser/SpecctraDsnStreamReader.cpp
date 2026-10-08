#include "designforms/specctra/parser/SpecctraDsnStreamReader.h"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <iterator>
#include <limits>
#include <stdexcept>

#include "designforms/specctra/parser/Keyword.h"
#include "designforms/specctra/parser/ScopeKeyword.h"
#include "logger/FRLogger.h"

namespace freerouting::designforms::specctra::parser {

namespace {

struct KeywordSpelling
{
    const char* spelling;
    const Keyword* keyword;
    int next_state;
};

// The keywords recognized in state YYINITIAL with the lexical state the rule
// switches to. Extracted from the DFA of the generated Java scanner.
const KeywordSpelling kKeywordTable[] = {
    { "absolute", Keyword::ABSOLUTE, SpecctraDsnStreamReader::YYINITIAL },
    { "active", Keyword::ACTIVE, SpecctraDsnStreamReader::YYINITIAL },
    { "against_prefered_direction_trace_costs", Keyword::AGAINST_PREFERRED_DIRECTION_TRACE_COSTS, SpecctraDsnStreamReader::YYINITIAL },
    { "against_preferred_direction_trace_costs", Keyword::AGAINST_PREFERRED_DIRECTION_TRACE_COSTS, SpecctraDsnStreamReader::YYINITIAL },
    { "attach", Keyword::ATTACH, SpecctraDsnStreamReader::YYINITIAL },
    { "autoroute", Keyword::AUTOROUTE, SpecctraDsnStreamReader::YYINITIAL },
    { "autoroute_settings", Keyword::AUTOROUTE_SETTINGS, SpecctraDsnStreamReader::YYINITIAL },
    { "back", Keyword::BACK, SpecctraDsnStreamReader::YYINITIAL },
    { "boundary", Keyword::BOUNDARY, SpecctraDsnStreamReader::YYINITIAL },
    { "circ", Keyword::CIRCLE, SpecctraDsnStreamReader::LAYER_NAME },
    { "circle", Keyword::CIRCLE, SpecctraDsnStreamReader::LAYER_NAME },
    { "circuit", Keyword::CIRCUIT, SpecctraDsnStreamReader::YYINITIAL },
    { "class", Keyword::CLASS, SpecctraDsnStreamReader::NAME },
    { "class_class", Keyword::CLASS_CLASS, SpecctraDsnStreamReader::YYINITIAL },
    { "classes", Keyword::CLASSES, SpecctraDsnStreamReader::YYINITIAL },
    { "clear", Keyword::CLEARANCE, SpecctraDsnStreamReader::YYINITIAL },
    { "clearance", Keyword::CLEARANCE, SpecctraDsnStreamReader::YYINITIAL },
    { "clearance_class", Keyword::CLEARANCE_CLASS, SpecctraDsnStreamReader::NAME },
    { "comp", Keyword::COMPONENT_SCOPE, SpecctraDsnStreamReader::NAME },
    { "component", Keyword::COMPONENT_SCOPE, SpecctraDsnStreamReader::NAME },
    { "constant", Keyword::CONSTANT, SpecctraDsnStreamReader::YYINITIAL },
    { "control", Keyword::CONTROL, SpecctraDsnStreamReader::YYINITIAL },
    { "fanout", Keyword::FANOUT, SpecctraDsnStreamReader::YYINITIAL },
    { "fix", Keyword::FIX, SpecctraDsnStreamReader::YYINITIAL },
    { "flip_style", Keyword::FLIP_STYLE, SpecctraDsnStreamReader::YYINITIAL },
    { "fortyfive_degree", Keyword::FORTYFIVE_DEGREE, SpecctraDsnStreamReader::YYINITIAL },
    { "fromto", Keyword::FROMTO, SpecctraDsnStreamReader::YYINITIAL },
    { "front", Keyword::FRONT, SpecctraDsnStreamReader::YYINITIAL },
    { "generated_by_freeroute", Keyword::GENERATED_BY_FREEROUTING, SpecctraDsnStreamReader::YYINITIAL },
    { "horizontal", Keyword::HORIZONTAL, SpecctraDsnStreamReader::YYINITIAL },
    { "host_cad", Keyword::HOST_CAD, SpecctraDsnStreamReader::NAME },
    { "host_version", Keyword::HOST_VERSION, SpecctraDsnStreamReader::NAME },
    { "image", Keyword::IMAGE, SpecctraDsnStreamReader::NAME },
    { "keepout", Keyword::KEEPOUT, SpecctraDsnStreamReader::NAME },
    { "layer", Keyword::LAYER, SpecctraDsnStreamReader::NAME },
    { "layer_rule", Keyword::LAYER_RULE, SpecctraDsnStreamReader::NAME },
    { "length", Keyword::LENGTH, SpecctraDsnStreamReader::YYINITIAL },
    { "library", Keyword::LIBRARY_SCOPE, SpecctraDsnStreamReader::YYINITIAL },
    { "lock_type", Keyword::LOCK_TYPE, SpecctraDsnStreamReader::YYINITIAL },
    { "logical_part", Keyword::LOGICAL_PART, SpecctraDsnStreamReader::NAME },
    { "logical_part_mapping", Keyword::LOGICAL_PART_MAPPING, SpecctraDsnStreamReader::NAME },
    { "net", Keyword::NET, SpecctraDsnStreamReader::NAME },
    { "network", Keyword::NETWORK_SCOPE, SpecctraDsnStreamReader::YYINITIAL },
    { "network_out", Keyword::NETWORK_OUT, SpecctraDsnStreamReader::YYINITIAL },
    { "ninety_degree", Keyword::NINETY_DEGREE, SpecctraDsnStreamReader::YYINITIAL },
    { "none", Keyword::NONE, SpecctraDsnStreamReader::YYINITIAL },
    { "normal", Keyword::NORMAL, SpecctraDsnStreamReader::YYINITIAL },
    { "off", Keyword::OFF, SpecctraDsnStreamReader::YYINITIAL },
    { "on", Keyword::ON, SpecctraDsnStreamReader::YYINITIAL },
    { "order", Keyword::ORDER, SpecctraDsnStreamReader::YYINITIAL },
    { "outline", Keyword::OUTLINE, SpecctraDsnStreamReader::YYINITIAL },
    { "padstack", Keyword::PADSTACK, SpecctraDsnStreamReader::NAME },
    { "parser", Keyword::PARSER_SCOPE, SpecctraDsnStreamReader::YYINITIAL },
    { "part_library", Keyword::PART_LIBRARY_SCOPE, SpecctraDsnStreamReader::YYINITIAL },
    { "path", Keyword::POLYGON_PATH, SpecctraDsnStreamReader::LAYER_NAME },
    { "pcb", Keyword::PCB_SCOPE, SpecctraDsnStreamReader::YYINITIAL },
    { "pin", Keyword::PIN, SpecctraDsnStreamReader::YYINITIAL },
    { "pins", Keyword::PINS, SpecctraDsnStreamReader::YYINITIAL },
    { "place", Keyword::PLACE, SpecctraDsnStreamReader::NAME },
    { "place_control", Keyword::PLACE_CONTROL, SpecctraDsnStreamReader::YYINITIAL },
    { "place_keepout", Keyword::PLACE_KEEPOUT, SpecctraDsnStreamReader::NAME },
    { "placement", Keyword::PLACEMENT_SCOPE, SpecctraDsnStreamReader::YYINITIAL },
    { "plane", Keyword::PLANE_SCOPE, SpecctraDsnStreamReader::NAME },
    { "plane_via_costs", Keyword::PLANE_VIA_COSTS, SpecctraDsnStreamReader::YYINITIAL },
    { "poly", Keyword::POLYGON, SpecctraDsnStreamReader::LAYER_NAME },
    { "polygon", Keyword::POLYGON, SpecctraDsnStreamReader::LAYER_NAME },
    { "polyline_path", Keyword::POLYLINE_PATH, SpecctraDsnStreamReader::LAYER_NAME },
    { "position", Keyword::POSITION, SpecctraDsnStreamReader::YYINITIAL },
    { "postroute", Keyword::POSTROUTE, SpecctraDsnStreamReader::YYINITIAL },
    { "power", Keyword::POWER, SpecctraDsnStreamReader::YYINITIAL },
    { "prefered_direction", Keyword::PREFERRED_DIRECTION, SpecctraDsnStreamReader::YYINITIAL },
    { "prefered_direction_trace_costs", Keyword::PREFERRED_DIRECTION_TRACE_COSTS, SpecctraDsnStreamReader::YYINITIAL },
    { "preferred_direction", Keyword::PREFERRED_DIRECTION, SpecctraDsnStreamReader::YYINITIAL },
    { "preferred_direction_trace_costs", Keyword::PREFERRED_DIRECTION_TRACE_COSTS, SpecctraDsnStreamReader::YYINITIAL },
    { "pull_tight", Keyword::PULL_TIGHT, SpecctraDsnStreamReader::YYINITIAL },
    { "rect", Keyword::RECTANGLE, SpecctraDsnStreamReader::LAYER_NAME },
    { "rectangle", Keyword::RECTANGLE, SpecctraDsnStreamReader::LAYER_NAME },
    { "resolution", Keyword::RESOLUTION_SCOPE, SpecctraDsnStreamReader::YYINITIAL },
    { "rotate", Keyword::ROTATE, SpecctraDsnStreamReader::YYINITIAL },
    { "rotate_first", Keyword::ROTATE_FIRST, SpecctraDsnStreamReader::YYINITIAL },
    { "routes", Keyword::ROUTES, SpecctraDsnStreamReader::YYINITIAL },
    { "rule", Keyword::RULE, SpecctraDsnStreamReader::YYINITIAL },
    { "rules", Keyword::RULES, SpecctraDsnStreamReader::YYINITIAL },
    { "session", Keyword::SESSION, SpecctraDsnStreamReader::YYINITIAL },
    { "shape", Keyword::SHAPE, SpecctraDsnStreamReader::YYINITIAL },
    { "shove_fixed", Keyword::SHOVE_FIXED, SpecctraDsnStreamReader::YYINITIAL },
    { "side", Keyword::SIDE, SpecctraDsnStreamReader::YYINITIAL },
    { "signal", Keyword::SIGNAL, SpecctraDsnStreamReader::YYINITIAL },
    { "snap_angle", Keyword::SNAP_ANGLE, SpecctraDsnStreamReader::YYINITIAL },
    { "spare", Keyword::SPARE, SpecctraDsnStreamReader::YYINITIAL },
    { "start_pass_no", Keyword::START_PASS_NO, SpecctraDsnStreamReader::YYINITIAL },
    { "start_ripup_costs", Keyword::START_RIPUP_COSTS, SpecctraDsnStreamReader::YYINITIAL },
    { "string_quote", Keyword::STRING_QUOTE, SpecctraDsnStreamReader::IGNORE_QUOTE },
    { "structure", Keyword::STRUCTURE_SCOPE, SpecctraDsnStreamReader::YYINITIAL },
    { "type", Keyword::TYPE, SpecctraDsnStreamReader::YYINITIAL },
    { "use_layer", Keyword::USE_LAYER, SpecctraDsnStreamReader::NAME },
    { "use_net", Keyword::USE_NET, SpecctraDsnStreamReader::NAME },
    { "use_via", Keyword::USE_VIA, SpecctraDsnStreamReader::NAME },
    { "vertical", Keyword::VERTICAL, SpecctraDsnStreamReader::YYINITIAL },
    { "via", Keyword::VIA, SpecctraDsnStreamReader::NAME },
    { "via_at_smd", Keyword::VIA_AT_SMD, SpecctraDsnStreamReader::YYINITIAL },
    { "via_costs", Keyword::VIA_COSTS, SpecctraDsnStreamReader::YYINITIAL },
    { "via_keepout", Keyword::VIA_KEEPOUT, SpecctraDsnStreamReader::NAME },
    { "via_rule", Keyword::VIA_RULE, SpecctraDsnStreamReader::YYINITIAL },
    { "vias", Keyword::VIAS, SpecctraDsnStreamReader::YYINITIAL },
    { "width", Keyword::WIDTH, SpecctraDsnStreamReader::YYINITIAL },
    { "window", Keyword::WINDOW, SpecctraDsnStreamReader::YYINITIAL },
    { "wire", Keyword::WIRE, SpecctraDsnStreamReader::NAME },
    { "wire_keepout", Keyword::KEEPOUT, SpecctraDsnStreamReader::YYINITIAL },
    { "wiring", Keyword::WIRING_SCOPE, SpecctraDsnStreamReader::YYINITIAL },
    { "write_resolution", Keyword::WRITE_RESOLUTION, SpecctraDsnStreamReader::YYINITIAL },
};

bool is_letter(char32_t c) { return (c >= U'A' && c <= U'Z') || (c >= U'a' && c <= U'z'); }
bool is_digit(char32_t c) { return c >= U'0' && c <= U'9'; }
bool is_white_space(char32_t c) { return c == U'\r' || c == U'\n' || c == U' ' || c == U'\t' || c == U'\f'; }

// SpecChar1: the ASCII special characters of the flex spec plus the non-ASCII
// characters of character class 14 of the generated Java scanner.
bool is_spec_char1(char32_t c)
{
    switch (c)
    {
    case U'_': case U'.': case U'/': case U'\\': case U':': case U'#': case U'$': case U'&': case U'>':
    case U'<': case U',': case U';': case U'=': case U'@': case U'[': case U']': case U'~': case U'*':
    case U'?': case U'!': case U'%': case U'^': case 31:
        return true;
    default:
        break;
    }
    return (c >= 0xA1 && c <= 0xFF) || c == 0x152 || c == 0x153 || c == 0x160 || c == 0x161 || c == 0x178
        || c == 0x17D || c == 0x17E || c == 0x191 || c == 0x192 || c == 0x2C6 || c == 0x2DC || c == 0x2013
        || c == 0x2014 || (c >= 0x2018 && c <= 0x201A) || (c >= 0x201C && c <= 0x201E) || (c >= 0x2020 && c <= 0x2022)
        || c == 0x2026 || c == 0x2030 || c == 0x2039 || c == 0x203A || c == 0x20AC || c == 0x2122;
}
bool is_spec_char2(char32_t c) { return is_spec_char1(c) || c == U'-' || c == U'+'; }
bool is_spec_char3(char32_t c) { return is_spec_char2(c) || c == U'"' || c == U'\''; }
bool is_spec_char4(char32_t c) { return is_spec_char1(c) || c == U'+'; }
bool is_spec_char5(char32_t c) { return is_spec_char4(c) || c == U'"' || c == U'\''; }

std::u32string decode_utf8(const std::string& p_bytes)
{
    // Like Java's UTF-8 decoder: every malformed sequence (its maximal valid
    // prefix) becomes one U+FFFD.
    std::u32string result;
    result.reserve(p_bytes.size());
    const size_t n = p_bytes.size();
    auto byte_at = [&](size_t i) { return static_cast<unsigned char>(p_bytes[i]); };
    size_t i = 0;
    while (i < n)
    {
        const unsigned char b0 = byte_at(i);
        if (b0 < 0x80)
        {
            result.push_back(b0);
            ++i;
            continue;
        }
        int length = 0;
        char32_t cp = 0;
        unsigned char lo = 0x80;
        unsigned char hi = 0xBF;
        if (b0 >= 0xC2 && b0 <= 0xDF) { length = 2; cp = b0 & 0x1F; }
        else if (b0 >= 0xE0 && b0 <= 0xEF)
        {
            length = 3;
            cp = b0 & 0x0F;
            if (b0 == 0xE0) lo = 0xA0;
            if (b0 == 0xED) hi = 0x9F;
        }
        else if (b0 >= 0xF0 && b0 <= 0xF4)
        {
            length = 4;
            cp = b0 & 0x07;
            if (b0 == 0xF0) lo = 0x90;
            if (b0 == 0xF4) hi = 0x8F;
        }
        else
        {
            result.push_back(0xFFFD);
            ++i;
            continue;
        }
        int k = 1;
        for (; k < length; ++k)
        {
            if (i + k >= n)
            {
                break;
            }
            const unsigned char b = byte_at(i + k);
            const unsigned char min = k == 1 ? lo : 0x80;
            const unsigned char max = k == 1 ? hi : 0xBF;
            if (b < min || b > max)
            {
                break;
            }
            cp = (cp << 6) | (b & 0x3F);
        }
        if (k < length)
        {
            result.push_back(0xFFFD);
            i += k;
            continue;
        }
        result.push_back(cp);
        i += length;
    }
    return result;
}

void append_utf8(std::string& p_out, char32_t c)
{
    if (c < 0x80) { p_out.push_back(static_cast<char>(c)); }
    else if (c < 0x800) { p_out.push_back(static_cast<char>(0xC0 | (c >> 6))); p_out.push_back(static_cast<char>(0x80 | (c & 0x3F))); }
    else if (c < 0x10000)
    {
        p_out.push_back(static_cast<char>(0xE0 | (c >> 12)));
        p_out.push_back(static_cast<char>(0x80 | ((c >> 6) & 0x3F)));
        p_out.push_back(static_cast<char>(0x80 | (c & 0x3F)));
    }
    else
    {
        p_out.push_back(static_cast<char>(0xF0 | (c >> 18)));
        p_out.push_back(static_cast<char>(0x80 | ((c >> 12) & 0x3F)));
        p_out.push_back(static_cast<char>(0x80 | ((c >> 6) & 0x3F)));
        p_out.push_back(static_cast<char>(0x80 | (c & 0x3F)));
    }
}

bool equals_ignore_case(const std::u32string& p_text, const char* p_ascii)
{
    size_t i = 0;
    for (; p_ascii[i] != '\0'; ++i)
    {
        if (i >= p_text.size())
        {
            return false;
        }
        const char32_t c = p_text[i];
        if (c >= 0x80 || std::tolower(static_cast<int>(c)) != std::tolower(static_cast<unsigned char>(p_ascii[i])))
        {
            return false;
        }
    }
    return i == p_text.size();
}

} // namespace

std::string Token::to_string() const
{
    switch (kind)
    {
    case Kind::END_OF_FILE: return "null";
    case Kind::KEYWORD: return keyword != nullptr ? keyword->get_name() : "?";
    case Kind::STRING: return string_value;
    case Kind::INTEGER: return std::to_string(int_value);
    case Kind::DOUBLE: return std::to_string(double_value);
    }
    return "?";
}

SpecctraDsnStreamReader::SpecctraDsnStreamReader(std::istream& p_input)
{
    const std::string bytes((std::istreambuf_iterator<char>(p_input)), std::istreambuf_iterator<char>());
    buffer_ = decode_utf8(bytes);
}

SpecctraDsnStreamReader::SpecctraDsnStreamReader(const std::string& p_text, bool /*p_is_text*/)
    : buffer_(decode_utf8(p_text))
{
}

std::string SpecctraDsnStreamReader::text(size_t p_start, size_t p_end) const
{
    std::string result;
    for (size_t i = p_start; i < p_end; ++i)
    {
        append_utf8(result, buffer_[i]);
    }
    return result;
}

void SpecctraDsnStreamReader::skip_illegal_character()
{
    // The fallback rule ".|\n" of the shipped Java scanner (action 2) logs the
    // character and skips it; the 2008 flex file throws an Error instead.
    std::string ch;
    append_utf8(ch, char_at(pos_));
    logger::FRLogger::instance().warn("Non-ansi character '" + ch + "' found at position " + std::to_string(pos_) + ".");
    ++pos_;
}

Token SpecctraDsnStreamReader::read_quoted_string(char32_t p_quote, size_t p_start)
{
    // States STRING1 / STRING2: everything from p_start up to the closing
    // quote (a backslash is appended as a normal character).
    size_t end = p_start;
    while (end < buffer_.size() && buffer_[end] != p_quote)
    {
        ++end;
    }
    if (end >= buffer_.size())
    {
        // JFlex returns null at the end of the input inside a string.
        pos_ = buffer_.size();
        lexical_state_ = YYINITIAL;
        return Token::end_of_file();
    }
    std::string value = text(p_start, end);
    pos_ = end + 1;
    lexical_state_ = YYINITIAL;
    return Token::of_string(std::move(value));
}

Token SpecctraDsnStreamReader::next_token()
{
    for (;;)
    {
        if (pos_ >= buffer_.size())
        {
            return Token::end_of_file();
        }
        const char32_t c = buffer_[pos_];
        switch (lexical_state_)
        {
        case STRING1:
            return read_quoted_string(U'"', pos_);
        case STRING2:
            return read_quoted_string(U'\'', pos_);
        case SPEC_CHAR:
            if (is_spec_char2(c))
            {
                ++pos_;
                std::string value;
                append_utf8(value, c);
                return Token::of_string(std::move(value));
            }
            skip_illegal_character();
            continue;
        case NAME:
        case LAYER_NAME:
        case COMPONENT_NAME:
        case IGNORE_QUOTE:
        {
            if (is_white_space(c))
            {
                ++pos_;
                continue;
            }
            if (c == U'(' || c == U')')
            {
                ++pos_;
                lexical_state_ = YYINITIAL;
                return Token::of_keyword(c == U'(' ? Keyword::OPEN_BRACKET : Keyword::CLOSED_BRACKET);
            }
            if (lexical_state_ != IGNORE_QUOTE && (c == U'"' || c == U'\''))
            {
                return read_quoted_string(c, pos_ + 1);
            }
            size_t end = pos_;
            if (lexical_state_ == COMPONENT_NAME)
            {
                if (is_letter(c) || is_digit(c) || is_spec_char4(c))
                {
                    end = pos_ + 1;
                    while (end < buffer_.size()
                        && (is_letter(buffer_[end]) || is_digit(buffer_[end]) || is_spec_char5(buffer_[end])))
                    {
                        ++end;
                    }
                }
            }
            else if (lexical_state_ == IGNORE_QUOTE)
            {
                while (end < buffer_.size()
                    && (is_letter(buffer_[end]) || is_digit(buffer_[end]) || is_spec_char3(buffer_[end])))
                {
                    ++end;
                }
            }
            else if (is_letter(c) || is_digit(c) || is_spec_char2(c))
            {
                end = pos_ + 1;
                while (end < buffer_.size()
                    && (is_letter(buffer_[end]) || is_digit(buffer_[end]) || is_spec_char3(buffer_[end])))
                {
                    ++end;
                }
            }
            if (end == pos_)
            {
                skip_illegal_character();
                continue;
            }
            const std::u32string word = buffer_.substr(pos_, end - pos_);
            const int state = lexical_state_;
            pos_ = end;
            lexical_state_ = YYINITIAL;
            if (state == LAYER_NAME)
            {
                if (equals_ignore_case(word, "pcb"))
                {
                    return Token::of_keyword(Keyword::PCB_SCOPE);
                }
                if (equals_ignore_case(word, "signal"))
                {
                    return Token::of_keyword(Keyword::SIGNAL);
                }
            }
            std::string value;
            for (char32_t ch : word)
            {
                append_utf8(value, ch);
            }
            return Token::of_string(std::move(value));
        }
        default:
            break;
        }

        // YYINITIAL: longest match of the rules, ties resolved by rule order
        // (keyword, bracket, identifier, quote, integer, float, comment, white space).
        if (c == U'(' || c == U')')
        {
            ++pos_;
            return Token::of_keyword(c == U'(' ? Keyword::OPEN_BRACKET : Keyword::CLOSED_BRACKET);
        }
        if (c == U'"' || c == U'\'')
        {
            return read_quoted_string(c, pos_ + 1);
        }
        if (is_white_space(c))
        {
            ++pos_;
            continue;
        }
        const size_t size = buffer_.size();
        // Identifier = ({Letter}|{SpecChar1})({Letter}|{Digit}|{SpecChar3})*
        size_t ident_len = 0;
        if (is_letter(c) || is_spec_char1(c))
        {
            ident_len = 1;
            while (pos_ + ident_len < size
                && (is_letter(buffer_[pos_ + ident_len]) || is_digit(buffer_[pos_ + ident_len])
                    || is_spec_char3(buffer_[pos_ + ident_len])))
            {
                ++ident_len;
            }
        }
        // DecIntegerLiteral = [+-]? (0 | [1-9][0-9]*)
        size_t int_len = 0;
        {
            size_t p = pos_;
            if (char_at(p) == U'+' || char_at(p) == U'-')
            {
                ++p;
            }
            if (char_at(p) == U'0')
            {
                int_len = p + 1 - pos_;
            }
            else if (char_at(p) >= U'1' && char_at(p) <= U'9')
            {
                ++p;
                while (is_digit(char_at(p)))
                {
                    ++p;
                }
                int_len = p - pos_;
            }
        }
        // DecFloatLiteral = [+-]? [0-9]+ ("." [0-9]+)? ([Ee] {DecIntegerLiteral})?
        size_t float_len = 0;
        {
            size_t p = pos_;
            if (char_at(p) == U'+' || char_at(p) == U'-')
            {
                ++p;
            }
            if (is_digit(char_at(p)))
            {
                while (is_digit(char_at(p)))
                {
                    ++p;
                }
                if (char_at(p) == U'.' && is_digit(char_at(p + 1)))
                {
                    p += 1;
                    while (is_digit(char_at(p)))
                    {
                        ++p;
                    }
                }
                if (char_at(p) == U'e' || char_at(p) == U'E')
                {
                    size_t q = p + 1;
                    if (char_at(q) == U'+' || char_at(q) == U'-')
                    {
                        ++q;
                    }
                    if (char_at(q) == U'0')
                    {
                        p = q + 1;
                    }
                    else if (char_at(q) >= U'1' && char_at(q) <= U'9')
                    {
                        ++q;
                        while (is_digit(char_at(q)))
                        {
                            ++q;
                        }
                        p = q;
                    }
                }
                float_len = p - pos_;
            }
        }
        // Comment = TraditionalComment | EndOfLineComment
        size_t comment_len = 0;
        if (c == U'/' && char_at(pos_ + 1) == U'*')
        {
            // "/*" [^*] ~"*/"
            if (pos_ + 2 < size && buffer_[pos_ + 2] != U'*')
            {
                for (size_t p = pos_ + 3; p + 1 < size; ++p)
                {
                    if (buffer_[p] == U'*' && buffer_[p + 1] == U'/')
                    {
                        comment_len = p + 2 - pos_;
                        break;
                    }
                }
            }
            // "/*" "*"+ "/"
            size_t p = pos_ + 2;
            while (p < size && buffer_[p] == U'*')
            {
                ++p;
            }
            if (p > pos_ + 2 && p < size && buffer_[p] == U'/')
            {
                comment_len = std::max(comment_len, p + 1 - pos_);
            }
        }
        else if (c == U'#')
        {
            // "#" {InputCharacter}* {LineTerminator}
            size_t p = pos_ + 1;
            while (p < size && buffer_[p] != U'\r' && buffer_[p] != U'\n')
            {
                ++p;
            }
            if (p < size)
            {
                p += (buffer_[p] == U'\r' && char_at(p + 1) == U'\n') ? 2 : 1;
                comment_len = p - pos_;
            }
        }

        const size_t best = std::max({ ident_len, int_len, float_len, comment_len });
        if (best == 0)
        {
            skip_illegal_character();
            continue;
        }
        if (ident_len == best)
        {
            const std::u32string word = buffer_.substr(pos_, ident_len);
            pos_ += ident_len;
            for (const KeywordSpelling& entry : kKeywordTable)
            {
                if (equals_ignore_case(word, entry.spelling))
                {
                    lexical_state_ = entry.next_state;
                    return Token::of_keyword(entry.keyword);
                }
            }
            std::string value;
            for (char32_t ch : word)
            {
                append_utf8(value, ch);
            }
            return Token::of_string(std::move(value));
        }
        if (int_len == best)
        {
            const std::string digits = text(pos_, pos_ + int_len);
            pos_ += int_len;
            errno = 0;
            const long long value = std::strtoll(digits.c_str(), nullptr, 10);
            if (errno == ERANGE || value > std::numeric_limits<int>::max() || value < std::numeric_limits<int>::min())
            {
                // Java: new Integer(text) throws NumberFormatException
                throw std::runtime_error("For input string: \"" + digits + "\"");
            }
            return Token::of_int(static_cast<int>(value));
        }
        if (float_len == best)
        {
            const std::string digits = text(pos_, pos_ + float_len);
            pos_ += float_len;
            return Token::of_double(std::strtod(digits.c_str(), nullptr));
        }
        // comment: ignore
        pos_ += comment_len;
    }
}

std::string SpecctraDsnStreamReader::next_string(bool p_ignore_newline, char32_t p_leading)
{
    // Port of the hand-written Java nextString(boolean, char), which reads the
    // raw character buffer. Java bounds the loops by the buffer capacity
    // (16M chars, padded with '\0'); here they stop at the end of the input.
    // Java quirk kept: the skipped "trailing" characters are 8 (backspace) and
    // 32, not the tab character.
    std::string result;
    size_t i = 0;
    auto skip_leading = [&](char32_t ch) {
        if (ch == 8 || ch == 32 || ch == p_leading)
        {
            return true;
        }
        return p_ignore_newline && (ch == 10 || ch == 13);
    };
    while (pos_ + i < buffer_.size() && skip_leading(buffer_[pos_ + i]))
    {
        ++i;
    }
    bool skip_last_char = false;
    bool stop_at_quotes = false;
    if (char_at(pos_ + i) == 34)
    {
        stop_at_quotes = true;
        ++i;
        skip_last_char = true;
    }
    auto stop_at = [&](char32_t ch) {
        if (stop_at_quotes)
        {
            return ch == 10 || ch == 13 || ch == 34;
        }
        return ch == 8 || ch == 10 || ch == 13 || ch == 32 || ch == 40 || ch == 41 || ch == p_leading;
    };
    while (pos_ + i < buffer_.size() && !stop_at(buffer_[pos_ + i]))
    {
        append_utf8(result, buffer_[pos_ + i]);
        ++i;
    }
    if (skip_last_char)
    {
        ++i;
    }
    if (i > 0)
    {
        pos_ += i;
        lexical_state_ = YYINITIAL;
        if (char_at(pos_ - 1) == 40 || char_at(pos_ - 1) == 41)
        {
            // the string ended with a bracket, which is the next token
            --pos_;
        }
    }
    return result;
}

std::vector<std::string> SpecctraDsnStreamReader::next_string_list(char p_separator)
{
    // Every string list has at least one item, but the first item may be
    // empty (KiCad 8 starts the net list with ""): it is ignored then.
    std::vector<std::string> result;
    std::string first_string = next_string(true, static_cast<char32_t>(static_cast<unsigned char>(p_separator)));
    if (!first_string.empty())
    {
        result.push_back(std::move(first_string));
    }
    for (;;)
    {
        std::string next = next_string(true, static_cast<char32_t>(static_cast<unsigned char>(p_separator)));
        if (next.empty())
        {
            break;
        }
        result.push_back(std::move(next));
    }
    return result;
}

std::optional<double> SpecctraDsnStreamReader::next_double()
{
    // Java NumberFormat.getInstance(Locale.US).parse(s): parses the longest
    // numeric prefix, with ',' grouping separators.
    const std::string s = next_string();
    size_t i = 0;
    bool negative = false;
    if (i < s.size() && s[i] == '-')
    {
        negative = true;
        ++i;
    }
    std::string number;
    bool has_digits = false;
    while (i < s.size() && (std::isdigit(static_cast<unsigned char>(s[i])) || s[i] == ','))
    {
        if (s[i] != ',')
        {
            number.push_back(s[i]);
            has_digits = true;
        }
        ++i;
    }
    if (i < s.size() && s[i] == '.')
    {
        number.push_back('.');
        ++i;
        while (i < s.size() && std::isdigit(static_cast<unsigned char>(s[i])))
        {
            number.push_back(s[i]);
            has_digits = true;
            ++i;
        }
    }
    if (!has_digits)
    {
        return std::nullopt;
    }
    const double value = std::strtod(number.c_str(), nullptr);
    return negative ? -value : value;
}

bool SpecctraDsnStreamReader::next_closing_bracket()
{
    Token next = next_token();
    if (next.is_eof())
    {
        logger::FRLogger::instance().warn("Network.read_net_pins: unexpected end of file");
        return false;
    }
    if (!next.is(Keyword::CLOSED_BRACKET))
    {
        logger::FRLogger::instance().warn("Network.read_net_pins: expected closed bracket is missing");
        return false;
    }
    return true;
}

} // namespace freerouting::designforms::specctra::parser
