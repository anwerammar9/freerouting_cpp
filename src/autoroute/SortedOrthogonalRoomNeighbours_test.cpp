#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <memory>
#include <set>
#include <vector>

#include "autoroute/AutorouteEngine.h"
#include "autoroute/ExpansionRoom.h"
#include "autoroute/SortedOrthogonalRoomNeighbours.h"
#include "autoroute/SortedRoomNeighbours.h"
#include "board/FixedState.h"
#include "board/Layer.h"
#include "board/PolylineTrace.h"
#include "board/RoutingBoard.h"
#include "board/SearchTreeObject.h"
#include "board/ShapeSearchTree.h"
#include "geometry/planar/ConvexShape.h"
#include "geometry/planar/FortyfiveDegreeBoundingDirections.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/IntOctagon.h"
#include "geometry/planar/Point.h"
#include "geometry/planar/Polyline.h"
#include "geometry/planar/ShapeBoundingDirections.h"
#include "geometry/planar/Simplex.h"
#include "geometry/planar/TileShape.h"
#include "rules/BoardRules.h"

namespace fr = freerouting;
namespace fp = fr::geometry::planar;

namespace {

constexpr int64_t kBoardLl = -5000;
constexpr int64_t kBoardUr = 5000;

/** The room under test: the box 0..100 in both directions. */
fp::IntBox room_box() { return fp::IntBox(0, 0, 100, 100); }

std::shared_ptr<fr::board::RoutingBoard> make_routing_board()
{
    std::vector<std::shared_ptr<fr::board::Layer>> layers;
    layers.push_back(std::make_shared<fr::board::Layer>("Top", true));
    layers.push_back(std::make_shared<fr::board::Layer>("Bottom", true));
    fr::board::LayerStructure layer_structure(layers);
    auto board_rules = std::make_unique<fr::rules::BoardRules>(layer_structure, 10);
    return std::make_shared<fr::board::RoutingBoard>(fp::IntBox(kBoardLl, kBoardLl, kBoardUr, kBoardUr),
        layer_structure, std::move(board_rules), nullptr);
}

/** An incomplete free space room with the box room shape under test. */
std::unique_ptr<fr::autoroute::IncompleteFreeSpaceExpansionRoom> make_incomplete_room()
{
    const fp::Simplex shape = room_box().to_Simplex();
    return std::make_unique<fr::autoroute::IncompleteFreeSpaceExpansionRoom>(nullptr, 0, shape, shape);
}

/**
 * Registers a complete free space room with a box shape in the tree. The room
 * is returned, because the search tree keeps pointers to it and the caller has
 * to keep it alive (see 01_MEMORY.md gotcha 16).
 */
std::unique_ptr<fr::autoroute::CompleteFreeSpaceExpansionRoom> insert_box_room(
    fr::board::ShapeSearchTree* p_tree, int p_room_id, const fp::IntBox& p_box)
{
    auto room = std::make_unique<fr::autoroute::CompleteFreeSpaceExpansionRoom>(0, p_room_id, p_box);
    p_tree->insert(*room);
    return room;
}

/**
 * A net-1 obstacle with a box shape touching the left side of room_box() in the
 * interior of the side only, so that the first and the last touching side are
 * both side 3.
 */
fp::IntBox left_neighbour_box() { return fp::IntBox(-200, 20, 0, 80); }

/** The mirrored neighbour on the right side (first and last touching side 1). */
fp::IntBox right_neighbour_box() { return fp::IntBox(100, 20, 300, 80); }

fr::board::PolylineTrace* insert_trace(
    fr::board::RoutingBoard* p_board, const std::vector<fp::IntPoint>& p_corners, const std::vector<int>& p_nets)
{
    auto polyline = std::make_shared<fp::Polyline>(fp::Polyline::from_corners(p_corners));
    return p_board->insert_trace_without_cleaning(
        polyline, 0, 50, p_nets, 1, fr::board::FixedState::NOT_FIXED);
}

/** A search tree object with a shape that is not an IntBox. */
class FakeOctagonObject : public fr::board::SearchTreeObject
{
public:
    FakeOctagonObject(int p_layer, fp::IntOctagon p_shape, int p_id)
        : layer_(p_layer)
        , shape_(p_shape)
        , id_(p_id)
    {
    }

