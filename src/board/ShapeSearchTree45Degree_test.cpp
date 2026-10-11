// Tests for ShapeSearchTree45Degree (Java board.searchtree.ShapeSearchTree45Degree, T506a).
// The golden rooms in tests/data/expected/Issue026-J2_reference.dsn.tree45.txt were produced by a
// throwaway Java program calling completeShape on the Java autoroute tree of the same board
// (see docs/migration/MIGRATION_board.md, T506a).

#include "board/ShapeSearchTree.h"

#include <gtest/gtest.h>

#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <variant>
#include <vector>

#include "board/RoutingBoard.h"
#include "board/SearchTreeManager.h"
#include "designforms/specctra/DsnReader.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/IntOctagon.h"

namespace freerouting::board {
namespace {

namespace fp = geometry::planar;

std::unique_ptr<RoutingBoard> load_board(const std::string& p_dsn)
{
    std::ifstream input(std::string(FREEROUTING_TEST_DATA_DIR) + "/" + p_dsn, std::ios::binary);
    designforms::specctra::BoardReadResult result = designforms::specctra::DsnReader::read_board(input, p_dsn);
    auto* success = std::get_if<designforms::specctra::board_read_result::Success>(&result);
    return success != nullptr ? std::move(success->board) : nullptr;
}

std::string octagon_text(const fp::IntOctagon& p_oct)
{
    std::ostringstream out;
    out << p_oct.left << ' ' << p_oct.bottom << ' ' << p_oct.right << ' ' << p_oct.top << ' ' << p_oct.left_top << ' '
        << p_oct.right_bottom << ' ' << p_oct.left_bottom << ' ' << p_oct.right_top;
    return out.str();
}

TEST(ShapeSearchTree45DegreeTest, RestrainedShapesCutTheRoomAtAnObstacleLine)
{
    const fp::IntOctagon room = fp::IntBox(0, 0, 100, 100).bounding_octagon();
    const fp::IntOctagon obstacle = fp::IntBox(40, 40, 60, 60).bounding_octagon();
    // Outside of the left border line (line 6): x <= 40; inside: x >= 40.
    const fp::IntOctagon outside = ShapeSearchTree45Degree::calc_outside_restrained_shape(obstacle, 6, room);
    EXPECT_EQ(0, outside.left);
    EXPECT_EQ(40, outside.right);
    const fp::IntOctagon inside = ShapeSearchTree45Degree::calc_inside_restrained_shape(obstacle, 6, room);
    EXPECT_EQ(40, inside.left);
    EXPECT_EQ(100, inside.right);
    // Outside of the upper border line (line 4): y >= 60.
    const fp::IntOctagon above = ShapeSearchTree45Degree::calc_outside_restrained_shape(obstacle, 4, room);
    EXPECT_EQ(60, above.bottom);
    EXPECT_EQ(100, above.top);
}

TEST(ShapeSearchTree45DegreeTest, AutorouteTreeOfA45DegreeBoardIsThe45DegreeTree)
{
    std::unique_ptr<RoutingBoard> board = load_board("Issue026-J2_reference.dsn");
    ASSERT_NE(nullptr, board);
    ShapeSearchTree* tree = board->get_search_tree_manager()->get_autoroute_tree(1);
    EXPECT_NE(nullptr, dynamic_cast<ShapeSearchTree45Degree*>(tree));
}

TEST(ShapeSearchTree45DegreeTest, CompleteShapeMatchesJavaOnIssue026)
{
    std::unique_ptr<RoutingBoard> board = load_board("Issue026-J2_reference.dsn");
    ASSERT_NE(nullptr, board);
    ShapeSearchTree* tree = board->get_search_tree_manager()->get_autoroute_tree(1);

    std::ifstream golden(std::string(FREEROUTING_TEST_DATA_DIR) + "/expected/Issue026-J2_reference.dsn.tree45.txt");
    ASSERT_TRUE(golden.good());
    std::string first_line;
    std::getline(golden, first_line);
    std::istringstream query_stream(first_line.substr(std::string("QUERIES ").size()));
    std::vector<int64_t> queries;
    for (int64_t value; query_stream >> value;)
    {
        queries.push_back(value);
    }
    std::vector<std::string> expected;
    for (std::string line; std::getline(golden, line);)
    {
        expected.push_back(line);
    }

    std::vector<std::string> actual;
    for (int layer = 0; layer <= 1; ++layer)
    {
        actual.push_back("LAYER " + std::to_string(layer));
        for (std::size_t k = 0; k + 3 < queries.size(); k += 4)
        {
            const fp::IntBox contained(queries[k], queries[k + 1], queries[k + 2], queries[k + 3]);
            ShapeSearchTree::CompleteShapeInput input;
            input.contained_shape = &contained;
            input.layer = layer;
            input.net_no = 1;
            std::vector<ShapeSearchTree::CompleteShapePiece> rooms = tree->complete_shape(input);
            std::ostringstream header;
            header << "QUERY " << queries[k] << ',' << queries[k + 1] << ',' << queries[k + 2] << ',' << queries[k + 3]
                   << " rooms=" << rooms.size();
            actual.push_back(header.str());
            for (const ShapeSearchTree::CompleteShapePiece& room : rooms)
            {
                const fp::IntOctagon contained_oct = room.contained_shape->bounding_octagon();
                // Java prints IntOctagon.EMPTY with its sentinel coordinates.
                const std::string contained_text = contained_oct == fp::IntOctagon::EMPTY
                    ? std::string("33554432 33554432 -33554432 -33554432")
                    : std::to_string(contained_oct.left) + ' ' + std::to_string(contained_oct.bottom) + ' '
                        + std::to_string(contained_oct.right) + ' ' + std::to_string(contained_oct.top);
                actual.push_back("  ROOM " + octagon_text(room.shape->bounding_octagon()) + " | " + contained_text);
            }
        }
    }
    if (const char* dump = std::getenv("FREEROUTING_DUMP_TREE45"))
    {
        std::ofstream out(dump);
        for (const std::string& line : actual)
        {
            out << line << "\n";
        }
    }
    ASSERT_EQ(expected.size(), actual.size());
    for (std::size_t i = 0; i < expected.size(); ++i)
    {
        EXPECT_EQ(expected[i], actual[i]) << "line " << i;
    }
}

} // namespace
} // namespace freerouting::board
