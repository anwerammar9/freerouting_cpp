#include <memory>
#include <vector>

#include "board/BasicBoard.h"
#include "board/ConductionArea.h"
#include "board/ComponentObstacleArea.h"
#include "board/FixedState.h"
#include "board/Layer.h"
#include "board/SearchTreeManager.h"
#include "board/Trace.h"
#include "board/Via.h"
#include "board/ViaObstacleArea.h"
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
        std::vector<planar::IntPoint>{planar::IntPoint(0, 0), planar::IntPoint(10, 0),
                                     planar::IntPoint(10, 10), planar::IntPoint(0, 10)});
}

std::shared_ptr<planar::Area> make_small_area()
{
    return std::make_shared<planar::PolygonArea>(
        std::vector<planar::IntPoint>{planar::IntPoint(0, 0), planar::IntPoint(3, 0),
                                     planar::IntPoint(3, 3), planar::IntPoint(0, 3)});
}

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

class FakeVia : public Via
{
public:
    FakeVia(int p_net_no)
        : Via(nullptr, planar::IntPoint(1, 1), {p_net_no}, 0, 2, 0, FIXED, false, nullptr)
    {
    }
};

} // namespace

// ==================== ComponentObstacleArea ====================

TEST(ComponentObstacleAreaTest, ConstructorSetsLayerAndComponentNo)
{
    auto board = make_board(4);
    ComponentObstacleArea area(make_square_area(), 2, planar::IntVector(0, 0), 0.0, false, 0, 1, 5,
        "comp_keepout", FIXED, board.get());
    EXPECT_EQ(area.get_layer(), 2);
    EXPECT_EQ(area.get_component_no(), 5);
    EXPECT_TRUE(area.is_on_layer(2));
    EXPECT_FALSE(area.is_on_layer(1));
}

TEST(ComponentObstacleAreaTest, IsObstacleTrueForDifferentComponent)
{
    auto board = make_board(4);
    ComponentObstacleArea area(make_square_area(), 2, planar::IntVector(0, 0), 0.0, false, 0, 1, 5,
        "comp_keepout", FIXED, board.get());
    ComponentObstacleArea other(make_square_area(), 2, planar::IntVector(0, 0), 0.0, false, 0, 2, 6,
        "other", FIXED, board.get());
    EXPECT_TRUE(area.is_obstacle(&other));
}

TEST(ComponentObstacleAreaTest, IsObstacleFalseForSameComponent)
{
    auto board = make_board(4);
    ComponentObstacleArea area(make_square_area(), 2, planar::IntVector(0, 0), 0.0, false, 0, 1, 5,
        "comp_keepout", FIXED, board.get());
    ComponentObstacleArea same(make_square_area(), 2, planar::IntVector(0, 0), 0.0, false, 0, 2, 5,
        "same_comp", FIXED, board.get());
    EXPECT_FALSE(area.is_obstacle(&same));
}

TEST(ComponentObstacleAreaTest, IsObstacleFalseForSelf)
{
    auto board = make_board(4);
    ComponentObstacleArea area(make_square_area(), 2, planar::IntVector(0, 0), 0.0, false, 0, 1, 5,
        "comp_keepout", FIXED, board.get());
    EXPECT_FALSE(area.is_obstacle(&area));
}

TEST(ComponentObstacleAreaTest, IsObstacleFalseForNonComponent)
{
    auto board = make_board(4);
    ComponentObstacleArea area(make_square_area(), 2, planar::IntVector(0, 0), 0.0, false, 0, 1, 5,
        "comp_keepout", FIXED, board.get());
    FakeTrace trace(0);
    EXPECT_FALSE(area.is_obstacle(&trace));
}

TEST(ComponentObstacleAreaTest, IsTraceObstacleAlwaysFalse)
{
    auto board = make_board(4);
    ComponentObstacleArea area(make_square_area(), 2, planar::IntVector(0, 0), 0.0, false, 0, 1, 5,
        "comp_keepout", FIXED, board.get());
    EXPECT_FALSE(area.is_trace_obstacle(0));
    EXPECT_FALSE(area.is_trace_obstacle(99));
}

TEST(ComponentObstacleAreaTest, CopyPreservesProperties)
{
    auto board = make_board(4);
    ComponentObstacleArea area(make_square_area(), 2, planar::IntVector(10, 10), 0.0, false, 3, 1, 5,
        "comp_keepout", FIXED, board.get());
    auto copied = area.copy(99);
    ASSERT_NE(copied, nullptr);
    auto* area_copy = dynamic_cast<ComponentObstacleArea*>(copied.get());
    ASSERT_NE(area_copy, nullptr);
    EXPECT_EQ(area_copy->get_layer(), 2);
    EXPECT_EQ(area_copy->get_component_no(), 5);
    EXPECT_EQ(area_copy->get_clearance_class(), 3);
    EXPECT_EQ(area_copy->get_name(), "comp_keepout");
}

