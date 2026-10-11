#include <gtest/gtest.h>

#include <algorithm>
#include <memory>
#include <vector>

#include "autoroute/ExpansionRoom.h"
#include "board/Layer.h"
#include "board/RoutingBoard.h"
#include "board/SearchTreeObject.h"
#include "board/ShapeSearchTree.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/IntOctagon.h"
#include "geometry/planar/Simplex.h"
#include "geometry/planar/TileShape.h"
#include "rules/BoardRules.h"

namespace fr = freerouting;
namespace fp = freerouting::geometry::planar;
using fr::datastructures::ShapeTree;
using fr::board::ShapeSearchTree;
using ShapeSearchTreeInput = ShapeSearchTree::CompleteShapeInput;
using ShapeSearchTreePiece = ShapeSearchTree::CompleteShapePiece;

namespace {

std::shared_ptr<fr::board::RoutingBoard> make_routing_board()
{
    std::vector<std::shared_ptr<fr::board::Layer>> layers;
    layers.push_back(std::make_shared<fr::board::Layer>("Top", true));
    layers.push_back(std::make_shared<fr::board::Layer>("Bottom", true));
    fr::board::LayerStructure layer_structure(layers);
    auto board_rules = std::make_unique<fr::rules::BoardRules>(layer_structure, 10);
    return std::make_shared<fr::board::RoutingBoard>(
        fp::IntBox(0, 0, 10000, 10000), layer_structure, std::move(board_rules), nullptr);
}

/** Minimal ShapeTree::Storable search tree object covering one IntBox shape. */
class FakeObstacle : public fr::board::SearchTreeObject
{
public:
    FakeObstacle(int p_layer, fp::IntBox p_shape, int p_blocking_net, int p_id)
        : layer_(p_layer), shape_(p_shape), blocking_net_(p_blocking_net), id_(p_id)
    {
    }

    int tree_shape_count(const ShapeTree&) const override { return 1; }
    const fp::TileShape* get_tree_shape(const ShapeTree&, int) const override { return &shape_; }
    void set_search_tree_entries(std::vector<std::unique_ptr<ShapeTree::Leaf>> p_entries,
        ShapeTree&) override
    {
        // Keep the leaves alive: the tree only references them, ownership is
        // transferred to the storable (mirrors the Java Leaf[] contract).
        tree_entries_ = std::move(p_entries);
    }
    int compare_to(const ShapeTree::Storable& p_other) const override
    {
        const auto* other_obstacle = dynamic_cast<const FakeObstacle*>(&p_other);
        if (other_obstacle == nullptr)
        {
            return -1;
        }
        if (id_ == other_obstacle->id_)
        {
            return 0;
        }
        return id_ < other_obstacle->id_ ? -1 : 1;
    }
    bool is_obstacle(int) const override { return true; }
    bool is_trace_obstacle(int p_net_no) const override { return p_net_no != blocking_net_; }
    int shape_layer(int) const override { return layer_; }
    int get_id() const override { return id_; }

private:
    int layer_;
    fp::IntBox shape_;
    int blocking_net_;
    int id_;
    std::vector<std::unique_ptr<ShapeTree::Leaf>> tree_entries_;
};

/** Checks that p_piece does not overlap the interior of the obstacle box. */
bool piece_avoids(const ShapeSearchTreePiece& p_piece, const fp::IntBox& p_obstacle)
{
    return p_piece.shape->to_Simplex().intersection(p_obstacle.to_Simplex()).dimension() < 2;
}

} // namespace

TEST(ShapeSearchTreeCompleteShape, EmptyTreeCompletesSmallRoomToRoomBox)
{
    auto board = make_routing_board();
    ShapeSearchTree tree(board.get(), 0);

    fp::IntBox room_box(0, 0, 1000, 1000);
    ShapeSearchTreeInput input;
    input.shape = &room_box;
    fp::IntBox contained(400, 400, 600, 600);
    input.contained_shape = &contained;
    input.layer = 0;
    input.net_no = 1;

    std::vector<ShapeSearchTreePiece> result = tree.complete_shape(input);
    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0].shape->bounding_box(), room_box);
    EXPECT_EQ(result[0].shape->dimension(), 2);
    ASSERT_NE(result[0].contained_shape, nullptr);
    EXPECT_GE(result[0].contained_shape->dimension(), 1);
}