    int tree_shape_count(const fr::datastructures::ShapeTree&) const override { return 1; }
    const fp::TileShape* get_tree_shape(const fr::datastructures::ShapeTree&, int) const override
    {
        return &shape_;
    }
    void set_search_tree_entries(std::vector<std::unique_ptr<fr::datastructures::ShapeTree::Leaf>> p_entries,
        fr::datastructures::ShapeTree&) override
    {
        tree_entries_ = std::move(p_entries);
    }
    int compare_to(const fr::datastructures::ShapeTree::Storable& p_other) const override
    {
        const auto* other = dynamic_cast<const FakeOctagonObject*>(&p_other);
        if (other == nullptr)
        {
            return -1;
        }
        if (id_ == other->id_)
        {
            return 0;
        }
        return id_ < other->id_ ? -1 : 1;
    }
    bool is_obstacle(int) const override { return true; }
    bool is_trace_obstacle(int) const override { return true; }
    int shape_layer(int) const override { return layer_; }
    int get_id() const override { return id_; }

private:
    int layer_;
    fp::IntOctagon shape_;
    int id_;
    std::vector<std::unique_ptr<fr::datastructures::ShapeTree::Leaf>> tree_entries_;
};

/**
 * A bounding direction set which is neither orthogonal (4) nor 45 degree (8),
 * so that the any-angle room neighbour implementation is selected.
 */
class SixDirectionBoundingDirections : public fp::ShapeBoundingDirections
{
public:
    int count() const override { return 6; }
    fp::IntOctagon bounds(const fp::ConvexShape& p_shape) const override
    {
        return fp::FortyfiveDegreeBoundingDirections::INSTANCE.bounds(p_shape);
    }
    fp::IntOctagon bounds(const fp::IntBox& p_box) const override
    {
        return fp::FortyfiveDegreeBoundingDirections::INSTANCE.bounds(p_box);
    }
    fp::IntOctagon bounds(const fp::IntOctagon& p_oct) const override
    {
        return fp::FortyfiveDegreeBoundingDirections::INSTANCE.bounds(p_oct);
    }
    fp::IntOctagon bounds(const fp::Simplex& p_simplex) const override
    {
        return fp::FortyfiveDegreeBoundingDirections::INSTANCE.bounds(p_simplex);
    }
    fp::IntOctagon bounds(const fp::Circle& p_circle) const override
    {
        return fp::FortyfiveDegreeBoundingDirections::INSTANCE.bounds(p_circle);
    }
    fp::IntOctagon bounds(const fp::PolygonShape& p_polygon) const override
    {
        return fp::FortyfiveDegreeBoundingDirections::INSTANCE.bounds(p_polygon);
    }
};

/** A square with all 45 degree corners cut by 30 units. */
fp::IntOctagon make_cut_square()
{
    return fp::IntOctagon(0, 0, 100, 100, 0 - 100 + 30, 100 - 0 - 30, 0 + 0 + 30, 100 + 100 - 30);
}

} // namespace

TEST(SortedOrthogonalRoomNeighboursTest, EmptyTreeHasNoNeighbours)
{
    auto board = make_routing_board();
    fr::board::ShapeSearchTree90Degree tree(board.get(), 0);
    ASSERT_EQ(tree.get_bounding_directions().count(), 4);

    auto room = make_incomplete_room();
    auto neighbours = fr::autoroute::SortedOrthogonalRoomNeighbours::calculate_neighbours(
        room.get(), 1, &tree, 7);
    ASSERT_NE(neighbours, nullptr);
    EXPECT_TRUE(neighbours->get_sorted_neighbours().empty());
    EXPECT_EQ(neighbours->get_room_shape(), room_box());
    EXPECT_FALSE(neighbours->is_obstacle_expansion_room());
    const std::array<bool, 4> no_flags = {};
    EXPECT_EQ(neighbours->get_edge_interior_touches_obstacle(), no_flags);

    // A new complete free space room is created for the incomplete from room.
    auto* completed_room = neighbours->get_completed_room();
    ASSERT_NE(completed_room, nullptr);
    EXPECT_EQ(completed_room->get_layer(), 0);
    EXPECT_EQ(completed_room->get_room_id(), 7);
    EXPECT_EQ(completed_room->get_shape(), room_box());
    EXPECT_TRUE(completed_room->get_doors().empty());
    EXPECT_EQ(neighbours->release_completed_room(), completed_room);
    EXPECT_EQ(neighbours->release_completed_room(), nullptr);
}