TEST(ComponentObstacleAreaTest, GetAreaTranslatesCorrectly)
{
    auto board = make_board(4);
    ComponentObstacleArea area(make_square_area(), 1, planar::IntVector(10, 20), 0.0, false, 0, 1, 0,
        "comp_keepout", FIXED, board.get());
    auto absolute = area.get_area();
    ASSERT_NE(absolute, nullptr);
    EXPECT_EQ(absolute->bounding_box(), planar::IntBox(10, 20, 20, 30));
}

TEST(ComponentObstacleAreaTest, TileShapeCount)
{
    auto board = make_board(4);
    ComponentObstacleArea area(make_square_area(), 1, planar::IntVector(0, 0), 0.0, false, 0, 1, 0,
        "comp_keepout", FIXED, board.get());
    EXPECT_EQ(area.tile_shape_count(), 1);
}

// ==================== ViaObstacleArea ====================

TEST(ViaObstacleAreaTest, ConstructorSetsLayerAndNets)
{
    auto board = make_board(4);
    ViaObstacleArea area(make_square_area(), 1, planar::IntVector(0, 0), 0.0, false, {3, 7}, 0, 1, 0,
        "via_keepout", FIXED, board.get());
    EXPECT_EQ(area.get_layer(), 1);
    EXPECT_TRUE(area.contains_net(3));
    EXPECT_TRUE(area.contains_net(7));
    EXPECT_FALSE(area.contains_net(5));
}

TEST(ViaObstacleAreaTest, ConstructorWithoutNetNos)
{
    auto board = make_board(4);
    ViaObstacleArea area(make_square_area(), 1, planar::IntVector(0, 0), 0.0, false, 0, 1, 0,
        "via_keepout", FIXED, board.get());
    EXPECT_TRUE(area.get_net_nos().empty());
}

TEST(ViaObstacleAreaTest, IsObstacleTrueForViaOnOtherNet)
{
    auto board = make_board(4);
    ViaObstacleArea area(make_square_area(), 1, planar::IntVector(0, 0), 0.0, false, {3}, 0, 1, 0,
        "via_keepout", FIXED, board.get());
    FakeVia other_via(7);
    EXPECT_TRUE(area.is_obstacle(&other_via));
}

TEST(ViaObstacleAreaTest, IsObstacleFalseForAnotherViaKeepout)
{
    auto board = make_board(4);
    ViaObstacleArea area(make_square_area(), 1, planar::IntVector(0, 0), 0.0, false, {3}, 0, 1, 0,
        "via_keepout", FIXED, board.get());
    ViaObstacleArea other(make_small_area(), 1, planar::IntVector(0, 0), 0.0, false, {7}, 0, 2, 0,
        "other_keepout", FIXED, board.get());
    EXPECT_FALSE(area.is_obstacle(&other));
}

TEST(ViaObstacleAreaTest, IsObstacleFalseForSameNet)
{
    auto board = make_board(4);
    ViaObstacleArea area(make_square_area(), 1, planar::IntVector(0, 0), 0.0, false, {3}, 0, 1, 0,
        "via_keepout", FIXED, board.get());
    FakeVia same_net_via(3);
    EXPECT_FALSE(area.is_obstacle(&same_net_via));
}

TEST(ViaObstacleAreaTest, IsObstacleFalseForTrace)
{
    auto board = make_board(4);
    ViaObstacleArea area(make_square_area(), 1, planar::IntVector(0, 0), 0.0, false, {3}, 0, 1, 0,
        "via_keepout", FIXED, board.get());
    FakeTrace trace(7);
    EXPECT_FALSE(area.is_obstacle(&trace));
}

TEST(ViaObstacleAreaTest, IsTraceObstacleAlwaysFalse)
{
    auto board = make_board(4);
    ViaObstacleArea area(make_square_area(), 1, planar::IntVector(0, 0), 0.0, false, {3}, 0, 1, 0,
        "via_keepout", FIXED, board.get());
    EXPECT_FALSE(area.is_trace_obstacle(7));
}

