// Tests for SessionWriter (Java io.specctra.SesWriter, T201/T202). The expected .ses files were
// written by the Java 2.1.0 jar with DsnReader.readBoard + SesWriter.write (no routing), see
// docs/migration/MIGRATION_designforms.md.

#include "designforms/specctra/SessionWriter.h"

#include <gtest/gtest.h>

#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <variant>

#include "designforms/specctra/DsnReader.h"

namespace freerouting::designforms::specctra {
namespace {

std::string read_file(const std::string& p_path)
{
    std::ifstream file(p_path, std::ios::binary);
    std::stringstream text;
    text << file.rdbuf();
    return text.str();
}

/** Loads tests/data/<p_dsn> and writes its session like the Java SesDump harness. */
std::string write_session_of(const std::string& p_dsn)
{
    std::ifstream input(std::string(FREEROUTING_TEST_DATA_DIR) + "/" + p_dsn, std::ios::binary);
    BoardReadResult result = DsnReader::read_board(input, p_dsn);
    auto* success = std::get_if<board_read_result::Success>(&result);
    if (success == nullptr || success->board == nullptr)
    {
        return "<load failed>";
    }
    const std::string file_name = p_dsn.substr(p_dsn.find_last_of('/') + 1);
    std::ostringstream out;
    SessionWriter::write(*success->board, out, file_name);
    if (const char* dump_dir = std::getenv("FREEROUTING_DUMP_SES"))
    {
        std::ofstream(std::string(dump_dir) + "/" + file_name + ".cpp.ses", std::ios::binary) << out.str();
    }
    return out.str();
}

void expect_equal_to_java(const std::string& p_dsn)
{
    const std::string file_name = p_dsn.substr(p_dsn.find_last_of('/') + 1);
    const std::string expected = read_file(std::string(FREEROUTING_TEST_DATA_DIR) + "/expected/" + file_name + ".ses");
    ASSERT_FALSE(expected.empty()) << "missing expected file for " << p_dsn;
    const std::string actual = write_session_of(p_dsn);
    if (expected == actual)
    {
        return;
    }
    // Report the first differing line.
    std::istringstream expected_lines(expected);
    std::istringstream actual_lines(actual);
    std::string expected_line;
    std::string actual_line;
    int line_no = 0;
    while (true)
    {
        const bool has_expected = static_cast<bool>(std::getline(expected_lines, expected_line));
        const bool has_actual = static_cast<bool>(std::getline(actual_lines, actual_line));
        ++line_no;
        if (!has_expected && !has_actual)
        {
            break;
        }
        if (!has_expected || !has_actual || expected_line != actual_line)
        {
            ADD_FAILURE() << p_dsn << " differs at line " << line_no << "\n  Java: " << expected_line
                          << "\n  C++ : " << actual_line;
            return;
        }
    }
    ADD_FAILURE() << p_dsn << " differs (line endings or trailing data)";
}

TEST(SessionWriterTest, FormatPlacementRotation)
{
    EXPECT_EQ("0", SessionWriter::format_placement_rotation(0.0));
    EXPECT_EQ("339", SessionWriter::format_placement_rotation(339.0));
    EXPECT_EQ("338.5", SessionWriter::format_placement_rotation(338.5));
    EXPECT_EQ("12.346", SessionWriter::format_placement_rotation(12.3456));
    EXPECT_EQ("90", SessionWriter::format_placement_rotation(89.99999));
}

TEST(SessionWriterTest, EmptyBoardMatchesJava) { expect_equal_to_java("empty_board.dsn"); }

TEST(SessionWriterTest, TestSenselMatchesJava) { expect_equal_to_java("Issue069-TestSensel/TestSensel.dsn"); }

TEST(SessionWriterTest, Issue029MatchesJava) { expect_equal_to_java("Issue029-hw48na.dsn"); }

TEST(SessionWriterTest, Issue103RoutedMatchesJava) { expect_equal_to_java("Issue103-Board-Routed.dsn"); }

// (host_version "") is a value, not Java null: it is written back.
TEST(SessionWriterTest, EmptyHostVersionIsWrittenLikeJava) { expect_equal_to_java("Issue420-contribution-board.dsn"); }

} // namespace
} // namespace freerouting::designforms::specctra
