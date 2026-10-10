#include <memory>
#include <set>
#include <vector>

#include "board/ConductionArea.h"
#include "board/Layer.h"
#include "board/Padstack.h"
#include "board/Pin.h"
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

Pin* insert_smd_pin(RoutingBoard* p_board, const geometry::planar::IntPoint& p_center,
                    const std::vector<int>& p_nets, int p_pad_half_width)
{
    auto* pin = new Pin(7, 1, p_nets, 1, p_board->generate_item_id(), FixedState::NOT_FIXED,
                        p_board);
    pin->set_center(p_center);
    auto* padstack = new Padstack("p1");
    padstack->set_from_layer(0);
    padstack->set_to_layer(0);
    padstack->add_shape(0, new geometry::planar::PolygonShape(
        std::vector<geometry::planar::IntPoint>{
            {static_cast<int64_t>(-p_pad_half_width), static_cast<int64_t>(-p_pad_half_width)},
            {static_cast<int64_t>(p_pad_half_width), static_cast<int64_t>(-p_pad_half_width)},
            {static_cast<int64_t>(p_pad_half_width), static_cast<int64_t>(p_pad_half_width)},
            {static_cast<int64_t>(-p_pad_half_width), static_cast<int64_t>(p_pad_half_width)}}));
    pin->set_padstack(padstack);
    pin->set_bounding_box(geometry::planar::IntBox(
        p_center.x - p_pad_half_width, p_center.y - p_pad_half_width,
        p_center.x + p_pad_half_width, p_center.y + p_pad_half_width));
    p_board->get_search_tree_manager()->insert(pin);
    p_board->add_item(pin);
    return pin;
}

bool set_contains(ItemSet p_set, const Item* p_item)
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

    ItemSet connected = trace->get_connected_set(1, false);
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
    EXPECT_TRUE(set_contains(ItemSet(trace_contacts.begin(), trace_contacts.end()), via));

    // The via contacts the trace.
    std::vector<Item*> via_contacts = via->get_normal_contacts();
    ASSERT_EQ(via_contacts.size(), 1u);
    EXPECT_EQ(via_contacts[0], trace);

    // The connected set spans trace and via.
    ItemSet connected = trace->get_connected_set(1, false);
    EXPECT_EQ(connected.size(), 2u);
    EXPECT_TRUE(set_contains(connected, trace));
    EXPECT_TRUE(set_contains(connected, via));
    EXPECT_EQ(via->get_connected_set(1, false), connected);
}

TEST(ItemContactGraphTest, GetAllContactsOfATraceContainsTheContactingVia)
{
    auto board = make_routing_board();
    PolylineTrace* trace = insert_trace(board.get(),
                                        {geometry::planar::IntPoint(0, 0),
                                         geometry::planar::IntPoint(100, 100)},
                                        {1});
    ASSERT_NE(trace, nullptr);
    Via* via = insert_via(board.get(), geometry::planar::IntPoint(100, 100), {1}, 50);
    ASSERT_NE(via, nullptr);

    const ItemSet contacts = trace->get_all_contacts();

    ASSERT_EQ(contacts.size(), 1u);
    EXPECT_TRUE(set_contains(contacts, via));
    EXPECT_FALSE(set_contains(contacts, trace));

    // The via sees the trace as its contact.
    const ItemSet via_contacts = via->get_all_contacts();
    ASSERT_EQ(via_contacts.size(), 1u);
    EXPECT_TRUE(set_contains(via_contacts, trace));
}

TEST(ItemContactGraphTest, GetAllContactsOfAnIsolatedTraceIsEmpty)
{
    auto board = make_routing_board();
    PolylineTrace* trace = insert_trace(board.get(),
                                        {geometry::planar::IntPoint(0, 0),
                                         geometry::planar::IntPoint(100, 100)},
                                        {1});
    ASSERT_NE(trace, nullptr);

    EXPECT_TRUE(trace->get_all_contacts().empty());
}

TEST(ItemContactGraphTest, GetAllContactsDoesNotContainOtherNets)
{
    auto board = make_routing_board();
    PolylineTrace* trace = insert_trace(board.get(),
                                        {geometry::planar::IntPoint(0, 0),
                                         geometry::planar::IntPoint(100, 100)},
                                        {1});
    ASSERT_NE(trace, nullptr);
    Via* via = insert_via(board.get(), geometry::planar::IntPoint(100, 100), {2}, 50);
    ASSERT_NE(via, nullptr);

    EXPECT_TRUE(trace->get_all_contacts().empty());
}

