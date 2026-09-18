#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include "core/scoring/BoardStatistics.h"
#include "board/RoutingBoard.h"
#include "board/Pin.h"
#include "board/Padstack.h"
#include "board/PolylineTrace.h"
#include "board/Layer.h"
#include "board/SearchTreeManager.h"
#include "settings/RouterScoringSettings.h"
#include "board/Unit.h"
#include "rules/BoardRules.h"
#include "core/FileFormat.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/Polyline.h"
#include "geometry/planar/PolygonShape.h"

using namespace freerouting::board;
using namespace freerouting::rules;
using namespace freerouting::core;
using namespace freerouting::core::scoring;
using namespace freerouting::settings;
using namespace freerouting::geometry::planar;

namespace {

std::shared_ptr<RoutingBoard> make_test_board()
{
    std::vector<std::shared_ptr<Layer>> layers;
    layers.push_back(std::make_shared<Layer>("Top", true));
    layers.push_back(std::make_shared<Layer>("Bottom", true));
    layers.push_back(std::make_shared<Layer>("Inner1", false));
    layers.push_back(std::make_shared<Layer>("Inner2", false));
    LayerStructure layer_structure(layers);
    auto rules = std::make_unique<BoardRules>(layer_structure, 1);
    rules->get_nets().add("net1", 1, false);
    rules->get_nets().add("net2", 1, false);
    rules->get_nets().add("net3", 1, false);
    return std::make_shared<RoutingBoard>(IntBox(0, 0, 10000, 10000), layer_structure,
                                          std::move(rules), nullptr);
}

PolylineTrace* insert_trace_on_layer0(RoutingBoard* p_board,
                                      const std::vector<IntPoint>& p_corners,
                                      const std::vector<int>& p_nets)
{
    auto polyline = std::make_shared<Polyline>(Polyline::from_corners(p_corners));
    return p_board->insert_trace_without_cleaning(polyline, 0, 50, p_nets, 1,
                                                  FixedState::NOT_FIXED);
}

Pin* insert_smd_pin(RoutingBoard* p_board, const IntPoint& p_center, const std::vector<int>& p_nets)
{
    auto* pin = new Pin(7, 1, p_nets, 1, p_board->generate_item_id(), FixedState::NOT_FIXED,
                        p_board);
    pin->set_center(p_center);
    auto* padstack = new Padstack("p1");
    padstack->set_from_layer(0);
    padstack->set_to_layer(0);
    padstack->add_shape(0, new PolygonShape(std::vector<IntPoint>{
                               {-50, -50}, {50, -50}, {50, 50}, {-50, 50}}));
    pin->set_padstack(padstack);
    pin->set_bounding_box(IntBox(p_center.x - 50, p_center.y - 50, p_center.x + 50,
                                 p_center.y + 50));
    p_board->get_search_tree_manager()->insert(pin);
    p_board->add_item(pin);
    return pin;
}

} // namespace

TEST(BoardStatisticsTest, CanCreateFromBoard) {
    std::vector<std::shared_ptr<Layer>> layers;
    layers.push_back(std::make_shared<Layer>("Top", true));
    layers.push_back(std::make_shared<Layer>("Bottom", true));
    layers.push_back(std::make_shared<Layer>("Inner1", false));
    layers.push_back(std::make_shared<Layer>("Vcc", false));
    LayerStructure layerStructure(layers);

    auto rules = std::make_unique<BoardRules>(layerStructure, 1);

    RoutingBoard board(
        IntBox(0, 0, 1000, 1000),
        layerStructure,
        std::move(rules)
    );

    BoardStatistics stats(&board);
    EXPECT_NE(&stats, nullptr);
}

TEST(BoardStatisticsTest, CanCreateFromDSN) {
    std::string dsnContent = R"(
(host_cad "test")
(host_version "1.0")
(layer F.Cu)
(layer B.Cu)
(layer F.Paste)
)";
    BoardStatistics stats(dsnContent, FileFormat::DSN);
    EXPECT_NE(&stats, nullptr);
}