TEST(ViaObstacleAreaTest, CopyPreservesNetAndLayer)
{
    auto board = make_board(4);
    ViaObstacleArea area(make_square_area(), 2, planar::IntVector(5, 5), 0.0, false, {3, 7}, 0, 1, 0,
        "via_keepout", FIXED, board.get());
    auto copied = area.copy(99);
    ASSERT_NE(copied, nullptr);
    auto* area_copy = dynamic_cast<ViaObstacleArea*>(copied.get());
    ASSERT_NE(area_copy, nullptr);
    EXPECT_EQ(area_copy->get_layer(), 2);
    EXPECT_TRUE(area_copy->contains_net(3));
    EXPECT_TRUE(area_copy->contains_net(7));
}

TEST(ViaObstacleAreaTest, GetAreaAppliesSideChangeAndRotation)
{
    auto board = make_board(4);
    ViaObstacleArea area(make_square_area(), 1, planar::IntVector(5, 0), 90.0, false, {3}, 0, 1, 0,
        "via_keepout", FIXED, board.get());
    auto absolute = area.get_area();
    ASSERT_NE(absolute, nullptr);
    EXPECT_EQ(absolute->bounding_box(), planar::IntBox(-5, 0, 5, 10));
}

// ==================== ConductionArea ====================

TEST(ConductionAreaTest, ConstructorSetsObstacleFlag)
{
    auto board = make_board(4);
    ConductionArea area(make_square_area(), 1, planar::IntVector(0, 0), 0.0, false, {3}, 0, 1, 0,
        "conduction", true, FIXED, board.get());
    EXPECT_TRUE(area.get_is_obstacle());
}

TEST(ConductionAreaTest, SetIsObstacle)
{
    auto board = make_board(4);
    ConductionArea area(make_square_area(), 1, planar::IntVector(0, 0), 0.0, false, {3}, 0, 1, 0,
        "conduction", true, FIXED, board.get());
    area.set_is_obstacle(false);
    EXPECT_FALSE(area.get_is_obstacle());
    area.set_is_obstacle(true);
    EXPECT_TRUE(area.get_is_obstacle());
}

TEST(ConductionAreaTest, IsObstacleFalseWhenFlagFalse)
{
    auto board = make_board(4);
    ConductionArea area(make_square_area(), 1, planar::IntVector(0, 0), 0.0, false, {3}, 0, 1, 0,
        "conduction", false, FIXED, board.get());
    FakeTrace trace(7);
    EXPECT_FALSE(area.is_obstacle(&trace));
}

TEST(ConductionAreaTest, IsObstacleTrueWhenFlagTrueAndForeignTrace)
{
    auto board = make_board(4);
    ConductionArea area(make_square_area(), 1, planar::IntVector(0, 0), 0.0, false, {3}, 0, 1, 0,
        "conduction", true, FIXED, board.get());
    FakeTrace trace(7);
    EXPECT_TRUE(area.is_obstacle(&trace));
}

TEST(ConductionAreaTest, IsObstacleFalseWhenFlagTrueAndSameNetTrace)
{
    auto board = make_board(4);
    ConductionArea area(make_square_area(), 1, planar::IntVector(0, 0), 0.0, false, {3}, 0, 1, 0,
        "conduction", true, FIXED, board.get());
    FakeTrace trace(3);
    EXPECT_FALSE(area.is_obstacle(&trace));
}

TEST(ConductionAreaTest, IsTraceObstacleWithFlagTrue)
{
    auto board = make_board(4);
    ConductionArea area(make_square_area(), 1, planar::IntVector(0, 0), 0.0, false, {3}, 0, 1, 0,
        "conduction", true, FIXED, board.get());
    EXPECT_TRUE(area.is_trace_obstacle(7));
    EXPECT_FALSE(area.is_trace_obstacle(3));
}

TEST(ConductionAreaTest, IsTraceObstacleWithFlagFalse)
{
    auto board = make_board(4);
    ConductionArea area(make_square_area(), 1, planar::IntVector(0, 0), 0.0, false, {3}, 0, 1, 0,
        "conduction", false, FIXED, board.get());
    EXPECT_FALSE(area.is_trace_obstacle(7));
}

TEST(ConductionAreaTest, IsDrillableWhenNotObstacle)
{
    auto board = make_board(4);
    ConductionArea area(make_square_area(), 1, planar::IntVector(0, 0), 0.0, false, {3}, 0, 1, 0,
        "conduction", false, FIXED, board.get());
    EXPECT_TRUE(area.is_drillable(7));
    EXPECT_TRUE(area.is_drillable(3));
}

TEST(ConductionAreaTest, IsDrillableWhenObstacleSameNet)
{
    auto board = make_board(4);
    ConductionArea area(make_square_area(), 1, planar::IntVector(0, 0), 0.0, false, {3}, 0, 1, 0,
        "conduction", true, FIXED, board.get());
    EXPECT_TRUE(area.is_drillable(3));
    EXPECT_FALSE(area.is_drillable(7));
}