TEST(ShapeSearchTreeCompleteShape, EmptyTreeWholeBoardIsDivided)
{
    auto board = make_routing_board();
    ShapeSearchTree tree(board.get(), 0);

    // No room shape (nullptr) means the room covers the whole board box. The
    // large room is split into 4 sections so that the maze search has more
    // than one room per layer.
    fp::IntBox contained(3000, 3000, 4000, 4000);
    ShapeSearchTreeInput input;
    input.shape = nullptr;
    input.contained_shape = &contained;
    input.layer = 0;
    input.net_no = 1;

    std::vector<ShapeSearchTreePiece> result = tree.complete_shape(input);
    ASSERT_EQ(result.size(), 4u);
    for (const ShapeSearchTreePiece& piece : result)
    {
        ASSERT_NE(piece.shape, nullptr);
        EXPECT_EQ(piece.shape->dimension(), 2);
    }
}

TEST(ShapeSearchTreeCompleteShape, SingleObstacleRestrainsRoom)
{
    auto board = make_routing_board();
    ShapeSearchTree tree(board.get(), 0);

    FakeObstacle wall(0, fp::IntBox(4000, 0, 6000, 10000), /*blocking_net=*/2, /*id=*/1);
    tree.insert(wall);

    fp::IntBox contained(100, 100, 200, 200);
    ShapeSearchTreeInput input;
    input.shape = nullptr;
    input.contained_shape = &contained;
    input.layer = 0;
    input.net_no = 1;

    std::vector<ShapeSearchTreePiece> result = tree.complete_shape(input);
    ASSERT_FALSE(result.empty());

    // Every piece is a valid free space section: 2-dimensional, avoiding the
    // obstacle interior and still covering the contained shape.
    bool contained_is_covered = false;
    for (const ShapeSearchTreePiece& piece : result)
    {
        ASSERT_NE(piece.shape, nullptr);
        EXPECT_EQ(piece.shape->dimension(), 2);
        EXPECT_TRUE(piece_avoids(piece, fp::IntBox(4000, 0, 6000, 10000)));
        if (piece.shape->contains(fp::IntPoint(150, 150)))
        {
            contained_is_covered = true;
        }
    }
    EXPECT_TRUE(contained_is_covered);
}

TEST(ShapeSearchTreeCompleteShape, NonBlockingObstacleIgnored)
{
    auto board = make_routing_board();
    ShapeSearchTree tree(board.get(), 0);

    // The obstacle blocks net 2, so it must not restrain a room of net 1.
    FakeObstacle obstacle(0, fp::IntBox(4000, 4000, 6000, 6000), /*blocking_net=*/2, /*id=*/1);
    tree.insert(obstacle);

    fp::IntBox room_box(0, 0, 1000, 1000);
    fp::IntBox contained(400, 400, 600, 600);
    ShapeSearchTreeInput input;
    input.shape = &room_box;
    input.contained_shape = &contained;
    input.layer = 0;
    input.net_no = 1;

    std::vector<ShapeSearchTreePiece> result = tree.complete_shape(input);
    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0].shape->bounding_box(), room_box);
}

TEST(ShapeSearchTreeCompleteShape, WrongLayerObstacleIgnored)
{
    auto board = make_routing_board();
    ShapeSearchTree tree(board.get(), 0);

    // Obstacle on layer 1 must not restrain a room on layer 0.
    FakeObstacle obstacle(1, fp::IntBox(0, 0, 1000, 1000), /*blocking_net=*/1, /*id=*/1);
    tree.insert(obstacle);

    fp::IntBox room_box(0, 0, 1000, 1000);
    fp::IntBox contained(400, 400, 600, 600);
    ShapeSearchTreeInput input;
    input.shape = &room_box;
    input.contained_shape = &contained;
    input.layer = 0;
    input.net_no = 1;

    std::vector<ShapeSearchTreePiece> result = tree.complete_shape(input);
    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0].shape->bounding_box(), room_box);
}

TEST(ShapeSearchTreeCompleteShape, IgnoreObjectSkipsObstacle)
{
    auto board = make_routing_board();
    ShapeSearchTree tree(board.get(), 0);

    FakeObstacle obstacle(0, fp::IntBox(0, 0, 1000, 1000), /*blocking_net=*/1, /*id=*/1);
    tree.insert(obstacle);

    fp::IntBox room_box(0, 0, 1000, 1000);
    fp::IntBox contained(400, 400, 600, 600);
    ShapeSearchTreeInput input;
    input.shape = &room_box;
    input.contained_shape = &contained;
    input.layer = 0;
    input.net_no = 1;
    input.ignore_object = &obstacle;

    // The obstacle is passed as ignore_object, so it is not processed and the
    // room stays completely free.
    std::vector<ShapeSearchTreePiece> result = tree.complete_shape(input);
    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0].shape->bounding_box(), room_box);
}