TEST(BoardStatisticsTest, CanCreateFromSES) {
    std::string sesContent = R"(
(path F.Cu)
(path B.Cu)
(component R1 0 0 0 SMD)
)";
    BoardStatistics stats(sesContent, FileFormat::SES);
    EXPECT_NE(&stats, nullptr);
}

TEST(BoardStatisticsTest, ScoreCalculation) {
    std::vector<std::shared_ptr<Layer>> layers;
    layers.push_back(std::make_shared<Layer>("Top", true));
    layers.push_back(std::make_shared<Layer>("Bottom", true));
    LayerStructure layerStructure(layers);

    auto rules = std::make_unique<BoardRules>(layerStructure, 1);

    RoutingBoard board(
        IntBox(0, 0, 1000, 1000),
        layerStructure,
        std::move(rules)
    );

    BoardStatistics stats(&board);
    RouterScoringSettings settings;
    float score = stats.calculateScore(settings);
    EXPECT_GE(score, 0.0f);
}

TEST(BoardStatisticsTest, AreaDifficultyAndLengthMm) {
    auto board = make_test_board();
    insert_trace_on_layer0(board.get(),
                           {IntPoint(0, 0), IntPoint(200, 0), IntPoint(200, 100)},
                           {1});
    BoardStatistics stats(board.get());

    // 10000 x 10000 board in mils (default board unit) with resolution 1.
    double factor = 25.4 / 1000.0;
    double area = 10000.0 * factor * 10000.0 * factor / 100.0;
    EXPECT_NEAR(stats.board_area_cm2(), area, 0.01);
    EXPECT_NEAR(stats.total_length_mm(), 300.0f * (float)factor, 0.001);
    // 2 pins? none, so difficulty C falls back to max(1, 0 * signal layers) = 1.
    EXPECT_EQ(stats.difficulty_pin_count(), 0);
    EXPECT_EQ(stats.difficulty_signal_layer_count(), 2);
    EXPECT_EQ(stats.difficulty_complexity_c(), 1);
    EXPECT_EQ(stats.difficulty_d(), 1.0f);
}

TEST(BoardStatisticsTest, DifficultyFromPinCount) {
    auto board = make_test_board();
    insert_smd_pin(board.get(), IntPoint(200, 200), {1});
    insert_smd_pin(board.get(), IntPoint(4000, 200), {1});
    insert_smd_pin(board.get(), IntPoint(200, 6000), {3});
    insert_smd_pin(board.get(), IntPoint(4000, 6000), {3});
    BoardStatistics stats(board.get());

    EXPECT_EQ(stats.difficulty_pin_count(), 4);
    EXPECT_EQ(stats.difficulty_signal_layer_count(), 2);
    EXPECT_EQ(stats.difficulty_complexity_c(), 8);
    EXPECT_FLOAT_EQ(stats.difficulty_d(), 8.0f);
}

TEST(BoardStatisticsTest, FanoutOfSmdPins) {
    auto board = make_test_board();
    // Net 1: two SMD pins connected by a trace -> both connected and escaped.
    Pin* a = insert_smd_pin(board.get(), IntPoint(200, 200), {1});
    Pin* b = insert_smd_pin(board.get(), IntPoint(4000, 200), {1});
    ASSERT_NE(a, nullptr);
    ASSERT_NE(b, nullptr);
    insert_trace_on_layer0(board.get(), {IntPoint(200, 200), IntPoint(4000, 200)}, {1});
    // Net 3: two isolated SMD pins -> not escaped.
    Pin* c = insert_smd_pin(board.get(), IntPoint(200, 6000), {3});
    Pin* d = insert_smd_pin(board.get(), IntPoint(4000, 6000), {3});
    ASSERT_NE(c, nullptr);
    ASSERT_NE(d, nullptr);

    BoardStatistics stats(board.get());

    EXPECT_EQ(stats.fanout_total_smd_pins(), 4);
    EXPECT_EQ(stats.fanout_escaped_count(), 2);
    EXPECT_EQ(stats.fanout_pins_to_escape(), 2);
}