TEST(SortedOrthogonalRoomNeighboursTest, UnexpectedRoomTypeReturnsNull)
{
    auto board = make_routing_board();
    fr::board::ShapeSearchTree90Degree tree(board.get(), 0);
    fr::autoroute::AutorouteEngine engine(board.get(), 1, true);
    engine.set_net_no(1);

    // A plain IncompleteExpansionRoom is not a supported from room type.
    fr::autoroute::IncompleteExpansionRoom room(nullptr, 0, room_box());
    EXPECT_EQ(fr::autoroute::SortedOrthogonalRoomNeighbours::calculate_neighbours(&room, 1, &tree, 1), nullptr);
    EXPECT_EQ(fr::autoroute::SortedOrthogonalRoomNeighbours::calculate(&room, &engine), nullptr);
    EXPECT_EQ(fr::autoroute::SortedOrthogonalRoomNeighbours::calculate(nullptr, &engine), nullptr);
    EXPECT_EQ(fr::autoroute::SortedOrthogonalRoomNeighbours::calculate(&room, nullptr), nullptr);
    EXPECT_EQ(fr::autoroute::SortedOrthogonalRoomNeighbours::calculate_neighbours(&room, 1, nullptr, 1), nullptr);
}

TEST(SortedOrthogonalRoomNeighboursTest, NonBoxRoomShapeIsRejected)
{
    auto board = make_routing_board();
    fr::board::ShapeSearchTree90Degree tree(board.get(), 0);

    // The orthogonal calculation requires IntBox room shapes, so a room with a
    // 45 degree octagon shape is rejected.
    const fp::Simplex shape = make_cut_square().to_Simplex();
    ASSERT_FALSE(shape.is_IntBox());
    fr::autoroute::IncompleteFreeSpaceExpansionRoom room(nullptr, 0, shape, shape);
    EXPECT_EQ(fr::autoroute::SortedOrthogonalRoomNeighbours::calculate_neighbours(&room, 1, &tree, 1), nullptr);
}

TEST(SortedOrthogonalRoomNeighboursTest, NonBoxNeighbourShapeAbortsTheCalculation)
{
    auto board = make_routing_board();
    fr::board::ShapeSearchTree90Degree tree(board.get(), 0);
    // A neighbour with a non IntBox tree shape cannot be handled by the
    // orthogonal calculation, so the whole neighbour calculation is aborted.
    FakeOctagonObject octagon_object(0, make_cut_square(), /*p_id=*/1);
    tree.insert(octagon_object);

    auto room = make_incomplete_room();
    EXPECT_EQ(fr::autoroute::SortedOrthogonalRoomNeighbours::calculate_neighbours(room.get(), 1, &tree, 1), nullptr);
}

