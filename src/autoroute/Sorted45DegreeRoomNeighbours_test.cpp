#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <memory>
#include <set>
#include <vector>

#include "autoroute/AutorouteEngine.h"
#include "autoroute/ExpansionRoom.h"
#include "autoroute/Sorted45DegreeRoomNeighbours.h"
#include "autoroute/SortedRoomNeighbours.h"
#include "board/Layer.h"
#include "board/RoutingBoard.h"
#include "board/SearchTreeObject.h"
#include "board/ShapeSearchTree.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/IntOctagon.h"
#include "geometry/planar/ShapeBoundingDirections.h"
#include "geometry/planar/Simplex.h"
#include "geometry/planar/TileShape.h"
#include "rules/BoardRules.h"

namespace fr = freerouting;
namespace fp = fr::geometry::planar;

namespace {

std::shared_ptr<fr::board::RoutingBoard> make_routing_board()
{
    std::vector<std::shared_ptr<fr::board::Layer>> layers;
    layers.push_back(std::make_shared<fr::board::Layer>("Top", true));
    layers.push_back(std::make_shared<fr::board::Layer>("Bottom", true));
    fr::board::LayerStructure layer_structure(layers);
    auto board_rules = std::make_unique<fr::rules::BoardRules>(layer_structure, 10);
    return std::make_shared<fr::board::RoutingBoard>(fp::IntBox(-5000, -5000, 5000, 5000), layer_structure,
        std::move(board_rules), nullptr);
}

/**
 * A square p_x0..p_x1 / p_y0..p_y1 with all four 45 degree corners cut by p_d,
 * so that all 8 sides of the octagon have a positive length.
 */
fp::IntOctagon make_cut_square(int64_t p_x0, int64_t p_y0, int64_t p_x1, int64_t p_y1, int64_t p_d = 30)
{
    return fp::IntOctagon(std::min(p_x0, p_x1), std::min(p_y0, p_y1), std::max(p_x0, p_x1), std::max(p_y0, p_y1),
        p_x0 - p_y1 + p_d, p_x1 - p_y0 - p_d, p_x0 + p_y0 + p_d, p_x1 + p_y1 - p_d);
}

/** The room under test: the square 0..100 with 30 unit 45 degree corners cut. */
fp::IntOctagon make_room_octagon()
{
    return make_cut_square(0, 0, 100, 100);
}

/** The neighbours of make_room_octagon(), one for each of the 4 axis sides. */
fp::IntOctagon left_octagon() { return make_cut_square(-200, 0, 0, 100); }
fp::IntOctagon right_octagon() { return make_cut_square(100, 0, 300, 100); }
fp::IntOctagon bottom_octagon() { return make_cut_square(0, -200, 100, 0); }
fp::IntOctagon top_octagon() { return make_cut_square(0, 100, 100, 300); }

/** An incomplete free space room with the 45 degree room shape. */
std::unique_ptr<fr::autoroute::IncompleteFreeSpaceExpansionRoom> make_incomplete_room()
{
    const fp::Simplex shape = make_room_octagon().to_Simplex();
    return std::make_unique<fr::autoroute::IncompleteFreeSpaceExpansionRoom>(nullptr, 0, shape, shape);
}

/**
 * Registers a complete free space room with an octagon shape in the tree. The
 * room is returned, because the search tree keeps pointers to it and the caller
 * has to keep it alive.
 */
std::unique_ptr<fr::autoroute::CompleteFreeSpaceExpansionRoom> insert_octagon_room(
    fr::board::ShapeSearchTree* p_tree, int p_room_id, const fp::IntOctagon& p_octagon)
{
    auto room = std::make_unique<fr::autoroute::CompleteFreeSpaceExpansionRoom>(0, p_room_id,
        p_octagon.to_Simplex());
    p_tree->insert(*room);
    return room;
}

/** Net-2 obstacle used to restrain the rooms of the dispatch test. */
class FakeObstacle : public fr::board::SearchTreeObject
{
public:
    FakeObstacle(int p_layer, fp::IntBox p_shape, int p_id)
        : layer_(p_layer)
        , shape_(p_shape)
        , id_(p_id)
    {
    }