TEST(BoardStatisticsTest, DsnCountsAndHost) {
    std::string dsn = R"((parser name "test") (hostCad "CadSoft") (hostVersion "v8")
(layer F.Cu)
(layer B.Cu)
(class default 10 10)
(net "n1")
(net "n2")
(wire F.Cu path)
(via 0 0)
(component R1)
)";
    BoardStatistics stats(dsn, FileFormat::DSN);

    EXPECT_EQ(stats.host().find("CadSoft, v8") != std::string::npos, true);
    EXPECT_EQ(stats.total_layers(), 2);
    EXPECT_EQ(stats.net_count(), 2);
    EXPECT_EQ(stats.net_class_count(), 1);
    EXPECT_EQ(stats.component_count(), 1);
    EXPECT_EQ(stats.trace_count(), 1);
    EXPECT_EQ(stats.via_count(), 1);
}

TEST(BoardStatisticsTest, SesLayerAndComponentCounts) {
    std::string ses = R"((path F.Cu)
(path B.Cu)
(component R1 0 0 0 SMD)
(net "n1")
(wire F.Cu path)
)";
    BoardStatistics stats(ses, FileFormat::SES);

    EXPECT_EQ(stats.total_layers(), 2);
    EXPECT_EQ(stats.component_count(), 1);
    EXPECT_EQ(stats.net_count(), 1);
    EXPECT_EQ(stats.trace_count(), 1);
}

TEST(BoardStatisticsTest, NormalizedScoreUsesMmLength) {
    std::vector<std::shared_ptr<Layer>> layers;
    layers.push_back(std::make_shared<Layer>("Top", true));
    layers.push_back(std::make_shared<Layer>("Bottom", true));
    LayerStructure layer_structure(layers);
    auto rules = std::make_unique<BoardRules>(layer_structure, 1);
    // Only net1 is registered so that max_connections (total items - net
    // count) reflects the single 2-pin net instead of being diluted by empty
    // nets (mirrors the Java RatsNest constructor).
    rules->get_nets().add("net1", 1, false);
    auto board = std::make_shared<RoutingBoard>(IntBox(0, 0, 10000, 10000),
                                                layer_structure, std::move(rules), nullptr);

    insert_smd_pin(board.get(), IntPoint(200, 200), {1});
    insert_smd_pin(board.get(), IntPoint(4000, 200), {1});
    insert_trace_on_layer0(board.get(), {IntPoint(200, 200), IntPoint(4000, 200)}, {1});

    BoardStatistics stats(board.get());
    RouterScoringSettings settings;

    // 3 items (2 pins + 1 trace) on 1 net -> max_connections = 2.
    EXPECT_FLOAT_EQ(stats.getMaximumScore(settings), 2.0f * settings.unroutedNetPenalty);
    float score = stats.calculateScore(settings);
    EXPECT_GT(score, 0.0f);
    float normalized = stats.getNormalizedScore(settings);
    EXPECT_GT(normalized, 0.0f);
    EXPECT_LE(normalized, 1000.0f);
}

TEST(BoardStatisticsTest, NormalizedScoreZeroForNoConnections) {
    auto board = make_test_board();
    BoardStatistics stats(board.get());
    RouterScoringSettings settings;

    // Empty board -> total items (0) - registered nets (3) is negative, the
    // same degenerate value the Java RatsNest computes. The normalisation
    // guard must clamp the result to zero.
    EXPECT_LE(stats.getMaximumScore(settings), 0.0f);
    EXPECT_FLOAT_EQ(stats.getNormalizedScore(settings), 0.0f);
}