TEST(SortedOrthogonalRoomNeighboursTest, LeftNeighbourTouchesTheLeftSideOnly)
{
    auto board = make_routing_board();
    fr::board::ShapeSearchTree90Degree tree(board.get(), 0);
    auto room_2 = insert_box_room(&tree, 2, left_neighbour_box());

    auto room = make_incomplete_room();
    auto neighbours = fr::autoroute::SortedOrthogonalRoomNeighbours::calculate_neighbours(
        room.get(), 1, &tree, 1);
    ASSERT_NE(neighbours, nullptr);
    ASSERT_EQ(neighbours->get_sorted_neighbours().size(), 1u);

    // The intersection with the left neighbour is the shared edge x == 0, so the
    // first and the last touching side are both the left side (no 3) and only
    // the interior of the left side touches the neighbour.
    const auto& neighbour = neighbours->get_sorted_neighbours().front();
    EXPECT_EQ(neighbour.first_touching_side, 3);
    EXPECT_EQ(neighbour.last_touching_side, 3);
    EXPECT_EQ(neighbour.intersection, fp::IntBox(0, 20, 0, 80));
    EXPECT_EQ(neighbour.shape, left_neighbour_box());
    EXPECT_EQ(neighbour.search_tree_object, room_2.get());
    const std::array<bool, 4> expected_flags = {false, false, false, true};
    EXPECT_EQ(neighbours->get_edge_interior_touches_obstacle(), expected_flags);

    // A 1-dimensional door is created to the neighbour room.
    auto* completed_room = neighbours->get_completed_room();
    ASSERT_NE(completed_room, nullptr);
    ASSERT_EQ(completed_room->get_doors().size(), 1u);
    auto* door = completed_room->get_doors().front();
    // The C++ ExpansionDoor has no own door shape, so the door shape is the
    // intersection of the bounding boxes of the 2 door rooms.
    EXPECT_EQ(door->get_dimension(), 1);
    EXPECT_EQ(door->get_shape(), fp::IntBox(0, 20, 0, 80));
    EXPECT_EQ(door->other_room(completed_room), room_2.get());
    // The door is registered on both sides.
    EXPECT_EQ(room_2->get_doors().size(), 1u);
    EXPECT_EQ(room_2->get_doors().front(), door);
}

TEST(SortedOrthogonalRoomNeighboursTest, NeighboursAreSortedCounterclockwise)
{
    auto board = make_routing_board();
    fr::board::ShapeSearchTree90Degree tree(board.get(), 0);
    auto right = insert_box_room(&tree, 2, fp::IntBox(100, 0, 300, 100));
    auto top = insert_box_room(&tree, 3, fp::IntBox(0, 100, 100, 300));
    auto left = insert_box_room(&tree, 4, fp::IntBox(-200, 0, 0, 100));
    auto bottom = insert_box_room(&tree, 5, fp::IntBox(0, -200, 100, 0));

    auto room = make_incomplete_room();
    auto neighbours = fr::autoroute::SortedOrthogonalRoomNeighbours::calculate_neighbours(
        room.get(), 1, &tree, 1);
    ASSERT_NE(neighbours, nullptr);

    const auto& sorted = neighbours->get_sorted_neighbours();
    ASSERT_EQ(sorted.size(), 4u);
    // The sorting starts at the bottom side (0) and runs counterclockwise, so
    // the first touching sides are ascending.
    for (size_t i = 1; i < sorted.size(); i++)
    {
        EXPECT_LE(sorted[i - 1].compare_to(sorted[i]), 0);
        EXPECT_LE(sorted[i - 1].first_touching_side, sorted[i].first_touching_side);
    }
    EXPECT_EQ(sorted[0].first_touching_side, 0);
    EXPECT_EQ(sorted[0].search_tree_object, right.get());
    EXPECT_EQ(sorted[1].first_touching_side, 1);
    EXPECT_EQ(sorted[1].search_tree_object, top.get());
    EXPECT_EQ(sorted[2].first_touching_side, 2);
    EXPECT_EQ(sorted[2].search_tree_object, left.get());
    EXPECT_EQ(sorted[3].first_touching_side, 3);
    EXPECT_EQ(sorted[3].search_tree_object, bottom.get());

    // The interior of all 4 sides touches a neighbour.
    const std::array<bool, 4> expected_flags = {true, true, true, true};
    EXPECT_EQ(neighbours->get_edge_interior_touches_obstacle(), expected_flags);

    // Every neighbour is a distinct tree object with its own 1-dimensional door.
    std::set<const fr::board::SearchTreeObject*> neighbour_objects;
    for (const auto& neighbour : sorted)
    {
        neighbour_objects.insert(neighbour.search_tree_object);
    }
    EXPECT_EQ(neighbour_objects.size(), 4u);
    auto* completed_room = neighbours->get_completed_room();
    ASSERT_NE(completed_room, nullptr);
    ASSERT_EQ(completed_room->get_doors().size(), 4u);
    for (auto* door : completed_room->get_doors())
    {
        EXPECT_EQ(door->get_dimension(), 1);
    }
}

