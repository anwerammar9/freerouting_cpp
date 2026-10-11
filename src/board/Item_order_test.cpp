// Item ordering parity (T011): Java Item.compareTo returns other.id - this.id and the board item
// list is a ConcurrentSkipListMap, so items are visited newest first (descending id).

#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include "board/Item.h"
#include "board/ItemSet.h"
#include "board/Layer.h"
#include "board/PolylineTrace.h"
#include "board/RoutingBoard.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/Polyline.h"
#include "rules/BoardRules.h"

namespace freerouting::board {
namespace {

std::unique_ptr<RoutingBoard> make_board()
{
    std::vector<std::shared_ptr<Layer>> layers{std::make_shared<Layer>("Top", true)};
    LayerStructure layer_structure(layers);
    auto rules = std::make_unique<rules::BoardRules>(layer_structure, 100);
    rules->get_nets().add("n1", 1, false);
    return std::make_unique<RoutingBoard>(
        geometry::planar::IntBox(0, 0, 10000, 10000), layer_structure, std::move(rules), nullptr);
}

std::shared_ptr<geometry::planar::Polyline> segment(int p_y)
{
    return std::make_shared<geometry::planar::Polyline>(geometry::planar::Polyline::from_corners(
        {geometry::planar::IntPoint(100, p_y), geometry::planar::IntPoint(900, p_y)}));
}

PolylineTrace* insert(RoutingBoard& p_board, int p_y)
{
    return p_board.insert_trace_without_cleaning(segment(p_y), 0, 10, {1}, 1, FixedState::NOT_FIXED);
}

std::vector<int> ids(const std::vector<Item*>& p_items)
{
    std::vector<int> result;
    for (const Item* item : p_items)
    {
        result.push_back(item->get_id());
    }
    return result;
}

TEST(ItemOrderTest, GetItemsIsNewestFirst)
{
    auto board = make_board();
    PolylineTrace* a = insert(*board, 100);
    PolylineTrace* b = insert(*board, 200);
    PolylineTrace* c = insert(*board, 300);
    ASSERT_LT(a->get_id(), b->get_id());
    ASSERT_LT(b->get_id(), c->get_id());
    EXPECT_EQ((std::vector<int>{c->get_id(), b->get_id(), a->get_id()}), ids(board->get_items()));
    EXPECT_EQ(c, board->get_item(0)); // index loops follow the same order
    EXPECT_EQ(a, board->get_item(2));
}

TEST(ItemOrderTest, RemovalKeepsTheOrder)
{
    auto board = make_board();
    PolylineTrace* a = insert(*board, 100);
    PolylineTrace* b = insert(*board, 200);
    PolylineTrace* c = insert(*board, 300);
    board->remove_item(b);
    EXPECT_EQ((std::vector<int>{c->get_id(), a->get_id()}), ids(board->get_items()));
}

TEST(ItemOrderTest, ItemCreatedEarlierButInsertedLaterIsPlacedByItsId)
{
    auto board = make_board();
    const int early_id = board->generate_item_id();
    PolylineTrace* a = insert(*board, 100);
    PolylineTrace* b = insert(*board, 200);
    auto* early = new PolylineTrace(segment(300), 0, 10, {1}, 1, early_id, 0, FixedState::NOT_FIXED);
    board->add_item(early);
    EXPECT_EQ((std::vector<int>{b->get_id(), a->get_id(), early_id}), ids(board->get_items()));
}

TEST(ItemOrderTest, ItemSetOrdersByDescendingId)
{
    auto board = make_board();
    PolylineTrace* a = insert(*board, 100);
    PolylineTrace* b = insert(*board, 200);
    PolylineTrace* c = insert(*board, 300);
    ItemSet set{b, a, c};
    EXPECT_EQ((std::vector<int>{c->get_id(), b->get_id(), a->get_id()}),
        ids(std::vector<Item*>(set.begin(), set.end())));
}

} // namespace
} // namespace freerouting::board
