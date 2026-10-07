// Tests for the port of Java RoutingBoard.autoroute().

#include "board/RoutingBoard.h"

#include "autoroute/AutorouteAttemptResult.h"
#include "autoroute/AutorouteAttemptState.h"
#include "board/ComponentObstacleArea.h"
#include "board/ConductionArea.h"
#include "board/Item.h"
#include "board/Layer.h"
#include "board/Padstack.h"
#include "board/Pin.h"
#include "board/SearchTreeManager.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/Point.h"
#include "geometry/planar/PolygonShape.h"
#include "geometry/planar/Shape.h"
#include "geometry/planar/Vector.h"
#include "rules/BoardRules.h"
#include "settings/RouterSettings.h"

#include <gtest/gtest.h>

#include <memory>
#include <vector>

namespace freerouting {
namespace board {

namespace {

std::shared_ptr<RoutingBoard> make_routing_board()
{
    std::vector<std::shared_ptr<Layer>> layers;
    layers.push_back(std::make_shared<Layer>("Top", true));
    layers.push_back(std::make_shared<Layer>("Bottom", true));
    LayerStructure layer_structure(layers);
    auto board_rules = std::make_unique<rules::BoardRules>(layer_structure, 10);
    return std::make_shared<RoutingBoard>(geometry::planar::IntBox(0, 0, 100000, 100000),
        layer_structure, std::move(board_rules), nullptr);
}

Pin* insert_smd_pin(RoutingBoard* p_board, const geometry::planar::IntPoint& p_center,
    const std::vector<int>& p_nets, int p_pad_half_width)
{
    auto* pin = new Pin(0, 0, p_nets, 1, p_board->generate_item_id(), FixedState::NOT_FIXED,
        p_board);
    pin->set_center(p_center);
    auto* padstack = new Padstack("p1");
    padstack->set_from_layer(0);
    padstack->set_to_layer(0);
    const int64_t half = p_pad_half_width;
    padstack->add_shape(0, new geometry::planar::PolygonShape(
        std::vector<geometry::planar::IntPoint>{
            { -half, -half }, { half, -half }, { half, half }, { -half, half } }));
    pin->set_padstack(padstack);
    pin->set_bounding_box(geometry::planar::IntBox(
        p_center.x - half, p_center.y - half, p_center.x + half, p_center.y + half));
    p_board->get_search_tree_manager()->insert(pin);
    p_board->add_item(pin);
    return pin;
}

std::shared_ptr<geometry::planar::Area> make_square_area(int p_size)
{
    return std::make_shared<geometry::planar::PolygonArea>(
        std::vector<geometry::planar::IntPoint>{ geometry::planar::IntPoint(0, 0),
            geometry::planar::IntPoint(p_size, 0), geometry::planar::IntPoint(p_size, p_size),
            geometry::planar::IntPoint(0, p_size) });
}

ConductionArea* insert_conduction_area(RoutingBoard* p_board,
    const std::shared_ptr<geometry::planar::Area>& p_area, int p_layer, const std::vector<int>& p_nets)
{
    auto* area = new ConductionArea(p_area, p_layer, geometry::planar::IntVector(0, 0), 0.0, false,
        p_nets, 1, p_board->generate_item_id(), 0, "conduction", false, FixedState::NOT_FIXED,
        p_board);
    p_board->get_search_tree_manager()->insert(area);
    p_board->add_item(area);
    return area;
}

} // namespace

TEST(RoutingBoardAutorouteTest, NullItemIsNotConnectable)
{
    auto board = make_routing_board();
    settings::RouterSettings router_settings(2);
    const auto result = board->autoroute(nullptr, &router_settings, 1, nullptr, nullptr);
    EXPECT_EQ(result.state, autoroute::AutorouteAttemptState::NO_CONNECTIONS);
    EXPECT_NE(result.details.find("not connectable"), std::string::npos);
}

TEST(RoutingBoardAutorouteTest, NonConnectableItemIsNotConnectable)
{
    auto board = make_routing_board();
    // A component keepout is an obstacle area, which is not a Java Connectable.
    auto* keepout = new ComponentObstacleArea(make_square_area(1000), 0,
        geometry::planar::IntVector(0, 0), 0.0, false, 1, board->generate_item_id(), 5,
        "comp_keepout", FixedState::NOT_FIXED, board.get());
    board->get_search_tree_manager()->insert(keepout);
    board->add_item(keepout);

    settings::RouterSettings router_settings(2);
    const auto result = board->autoroute(keepout, &router_settings, 1, nullptr, nullptr);
    EXPECT_EQ(result.state, autoroute::AutorouteAttemptState::NO_CONNECTIONS);
}

TEST(RoutingBoardAutorouteTest, ItemWithoutNetIsNotConnectable)
{
    auto board = make_routing_board();
    // A pin with an empty net list. The guard clause returns before any search
    // tree query, so the pin does not need to be inserted into the board.
    auto* pin = new Pin(0, 0, std::vector<int>{}, 1, board->generate_item_id(),
        FixedState::NOT_FIXED, board.get());
    pin->set_center(geometry::planar::IntPoint(1000, 1000));
    auto* padstack = new Padstack("p1");
    padstack->set_from_layer(0);
    padstack->set_to_layer(0);
    padstack->add_shape(0, new geometry::planar::PolygonShape(
        std::vector<geometry::planar::IntPoint>{ { -100, -100 }, { 100, -100 },
            { 100, 100 }, { -100, 100 } }));
    pin->set_padstack(padstack);
    pin->set_bounding_box(geometry::planar::IntBox(900, 900, 1100, 1100));

    settings::RouterSettings router_settings(2);
    const auto result = board->autoroute(pin, &router_settings, 1, nullptr, nullptr);
    EXPECT_EQ(result.state, autoroute::AutorouteAttemptState::NO_CONNECTIONS);
}

TEST(RoutingBoardAutorouteTest, ItemConnectedToPlaneIsConnectedToPlane)
{
    auto board = make_routing_board();
    // The first net added is net 1; it is flagged as a power plane.
    rules::Net* plane_net = board->mutable_rules().get_nets().add("GND", 0, true);
    ASSERT_NE(plane_net, nullptr);
    EXPECT_TRUE(plane_net->contains_plane());

    // The conduction area spans (0,0)-(1000,1000) on layer 0.
    insert_conduction_area(board.get(), make_square_area(1000), 0, { 1 });
    auto* pin = insert_smd_pin(board.get(), geometry::planar::IntPoint(500, 500), { 1 }, 100);
    // The pin overlaps the plane, so the connected set includes the area.
    ASSERT_EQ(pin->get_connected_set(1).size(), 2u);

    settings::RouterSettings router_settings(2);
    const auto result = board->autoroute(pin, &router_settings, 1, nullptr, nullptr);
    EXPECT_EQ(result.state, autoroute::AutorouteAttemptState::CONNECTED_TO_PLANE);
    EXPECT_NE(result.details.find("plane"), std::string::npos);
}

TEST(RoutingBoardAutorouteTest, SingleIsolatedItemIsAlreadyConnected)
{
    auto board = make_routing_board();
    // The only item of net 1 is the pin itself, so there is no unconnected
    // destination set.
    auto* pin = insert_smd_pin(board.get(), geometry::planar::IntPoint(1000, 1000), { 1 }, 100);
    ASSERT_TRUE(pin->get_unconnected_set(1).empty());

    settings::RouterSettings router_settings(2);
    const auto result = board->autoroute(pin, &router_settings, 1, nullptr, nullptr);
    EXPECT_EQ(result.state, autoroute::AutorouteAttemptState::ALREADY_CONNECTED);
}

TEST(RoutingBoardAutorouteTest, SingleLayerPinWithTargetAttemptsTheMaze)
{
    auto board = make_routing_board();
    auto* pin = insert_smd_pin(board.get(), geometry::planar::IntPoint(1000, 1000), { 1 }, 100);
    insert_smd_pin(board.get(), geometry::planar::IntPoint(5000, 1000), { 1 }, 100);
    // The unconnected set contains the second pin of net 1.
    ASSERT_EQ(pin->get_unconnected_set(1).size(), 1u);

    settings::RouterSettings router_settings(2);
    const auto result = board->autoroute(pin, &router_settings, 1, nullptr, nullptr);
    // The pin leaves the guard clauses and is handed to the maze search, which
    // decides the outcome.
    EXPECT_NE(result.state, autoroute::AutorouteAttemptState::ALREADY_CONNECTED);
    EXPECT_NE(result.state, autoroute::AutorouteAttemptState::NO_CONNECTIONS);
    EXPECT_TRUE(result.state == autoroute::AutorouteAttemptState::ROUTED
        || result.state == autoroute::AutorouteAttemptState::NOT_ROUTED)
        << "state=" << static_cast<int>(result.state) << ", details=" << result.details;
}

} // namespace board
} // namespace freerouting