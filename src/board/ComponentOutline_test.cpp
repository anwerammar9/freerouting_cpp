#include <memory>
#include <vector>

#include "board/BasicBoard.h"
#include "board/ComponentOutline.h"
#include "board/FixedState.h"
#include "board/Layer.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/Point.h"
#include "geometry/planar/Shape.h"
#include "geometry/planar/Vector.h"
#include "gtest/gtest.h"
#include "rules/BoardRules.h"

using namespace freerouting::board;
namespace planar = freerouting::geometry::planar;

namespace
{

constexpr FixedState NOT_FIXED = FixedState::NOT_FIXED;

std::shared_ptr<BasicBoard> make_board()
{
    std::vector<std::shared_ptr<Layer>> layers;
    layers.push_back(std::make_shared<Layer>("Front", true));
    layers.push_back(std::make_shared<Layer>("Back", true));
    LayerStructure layer_structure(layers);
    auto board_rules = std::make_unique<::freerouting::rules::BoardRules>(layer_structure, 10);
    return std::make_shared<BasicBoard>(layer_structure, std::move(board_rules));
}

std::shared_ptr<planar::Area> make_square_area()
{
    return std::make_shared<planar::PolygonArea>(
        std::vector<planar::IntPoint>{planar::IntPoint(0, 0), planar::IntPoint(100, 0),
                                     planar::IntPoint(100, 100), planar::IntPoint(0, 100)});
}

bool boxes_equal(const planar::IntBox& p_a, const planar::IntBox& p_b)
{
    return p_a.ll.x == p_b.ll.x && p_a.ll.y == p_b.ll.y && p_a.ur.x == p_b.ur.x
           && p_a.ur.y == p_b.ur.y;
}

} // namespace

TEST(ComponentOutlineTest, FrontLayerIsZero)
{
    auto board = make_board();
    ComponentOutline outline(make_square_area(), true, planar::IntVector(0, 0), 0.0, 1, 0,
                             NOT_FIXED, board.get());
    EXPECT_EQ(outline.get_layer(), 0);
    EXPECT_EQ(outline.first_layer(), 0);
    EXPECT_EQ(outline.last_layer(), 0);
    EXPECT_TRUE(outline.is_on_layer(0));
    EXPECT_FALSE(outline.is_on_layer(1));
}

TEST(ComponentOutlineTest, BackLayerIsLast)
{
    auto board = make_board();
    ComponentOutline outline(make_square_area(), false, planar::IntVector(0, 0), 0.0, 1, 0,
                             NOT_FIXED, board.get());
    EXPECT_EQ(outline.get_layer(), 1);
    EXPECT_TRUE(outline.is_on_layer(1));
    EXPECT_FALSE(outline.is_on_layer(0));
}

TEST(ComponentOutlineTest, BasicBoundingBox)
{
    auto board = make_board();
    const planar::IntBox expected(0, 0, 100, 100);
    ComponentOutline outline(make_square_area(), true, planar::IntVector(0, 0), 0.0, 1, 0,
                             NOT_FIXED, board.get());
    EXPECT_TRUE(boxes_equal(outline.bounding_box(), expected));
}

TEST(ComponentOutlineTest, TranslateBy)
{
    auto board = make_board();
    ComponentOutline outline(make_square_area(), true, planar::IntVector(0, 0), 0.0, 1, 0,
                             NOT_FIXED, board.get());
    outline.translate_by(planar::IntVector(50, 50));
    const planar::IntBox expected(50, 50, 150, 150);
    EXPECT_TRUE(boxes_equal(outline.bounding_box(), expected));
}

TEST(ComponentOutlineTest, ChangePlacementSideFlipsLayer)
{
    auto board = make_board();
    ComponentOutline outline(make_square_area(), true, planar::IntVector(0, 0), 0.0, 1, 0,
                             NOT_FIXED, board.get());
    EXPECT_EQ(outline.get_layer(), 0);
    outline.change_placement_side(planar::IntPoint(0, 0));
    EXPECT_EQ(outline.get_layer(), 1);
    EXPECT_TRUE(outline.is_front() == false);
}

TEST(ComponentOutlineTest, IsObstacleAlwaysFalse)
{
    auto board = make_board();
    ComponentOutline outline(make_square_area(), true, planar::IntVector(0, 0), 0.0, 1, 0,
                             NOT_FIXED, board.get());
    EXPECT_FALSE(outline.is_obstacle(nullptr));
    EXPECT_FALSE(outline.is_obstacle(&outline));
}

TEST(ComponentOutlineTest, TileShapeCountZero)
{
    auto board = make_board();
    ComponentOutline outline(make_square_area(), true, planar::IntVector(0, 0), 0.0, 1, 0,
                             NOT_FIXED, board.get());
    EXPECT_EQ(outline.tile_shape_count(), 0);
}

TEST(ComponentOutlineTest, CopyPreservesGeometry)
{
    auto board = make_board();
    ComponentOutline outline(make_square_area(), false, planar::IntVector(20, 30), 45.0, 5, 1,
                             NOT_FIXED, board.get());
    auto copy = outline.copy(99);
    ASSERT_NE(copy, nullptr);
    const auto* component_copy = dynamic_cast<ComponentOutline*>(copy.get());
    ASSERT_NE(component_copy, nullptr);
    EXPECT_TRUE(boxes_equal(component_copy->bounding_box(), outline.bounding_box()));
    EXPECT_EQ(component_copy->get_layer(), outline.get_layer());
    EXPECT_EQ(component_copy->get_rotation_in_degree(), 45.0);
}