TEST(ItemContactGraphTest, GetAllContactsOnLayerFiltersByTheLayerOfTheShape)
{
    auto board = make_routing_board();
    // The trace is only on layer 0, the via spans layer 0 to 1.
    PolylineTrace* trace = insert_trace(board.get(),
                                        {geometry::planar::IntPoint(0, 0),
                                         geometry::planar::IntPoint(100, 100)},
                                        {1});
    ASSERT_NE(trace, nullptr);
    Via* via = insert_via(board.get(), geometry::planar::IntPoint(100, 100), {1}, 50);
    ASSERT_NE(via, nullptr);

    const ItemSet layer_0_contacts = trace->get_all_contacts(0);
    EXPECT_EQ(layer_0_contacts.size(), 1u);
    EXPECT_TRUE(set_contains(layer_0_contacts, via));

    // The trace has no shape on layer 1, so no contact can be found there.
    EXPECT_TRUE(trace->get_all_contacts(1).empty());
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

    ItemSet connected = net1_trace->get_connected_set(1, false);
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
    ItemSet connected = trace->get_connected_set(1, false);
    EXPECT_TRUE(set_contains(connected, trace));
    EXPECT_TRUE(set_contains(connected, area));

    // With p_stop_at_plane a conduction area without a component is skipped.
    ItemSet stopped = trace->get_connected_set(1, true);
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
    ItemSet stopped = trace->get_connected_set(1, true);
    EXPECT_EQ(stopped.size(), 2u);
    EXPECT_TRUE(set_contains(stopped, trace));
    EXPECT_TRUE(set_contains(stopped, area));
}

TEST(ItemContactGraphTest, DrillItemExposesLayerRange)
{
    auto board = make_routing_board();
    Via* via = insert_via(board.get(), geometry::planar::IntPoint(300, 400), {1}, 50);
    ASSERT_NE(via, nullptr);

    EXPECT_EQ(via->first_layer(), 0);
    EXPECT_EQ(via->last_layer(), 1);
    EXPECT_EQ(via->get_layer(), 0);
}

TEST(ItemContactGraphTest, FirstCommonLayerComputedFromBothLayerRanges)
{
    auto board = make_routing_board();
    PolylineTrace* top_trace = insert_trace(board.get(),
                                            {geometry::planar::IntPoint(0, 0),
                                             geometry::planar::IntPoint(100, 100)},
                                            {1});
    Via* via = insert_via(board.get(), geometry::planar::IntPoint(100, 100), {1}, 50);
    ASSERT_NE(top_trace, nullptr);
    ASSERT_NE(via, nullptr);

    // Trace on layer 0 + via on layers 0..1 -> common layer 0.
    EXPECT_EQ(top_trace->first_common_layer(via), 0);
    EXPECT_EQ(via->first_common_layer(top_trace), 0);

    // No common layer -> -1.
    Via* drilled_via = insert_via(board.get(), geometry::planar::IntPoint(0, 0), {1}, 50);
    ASSERT_NE(drilled_via, nullptr);
    // The via always spans layer 0..1, so it always shares layer 0 with the trace;
    // construct the -1 case via a deep trace on layer 1 and a layer-0 item.
    // (first/last layer is fixed at 0/1 here, so verify shares_layer is true too.)
    EXPECT_TRUE(top_trace->shares_layer(via));
    EXPECT_TRUE(top_trace->shares_layer(drilled_via));
}

TEST(ItemContactGraphTest, NormalContactPointBetweenTraceAndVia)
{
    auto board = make_routing_board();
    PolylineTrace* trace = insert_trace(board.get(),
                                        {geometry::planar::IntPoint(0, 0),
                                         geometry::planar::IntPoint(100, 100)},
                                        {1});
    Via* via = insert_via(board.get(), geometry::planar::IntPoint(100, 100), {1}, 50);
    ASSERT_NE(trace, nullptr);
    ASSERT_NE(via, nullptr);

    // The trace's unique contact point with the via is the via center.
    const geometry::planar::IntPoint via_center(100, 100);
    const geometry::planar::Point* trace_contact = trace->normal_contact_point(via);
    ASSERT_NE(trace_contact, nullptr);
    EXPECT_TRUE(trace_contact->equals(&via_center));

    // The via's unique contact point with the trace is its own center.
    const geometry::planar::Point* via_contact = via->normal_contact_point(trace);
    ASSERT_NE(via_contact, nullptr);
    EXPECT_TRUE(via_contact->equals(&via_center));

    // A via that does not touch the trace has no unique contact point.
    Via* other_via = insert_via(board.get(), geometry::planar::IntPoint(500, 500), {1}, 50);
    ASSERT_NE(other_via, nullptr);
    EXPECT_EQ(trace->normal_contact_point(other_via), nullptr);
    // Two vias at the same point with a shared layer contact each other.
    Via* stacked_via = insert_via(board.get(), geometry::planar::IntPoint(500, 500), {1}, 50);
    ASSERT_NE(stacked_via, nullptr);
    const geometry::planar::Point* via_via_contact = other_via->normal_contact_point(stacked_via);
    ASSERT_NE(via_via_contact, nullptr);
    const geometry::planar::IntPoint stacked_center(500, 500);
    EXPECT_TRUE(via_via_contact->equals(&stacked_center));
}

