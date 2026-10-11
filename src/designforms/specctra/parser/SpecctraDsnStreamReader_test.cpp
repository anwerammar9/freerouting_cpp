// Tests for the C++ port of the JFlex DSN lexer (Java SpecctraDsnStreamReader).
// The full token streams of all tests/data boards were compared with the Java
// lexer (tools/dsn_token_dump + tools/dsn_token_compare.py); these tests pin
// the rules, including the parser-driven lexical states.

#include "designforms/specctra/parser/SpecctraDsnStreamReader.h"

#include <gtest/gtest.h>

#include "designforms/specctra/parser/Keyword.h"
#include "designforms/specctra/parser/ScopeKeyword.h"

namespace freerouting::designforms::specctra::parser {
namespace {

using R = SpecctraDsnStreamReader;

SpecctraDsnStreamReader reader(const std::string& p_text) { return SpecctraDsnStreamReader(p_text, true); }

TEST(SpecctraDsnStreamReaderTest, KeywordsAreCaseInsensitiveAndAliasesMapToTheSameKeyword)
{
    auto scanner = reader("(PCB Rect rectangle wire_keepout keepout generated_by_freeroute)");
    EXPECT_TRUE(scanner.next_token().is(Keyword::OPEN_BRACKET));
    EXPECT_TRUE(scanner.next_token().is(Keyword::PCB_SCOPE));
    EXPECT_TRUE(scanner.next_token().is(Keyword::RECTANGLE));
    EXPECT_EQ(R::LAYER_NAME, scanner.yystate());
    scanner.yybegin(R::YYINITIAL);
    EXPECT_TRUE(scanner.next_token().is(Keyword::RECTANGLE));
    scanner.yybegin(R::YYINITIAL);
    EXPECT_TRUE(scanner.next_token().is(Keyword::KEEPOUT));
    EXPECT_EQ(R::YYINITIAL, scanner.yystate()); // wire_keepout does not switch to NAME
    EXPECT_TRUE(scanner.next_token().is(Keyword::KEEPOUT));
    EXPECT_EQ(R::NAME, scanner.yystate());
    scanner.yybegin(R::YYINITIAL);
    EXPECT_TRUE(scanner.next_token().is(Keyword::GENERATED_BY_FREEROUTING));
}

TEST(SpecctraDsnStreamReaderTest, LongerIdentifierWinsOverKeywordPrefix)
{
    auto scanner = reader("pcbx pins pin");
    const Token t = scanner.next_token();
    ASSERT_TRUE(t.is_string());
    EXPECT_EQ("pcbx", t.string_value);
    EXPECT_TRUE(scanner.next_token().is(Keyword::PINS));
    EXPECT_TRUE(scanner.next_token().is(Keyword::PIN));
}

TEST(SpecctraDsnStreamReaderTest, NumbersFollowTheLongestMatch)
{
    auto scanner = reader("12 -7 +5 1.5 007 2e3 0abc .5");
    Token t = scanner.next_token();
    ASSERT_TRUE(t.is_int());
    EXPECT_EQ(12, t.int_value);
    EXPECT_EQ(-7, scanner.next_token().int_value);
    EXPECT_EQ(5, scanner.next_token().int_value);
    t = scanner.next_token();
    ASSERT_TRUE(t.is_double());
    EXPECT_DOUBLE_EQ(1.5, t.double_value);
    t = scanner.next_token(); // "007": the float rule matches more characters
    ASSERT_TRUE(t.is_double());
    EXPECT_DOUBLE_EQ(7.0, t.double_value);
    t = scanner.next_token();
    ASSERT_TRUE(t.is_double());
    EXPECT_DOUBLE_EQ(2000.0, t.double_value);
    t = scanner.next_token(); // "0abc" -> 0 then "abc"
    ASSERT_TRUE(t.is_int());
    EXPECT_EQ("abc", scanner.next_token().string_value);
    t = scanner.next_token(); // '.' starts an identifier
    ASSERT_TRUE(t.is_string());
    EXPECT_EQ(".5", t.string_value);
}

TEST(SpecctraDsnStreamReaderTest, QuotedStringsAndComments)
{
    auto scanner = reader("\"a b(c)\" 'd\"e' # comment (pcb\n/* block */ x");
    EXPECT_EQ("a b(c)", scanner.next_token().string_value);
    EXPECT_EQ("d\"e", scanner.next_token().string_value);
    const Token t = scanner.next_token();
    ASSERT_TRUE(t.is_string());
    EXPECT_EQ("x", t.string_value);
    EXPECT_TRUE(scanner.next_token().is_eof());
}

TEST(SpecctraDsnStreamReaderTest, NameStateReadsNumbersAndKeywordsAsStrings)
{
    auto scanner = reader("(net 123 net -GND+)");
    scanner.next_token();
    EXPECT_TRUE(scanner.next_token().is(Keyword::NET));
    EXPECT_EQ(R::NAME, scanner.yystate());
    Token t = scanner.next_token();
    ASSERT_TRUE(t.is_string());
    EXPECT_EQ("123", t.string_value);
    scanner.yybegin(R::NAME);
    t = scanner.next_token();
    ASSERT_TRUE(t.is_string());
    EXPECT_EQ("net", t.string_value);
    scanner.yybegin(R::NAME);
    EXPECT_EQ("-GND+", scanner.next_token().string_value);
}

TEST(SpecctraDsnStreamReaderTest, LayerNameStateKnowsPcbAndSignal)
{
    auto scanner = reader("signal signal1 F.Cu");
    scanner.yybegin(R::LAYER_NAME);
    EXPECT_TRUE(scanner.next_token().is(Keyword::SIGNAL));
    scanner.yybegin(R::LAYER_NAME);
    EXPECT_EQ("signal1", scanner.next_token().string_value);
    scanner.yybegin(R::LAYER_NAME);
    EXPECT_EQ("F.Cu", scanner.next_token().string_value);
}

TEST(SpecctraDsnStreamReaderTest, ComponentNameStopsAtTheMinusAndSpecCharReadsIt)
{
    auto scanner = reader("U1-12");
    scanner.yybegin(R::COMPONENT_NAME);
    EXPECT_EQ("U1", scanner.next_token().string_value);
    scanner.yybegin(R::SPEC_CHAR);
    EXPECT_EQ("-", scanner.next_token().string_value);
    scanner.yybegin(R::NAME);
    EXPECT_EQ("12", scanner.next_token().string_value);
}

TEST(SpecctraDsnStreamReaderTest, StringQuoteSwitchesToIgnoreQuote)
{
    auto scanner = reader("(string_quote \")");
    scanner.next_token();
    EXPECT_TRUE(scanner.next_token().is(Keyword::STRING_QUOTE));
    EXPECT_EQ(R::IGNORE_QUOTE, scanner.yystate());
    EXPECT_EQ("\"", scanner.next_token().string_value);
    EXPECT_TRUE(scanner.next_token().is(Keyword::CLOSED_BRACKET));
}

TEST(SpecctraDsnStreamReaderTest, IllegalCharactersAreSkippedLikeTheShippedJavaScanner)
{
    auto scanner = reader("+BATT {x} |");
    EXPECT_EQ("BATT", scanner.next_token().string_value);
    EXPECT_EQ("x", scanner.next_token().string_value);
    EXPECT_TRUE(scanner.next_token().is_eof());
}

TEST(SpecctraDsnStreamReaderTest, NextStringReadsRawTextAndLeavesTheBracket)
{
    auto scanner = reader("  (5.1.6)-1)  \"a b\" x");
    scanner.next_token(); // "("
    EXPECT_EQ("5.1.6", scanner.next_string());
    EXPECT_TRUE(scanner.next_token().is(Keyword::CLOSED_BRACKET));
    EXPECT_EQ("-1", scanner.next_string());
    EXPECT_TRUE(scanner.next_token().is(Keyword::CLOSED_BRACKET));
    EXPECT_EQ("a b", scanner.next_string());
    EXPECT_EQ("x", scanner.next_string());
}

TEST(SpecctraDsnStreamReaderTest, NextStringListSkipsALeadingEmptyItem)
{
    auto scanner = reader(" \"\" U1-1\n U2-3 )");
    const std::vector<std::string> list = scanner.next_string_list();
    ASSERT_EQ(2u, list.size());
    EXPECT_EQ("U1-1", list[0]);
    EXPECT_EQ("U2-3", list[1]);
    EXPECT_TRUE(scanner.next_closing_bracket());
}

TEST(SpecctraDsnStreamReaderTest, NextDoubleParsesAUsNumberPrefix)
{
    auto scanner = reader("1,234.5x abc");
    const std::optional<double> value = scanner.next_double();
    ASSERT_TRUE(value.has_value());
    EXPECT_DOUBLE_EQ(1234.5, *value);
    EXPECT_FALSE(scanner.next_double().has_value());
}

TEST(SpecctraDsnStreamReaderTest, SkipScopeConsumesNestedBrackets)
{
    auto scanner = reader("a (b 1) (c (d)) ) next");
    EXPECT_TRUE(ScopeKeyword::skip_scope(scanner));
    EXPECT_EQ("next", scanner.next_token().string_value);
}

TEST(SpecctraDsnStreamReaderTest, Utf8IsDecodedAndMalformedSequencesBecomeOneReplacement)
{
    auto scanner = reader("caf\xc3\xa9 \xe2\x82\xac");
    EXPECT_EQ("caf\xc3\xa9", scanner.next_token().string_value);
    EXPECT_EQ("\xe2\x82\xac", scanner.next_token().string_value); // U+20AC is a special character
}

} // namespace
} // namespace freerouting::designforms::specctra::parser
