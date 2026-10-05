#include <cmath>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "board/Layer.h"
#include "board/Padstack.h"
#include "board/Pin.h"
#include "board/PolylineTrace.h"
#include "board/RoutingBoard.h"
#include "board/ShapeSearchTree.h"
#include "board/Trace.h"
#include "board/Via.h"
#include "drc/DesignRulesChecker.h"
#include "drc/DrcReport.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/Point.h"
#include "geometry/planar/PolygonShape.h"
#include "geometry/planar/Polyline.h"
#include "geometry/planar/Simplex.h"
#include "gtest/gtest.h"
#include "rules/BoardRules.h"
#include "settings/ProfileSettings.h"

namespace freerouting::drc {
namespace {

std::shared_ptr<board::RoutingBoard> make_routing_board(int p_default_clearance)
{
    std::vector<std::shared_ptr<board::Layer>> layers;
    layers.push_back(std::make_shared<board::Layer>("Top", true));
    layers.push_back(std::make_shared<board::Layer>("Bottom", true));
    board::LayerStructure layer_structure(layers);
    auto board_rules = std::make_unique<rules::BoardRules>(layer_structure, p_default_clearance);
    board_rules->get_nets().add("net1", 1, false);
    board_rules->get_nets().add("net2", 1, false);
    return std::make_shared<board::RoutingBoard>(
        geometry::planar::IntBox(0, 0, 10000, 10000),
        layer_structure, std::move(board_rules), nullptr);
}

board::PolylineTrace* insert_trace(board::RoutingBoard* p_board,
                                   const std::vector<geometry::planar::IntPoint>& p_corners,
                                   const std::vector<int>& p_nets)
{
    auto polyline = std::make_shared<geometry::planar::Polyline>(
        geometry::planar::Polyline::from_corners(p_corners));
    return p_board->insert_trace_without_cleaning(polyline, 0, 50, p_nets, 1,
                                                  board::FixedState::NOT_FIXED);
}

board::Via* insert_via(board::RoutingBoard* p_board,
                       const geometry::planar::IntPoint& p_center,
                       const std::vector<int>& p_nets, int p_pad_half_width)
{
    auto* padstack = new board::Padstack("v1");
    padstack->set_from_layer(0);
    padstack->set_to_layer(1);
    padstack->add_shape(0, new geometry::planar::PolygonShape(
        std::vector<geometry::planar::IntPoint>{
            {static_cast<int64_t>(-p_pad_half_width), static_cast<int64_t>(-p_pad_half_width)},
            {static_cast<int64_t>(p_pad_half_width), static_cast<int64_t>(-p_pad_half_width)},
            {static_cast<int64_t>(p_pad_half_width), static_cast<int64_t>(p_pad_half_width)},
            {static_cast<int64_t>(-p_pad_half_width), static_cast<int64_t>(p_pad_half_width)}}));
    auto* via = new board::Via(padstack, p_center, p_nets, 1,
                               p_board->generate_item_id(), 0,
                               board::FixedState::NOT_FIXED, true, p_board);
    via->set_bounding_box(geometry::planar::IntBox(
        p_center.x - p_pad_half_width, p_center.y - p_pad_half_width,
        p_center.x + p_pad_half_width, p_center.y + p_pad_half_width));
    p_board->get_search_tree_manager()->insert(via);
    p_board->add_item(via);
    return via;
}

board::Pin* insert_smd_pin(board::RoutingBoard* p_board,
                           const geometry::planar::IntPoint& p_center,
                           const std::vector<int>& p_nets, int p_pad_half_width)
{
    auto* pin = new board::Pin(7, 1, p_nets, 1, p_board->generate_item_id(),
                               board::FixedState::NOT_FIXED, p_board);
    pin->set_center(p_center);
    auto* padstack = new board::Padstack("p1");
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

TEST(DesignRulesCheckerTest, DetectsClearanceViolationBetweenDifferentNets)
{
    // Default clearance 1000; two parallel traces on different nets separated
    // by 400 centre-to-centre (300 edge-to-edge) = a clearance violation.
    auto board = make_routing_board(1000);
    board::PolylineTrace* trace1 = insert_trace(
        board.get(), {geometry::planar::IntPoint(1000, 1000),
                      geometry::planar::IntPoint(3000, 1000)}, {1});
    board::PolylineTrace* trace2 = insert_trace(
        board.get(), {geometry::planar::IntPoint(1000, 1400),
                      geometry::planar::IntPoint(3000, 1400)}, {2});
    ASSERT_NE(trace1, nullptr);
    ASSERT_NE(trace2, nullptr);

    DesignRulesChecker checker(board.get(), settings::DesignRulesCheckerSettings());
    auto violations = checker.get_all_clearance_violations();

    ASSERT_GE(violations.size(), 1u);
    const auto& v = violations[0];
    EXPECT_EQ(v->get_layer(), 0);
    EXPECT_NEAR(v->get_expected_clearance(), 1000.0, 1e-6);
    EXPECT_LT(v->get_actual_clearance(), v->get_expected_clearance());
    EXPECT_NE(v->get_shape(), nullptr);
    EXPECT_EQ(v->get_first_item()->get_net_nos()[0], 1);
    EXPECT_EQ(v->get_second_item()->get_net_nos()[0], 2);
}

TEST(DesignRulesCheckerTest, NoClearanceViolationForFarApartTraces)
{
    auto board = make_routing_board(100);
    insert_trace(board.get(), {geometry::planar::IntPoint(1000, 1000),
                               geometry::planar::IntPoint(3000, 1000)}, {1});
    insert_trace(board.get(), {geometry::planar::IntPoint(1000, 4000),
                               geometry::planar::IntPoint(3000, 4000)}, {2});

    DesignRulesChecker checker(board.get(), settings::DesignRulesCheckerSettings());
    auto violations = checker.get_all_clearance_violations();
    EXPECT_TRUE(violations.empty());
}

TEST(DesignRulesCheckerTest, ReportJsonMatchesKiCadSchemaShape)
{
    auto board = make_routing_board(1000);
    insert_trace(board.get(), {geometry::planar::IntPoint(1000, 1000),
                               geometry::planar::IntPoint(3000, 1000)}, {1});
    insert_trace(board.get(), {geometry::planar::IntPoint(1000, 1400),
                               geometry::planar::IntPoint(3000, 1400)}, {2});

    DesignRulesChecker checker(board.get(), settings::DesignRulesCheckerSettings());
    std::string json = checker.generate_report_json("test.dsn", "mm");

    EXPECT_NE(json.find("\"$schema\""), std::string::npos);
    EXPECT_NE(json.find("\"coordinateUnits\":\"mm\""), std::string::npos);
    EXPECT_NE(json.find("\"kicadVersion\":\"N/A\""), std::string::npos);
    EXPECT_NE(json.find("\"source\":\"test.dsn\""), std::string::npos);
    EXPECT_NE(json.find("\"unconnectedItems\":["), std::string::npos);
    EXPECT_NE(json.find("\"type\":\"clearance\""), std::string::npos);
    EXPECT_NE(json.find("\"severity\":\"error\""), std::string::npos);
    EXPECT_NE(json.find("Clearance violation between Trace [net1] and Trace [net2]"),
              std::string::npos);
}

TEST(DesignRulesCheckerTest, DetectsIsolatedItemsOnSameNet)
{
    auto board = make_routing_board(10);
    board::Via* via1 = insert_via(board.get(), geometry::planar::IntPoint(200, 200), {1}, 50);
    board::Via* via2 = insert_via(board.get(), geometry::planar::IntPoint(6000, 5000), {1}, 50);
    ASSERT_NE(via1, nullptr);
    ASSERT_NE(via2, nullptr);

    DesignRulesChecker checker(board.get(), settings::DesignRulesCheckerSettings());
    auto unconnected = checker.get_all_unconnected_items();

    bool found_group = false;
    for (const UnconnectedItems& u : unconnected)
    {
        if (u.get_type() == "unconnectedItems")
        {
            found_group = true;
            std::set<board::Item*> items(u.get_all_items().begin(), u.get_all_items().end());
            EXPECT_TRUE(items.count(via1) > 0);
            EXPECT_TRUE(items.count(via2) > 0);
        }
    }
    EXPECT_TRUE(found_group);
}

TEST(DesignRulesCheckerTest, DetectsDanglingTrace)
{
    auto board = make_routing_board(10);
    board::Via* via1 = insert_via(board.get(), geometry::planar::IntPoint(200, 200), {1}, 50);
    board::PolylineTrace* tail_trace = insert_trace(
        board.get(), {geometry::planar::IntPoint(5000, 5000),
                      geometry::planar::IntPoint(6000, 4000)}, {2});
    ASSERT_NE(via1, nullptr);
    ASSERT_NE(tail_trace, nullptr);
    EXPECT_TRUE(tail_trace->is_tail());

    DesignRulesChecker checker(board.get(), settings::DesignRulesCheckerSettings());
    auto unconnected = checker.get_all_unconnected_items();

    bool found_dangling = false;
    for (const UnconnectedItems& u : unconnected)
    {
        if (u.get_type() == "track_dangling" && u.get_first() == tail_trace)
        {
            found_dangling = true;
        }
    }
    EXPECT_TRUE(found_dangling);
}

TEST(DesignRulesCheckerTest, ComputesIncompletesAndAirlines)
{
    auto board = make_routing_board(10);
    board::Pin* pin1 = insert_smd_pin(board.get(), geometry::planar::IntPoint(200, 200), {1}, 50);
    board::Pin* pin2 = insert_smd_pin(board.get(), geometry::planar::IntPoint(6000, 5000), {1}, 50);
    ASSERT_NE(pin1, nullptr);
    ASSERT_NE(pin2, nullptr);

    DesignRulesChecker checker(board.get(), settings::DesignRulesCheckerSettings());
    checker.calculate_all_incompletes();

    EXPECT_EQ(checker.get_incomplete_count(1), 1);
    EXPECT_EQ(checker.get_incomplete_count(2), 0);
    EXPECT_EQ(checker.get_incomplete_count(), 1);

    auto airlines = checker.get_all_airlines();
    ASSERT_EQ(airlines.size(), 1u);
    EXPECT_EQ(airlines[0]->from_item == pin1 || airlines[0]->from_item == pin2, true);

    EXPECT_EQ(checker.get_net_incompletes(42), nullptr);
}

TEST(DesignRulesCheckerTest, GeneratesSummary)
{
    auto board = make_routing_board(1000);
    insert_trace(board.get(), {geometry::planar::IntPoint(1000, 1000),
                               geometry::planar::IntPoint(3000, 1000)}, {1});
    insert_trace(board.get(), {geometry::planar::IntPoint(1000, 1400),
                               geometry::planar::IntPoint(3000, 1400)}, {2});

    DesignRulesChecker checker(board.get(), settings::DesignRulesCheckerSettings());
    DrcSummaryResponse summary = checker.generate_summary();

    EXPECT_EQ(summary.clearance_violations_count, 1);
    EXPECT_GE(summary.violations.size(), 1u);
    EXPECT_EQ(summary.violations[0].type, "clearance");
    EXPECT_EQ(summary.violations[0].severity, "error");
    ASSERT_TRUE(summary.violations[0].expected_clearance_mm.has_value());
    ASSERT_TRUE(summary.violations[0].shortfall_mm.has_value());
    EXPECT_NEAR(summary.violations[0].shortfall_mm.value()
                    - (summary.violations[0].expected_clearance_mm.value()
                       - summary.violations[0].actual_clearance_mm.value()),
                0.0, 1e-6);
    EXPECT_FALSE(summary.hints.empty());

    std::string json = summary.to_json();
    EXPECT_NE(json.find("\"clearance_violations_count\":1"), std::string::npos);
}

} // namespace
} // namespace freerouting::drc
