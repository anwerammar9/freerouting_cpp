#include <cmath>
#include <memory>
#include <vector>

#include "board/BasicBoard.h"
#include "board/FixedState.h"
#include "board/Layer.h"
#include "board/ObstacleArea.h"
#include "board/Trace.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/Point.h"
#include "geometry/planar/PolygonShape.h"
#include "geometry/planar/Shape.h"
#include "geometry/planar/Vector.h"
#include "gtest/gtest.h"
#include "rules/BoardRules.h"

using namespace freerouting::board;
namespace planar = freerouting::geometry::planar;

namespace
{

constexpr FixedState FIXED = FixedState::NOT_FIXED;

std::shared_ptr<BasicBoard> make_board(int p_layer_count)
{
    std::vector<std::shared_ptr<Layer>> layers;
    for (int i = 0; i < p_layer_count; ++i)
    {
        layers.push_back(std::make_shared<Layer>("Layer" + std::to_string(i), true));
    }
    LayerStructure layer_structure(layers);
    auto board_rules = std::make_unique<::freerouting::rules::BoardRules>(layer_structure, 10);
    return std::make_shared<BasicBoard>(layer_structure, std::move(board_rules));
}

std::shared_ptr<planar::Area> make_square_area()
{
    return std::make_shared<planar::PolygonArea>(
        std::vector<planar::IntPoint>{planar::IntPoint(0, 0), planar::IntPoint(10, 0), planar::IntPoint(10, 10), planar::IntPoint(0, 10)});
}

ObstacleArea make_keepout(const std::shared_ptr<BasicBoard>& p_board,
                          int p_layer,
                          const planar::Vector& p_translation,
                          double p_rotation,
                          bool p_side_changed,
                          const std::vector<int>& p_net_nos = {})
{
    return ObstacleArea(make_square_area(), p_layer, p_translation, p_rotation, p_side_changed, p_net_nos, 0, 1, 0,
        "keepout1", FIXED, p_board.get());
}

// Minimal concrete Trace subclass to exercise the is_trace branch of is_obstacle.
class FakeTrace : public Trace
{
public:
    FakeTrace(int p_net_no)
        : Trace(0, 100, {p_net_no}, 0, 2, 0, FIXED)
    {
    }

    const planar::Point& first_corner() const override { return corner_; }
    const planar::Point& last_corner() const override { return corner_; }
    const planar::Point& corner(int p_index) const override { (void)p_index; return corner_; }
    const std::vector<std::shared_ptr<planar::Point>>& corners() const override { return corners_; }
    int corner_count() const override { return 1; }
    double get_length() const override { return 0.0; }

private:
    planar::IntPoint corner_ = planar::IntPoint(0, 0);
    std::vector<std::shared_ptr<planar::Point>> corners_;
};

} // namespace

TEST(ObstacleAreaTest, LayerAccess)
{
    auto board = make_board(4);
    ObstacleArea area = make_keepout(board, 2, planar::IntVector(0, 0), 0.0, false);
    EXPECT_EQ(area.get_layer(), 2);
    EXPECT_EQ(area.first_layer(), 2);
    EXPECT_EQ(area.last_layer(), 2);
    EXPECT_TRUE(area.is_on_layer(2));
    EXPECT_FALSE(area.is_on_layer(1));
    EXPECT_EQ(area.shape_layer(0), 2);
}

TEST(ObstacleAreaTest, GetAreaTranslatesRelativeArea)
{
    auto board = make_board(4);
    ObstacleArea area = make_keepout(board, 1, planar::IntVector(10, 20), 0.0, false);
    auto absolute = area.get_area();
    ASSERT_NE(absolute, nullptr);
    EXPECT_EQ(absolute->bounding_box(), planar::IntBox(10, 20, 20, 30));
}

TEST(ObstacleAreaTest, GetAreaAppliesMirrorWhenSideChanged)
{
    auto board = make_board(4);
    ObstacleArea area = make_keepout(board, 1, planar::IntVector(5, 0), 0.0, true);
    auto absolute = area.get_area();
    ASSERT_NE(absolute, nullptr);
    EXPECT_EQ(absolute->bounding_box(), planar::IntBox(-5, 0, 5, 10));
}

TEST(ObstacleAreaTest, GetAreaAppliesQuarterTurnRotation)
{
    auto board = make_board(4);
    ObstacleArea area = make_keepout(board, 1, planar::IntVector(0, 0), 90.0, false);
    auto absolute = area.get_area();
    ASSERT_NE(absolute, nullptr);
    EXPECT_EQ(absolute->bounding_box(), planar::IntBox(-10, 0, 0, 10));
}

TEST(ObstacleAreaTest, GetAreaAppliesRotationThenTranslation)
{
    auto board = make_board(4);
    ObstacleArea area = make_keepout(board, 1, planar::IntVector(10, 20), 180.0, false);
    auto absolute = area.get_area();
    ASSERT_NE(absolute, nullptr);
    EXPECT_EQ(absolute->bounding_box(), planar::IntBox(0, 10, 10, 20));
}

TEST(ObstacleAreaTest, GetAreaIsCached)
{
    auto board = make_board(4);
    ObstacleArea area = make_keepout(board, 1, planar::IntVector(10, 20), 0.0, false);
    auto first = area.get_area();
    auto second = area.get_area();
    EXPECT_EQ(first.get(), second.get());
}