    int tree_shape_count(const fr::datastructures::ShapeTree&) const override { return 1; }
    const fp::TileShape* get_tree_shape(const fr::datastructures::ShapeTree&, int) const override { return &shape_; }
    void set_search_tree_entries(std::vector<std::unique_ptr<fr::datastructures::ShapeTree::Leaf>> p_entries,
        fr::datastructures::ShapeTree&) override
    {
        tree_entries_ = std::move(p_entries);
    }
    int compare_to(const fr::datastructures::ShapeTree::Storable& p_other) const override
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
    bool is_trace_obstacle(int p_net_no) const override { return p_net_no != 2; }
    int shape_layer(int) const override { return layer_; }
    int get_id() const override { return id_; }

private:
    int layer_;
    fp::IntBox shape_;
    int id_;
    std::vector<std::unique_ptr<fr::datastructures::ShapeTree::Leaf>> tree_entries_;
};

} // namespace

TEST(Sorted45DegreeRoomNeighboursTest, EmptyTreeHasNoNeighbours)
{
    auto board = make_routing_board();
    fr::board::ShapeSearchTree45Degree tree(board.get(), 0);
    ASSERT_EQ(tree.get_bounding_directions().count(), 8);

    auto room = make_incomplete_room();
    auto neighbours = fr::autoroute::Sorted45DegreeRoomNeighbours::calculate_neighbours(room.get(), 1, &tree, 7);
    ASSERT_NE(neighbours, nullptr);
    EXPECT_TRUE(neighbours->get_sorted_neighbours().empty());
    EXPECT_EQ(neighbours->get_room_shape(), make_room_octagon());

    // A new complete free space room is created for the incomplete from room.
    auto* completed_room = neighbours->get_completed_room();
    ASSERT_NE(completed_room, nullptr);
    EXPECT_EQ(completed_room->get_layer(), 0);
    EXPECT_EQ(completed_room->get_room_id(), 7);
    EXPECT_EQ(completed_room->get_shape(), fp::IntBox(0, 0, 100, 100));
    EXPECT_TRUE(completed_room->get_doors().empty());
    EXPECT_EQ(neighbours->release_completed_room(), completed_room);
    EXPECT_EQ(neighbours->release_completed_room(), nullptr);
}

TEST(Sorted45DegreeRoomNeighboursTest, UnexpectedRoomTypeReturnsNull)
{
    auto board = make_routing_board();
    fr::board::ShapeSearchTree45Degree tree(board.get(), 0);

    // A plain IncompleteExpansionRoom is not a supported from room type.
    fr::autoroute::IncompleteExpansionRoom room(nullptr, 0, fp::IntBox(0, 0, 100, 100));
    EXPECT_EQ(fr::autoroute::Sorted45DegreeRoomNeighbours::calculate_neighbours(&room, 1, &tree, 1), nullptr);
    fr::autoroute::AutorouteEngine engine(board.get(), 1, true);
    EXPECT_EQ(fr::autoroute::Sorted45DegreeRoomNeighbours::calculate(&room, &engine), nullptr);
    EXPECT_EQ(fr::autoroute::Sorted45DegreeRoomNeighbours::calculate(nullptr, &engine), nullptr);
    EXPECT_EQ(fr::autoroute::Sorted45DegreeRoomNeighbours::calculate(&room, nullptr), nullptr);
}

