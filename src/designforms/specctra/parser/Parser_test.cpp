// Tests for the DSN parser scope reader/writer (Java parser.Parser).

#include "designforms/specctra/parser/Parser.h"

#include <gtest/gtest.h>

#include <array>
#include <sstream>
#include <string>
#include <vector>

#include "datastructures/IdentifierType.h"
#include "datastructures/IndentFileWriter.h"
#include "designforms/specctra/parser/Keyword.h"
#include "designforms/specctra/parser/ReadScopeParameter.h"
#include "designforms/specctra/parser/ScopeKeyword.h"
#include "designforms/specctra/parser/SpecctraDsnStreamReader.h"
#include "designforms/specctra/parser/Structure.h"

namespace freerouting::designforms::specctra::parser {
namespace {

TEST(ParserTest, ReadScopeFillsParserFieldsAndSwitchesQuote)
{
    // read_scope is dispatched right after the `parser` keyword token, so the
    // scanner is positioned at the scope content.
    SpecctraDsnStreamReader scanner(
        "(string_quote ') (host_cad \"KiCad's Pcbnew\") (host_version \"(5.1.5)\")"
        " (constant A 1) (write_resolution D 1000))",
        true);
    ReadScopeParameter par(scanner);
    ASSERT_TRUE(Keyword::PARSER_SCOPE->read_scope(par));

    EXPECT_EQ("'", par.string_quote);
    EXPECT_EQ("KiCad's Pcbnew", par.host_cad);
    EXPECT_EQ("(5.1.5)", par.host_version);
    ASSERT_EQ(1u, par.constants.size());
    EXPECT_EQ("A", par.constants[0][0]);
    EXPECT_EQ("1", par.constants[0][1]);
    ASSERT_TRUE(par.write_resolution.has_value());
    EXPECT_EQ("D", par.write_resolution->char_name);
    EXPECT_EQ(1000, par.write_resolution->positive_int);
    EXPECT_TRUE(par.dsn_file_generated_by_host);
}

TEST(ParserTest, GeneratedByFreeroutingClearsHostFlag)
{
    SpecctraDsnStreamReader scanner("(generated_by_freeroute))", true);
    ReadScopeParameter par(scanner);
    ASSERT_TRUE(Keyword::PARSER_SCOPE->read_scope(par));
    EXPECT_FALSE(par.dsn_file_generated_by_host);
}

TEST(ParserTest, ReadScopeSkipsUnknownScopes)
{
    SpecctraDsnStreamReader scanner("(space_in_quoted_tokens on) (unknown 42))", true);
    ReadScopeParameter par(scanner);
    ASSERT_TRUE(Keyword::PARSER_SCOPE->read_scope(par));
    EXPECT_TRUE(par.constants.empty());
    EXPECT_FALSE(par.write_resolution.has_value());
}

TEST(ParserTest, ReadQuoteCharFailureReturnsFalse)
{
    SpecctraDsnStreamReader scanner("(string_quote ))", true);
    ReadScopeParameter par(scanner);
    // The scanner reads ')' as a keyword, not a string, so read_quote_char fails.
    EXPECT_FALSE(Keyword::PARSER_SCOPE->read_scope(par));
}

TEST(ParserTest, WriteScopeMatchesJavaFull)
{
    const std::string expected =
        "\n(parser\n"
        "  (string_quote \")\n"
        "  (space_in_quoted_tokens on)\n"
        "  (host_cad \"KiCad's Pcbnew\")\n"
        "  (host_version \"(5.1.5)\")\n"
        "  (constant A \"1\" )\n"
        "  (write_resolution D 1000)\n"
        "  (generated_by_freerouting)\n"
        ")";

    board::Communication::SpecctraParserInfo info;
    info.string_quote = "\"";
    info.host_cad = "KiCad's Pcbnew";
    info.host_version = "(5.1.5)";
    info.constants = { std::array<std::string, 2>{ "A", "1" } };
    info.write_resolution = board::Communication::WriteResolution{ "D", 1000 };
    const datastructures::IdentifierType identifier_type(
        { "(", ")", " ", ";", "-", "_" }, info.string_quote);

    std::ostringstream out;
    datastructures::IndentFileWriter file(out);
    Parser::write_scope(file, info, identifier_type, false);
    EXPECT_EQ(expected, out.str());
}

TEST(ParserTest, WriteScopeMatchesJavaMinimal)
{
    const std::string expected =
        "\n(parser\n"
        "  (string_quote ')\n"
        "  (space_in_quoted_tokens on)\n"
        "  (generated_by_freerouting)\n"
        ")";

    board::Communication::SpecctraParserInfo info;
    info.string_quote = "'";
    const datastructures::IdentifierType identifier_type(
        { "(", ")", " ", ";", "-", "_" }, info.string_quote);

    std::ostringstream out;
    datastructures::IndentFileWriter file(out);
    Parser::write_scope(file, info, identifier_type, false);
    EXPECT_EQ(expected, out.str());
}

TEST(ParserTest, WriteScopeReducedOmitsRedundantScopes)
{
    const std::string expected =
        "\n(parser\n"
        "  (host_cad \"KiCad's Pcbnew\")\n"
        "  (host_version \"(5.1.5)\")\n"
        "  (constant A \"1\" )\n"
        "  (write_resolution D 1000)\n"
        ")";

    board::Communication::SpecctraParserInfo info;
    info.string_quote = "\"";
    info.host_cad = "KiCad's Pcbnew";
    info.host_version = "(5.1.5)";
    info.constants = { std::array<std::string, 2>{ "A", "1" } };
    info.write_resolution = board::Communication::WriteResolution{ "D", 1000 };
    const datastructures::IdentifierType identifier_type(
        { "(", ")", " ", ";", "-", "_" }, info.string_quote);

    std::ostringstream out;
    datastructures::IndentFileWriter file(out);
    Parser::write_scope(file, info, identifier_type, true);
    EXPECT_EQ(expected, out.str());
}

TEST(ParserTest, HostCadAllegroSkipsPlaneWindows)
{
    const std::string structure_with_window =
        "(parser (host_cad Allegro))"
        "(structure (layer F.Cu (type signal)) (layer B.Cu (type signal))"
        " (boundary (rect pcb 0 0 1000 1000))"
        " (plane GND (rect F.Cu 0 0 100 100) (window (rect F.Cu 10 10 20 20))))";
    SpecctraDsnStreamReader scanner(structure_with_window, true);
    ReadScopeParameter par(scanner);
    ASSERT_TRUE(Keyword::PCB_SCOPE->read_scope(par));
    EXPECT_EQ("Allegro", par.host_cad);
    ASSERT_EQ(1u, par.plane_list.size());
    // Cadence Allegro cuts out the pins on power planes; the window is skipped.
    EXPECT_EQ(1u, par.plane_list[0].area.shape_list.size());
}

TEST(ParserTest, NonAllegroHostCadReadsPlaneWindows)
{
    const std::string structure_with_window =
        "(parser (host_cad KiCad))"
        "(structure (layer F.Cu (type signal)) (layer B.Cu (type signal))"
        " (boundary (rect pcb 0 0 1000 1000))"
        " (plane GND (rect F.Cu 0 0 100 100) (window (rect F.Cu 10 10 20 20))))";
    SpecctraDsnStreamReader scanner(structure_with_window, true);
    ReadScopeParameter par(scanner);
    ASSERT_TRUE(Keyword::PCB_SCOPE->read_scope(par));
    EXPECT_EQ("KiCad", par.host_cad);
    ASSERT_EQ(1u, par.plane_list.size());
    // A non-Allegro host keeps the window shapes as cutouts.
    EXPECT_EQ(2u, par.plane_list[0].area.shape_list.size());
}

} // namespace
} // namespace freerouting::designforms::specctra::parser