#include <memory>
#include <set>
#include <vector>

#include "board/ConductionArea.h"
#include "board/Layer.h"
#include "board/Padstack.h"
#include "board/PolylineTrace.h"
#include "board/RoutingBoard.h"
#include "board/SearchTreeManager.h"
#include "board/Trace.h"
#include "board/Via.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/Point.h"
#include "geometry/planar/PolygonShape.h"
#include "geometry/planar/Polyline.h"
#include "geometry/planar/Shape.h"
#include "geometry/planar/Vector.h"
#include "gtest/gtest.h"
#include "rules/BoardRules.h"

namespace freerouting::board {
namespace {

std::shared_ptr<RoutingBoard> make_routing_board()
{
    std::vector<std::shared_ptr<Layer>> layers;
    layers.push_back(std::make_shared<Layer>("Top", true));
    layers.push_back(std::make_shared<Layer>("Bottom", true));
    LayerStructure layer_structure(layers);
    auto board_rules = std::make_unique<rules::BoardRules>(layer_structure, 10);
    return std::make_shared<RoutingBoard>(geometry::planar::IntBox(0, 0, 10000, 10000),
                                          layer_structure, std::move(board_rules), nullptr);
}

PolylineTrace* insert_trace(RoutingBoard* p_board,
                            const std::vector<geometry::planar::IntPoint>& p_corners,
                            const std::vector<int>& p_nets)
{
    auto polyline = std::make_shared<geometry::planar::Polyline>(
        geometry::planar::Polyline::from_corners(p_corners));
    return p_board->insert_trace_without_cleaning(polyline, 0, 50, p_nets, 1,
                                                  FixedState::NOT_FIXED);
}

Via* insert_via(RoutingBoard* p_board, const geometry::planar::IntPoint& p_center,
                const std::vector<int>& p_nets, int p_pad_half_width)
{
    auto* padstack = new Padstack("v1");
    padstack->set_from_layer(0);
    padstack->set_to_layer(1);
    // Pad shape is relative to the via center; kept alive by the leaky
    // padstack (the test does not query it after the board is destroyed).
    padstack->add_shape(0, new geometry::planar::PolygonShape(
        std::vector<geometry::planar::IntPoint>{
            {static_cast<int64_t>(-p_pad_half_width), static_cast<int64_t>(-p_pad_half_width)},
            {static_cast<int64_t>(p_pad_half_width), static_cast<int64_t>(-p_pad_half_width)},
            {static_cast<int64_t>(p_pad_half_width), static_cast<int64_t>(p_pad_half_width)},
            {static_cast<int64_t>(-p_pad_half_width), static_cast<int64_t>(p_pad_half_width)}}));
    auto* via = new Via(padstack, p_center, p_nets, 1, p_board->generate_item_id(), 0,
                        FixedState::NOT_FIXED, true, p_board);
    // The search tree derives the via shape from its bounding box; set it
    // explicitly on the hand-built board.
    via->set_bounding_box(geometry::planar::IntBox(
        p_center.x - p_pad_half_width, p_center.y - p_pad_half_width,
        p_center.x + p_pad_half_width, p_center.y + p_pad_half_width));
    p_board->get_search_tree_manager()->insert(via);
    p_board->add_item(via);
    return via;
}

ConductionArea* insert_conduction_area(RoutingBoard* p_board,
                                       const std::shared_ptr<geometry::planar::Area>& p_area,
                                       int p_layer, const std::vector<int>& p_nets,
                                       int p_component_no)
{
    auto* area = new ConductionArea(p_area, p_layer, geometry::planar::IntVector(0, 0),
                                    0.0, false, p_nets, 1, p_board->generate_item_id(),
                                    p_component_no, "conduction", false, FixedState::NOT_FIXED,
                                    p_board);
    p_board->get_search_tree_manager()->insert(area);
    p_board->add_item(area);
    return area;
}

std::shared_ptr<geometry::planar::Area> make_square_area(int p_size)
{
    return std::make_shared<geometry::planar::PolygonArea>(
        std::vector<geometry::planar::IntPoint>{geometry::planar::IntPoint(0, 0),
                                                geometry::planar::IntPoint(p_size, 0),
                                                geometry::planar::IntPoint(p_size, p_size),
                                                geometry::planar::IntPoint(0, p_size)});
}

bool set_contains(std::set<Item*> p_set, const Item* p_item)
{
    return p_set.find(const_cast<Item*>(p_item)) != p_set.end();
}

TEST(ItemContactGraphTest, ConnectedSetOfDisconnectedTraceContainsItselfOnly)
{
    auto board = make_routing_board();
    PolylineTrace* trace = insert_trace(board.get(),
                                        {geometry::planar::IntPoint(0, 0),
                                         geometry::planar::IntPoint(100, 0)},
                                        {1});
    ASSERT_NE(trace, nullptr);

    std::set<Item*> connected = trace->get_connected_set(1, false);
    ASSERT_EQ(connected.size(), 1u);
    EXPECT_TRUE(set_contains(connected, trace));

    // A different net is not contained in the connected set.
    EXPECT_TRUE(trace->get_connected_set(2, false).empty());
}

TEST(ItemContactGraphTest, TraceContactsViaAtSharedEndpoint)
{
    auto board = make_routing_board();
    PolylineTrace* trace = insert_trace(board.get(),
                                        {geometry::planar::IntPoint(0, 0),
                                         geometry::planar::IntPoint(100, 100)},
                                        {1});
    ASSERT_NE(trace, nullptr);
    Via* via = insert_via(board.get(), geometry::planar::IntPoint(100, 100), {1}, 50);
    ASSERT_NE(via, nullptr);

    // The trace's endpoint contacts the via center.
    std::vector<Item*> trace_contacts = trace->get_normal_contacts();
    EXPECT_TRUE(set_contains(std::set<Item*>(trace_contacts.begin(), trace_contacts.end()), via));

    // The via contacts the trace.
    std::vector<Item*> via_contacts = via->get_normal_contacts();
    ASSERT_EQ(via_contacts.size(), 1u);
    EXPECT_EQ(via_contacts[0], trace);

    // The connected set spans trace and via.
    std::set<Item*> connected = trace->get_connected_set(1, false);
    EXPECT_EQ(connected.size(), 2u);
    EXPECT_TRUE(set_contains(connected, trace));
    EXPECT_TRUE(set_contains(connected, via));
    EXPECT_EQ(via->get_connected_set(1, false), connected);
}

TEST(ItemContactGraphTest, ViaRatsnestCornersReturnCenter)
{
    auto board = make_routing_board();
    Via* via = insert_via(board.get(), geometry::planar::IntPoint(300, 400), {1}, 50);
    ASSERT_NE(via, nullptr);

    auto corners = via->get_ratsnest_corners();
    ASSERT_EQ(corners.size(), 1u);
    const auto* corner = dynamic_cast<const geometry::planar::IntPoint*>(corners[0].get());
    ASSERT_NE(corner, nullptr);
    EXPECT_EQ(corner->x, 300);
    EXPECT_EQ(corner->y, 400);
}

TEST(ItemContactGraphTest, ConnectedSetDoesNotCrossToDifferentNet)
{
    auto board = make_routing_board();
    PolylineTrace* net1_trace = insert_trace(board.get(),
                                             {geometry::planar::IntPoint(0, 0),
                                              geometry::planar::IntPoint(100, 0)},
                                             {1});
    PolylineTrace* net2_trace = insert_trace(board.get(),
                                             {geometry::planar::IntPoint(100, 0),
                                              geometry::planar::IntPoint(200, 0)},
                                             {2});
    ASSERT_NE(net1_trace, nullptr);
    ASSERT_NE(net2_trace, nullptr);

    std::set<Item*> connected = net1_trace->get_connected_set(1, false);
    EXPECT_EQ(connected.size(), 1u);
    EXPECT_TRUE(set_contains(connected, net1_trace));
    EXPECT_FALSE(set_contains(connected, net2_trace));
}

TEST(ItemContactGraphTest, ConnectedSetIncludesSameNetConductionArea)
{
    auto board = make_routing_board();
    PolylineTrace* trace = insert_trace(board.get(),
                                        {geometry::planar::IntPoint(100, 100),
                                         geometry::planar::IntPoint(200, 200)},
                                        {1});
    ASSERT_NE(trace, nullptr);
    ConductionArea* area = insert_conduction_area(board.get(), make_square_area(1000), 0, {1}, 0);
    ASSERT_NE(area, nullptr);

    // Without p_stop_at_plane the conduction area belongs to the set.
    std::set<Item*> connected = trace->get_connected_set(1, false);
    EXPECT_TRUE(set_contains(connected, trace));
    EXPECT_TRUE(set_contains(connected, area));

    // With p_stop_at_plane a conduction area without a component is skipped.
    std::set<Item*> stopped = trace->get_connected_set(1, true);
    EXPECT_EQ(stopped.size(), 1u);
    EXPECT_TRUE(set_contains(stopped, trace));
}

TEST(ItemContactGraphTest, StopAtPlaneKeepsAreasWithComponent)
{
    auto board = make_routing_board();
    PolylineTrace* trace = insert_trace(board.get(),
                                        {geometry::planar::IntPoint(100, 100),
                                         geometry::planar::IntPoint(200, 200)},
                                        {1});
    ASSERT_NE(trace, nullptr);
    ConductionArea* area = insert_conduction_area(board.get(), make_square_area(1000), 0, {1}, 7);
    ASSERT_NE(area, nullptr);

    // An area belonging to a component (component_no > 0) is not skipped.
    std::set<Item*> stopped = trace->get_connected_set(1, true);
    EXPECT_EQ(stopped.size(), 2u);
    EXPECT_TRUE(set_contains(stopped, trace));
    EXPECT_TRUE(set_contains(stopped, area));
}

} // namespace
} // namespace freerouting::board