TEST(SortedOrthogonalRoomNeighboursTest, TryRemoveEdgeEnlargesTheRoom)
{
    auto board = make_routing_board();
    fr::board::ShapeSearchTree90Degree tree(board.get(), 0);
    auto left = insert_box_room(&tree, 2, left_neighbour_box());
    auto right = insert_box_room(&tree, 3, right_neighbour_box());

    auto room = make_incomplete_room();
    auto neighbours = fr::autoroute::SortedOrthogonalRoomNeighbours::calculate_neighbours(
        room.get(), 1, &tree, 1);
    ASSERT_NE(neighbours, nullptr);
    ASSERT_EQ(neighbours->get_sorted_neighbours().size(), 2u);
    const std::array<bool, 4> side_flags = {false, true, false, true};
    ASSERT_EQ(neighbours->get_edge_interior_touches_obstacle(), side_flags);

    // The bottom and the top side of the room have no touching neighbour, so the
    // first missing border line (side 0 = bottom) is removed and the room grows
    // down to the board border.
    EXPECT_TRUE(neighbours->try_remove_edge(1, &tree));
    const fp::IntBox enlarged = room->get_shape_tile().bounding_box();
    EXPECT_EQ(enlarged, fp::IntBox(0, kBoardLl, 100, 100));
    EXPECT_GT(enlarged.area(), room_box().area());
    // The touching neighbour flags are not changed by the enlargement.
    EXPECT_EQ(neighbours->get_edge_interior_touches_obstacle(), side_flags);

    // The second enlargement returns the same shape, so it is not applied again
    // (the Java endless loop guard).
    EXPECT_FALSE(neighbours->try_remove_edge(1, &tree));
    EXPECT_EQ(room->get_shape_tile().bounding_box(), enlarged);
}

TEST(SortedOrthogonalRoomNeighboursTest, TryRemoveEdgeIsSkippedForObstacleFromRoom)
{
    auto board = make_routing_board();
    fr::board::ShapeSearchTree90Degree tree(board.get(), 0);
    fr::board::PolylineTrace* trace = insert_trace(board.get(), {fp::IntPoint(0, 0), fp::IntPoint(200, 0)}, {1});
    ASSERT_NE(trace, nullptr);

    // Only the shape of an incomplete free space room may be enlarged.
    fr::autoroute::ObstacleExpansionRoom obstacle_room(trace, 0, 0, room_box());
    auto neighbours = fr::autoroute::SortedOrthogonalRoomNeighbours::calculate_neighbours(
        &obstacle_room, 1, &tree, 1);
    ASSERT_NE(neighbours, nullptr);
    EXPECT_TRUE(neighbours->is_obstacle_expansion_room());
    EXPECT_EQ(neighbours->get_completed_room(), &obstacle_room);
    EXPECT_FALSE(neighbours->try_remove_edge(1, &tree));
    EXPECT_FALSE(neighbours->release_completed_room());
}