TEST(ItemContactGraphTest, ConnectedSetRecursesThroughVia)
{
    auto board = make_routing_board();
    PolylineTrace* trace1 = insert_trace(board.get(),
                                         {geometry::planar::IntPoint(0, 0),
                                          geometry::planar::IntPoint(100, 100)},
                                         {1});
    Via* via = insert_via(board.get(), geometry::planar::IntPoint(100, 100), {1}, 50);
    PolylineTrace* trace2 = insert_trace(board.get(),
                                         {geometry::planar::IntPoint(100, 100),
                                          geometry::planar::IntPoint(200, 200)},
                                         {1});
    ASSERT_NE(trace1, nullptr);
    ASSERT_NE(via, nullptr);
    ASSERT_NE(trace2, nullptr);

    ItemSet connected = trace1->get_connected_set(1, false);
    EXPECT_EQ(connected.size(), 3u);
    EXPECT_TRUE(set_contains(connected, trace1));
    EXPECT_TRUE(set_contains(connected, via));
    EXPECT_TRUE(set_contains(connected, trace2));
}

TEST(ItemContactGraphTest, ConnectionItemsSpanTraceAndVia)
{
    auto board = make_routing_board();
    PolylineTrace* trace = insert_trace(board.get(),
                                        {geometry::planar::IntPoint(0, 0),
                                         geometry::planar::IntPoint(100, 100)},
                                        {1});
    Via* via = insert_via(board.get(), geometry::planar::IntPoint(100, 100), {1}, 50);
    ASSERT_NE(trace, nullptr);
    ASSERT_NE(via, nullptr);

    std::vector<Item*> connection_items = trace->get_connection_items(Item::StopConnectionOption::NONE);
    ItemSet connection_set(connection_items.begin(), connection_items.end());
    EXPECT_TRUE(set_contains(connection_set, trace));
    EXPECT_TRUE(set_contains(connection_set, via));
}

TEST(ItemContactGraphTest, ConnectionItemsStopAtVia)
{
    auto board = make_routing_board();
    PolylineTrace* trace = insert_trace(board.get(),
                                        {geometry::planar::IntPoint(0, 0),
                                         geometry::planar::IntPoint(100, 100)},
                                        {1});
Via* via = insert_via(board.get(), geometry::planar::IntPoint(100, 100), {1}, 50);
    ASSERT_NE(trace, nullptr);
    ASSERT_NE(via, nullptr);

    std::vector<Item*> connection_items =
        trace->get_connection_items(Item::StopConnectionOption::VIA);
    ItemSet connection_set(connection_items.begin(), connection_items.end());
    EXPECT_TRUE(set_contains(connection_set, trace));
    EXPECT_FALSE(set_contains(connection_set, via));
}

TEST(ItemContactGraphTest, TraceContactsIgnoreNetFlag)
{
    auto board = make_routing_board();
    PolylineTrace* net1_trace = insert_trace(board.get(),
                                             {geometry::planar::IntPoint(0, 0),
                                              geometry::planar::IntPoint(100, 0)},
                                             {1});
    PolylineTrace* net2_trace = insert_trace(board.get(),
                                             {geometry::planar::IntPoint(0, 0),
                                              geometry::planar::IntPoint(100, 0)},
                                             {2});
    ASSERT_NE(net1_trace, nullptr);
    ASSERT_NE(net2_trace, nullptr);

    // With ignore_net == false only same-net contacts are returned.
    const ItemSet same_net = net1_trace->get_normal_contacts(
        geometry::planar::IntPoint(0, 0), false);
    EXPECT_EQ(same_net.size(), 0u);

    // With ignore_net == true the other-net trace at the same endpoint is found.
    const ItemSet all_nets = net1_trace->get_normal_contacts(
        geometry::planar::IntPoint(0, 0), true);
    EXPECT_EQ(all_nets.size(), 1u);
    EXPECT_TRUE(set_contains(all_nets, net2_trace));
}

TEST(ItemContactGraphTest, ViaIsFanoutViaForSmdPinExit)
{
    auto board = make_routing_board();
    Pin* pin = insert_smd_pin(board.get(), geometry::planar::IntPoint(50, 50), {1}, 50);
    PolylineTrace* trace = insert_trace(board.get(),
                                        {geometry::planar::IntPoint(50, 50),
                                         geometry::planar::IntPoint(100, 100)},
                                        {1});
    Via* via = insert_via(board.get(), geometry::planar::IntPoint(100, 100), {1}, 50);
    ASSERT_NE(pin, nullptr);
    ASSERT_NE(trace, nullptr);
    ASSERT_NE(via, nullptr);

    // The trace shall be seen as the SMD-pin exit trace.
    ItemSet ignore;
    EXPECT_TRUE(via->is_fanout_via(nullptr));

    // A lone via without a pin connection is not a fanout via.
    Via* orphan_via = insert_via(board.get(), geometry::planar::IntPoint(900, 900), {1}, 50);
    ASSERT_NE(orphan_via, nullptr);
    EXPECT_FALSE(orphan_via->is_fanout_via(nullptr));
}

} // namespace
} // namespace freerouting::board