TEST(ShapeSearchTreeCompleteShape, FreeSpaceRoomIgnoredWhenContainedInIgnoreShape)
{
    auto board = make_routing_board();
    fp::IntBox room_box(0, 0, 1000, 1000);
    fr::autoroute::CompleteFreeSpaceExpansionRoom free_space_room(0, 5, room_box);
    ShapeSearchTree tree(board.get(), 0);
    tree.insert(free_space_room);

    fp::IntBox contained(400, 400, 600, 600);
    ShapeSearchTreeInput input;
    input.shape = &room_box;
    input.contained_shape = &contained;
    input.layer = 0;
    input.net_no = 1;
    input.ignore_shape = &room_box;

    // The already completed free space room is contained in the ignore shape,
    // so it must be reused instead of restraining the new room.
    std::vector<ShapeSearchTreePiece> result = tree.complete_shape(input);
    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0].shape->bounding_box(), room_box);
}

TEST(ShapeSearchTreeCompleteShape, FreeSpaceRoomRestrainsWithoutIgnoreShape)
{
    auto board = make_routing_board();
    // The new completion room is larger than the already completed free-space
    // room, and the contained shape lies outside it.
    fp::IntBox completion_room(0, 0, 2000, 2000);
    fp::IntBox stored_room(1000, 1000, 1600, 1600);
    fr::autoroute::CompleteFreeSpaceExpansionRoom free_space_room(0, 5, stored_room);
    ShapeSearchTree tree(board.get(), 0);
    tree.insert(free_space_room);

    fp::IntBox contained(100, 100, 300, 300);
    ShapeSearchTreeInput input;
    input.shape = &completion_room;
    input.contained_shape = &contained;
    input.layer = 0;
    input.net_no = 1;

    // Without an ignore shape the completed room is an obstacle and restrains
    // the new room. The result must not overlap the interior of the stored
    // room, and the contained shape must be covered by at least one piece.
    std::vector<ShapeSearchTreePiece> result = tree.complete_shape(input);
    ASSERT_FALSE(result.empty());
    for (const ShapeSearchTreePiece& piece : result)
    {
        ASSERT_NE(piece.shape, nullptr);
        ASSERT_NE(piece.contained_shape, nullptr);
        EXPECT_EQ(piece.shape->dimension(), 2);
        EXPECT_TRUE(piece_avoids(piece, stored_room));
    }
    // The contained shape is covered: its four corners are inside the result
    // pieces (mirrors the Java contract "room.getContainedShape() will be
    // contained in the shape of the result room").
    const std::vector<fp::IntPoint> contained_corners{
        fp::IntPoint(100, 100), fp::IntPoint(100, 300),
        fp::IntPoint(300, 100), fp::IntPoint(300, 300)};
    for (const fp::IntPoint& corner : contained_corners)
    {
        bool corner_covered =
            std::any_of(result.begin(), result.end(), [&corner](const ShapeSearchTreePiece& piece)
                { return piece.shape->contains(corner); });
        EXPECT_TRUE(corner_covered) << "contained corner not covered: " << corner.to_string();
    }
}

TEST(ShapeSearchTreeCompleteShape, ContainedStraddlingObstacleSplitsRoom)
{
    auto board = make_routing_board();
    ShapeSearchTree tree(board.get(), 0);

    // The contained shape straddles the obstacle edge, so the room must be
    // split into several pieces.
    FakeObstacle obstacle(0, fp::IntBox(400, 0, 600, 1000), /*blocking_net=*/2, /*id=*/1);
    tree.insert(obstacle);

    fp::IntBox contained(350, 450, 450, 550);
    ShapeSearchTreeInput input;
    input.shape = nullptr;
    input.contained_shape = &contained;
    input.layer = 0;
    input.net_no = 1;

    std::vector<ShapeSearchTreePiece> result = tree.complete_shape(input);
    ASSERT_FALSE(result.empty());
    for (const ShapeSearchTreePiece& piece : result)
    {
        ASSERT_NE(piece.shape, nullptr);
        EXPECT_EQ(piece.shape->dimension(), 2);
        EXPECT_TRUE(piece_avoids(piece, fp::IntBox(400, 0, 600, 1000)));
    }
}