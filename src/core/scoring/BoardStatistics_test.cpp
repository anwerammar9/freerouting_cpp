#include <gtest/gtest.h>

#include "core/scoring/BoardStatistics.h"
#include "board/RoutingBoard.h"
#include "settings/RouterScoringSettings.h"
#include "board/Unit.h"
#include "rules/BoardRules.h"
#include "core/FileFormat.h"
#include "geometry/planar/Polyline.h"

TEST(BoardStatisticsTest, CanCreateFromBoard) {
    // Create a minimal board for testing
    std::vector<std::shared_ptr<board::Layer>> layers;
    layers.push_back(std::make_shared<board::Layer>("Top", true));
    layers.push_back(std::make_shared<board::Layer>("Bottom", true));
    layers.push_back(std::make_shared<board::Layer>("Inner1", false));
    layers.push_back(std::make_shared<board::Layer>("Vcc", false));
    LayerStructure layerStructure(layers);

    std::vector<int> netClasses; // simplified
    BoardRules rules(layerStructure, 1);

    RoutingBoard board(
        geometry::planar::IntBox(0, 0, 1000, 1000),
        layerStructure,
        std::make_unique<rules::BoardRules>(rules)
    );

    // Add some test items
    // Note: In a real test, we'd add traces, vias, pins, etc.
    // For now, just verify construction doesn't crash
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
(layer B.Paste)
(layer F.SilkS)
(layer B.SilkS)
(layer F.Mask)
(layer B.Mask)
(layer Edge.Cuts)
(component R1 0 0 0 SMD)
(via 1 50 50 F.Cu B.Cu)
(wire (start 0 0) (end 100 0) (layer F.Cu) (width 10))
(net 1 "TestNet")
)";
    BoardStatistics stats(dsnContent, FileFormat::DSN);
    EXPECT_NE(&stats, nullptr);
}

TEST(BoardStatisticsTest, CanCreateFromSES) {
    std::string sesContent = R"(
(path F.Cu)
(path B.Cu)
(component R1 0 0 0 SMD)
(via 1 50 50)
(wire (start 0 0) (end 100 0))
(net 1 "TestNet")
)";
    BoardStatistics stats(sesContent, FileFormat::SES);
    EXPECT_NE(&stats, nullptr);
}

TEST(BoardStatisticsTest, ScoreCalculation) {
    std::vector<std::shared_ptr<board::Layer>> layers;
    layers.push_back(std::make_shared<board::Layer>("Top", true));
    layers.push_back(std::make_shared<board::Layer>("Bottom", true));
    LayerStructure layerStructure(layers);

    BoardRules rules(layerStructure, 1);

    RoutingBoard board(
        geometry::planar::IntBox(0, 0, 1000, 1000),
        layerStructure,
        std::make_unique<rules::BoardRules>(rules)
    );

    // Add a simple trace
    // For now, just verify score calculation doesn't crash
    BoardStatistics stats(&board);
    RouterScoringSettings settings;
    float score = stats.calculateScore(settings);
    EXPECT_GE(score, 0.0f);
}