TEST(ConductionAreaTest, CopyPreservesObstacleFlagAndNets)
{
    auto board = make_board(4);
    ConductionArea area(make_square_area(), 2, planar::IntVector(10, 10), 0.0, false, {3}, 0, 1, 0,
        "conduction", true, FIXED, board.get());
    auto copied = area.copy(99);
    ASSERT_NE(copied, nullptr);
    auto* area_copy = dynamic_cast<ConductionArea*>(copied.get());
    ASSERT_NE(area_copy, nullptr);
    EXPECT_EQ(area_copy->get_layer(), 2);
    EXPECT_TRUE(area_copy->get_is_obstacle());
    EXPECT_TRUE(area_copy->contains_net(3));
    EXPECT_EQ(area_copy->get_name(), "conduction");
}

TEST(ConductionAreaTest, GetRatsnestCornersReturnsAreaCorners)
{
    auto board = make_board(4);
    ConductionArea area(make_square_area(), 1, planar::IntVector(5, 5), 0.0, false, {3}, 0, 1, 0,
        "conduction", false, FIXED, board.get());
    auto corners = area.get_ratsnest_corners();
    ASSERT_EQ(corners.size(), 4u);
    std::set<std::pair<int, int>> corner_set;
    for (const auto& pt : corners)
    {
        const auto* int_pt = dynamic_cast<const planar::IntPoint*>(pt.get());
        ASSERT_NE(int_pt, nullptr);
        corner_set.emplace(int_pt->x, int_pt->y);
    }
    EXPECT_TRUE(corner_set.count({5, 5}));
    EXPECT_TRUE(corner_set.count({15, 5}));
    EXPECT_TRUE(corner_set.count({15, 15}));
    EXPECT_TRUE(corner_set.count({5, 15}));
}

TEST(ConductionAreaTest, GetNormalContactsWithEmptyBoard)
{
    auto board = make_board(4);
    ConductionArea area(make_square_area(), 1, planar::IntVector(0, 0), 0.0, false, {3}, 0, 1, 0,
        "conduction", false, FIXED, board.get());
    auto contacts = area.get_normal_contacts();
    EXPECT_TRUE(contacts.empty());
}

TEST(ConductionAreaTest, TraceIsObstacleVsConductionArea)
{
    auto board = make_board(4);
    FakeTrace trace(7);

    ConductionArea non_obstacle(make_square_area(), 1, planar::IntVector(0, 0), 0.0, false, {3}, 0, 1, 0,
        "conduction", false, FIXED, board.get());
    EXPECT_FALSE(trace.is_obstacle(&non_obstacle));

    ConductionArea obstacle(make_square_area(), 1, planar::IntVector(0, 0), 0.0, false, {3}, 0, 1, 0,
        "conduction", true, FIXED, board.get());
    EXPECT_TRUE(trace.is_obstacle(&obstacle));

    FakeTrace same_net_trace(3);
    EXPECT_FALSE(same_net_trace.is_obstacle(&obstacle));
}

TEST(ConductionAreaTest, TraceIsObstacleVsKeepouts)
{
    auto board = make_board(4);
    FakeTrace trace(7);

    ViaObstacleArea via_keepout(make_square_area(), 1, planar::IntVector(0, 0), 0.0, false, {7}, 0, 1, 0,
        "via_keepout", FIXED, board.get());
    EXPECT_FALSE(trace.is_obstacle(&via_keepout));

    ComponentObstacleArea comp_keepout(make_square_area(), 1, planar::IntVector(0, 0), 0.0, false, 0, 1, 0,
        "comp_keepout", FIXED, board.get());
    EXPECT_FALSE(trace.is_obstacle(&comp_keepout));
}

TEST(ConductionAreaTest, GetTraceConnectionShapeUsesTreeShapes)
{
    auto board = make_board(4);
    ConductionArea area(make_square_area(), 1, planar::IntVector(0, 0), 0.0, false, {3}, 0, 1, 0,
        "conduction", false, FIXED, board.get());
    auto shape = area.get_trace_connection_shape(*board->get_search_tree_manager()->get_default_tree(), 0);
    ASSERT_NE(shape, nullptr);
    EXPECT_FALSE(shape->is_empty());

    auto out_of_range = area.get_trace_connection_shape(*board->get_search_tree_manager()->get_default_tree(), 999);
    EXPECT_EQ(out_of_range, nullptr);
}