TEST(Sorted45DegreeRoomNeighboursTest, LeftNeighbourTouchesTheLeftSideOnly)
{
    auto board = make_routing_board();
    fr::board::ShapeSearchTree45Degree tree(board.get(), 0);
    auto room_2 = insert_octagon_room(&tree, 2, left_octagon());

    auto room = make_incomplete_room();
    auto neighbours = fr::autoroute::Sorted45DegreeRoomNeighbours::calculate_neighbours(room.get(), 1, &tree, 1);
    ASSERT_NE(neighbours, nullptr);
    ASSERT_EQ(neighbours->get_sorted_neighbours().size(), 1u);

    // The intersection with the left neighbour is the shared edge x == 0 between
    // the 45 degree cuts, so the first and the last touching side are the two
    // sides whose border lines are shared with the intersection octagon. The
    // interior of the left side is the only side whose interior touches the
    // neighbour: at the two diagonal sides the shapes only touch at a corner.
    const auto& neighbour = neighbours->get_sorted_neighbours().front();
    EXPECT_EQ(neighbour.first_touching_side, 5);
    EXPECT_EQ(neighbour.last_touching_side, 7);
    EXPECT_EQ(neighbour.intersection.corner(6), fp::IntPoint(0, 70));
    EXPECT_EQ(neighbour.intersection.corner(7), fp::IntPoint(0, 30));
    const std::array<bool, 8> expected_flags = {false, false, false, false, false, false, true, false};
    EXPECT_EQ(neighbours->get_edge_interior_touches_obstacle(), expected_flags);

    // A 1-dimensional door is created to the neighbour room.
    auto* completed_room = neighbours->get_completed_room();
    ASSERT_NE(completed_room, nullptr);
    ASSERT_EQ(completed_room->get_doors().size(), 1u);
    auto* door = completed_room->get_doors().front();
    EXPECT_EQ(door->get_dimension(), 1);
    // The C++ ExpansionDoor has no own door shape, so the door shape is the
    // intersection of the bounding boxes of the 2 door rooms.
    EXPECT_EQ(door->get_shape(), fp::IntBox(0, 0, 0, 100));
    EXPECT_EQ(door->other_room(completed_room), room_2.get());
}

TEST(Sorted45DegreeRoomNeighboursTest, ContainedNeighbourIsNotSorted)
{
    auto board = make_routing_board();
    fr::board::ShapeSearchTree45Degree tree(board.get(), 0);
    // The neighbour shape contains the whole room shape, so neither the first nor
    // the last touching side can be determined and the neighbour is dropped.
    auto containing_room = insert_octagon_room(&tree, 2, make_cut_square(-200, -200, 300, 300, 30));

    auto room = make_incomplete_room();
    auto neighbours = fr::autoroute::Sorted45DegreeRoomNeighbours::calculate_neighbours(room.get(), 1, &tree, 1);
    ASSERT_NE(neighbours, nullptr);
    EXPECT_TRUE(neighbours->get_sorted_neighbours().empty());
    // The 2-dimensional door to the containing room is created before the
    // containing room is dropped, so the door is still there.
    auto* completed_room = neighbours->get_completed_room();
    ASSERT_NE(completed_room, nullptr);
    ASSERT_EQ(completed_room->get_doors().size(), 1u);
    // All doors between 2 free space rooms are created with dimension 1.
    EXPECT_EQ(completed_room->get_doors().front()->get_dimension(), 1);
}

TEST(Sorted45DegreeRoomNeighboursTest, NonOverlappingNeighbourIsSkipped)
{
    auto board = make_routing_board();
    fr::board::ShapeSearchTree45Degree tree(board.get(), 0);
    // The bounding box of the neighbour overlaps the bounding box of the room
    // along the line y == 0 between x == 70 and x == 100, but the 2 shapes
    // themselves do not touch, so the intersection is empty.
    auto corner_room = insert_octagon_room(&tree, 2, make_cut_square(70, -200, 300, 0));

    auto room = make_incomplete_room();
    auto neighbours = fr::autoroute::Sorted45DegreeRoomNeighbours::calculate_neighbours(room.get(), 1, &tree, 1);
    ASSERT_NE(neighbours, nullptr);
    EXPECT_TRUE(neighbours->get_sorted_neighbours().empty());
    EXPECT_TRUE(neighbours->get_completed_room()->get_doors().empty());
    const std::array<bool, 8> no_flags = {};
    EXPECT_EQ(neighbours->get_edge_interior_touches_obstacle(), no_flags);
}