TEST(ObstacleAreaTest, TranslateByUpdatesAbsoluteArea)
{
    auto board = make_board(4);
    ObstacleArea area = make_keepout(board, 1, planar::IntVector(10, 20), 0.0, false);
    area.translate_by(planar::IntVector(1, 2));
    auto absolute = area.get_area();
    ASSERT_NE(absolute, nullptr);
    EXPECT_EQ(absolute->bounding_box(), planar::IntBox(11, 22, 21, 32));
}

TEST(ObstacleAreaTest, Turn90DegreeMovesTranslationAroundPole)
{
    auto board = make_board(4);
    ObstacleArea area = make_keepout(board, 1, planar::IntVector(10, 0), 0.0, false);
    area.turn_90_degree(1, planar::IntPoint(0, 0));
    EXPECT_EQ(area.get_rotation_in_degree(), 90.0);
    auto absolute = area.get_area();
    ASSERT_NE(absolute, nullptr);
    EXPECT_EQ(absolute->bounding_box(), planar::IntBox(-10, 10, 0, 20));
}

TEST(ObstacleAreaTest, Turn90DegreeWrapsRotation)
{
    auto board = make_board(4);
    ObstacleArea area = make_keepout(board, 1, planar::IntVector(10, 0), 90.0, false);
    area.turn_90_degree(3, planar::IntPoint(0, 0));
    EXPECT_EQ(area.get_rotation_in_degree(), 0.0);
}

TEST(ObstacleAreaTest, RotateApproxRotatesTranslation)
{
    auto board = make_board(4);
    ObstacleArea area = make_keepout(board, 1, planar::IntVector(0, 10), 0.0, false);
    area.rotate_approx(90.0, planar::FloatPoint(0.0, 0.0));
    EXPECT_DOUBLE_EQ(area.get_rotation_in_degree(), 90.0);
    auto absolute = area.get_area();
    ASSERT_NE(absolute, nullptr);
    EXPECT_EQ(absolute->bounding_box(), planar::IntBox(-20, 0, -10, 10));
}

TEST(ObstacleAreaTest, ChangePlacementSideFlipsLayerAndMirrors)
{
    auto board = make_board(4);
    ObstacleArea area = make_keepout(board, 1, planar::IntVector(5, 0), 0.0, false);
    area.change_placement_side(planar::IntPoint(0, 0));
    EXPECT_EQ(area.get_layer(), 2);
    EXPECT_TRUE(area.get_side_changed());
    auto absolute = area.get_area();
    ASSERT_NE(absolute, nullptr);
    EXPECT_EQ(absolute->bounding_box(), planar::IntBox(-15, 0, -5, 10));
}

TEST(ObstacleAreaTest, IsObstacleIgnoresSameNet)
{
    auto board = make_board(4);
    ObstacleArea area = make_keepout(board, 1, planar::IntVector(0, 0), 0.0, false, {3});
    ObstacleArea other = make_keepout(board, 1, planar::IntVector(0, 0), 0.0, false, {3});
    EXPECT_FALSE(area.is_obstacle(&other));

    ObstacleArea different_net = make_keepout(board, 1, planar::IntVector(0, 0), 0.0, false, {5});
    EXPECT_FALSE(area.is_obstacle(&different_net));
}

TEST(ObstacleAreaTest, IsObstacleForTraceOnOtherNet)
{
    auto board = make_board(4);
    ObstacleArea area = make_keepout(board, 1, planar::IntVector(0, 0), 0.0, false, {3});
    FakeTrace trace(7);
    EXPECT_TRUE(area.is_obstacle(&trace));

    FakeTrace same_net_trace(3);
    EXPECT_FALSE(area.is_obstacle(&same_net_trace));
}

TEST(ObstacleAreaTest, TileShapeCountAndGetTileShape)
{
    auto board = make_board(4);
    ObstacleArea area = make_keepout(board, 1, planar::IntVector(0, 0), 0.0, false);
    EXPECT_EQ(area.tile_shape_count(), 1);
    planar::IntOctagon tile = area.get_tile_shape(0);
    EXPECT_FALSE(tile.is_empty());

    planar::IntOctagon out_of_range = area.get_tile_shape(5);
    EXPECT_TRUE(out_of_range.is_empty());
}

TEST(ObstacleAreaTest, CopyPreservesProperties)
{
    auto board = make_board(4);
    ObstacleArea area = make_keepout(board, 2, planar::IntVector(10, 10), 0.0, false, {3});
    auto copied = area.copy(99);
    ASSERT_NE(copied, nullptr);
    auto* area_copy = dynamic_cast<ObstacleArea*>(copied.get());
    ASSERT_NE(area_copy, nullptr);
    EXPECT_EQ(area_copy->get_layer(), 2);
    EXPECT_EQ(area_copy->bounding_box(), planar::IntBox(10, 10, 20, 20));
    EXPECT_EQ(area_copy->get_name(), "keepout1");
    EXPECT_TRUE(area_copy->contains_net(3));
}

TEST(ObstacleAreaTest, NullRelativeAreaWarnsAndReturnsNull)
{
    auto board = make_board(4);
    std::shared_ptr<planar::Area> null_area;
    ObstacleArea area(null_area, 1, planar::IntVector(0, 0), 0.0, false, std::vector<int>{}, 0, 1, 0, "empty", FIXED,
        board.get());
    EXPECT_EQ(area.get_area(), nullptr);
    EXPECT_EQ(area.tile_shape_count(), 0);
}