TEST(SortedOrthogonalRoomNeighboursTest, NewIncompleteRoomsAreCreatedBetweenNeighbours)
{
    auto board = make_routing_board();
    fr::board::ShapeSearchTree90Degree tree(board.get(), 0);
    fr::autoroute::AutorouteEngine engine(board.get(), 1, true);
    engine.set_net_no(1);
    auto left = insert_box_room(&tree, 2, left_neighbour_box());
    auto right = insert_box_room(&tree, 3, right_neighbour_box());

    auto room = make_incomplete_room();
    auto neighbours = fr::autoroute::SortedOrthogonalRoomNeighbours::calculate_neighbours(
        room.get(), 1, &tree, 1);
    ASSERT_NE(neighbours, nullptr);
    ASSERT_EQ(neighbours->get_sorted_neighbours().size(), 2u);

    neighbours->calculate_new_incomplete_rooms(&engine);

    // The bottom and the top free side of the room get an incomplete room with a
    // door, in addition to the 2 doors to the neighbour rooms.
    auto* completed_room = neighbours->get_completed_room();
    ASSERT_NE(completed_room, nullptr);
    ASSERT_EQ(completed_room->get_doors().size(), 4u);
    const fp::IntBox board_bounds = board->get_bounding_box();
    int incomplete_room_door_count = 0;
    std::vector<fp::IntBox> new_room_boxes;
    for (auto* door : completed_room->get_doors())
    {
        ASSERT_NE(door, nullptr);
        auto* other_room = door->other_room(static_cast<fr::autoroute::ExpansionRoom*>(completed_room));
        ASSERT_NE(other_room, nullptr);
        auto* incomplete_room = dynamic_cast<fr::autoroute::IncompleteFreeSpaceExpansionRoom*>(other_room);
        if (incomplete_room == nullptr)
        {
            // The door to the 2 neighbour rooms.
            EXPECT_EQ(door->get_dimension(), 1);
            continue;
        }
        ++incomplete_room_door_count;
        // The new incomplete room is a 2-dimensional free space room between the
        // 2 neighbours, which is limited by the room and by the board border.
        EXPECT_EQ(door->get_dimension(), 2);
        EXPECT_TRUE(incomplete_room->get_shape_tile().is_bounded());
        const fp::IntBox new_bounds = incomplete_room->get_shape_tile().bounding_box();
        // The contained shape is the intersection with the completed room.
        EXPECT_EQ(incomplete_room->get_contained_shape_tile().bounding_box(), new_bounds.intersection(room_box()));
        new_room_boxes.push_back(new_bounds);
    }
    EXPECT_EQ(incomplete_room_door_count, 2);
    // The 2 new incomplete rooms fill the free space below and above the room.
    // The room below starts at the right neighbour and reaches to the bottom and
    // to the right board border, the room above starts at the left neighbour and
    // reaches to the top and to the left board border.
    const std::vector<fp::IntBox> expected_boxes = {
        fp::IntBox(0, board_bounds.ll.y, board_bounds.ur.x, left_neighbour_box().ll.y),
        fp::IntBox(board_bounds.ll.x, left_neighbour_box().ur.y, 100, board_bounds.ur.y),
    };
    EXPECT_EQ(new_room_boxes, expected_boxes);

    engine.remove_all_doors(completed_room);
    EXPECT_TRUE(completed_room->get_doors().empty());
}

TEST(SortedOrthogonalRoomNeighboursTest, CalculateEnlargesRoomAndReturnsCompletedRoom)
{
    auto board = make_routing_board();
    fr::autoroute::AutorouteEngine engine(board.get(), 1, true);
    engine.set_net_no(1);
    // calculate() uses the search tree of the engine, so the neighbour rooms
    // have to be registered there.
    fr::board::ShapeSearchTree* tree = engine.get_autoroute_search_tree();
    ASSERT_NE(tree, nullptr);
    auto left = insert_box_room(tree, 2, left_neighbour_box());
    auto right = insert_box_room(tree, 3, right_neighbour_box());

    auto room = make_incomplete_room();
    fr::autoroute::CompleteExpansionRoom* completed =
        fr::autoroute::SortedOrthogonalRoomNeighbours::calculate(room.get(), &engine);
    ASSERT_NE(completed, nullptr);
    // The bottom and the top side of the room are free, so the room is enlarged
    // down to the board border before the room calculation is completed.
    EXPECT_EQ(completed->get_shape(), fp::IntBox(0, kBoardLl, 100, 100));
    // The 2 doors to the neighbour rooms and the doors to the 2 incomplete
    // rooms created above the room are reachable through the completed room.
    ASSERT_EQ(completed->get_doors().size(), 4u);
    int one_dimensional_doors = 0;
    int two_dimensional_doors = 0;
    for (auto* door : completed->get_doors())
    {
        ASSERT_NE(door, nullptr);
        if (door->get_dimension() == 1)
        {
            ++one_dimensional_doors;
        }
        else
        {
            ASSERT_EQ(door->get_dimension(), 2);
            ++two_dimensional_doors;
        }
    }
    EXPECT_EQ(one_dimensional_doors, 2);
    EXPECT_EQ(two_dimensional_doors, 2);
    engine.remove_all_doors(completed);
    EXPECT_TRUE(completed->get_doors().empty());
}

