// Tests for the port of Java RoutingBoard.fanout().

#include "board/RoutingBoard.h"

#include "autoroute/AutorouteAttemptResult.h"
#include "autoroute/AutorouteAttemptState.h"
#include "board/Communication.h"
#include "board/Component.h"
#include "board/Item.h"
#include "board/Layer.h"
#include "board/Padstack.h"
#include "board/Pin.h"
#include "board/Via.h"
#include "board/SearchTreeManager.h"
#include "board/BoardLibrary.h"
#include "board/Package.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/PolygonShape.h"
#include "rules/BoardRules.h"
#include "settings/RouterSettings.h"

#include <gtest/gtest.h>

#include <memory>
#include <vector>

namespace freerouting {
namespace board {

namespace {

std::shared_ptr<RoutingBoard> make_routing_board(Communication* p_communication = nullptr)
{
    std::vector<std::shared_ptr<Layer>> layers;
    layers.push_back(std::make_shared<Layer>("Top", true));
    layers.push_back(std::make_shared<Layer>("Bottom", true));
    LayerStructure layer_structure(layers);
    auto board_rules = std::make_unique<rules::BoardRules>(layer_structure, 10);
    return std::make_shared<RoutingBoard>(geometry::planar::IntBox(0, 0, 100000, 100000),
        layer_structure, std::move(board_rules), p_communication);
}

/**
 * Inserts an smd pin on one layer with a square pad of 2 * p_pad_half_width
 * around p_center. The padstack shapes are relative to the pin center, like
 * in a specctra library.
 */
Pin* insert_smd_pin(RoutingBoard* p_board, const geometry::planar::IntPoint& p_center,
    const std::vector<int>& p_nets, int p_pad_half_width, int p_from_layer = 0, int p_to_layer = 0,
    int p_component_no = 0, int p_pin_no = 0)
{
    auto* pin = new Pin(p_component_no, p_pin_no, p_nets, 1, p_board->generate_item_id(),
        FixedState::NOT_FIXED, p_board);
    pin->set_center(p_center);
    auto* padstack = new Padstack("p1");
    padstack->set_from_layer(p_from_layer);
    padstack->set_to_layer(p_to_layer);
    const int64_t half = p_pad_half_width;
    padstack->add_shape(p_from_layer, new geometry::planar::PolygonShape(
        std::vector<geometry::planar::IntPoint>{
            { -half, -half }, { half, -half }, { half, half }, { -half, half } }));
    pin->set_padstack(padstack);
    pin->set_bounding_box(
        geometry::planar::IntBox(p_center.x - half, p_center.y - half, p_center.x + half, p_center.y + half));
    p_board->get_search_tree_manager()->insert(pin);
    p_board->add_item(pin);
    return pin;
}

/** Adds a component with one package pin at p_rel_x, p_rel_y. */
[[maybe_unused]] void add_component(RoutingBoard* p_board, const std::string& p_name, int p_no,
    const geometry::planar::IntPoint& p_location, int p_rel_x, int p_rel_y, int p_pin_no,
    double p_rotation = 0.0)
{
    (void)p_no;
    std::vector<Package::Pin> pins;
    pins.push_back(Package::Pin{ std::to_string(p_pin_no), 0, geometry::planar::IntVector(p_rel_x, p_rel_y), 0.0 });
    Package* lib_package = p_board->library().packages().add(p_name, std::move(pins));
    p_board->components()->add(p_name, p_location, p_rotation, true, lib_package, lib_package, false);
}

} // namespace

TEST(RoutingBoardFanoutTest, NullPinFails)
{
    auto board = make_routing_board();
    settings::RouterSettings router_settings(2);
    const auto result = board->fanout(nullptr, &router_settings, -1, nullptr, nullptr);
    EXPECT_EQ(result.state, autoroute::AutorouteAttemptState::FAILED);
}

TEST(RoutingBoardFanoutTest, MultilayerPinIsAlreadyConnected)
{
    auto board = make_routing_board();
    // A through pin spans layer 0 and layer 1, so it cannot be fanned out.
    auto* pin = insert_smd_pin(board.get(), geometry::planar::IntPoint(1000, 1000), { 1 }, 100, 0, 1);
    settings::RouterSettings router_settings(2);
    const auto result = board->fanout(pin, &router_settings, -1, nullptr, nullptr);
    EXPECT_EQ(result.state, autoroute::AutorouteAttemptState::ALREADY_CONNECTED);
    EXPECT_NE(result.details.find("already connected"), std::string::npos);
}

TEST(RoutingBoardFanoutTest, PinWithTwoNetsIsAlreadyConnected)
{
    auto board = make_routing_board();
    auto* pin = insert_smd_pin(board.get(), geometry::planar::IntPoint(1000, 1000), { 1, 2 }, 100);
    settings::RouterSettings router_settings(2);
    const auto result = board->fanout(pin, &router_settings, -1, nullptr, nullptr);
    EXPECT_EQ(result.state, autoroute::AutorouteAttemptState::ALREADY_CONNECTED);
}

TEST(RoutingBoardFanoutTest, PinConnectedToAMultilayerItemIsAlreadyConnected)
{
    auto board = make_routing_board();
    // A via of net 1 overlaps the pin, so the connected set of the pin is not
    // on a single layer any more.
    auto* via_padstack = new Padstack("v1");
    via_padstack->set_from_layer(0);
    via_padstack->set_to_layer(1);
    via_padstack->add_shape(0, new geometry::planar::PolygonShape(
        std::vector<geometry::planar::IntPoint>{ { -50, -50 }, { 50, -50 }, { 50, 50 }, { -50, 50 } }));
    auto* via = new Via(via_padstack, geometry::planar::IntPoint(1000, 1000),
        std::vector<int>{ 1 }, 1, board->generate_item_id(), 0, FixedState::NOT_FIXED, true,
        board.get());
    via->set_bounding_box(geometry::planar::IntBox(950, 950, 1050, 1050));
    board->get_search_tree_manager()->insert(via);
    board->add_item(via);
    auto* pin = insert_smd_pin(board.get(), geometry::planar::IntPoint(1000, 1000), { 1 }, 100);
    ASSERT_EQ(pin->get_connected_set(1).size(), 2u);

    settings::RouterSettings router_settings(2);
    const auto result = board->fanout(pin, &router_settings, -1, nullptr, nullptr);
    EXPECT_EQ(result.state, autoroute::AutorouteAttemptState::ALREADY_CONNECTED);
}

TEST(RoutingBoardFanoutTest, PinWithoutUnconnectedNetsReportsNoUnconnectedNets)
{
    auto board = make_routing_board();
    // The only item of net 1 is the pin itself, so nothing is left to route.
    auto* pin = insert_smd_pin(board.get(), geometry::planar::IntPoint(1000, 1000), { 1 }, 100);
    settings::RouterSettings router_settings(2);
    const auto result = board->fanout(pin, &router_settings, -1, nullptr, nullptr);
    EXPECT_EQ(result.state, autoroute::AutorouteAttemptState::NO_UNCONNECTED_NETS);
}

TEST(RoutingBoardFanoutTest, SingleLayerPinWithTargetAttemptsTheMaze)
{
    auto board = make_routing_board();
    auto* pin = insert_smd_pin(board.get(), geometry::planar::IntPoint(1000, 1000), { 1 }, 100);
    insert_smd_pin(board.get(), geometry::planar::IntPoint(5000, 1000), { 1 }, 100);
    // The unconnected set contains the second pin of net 1.
    ASSERT_EQ(pin->get_unconnected_set(1).size(), 1u);

    settings::RouterSettings router_settings(2);
    const auto result = board->fanout(pin, &router_settings, -1, nullptr, nullptr);
    // The pin leaves the guard clauses and is handed to the maze search, which
    // decides the outcome.
    EXPECT_NE(result.state, autoroute::AutorouteAttemptState::ALREADY_CONNECTED);
    EXPECT_NE(result.state, autoroute::AutorouteAttemptState::NO_UNCONNECTED_NETS);
    EXPECT_TRUE(result.state == autoroute::AutorouteAttemptState::ROUTED
        || result.state == autoroute::AutorouteAttemptState::NOT_ROUTED)
        << "state=" << static_cast<int>(result.state) << ", details=" << result.details;
}

} // namespace board
} // namespace freerouting