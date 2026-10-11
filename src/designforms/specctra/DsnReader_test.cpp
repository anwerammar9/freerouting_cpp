// Tests for DsnReader (Java io.specctra.DsnReader) and the --dump-board-counts JSON (T117).
// Expected values come from the Java 2.1.0 jar (DsnReader.readBoard / readMetadata).

#include "designforms/specctra/DsnReader.h"

#include <gtest/gtest.h>

#include <fstream>
#include <sstream>
#include <string>

#include "designforms/specctra/BoardCountsDump.h"
#include "settings/RouterSettings.h"

namespace freerouting::designforms::specctra {
namespace {

std::string read_file(const std::string& p_path)
{
    std::ifstream file(p_path);
    std::stringstream text;
    text << file.rdbuf();
    return text.str();
}

constexpr const char* kNoBoundary =
    "(pcb noboundary (resolution um 10) (unit um)"
    " (structure (layer F.Cu (type signal)) (layer B.Cu (type signal))))";

TEST(DsnReaderTest, EmptyBoardIsReadSuccessfully)
{
    std::istringstream input(read_file(std::string(FREEROUTING_TEST_DATA_DIR) + "/empty_board.dsn"));
    BoardReadResult result = DsnReader::read_board(input, "/some/dir/empty_board.dsn");
    auto* success = std::get_if<board_read_result::Success>(&result);
    ASSERT_NE(nullptr, success);
    ASSERT_NE(nullptr, success->board);
    EXPECT_FALSE(success->metadata.has_value());
    EXPECT_NE(nullptr, success->board->get_outline());
    EXPECT_EQ(2, success->board->get_layer_count());
}

TEST(DsnReaderTest, MissingHeaderIsAParseError)
{
    std::istringstream input("hello (pcb x)");
    BoardReadResult result = DsnReader::read_board(input);
    auto* error = std::get_if<board_read_result::ParseError>(&result);
    ASSERT_NE(nullptr, error);
    EXPECT_EQ("(pcb", error->location);
    EXPECT_EQ("Not a Specctra DSN file: expected '(pcb <name>' header", error->detail);

    std::istringstream metadata_input("(placement)");
    EXPECT_TRUE(std::holds_alternative<board_read_result::ParseError>(DsnReader::read_metadata(metadata_input)));
}

TEST(DsnReaderTest, BoundaryLessBoardIsOutlineMissingWithoutBoard)
{
    std::istringstream input(kNoBoundary);
    BoardReadResult result = DsnReader::read_board(input);
    auto* missing = std::get_if<board_read_result::OutlineMissing>(&result);
    ASSERT_NE(nullptr, missing);
    EXPECT_EQ(nullptr, missing->board);

    std::istringstream metadata_input(kNoBoundary);
    BoardReadResult metadata_result = DsnReader::read_metadata(metadata_input);
    auto* success = std::get_if<board_read_result::Success>(&metadata_result);
    ASSERT_NE(nullptr, success);
    EXPECT_EQ(nullptr, success->board);
    ASSERT_TRUE(success->metadata.has_value());
    EXPECT_EQ(0, success->metadata->layer_count); // Java: layer structure null without a board
    EXPECT_EQ(board::Unit::UM, success->metadata->unit);
    EXPECT_EQ(10, success->metadata->resolution);
}

TEST(DsnReaderTest, MetadataStopsAfterTheStructureScope)
{
    // The stream ends right after the structure scope (no closing bracket of the pcb scope).
    std::string text = read_file(std::string(FREEROUTING_TEST_DATA_DIR) + "/empty_board.dsn");
    std::istringstream lines(text);
    std::string truncated;
    std::string line;
    for (int i = 0; i < 26 && std::getline(lines, line); ++i)
    {
        truncated += line + "\n";
    }
    std::istringstream input(truncated);
    BoardReadResult result = DsnReader::read_metadata(input);
    auto* success = std::get_if<board_read_result::Success>(&result);
    ASSERT_NE(nullptr, success);
    ASSERT_TRUE(success->metadata.has_value());
    EXPECT_EQ(2, success->metadata->layer_count);
    EXPECT_EQ(board::Unit::UM, success->metadata->unit);
    EXPECT_EQ(10, success->metadata->resolution);
    EXPECT_NE(nullptr, success->board);
    EXPECT_EQ(nullptr, success->metadata->router_settings);

    // readBoard accepts the truncated stream too (EOF ends the pcb scope).
    std::istringstream board_input(truncated);
    EXPECT_TRUE(std::holds_alternative<board_read_result::Success>(DsnReader::read_board(board_input)));
}

TEST(BoardCountsDumpTest, EmptyBoardMatchesTheJavaHarness)
{
    const std::string dsn_path = std::string(FREEROUTING_TEST_DATA_DIR) + "/empty_board.dsn";
    std::istringstream input(read_file(dsn_path));
    const std::string json = BoardCountsDump::render(DsnReader::read_board(input), "tests/data/empty_board.dsn", false);
    // Everything after the "file" line equals tests/data/expected/empty_board.dsn.counts.json.
    const std::string expected = read_file(std::string(FREEROUTING_TEST_DATA_DIR) + "/expected/empty_board.dsn.counts.json");
    EXPECT_EQ(expected.substr(expected.find("\"result\"")), json.substr(json.find("\"result\"")));
}

TEST(BoardCountsDumpTest, ParseErrorAndJsonEscaping)
{
    std::istringstream input("hello");
    const std::string json = BoardCountsDump::render(DsnReader::read_board(input), "x.dsn", false);
    EXPECT_NE(std::string::npos,
        json.find("\"error\": \"(pcb: Not a Specctra DSN file: expected '(pcb <name>' header\""));
    EXPECT_NE(std::string::npos, json.find("\"result\": \"parse_error\""));
    EXPECT_EQ("\"a\\\"b\\\\c\\n\\u0001\"", BoardCountsDump::json_string(std::string("a\"b\\c\n\x01")));
}

} // namespace
} // namespace freerouting::designforms::specctra