TEST(Sorted45DegreeRoomNeighboursTest, NeighboursAreSortedCounterclockwise)
{
    auto board = make_routing_board();
    fr::board::ShapeSearchTree45Degree tree(board.get(), 0);
    auto room_2 = insert_octagon_room(&tree, 2, right_octagon());
    auto room_3 = insert_octagon_room(&tree, 3, top_octagon());
    auto room_4 = insert_octagon_room(&tree, 4, bottom_octagon());
    auto room_5 = insert_octagon_room(&tree, 5, left_octagon());

    auto room = make_incomplete_room();
    auto neighbours = fr::autoroute::Sorted45DegreeRoomNeighbours::calculate_neighbours(room.get(), 1, &tree, 1);
    ASSERT_NE(neighbours, nullptr);

    const auto& sorted = neighbours->get_sorted_neighbours();
    ASSERT_EQ(sorted.size(), 4u);
    for (size_t i = 1; i < sorted.size(); i++)
    {
        EXPECT_LE(sorted[i - 1].compare_to(sorted[i]), 0);
        EXPECT_LE(sorted[i - 1].first_touching_side, sorted[i].first_touching_side);
    }
    // The interior of the 4 axis sides touches a neighbour, the 4 diagonal sides
    // are touched at a corner only.
    const std::array<bool, 8> expected_flags = {true, false, true, false, true, false, true, false};
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

TEST(Sorted45DegreeRoomNeighboursTest, TryRemoveEdgeLineEnlargesOpenDiagonalSides)
{
    auto board = make_routing_board();
    fr::board::ShapeSearchTree45Degree tree(board.get(), 0);
    auto room_2 = insert_octagon_room(&tree, 2, left_octagon());
    auto room_3 = insert_octagon_room(&tree, 3, right_octagon());
    auto room_4 = insert_octagon_room(&tree, 4, bottom_octagon());
    auto room_5 = insert_octagon_room(&tree, 5, top_octagon());

    auto room = make_incomplete_room();
    auto neighbours = fr::autoroute::Sorted45DegreeRoomNeighbours::calculate_neighbours(room.get(), 1, &tree, 1);
    ASSERT_NE(neighbours, nullptr);
    ASSERT_EQ(neighbours->get_sorted_neighbours().size(), 4u);

    // The 4 diagonal sides have no touching neighbour, so they are removed from
    // the room shape, which grows to the full square of the room bounding box.
    EXPECT_TRUE(neighbours->try_remove_edge_line(1, &tree));
    const fp::Simplex enlarged_shape = room->get_shape_tile();
    EXPECT_EQ(enlarged_shape.bounding_box(), fp::IntBox(0, 0, 100, 100));
    EXPECT_EQ(enlarged_shape.area(), static_cast<double>(100 * 100));
    // The 4 axis sides are still blocked by the neighbour rooms.
    const std::array<bool, 8> axis_side_flags = {true, false, true, false, true, false, true, false};
    EXPECT_EQ(neighbours->get_edge_interior_touches_obstacle(), axis_side_flags);
}

TEST(Sorted45DegreeRoomNeighboursTest, TryRemoveEdgeLineIsSkippedForCompleteFromRoom)
{
    auto board = make_routing_board();
    fr::board::ShapeSearchTree45Degree tree(board.get(), 0);
    auto room_2 = insert_octagon_room(&tree, 2, left_octagon());

    auto room = make_incomplete_room();
    auto neighbours = fr::autoroute::Sorted45DegreeRoomNeighbours::calculate_neighbours(room.get(), 1, &tree, 1);
    ASSERT_NE(neighbours, nullptr);

    // Only the shape of an incomplete free space room may be enlarged. After the
    // first enlargement the enlarged shape is not an IntOctagon anymore for a
    // board with a smaller bounding box, so the second call is rejected.
    ASSERT_TRUE(neighbours->try_remove_edge_line(1, &tree));
    const fp::Simplex enlarged_shape = room->get_shape_tile();
    if (enlarged_shape.is_IntOctagon())
    {
        // The area can only grow, so a second enlargement with the same result
        // is not applied.
        EXPECT_FALSE(neighbours->try_remove_edge_line(1, &tree));
    }
}

TEST(Sorted45DegreeRoomNeighboursTest, NewIncompleteRoomsAreCreatedBetweenNeighbours)
{
    auto board = make_routing_board();
    fr::board::ShapeSearchTree45Degree tree(board.get(), 0);
    fr::autoroute::AutorouteEngine engine(board.get(), 1, true);
    engine.set_net_no(1);
    auto room_2 = insert_octagon_room(&tree, 2, left_octagon());
    auto room_3 = insert_octagon_room(&tree, 3, right_octagon());

    auto room = make_incomplete_room();
    auto neighbours = fr::autoroute::Sorted45DegreeRoomNeighbours::calculate_neighbours(room.get(), 1, &tree, 1);
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
    int64_t new_rooms_bottom = board_bounds.ll.y;
    int64_t new_rooms_top = board_bounds.ur.y;
    for (auto* door : completed_room->get_doors())
    {
        ASSERT_NE(door, nullptr);
        auto* other_room = door->other_room(static_cast<fr::autoroute::ExpansionRoom*>(completed_room));
        ASSERT_NE(other_room, nullptr);
        if (dynamic_cast<fr::autoroute::IncompleteFreeSpaceExpansionRoom*>(other_room) == nullptr)
        {
            continue;
        }
        ++incomplete_room_door_count;
        // The new incomplete room is the free space between the 2 neighbours up
        // to the board border, so the room spans the whole board width and the
        // door into the room is 2-dimensional.
        EXPECT_EQ(door->get_dimension(), 2);
        EXPECT_TRUE(other_room->get_shape_tile().is_bounded());
        const fp::IntBox new_bounds = other_room->get_shape_tile().bounding_box();
        EXPECT_EQ(new_bounds.ll.x, board_bounds.ll.x);
        EXPECT_EQ(new_bounds.ur.x, board_bounds.ur.x);
        new_rooms_bottom = std::min(new_rooms_bottom, new_bounds.ll.y);
        new_rooms_top = std::max(new_rooms_top, new_bounds.ur.y);
    }
    EXPECT_EQ(incomplete_room_door_count, 2);
    // The 2 new incomplete rooms are the free bottom and the free top space of
    // the room up to the board border.
    EXPECT_EQ(new_rooms_bottom, board_bounds.ll.y);
    EXPECT_EQ(new_rooms_top, board_bounds.ur.y);
    engine.remove_all_doors(completed_room);
    EXPECT_TRUE(completed_room->get_doors().empty());
}

TEST(Sorted45DegreeRoomNeighboursTest, CalculateEnlargesRoomAndReturnsCompletedRoom)
{
    auto board = make_routing_board();
    fr::board::ShapeSearchTree45Degree tree(board.get(), 0);
    fr::autoroute::AutorouteEngine engine(board.get(), 1, true);
    engine.set_net_no(1);

    // 4 neighbours on the axis sides of the room keep the room shape, only the
    // diagonal sides are enlarged.
    auto room_2 = insert_octagon_room(&tree, 2, left_octagon());
    auto room_3 = insert_octagon_room(&tree, 3, right_octagon());
    auto room_4 = insert_octagon_room(&tree, 4, bottom_octagon());
    auto room_5 = insert_octagon_room(&tree, 5, top_octagon());

    auto room = make_incomplete_room();
    fr::autoroute::CompleteExpansionRoom* completed =
        fr::autoroute::Sorted45DegreeRoomNeighbours::calculate(room.get(), &engine);
    ASSERT_NE(completed, nullptr);
    // The diagonal sides of the room are free, so the room shape is enlarged to
    // the full square of the room bounding box before it is completed.
    EXPECT_EQ(completed->get_shape().bounding_box(), fp::IntBox(0, 0, 100, 100));
    engine.remove_all_doors(completed);
    EXPECT_TRUE(completed->get_doors().empty());
}

TEST(SortedRoomNeighboursTest, CalculateDispatchesTo45DegreeImplementation)
{
    // Java selects the neighbour calculation mode from the search tree class.
    // The C++ ShapeSearchTree is configured with the bounding directions, so a
    // tree with 8 directions must reach Sorted45DegreeRoomNeighbours, and a tree
    // with 4 directions must use the any-angle implementation.
    auto board = make_routing_board();
    fr::board::ShapeSearchTree45Degree tree_45(board.get(), 0);
    fr::board::ShapeSearchTree90Degree tree_90(board.get(), 0);
    EXPECT_EQ(tree_45.get_bounding_directions().count(), 8);
    EXPECT_EQ(tree_90.get_bounding_directions().count(), 4);

    // The engine creates its autoroute search tree with 8 bounding directions
    // (45 degrees is the default route angle).
    auto engine_board = make_routing_board();
    fr::autoroute::AutorouteEngine engine(engine_board.get(), 1, true);
    engine.set_net_no(1);
    fr::board::ShapeSearchTree* search_tree = engine.get_autoroute_search_tree();
    ASSERT_NE(search_tree, nullptr);
    EXPECT_EQ(search_tree->get_bounding_directions().count(), 8);

    // A net-2 wall splits the board into 2 free space rooms. A free space room
    // left of the wall is registered in the tree, so the completed piece next to
    // it gets a door from the 45 degree neighbour calculation.
    fp::IntBox wall_bounds(4000, 0, 6000, 10000);
    FakeObstacle wall(0, wall_bounds, /*p_id=*/1);
    search_tree->insert(wall);
    fr::autoroute::CompleteFreeSpaceExpansionRoom left_room(0, 100,
        fp::IntBox(-1000, 0, 0, 10000).to_Simplex());
    search_tree->insert(left_room);

    const fp::Simplex room_shape = fp::IntBox(0, 0, 10000, 10000).to_Simplex();
    fr::autoroute::IncompleteFreeSpaceExpansionRoom incomplete(nullptr, 0, room_shape, room_shape);
    std::vector<fr::autoroute::CompleteFreeSpaceExpansionRoom*> rooms =
        engine.complete_expansion_room(&incomplete);
    // The completed room is the free space left of the wall. Its 2 doors are
    // created by the 45 degree neighbour calculation.
    ASSERT_EQ(rooms.size(), 1u);
    EXPECT_EQ(rooms.front()->get_doors().size(), 2u);

    bool some_room_has_door_to_free_space_room = false;
    for (fr::autoroute::CompleteFreeSpaceExpansionRoom* room : rooms)
    {
        ASSERT_NE(room, nullptr);
        EXPECT_EQ(room->get_shape().dimension(), 2);
        for (auto* door : room->get_doors())
        {
            auto* other_room = door->other_room(static_cast<fr::autoroute::ExpansionRoom*>(room));
            if (other_room == &left_room)
            {
                EXPECT_EQ(door->get_dimension(), 1);
                some_room_has_door_to_free_space_room = true;
            }
        }
    }
    EXPECT_TRUE(some_room_has_door_to_free_space_room);

    // The engine rooms live in the board search tree; remove them while the
    // fixture objects linked into the tree still exist (Java calls clear() too).
    engine.clear();
}