TEST(SortedOrthogonalRoomNeighboursTest, IncompleteRoomsWithEmptyNeighboursFillTheBoard)
{
    auto board = make_routing_board();
    fr::board::ShapeSearchTree90Degree tree(board.get(), 0);
    fr::autoroute::AutorouteEngine engine(board.get(), 1, true);
    engine.set_net_no(1);
    fr::board::PolylineTrace* trace = insert_trace(board.get(), {fp::IntPoint(0, 0), fp::IntPoint(200, 0)}, {1});
    ASSERT_NE(trace, nullptr);

    fr::autoroute::ObstacleExpansionRoom obstacle_room(trace, 0, 0, room_box());
    fr::autoroute::SortedOrthogonalRoomNeighbours::calculate_incomplete_rooms_with_empty_neighbours(
        &obstacle_room, &engine);

    // One incomplete room is created for each of the 4 border lines of the room
    // shape, each with a 1-dimensional door.
    const fp::IntBox board_bounds = board->get_bounding_box();
    const std::vector<fp::IntBox> expected_boxes = {
        fp::IntBox(board_bounds.ll.x, board_bounds.ll.y, board_bounds.ur.x, room_box().ll.y),
        fp::IntBox(room_box().ur.x, board_bounds.ll.y, board_bounds.ur.x, board_bounds.ur.y),
        fp::IntBox(board_bounds.ll.x, room_box().ur.y, board_bounds.ur.x, board_bounds.ur.y),
        fp::IntBox(board_bounds.ll.x, board_bounds.ll.y, room_box().ll.x, board_bounds.ur.y),
    };
    ASSERT_EQ(obstacle_room.get_doors().size(), expected_boxes.size());
    std::vector<fp::IntBox> actual_boxes;
    for (auto* door : obstacle_room.get_doors())
    {
        ASSERT_NE(door, nullptr);
        EXPECT_EQ(door->get_dimension(), 1);
        // The static cast selects the ExpansionRoom overload of other_room(),
        // because an ObstacleExpansionRoom is also a CompleteExpansionRoom.
        auto* other_room = door->other_room(static_cast<fr::autoroute::ExpansionRoom*>(&obstacle_room));
        ASSERT_NE(other_room, nullptr);
        auto* incomplete_room = dynamic_cast<fr::autoroute::IncompleteFreeSpaceExpansionRoom*>(other_room);
        ASSERT_NE(incomplete_room, nullptr);
        EXPECT_EQ(incomplete_room->get_layer(), 0);
        EXPECT_EQ(incomplete_room->get_doors().size(), 1u);
        actual_boxes.push_back(incomplete_room->get_shape_tile().bounding_box());
    }
    EXPECT_EQ(actual_boxes, expected_boxes);

    engine.remove_all_doors(static_cast<fr::autoroute::CompleteExpansionRoom*>(&obstacle_room));
    EXPECT_TRUE(obstacle_room.get_doors().empty());
}

