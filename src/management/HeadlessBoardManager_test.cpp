#include "management/HeadlessBoardManager.h"

#include <gtest/gtest.h>

#include <fstream>
#include <sstream>
#include <variant>

#include "board/RoutingBoard.h"
#include "core/RoutingJob.h"

namespace freerouting::management {
namespace {

std::string read_file(const std::string& p_path)
{
    std::ifstream file(p_path, std::ios::binary);
    std::stringstream text;
    text << file.rdbuf();
    return text.str();
}

TEST(HeadlessBoardManagerTest, LoadsADsnAndTakesTheBoardOver)
{
    core::RoutingJob job;
    HeadlessBoardManager manager(&job);
    EXPECT_EQ(&job, manager.get_current_routing_job());
    EXPECT_EQ(nullptr, manager.get_routing_board());

    std::ifstream input(std::string(FREEROUTING_TEST_DATA_DIR) + "/empty_board.dsn", std::ios::binary);
    designforms::specctra::BoardReadResult result = manager.load_from_specctra_dsn(input, "empty_board.dsn");
    EXPECT_TRUE(std::holds_alternative<designforms::specctra::board_read_result::Success>(result));
    ASSERT_NE(nullptr, manager.get_routing_board());
    EXPECT_EQ(2, manager.get_routing_board()->get_layer_count());
    // The result's board was moved into the manager.
    EXPECT_EQ(nullptr, std::get<designforms::specctra::board_read_result::Success>(result).board);
}

TEST(HeadlessBoardManagerTest, ParseErrorLeavesNoBoard)
{
    HeadlessBoardManager manager(nullptr);
    std::istringstream input("not a dsn");
    designforms::specctra::BoardReadResult result = manager.load_from_specctra_dsn(input);
    EXPECT_TRUE(std::holds_alternative<designforms::specctra::board_read_result::ParseError>(result));
    EXPECT_EQ(nullptr, manager.get_routing_board());
    std::ostringstream out;
    EXPECT_FALSE(manager.save_as_specctra_session_ses(out, "x"));
}

TEST(HeadlessBoardManagerTest, SavesTheSessionLikeJava)
{
    HeadlessBoardManager manager(nullptr);
    std::ifstream input(std::string(FREEROUTING_TEST_DATA_DIR) + "/empty_board.dsn", std::ios::binary);
    manager.load_from_specctra_dsn(input, "empty_board.dsn");
    std::ostringstream out;
    ASSERT_TRUE(manager.save_as_specctra_session_ses(out, "empty_board.dsn"));
    EXPECT_EQ(read_file(std::string(FREEROUTING_TEST_DATA_DIR) + "/expected/empty_board.dsn.ses"), out.str());
}

TEST(HeadlessBoardManagerTest, ReplaceRoutingBoard)
{
    HeadlessBoardManager manager(nullptr);
    std::ifstream input(std::string(FREEROUTING_TEST_DATA_DIR) + "/empty_board.dsn", std::ios::binary);
    manager.load_from_specctra_dsn(input);
    std::shared_ptr<board::RoutingBoard> board = manager.get_shared_routing_board();
    manager.replace_routing_board(nullptr);
    EXPECT_EQ(nullptr, manager.get_routing_board());
    manager.replace_routing_board(board);
    EXPECT_EQ(board.get(), manager.get_routing_board());
}

} // namespace
} // namespace freerouting::management
