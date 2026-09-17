#include <gtest/gtest.h>

#include "core/scoring/BoardStatistics.h"
#include "board/RoutingBoard.h"
#include "settings/RouterScoringSettings.h"
#include "board/Unit.h"
#include "rules/BoardRules.h"
#include "core/FileFormat.h"
#include "geometry/planar/IntBox.h"

using namespace freerouting::board;
using namespace freerouting::rules;
using namespace freerouting::core;
using namespace freerouting::core::scoring;
using namespace freerouting::settings;
using namespace freerouting::geometry::planar;

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