TEST(SortedOrthogonalRoomNeighboursTest, CompareToOrdersByTouchingSideAndCorner)
{
    const fp::IntBox box = room_box();
    std::array<bool, 4> flags;
    flags.fill(false);
    // Two neighbours touching the bottom side of the room with an equal first
    // touching corner, so the last touching corner decides the order.
    fr::autoroute::CompleteFreeSpaceExpansionRoom room_5(0, 5, fp::IntBox(-200, -200, 0, 0));
    fr::autoroute::CompleteFreeSpaceExpansionRoom room_7(0, 7, fp::IntBox(-200, -200, 0, 0));

    fr::autoroute::SortedOrthogonalRoomNeighbour short_neighbour(&room_5, room_5.get_shape(),
        fp::IntBox(10, 0, 50, 0), box, &flags);
    fr::autoroute::SortedOrthogonalRoomNeighbour long_neighbour(&room_5, room_5.get_shape(),
        fp::IntBox(10, 0, 70, 0), box, &flags);
    ASSERT_EQ(short_neighbour.first_touching_side, 0);
    ASSERT_EQ(short_neighbour.last_touching_side, 0);
    ASSERT_EQ(long_neighbour.first_touching_side, 0);
    ASSERT_EQ(long_neighbour.last_touching_side, 0);
    EXPECT_LT(short_neighbour.compare_to(long_neighbour), 0);
    EXPECT_GT(long_neighbour.compare_to(short_neighbour), 0);
    EXPECT_EQ(short_neighbour.compare_to(short_neighbour), 0);
    // The bottom side interior of the room is touched.
    EXPECT_TRUE(flags[0]);

    // Identical geometry is ordered by the id of the search tree object.
    fr::autoroute::SortedOrthogonalRoomNeighbour id_5(&room_5, room_5.get_shape(),
        fp::IntBox(10, 0, 50, 0), box, &flags);
    fr::autoroute::SortedOrthogonalRoomNeighbour id_7(&room_7, room_7.get_shape(),
        fp::IntBox(10, 0, 50, 0), box, &flags);
    EXPECT_LT(id_5.compare_to(id_7), 0);
    EXPECT_GT(id_7.compare_to(id_5), 0);

    // The first touching side dominates the comparison.
    fr::autoroute::SortedOrthogonalRoomNeighbour right_side(&room_5, room_5.get_shape(),
        fp::IntBox(100, 20, 100, 80), box, &flags);
    ASSERT_EQ(right_side.first_touching_side, 1);
    EXPECT_LT(id_5.compare_to(right_side), 0);
    EXPECT_GT(right_side.compare_to(id_5), 0);
}

TEST(SortedRoomNeighboursTest, SelectCalculationModeMatchesTheSearchTreeDirections)
{
    // Java selects the neighbour calculation mode from the search tree class.
    // The C++ ShapeSearchTree is configured with the bounding directions, so the
    // number of bounding directions selects the implementation.
    auto board = make_routing_board();
    fr::board::ShapeSearchTree45Degree tree_45(board.get(), 0);
    fr::board::ShapeSearchTree90Degree tree_90(board.get(), 0);
    fr::board::ShapeSearchTree default_tree(board.get(), 0);
    SixDirectionBoundingDirections six_directions;
    fr::board::ShapeSearchTree any_angle_tree(six_directions, board.get(), 0);

    EXPECT_EQ(fr::autoroute::select_calculation_mode(&tree_45), fr::autoroute::CalculationMode::DEGREE_45);
    EXPECT_EQ(fr::autoroute::select_calculation_mode(&tree_90), fr::autoroute::CalculationMode::ORTHOGONAL);
    EXPECT_EQ(fr::autoroute::select_calculation_mode(&default_tree), fr::autoroute::CalculationMode::DEGREE_45);
    EXPECT_EQ(fr::autoroute::select_calculation_mode(&any_angle_tree), fr::autoroute::CalculationMode::ANY_ANGLE);
    EXPECT_EQ(fr::autoroute::select_calculation_mode(nullptr), fr::autoroute::CalculationMode::ANY_ANGLE);

    // The default route angle of the autorouter is 45 degrees, so the engine
    // search tree is dispatched to the 45 degree implementation.
    auto engine_board = make_routing_board();
    fr::autoroute::AutorouteEngine engine(engine_board.get(), 1, true);
    engine.set_net_no(1);
    fr::board::ShapeSearchTree* search_tree = engine.get_autoroute_search_tree();
    ASSERT_NE(search_tree, nullptr);
    EXPECT_EQ(fr::autoroute::select_calculation_mode(search_tree), fr::autoroute::CalculationMode::DEGREE_45);
}
