#include "core/scoring/BoardStatistics.h"

#include <algorithm>
#include <cmath>

namespace freerouting {
namespace core {
namespace scoring {

BoardStatistics::BoardStatistics() {}

BoardStatistics::~BoardStatistics() {}

BoardStatistics::BoardStatistics(board::RoutingBoard* board) : BoardStatistics(board, "") {}

BoardStatistics::BoardStatistics(board::RoutingBoard* board, const std::string& unit) { init_from_board(board, unit); }

BoardStatistics::BoardStatistics(const std::string& data, FileFormat format) {
    (void)data;
    (void)format;
}

void BoardStatistics::init_from_board(board::RoutingBoard* board, const std::string& unit) {
    (void)board;
    (void)unit;
}

std::string BoardStatistics::toString() const {
    return "BoardStatistics{}";
}

float BoardStatistics::calculateScore(settings::RouterScoringSettings scoringSettings) {
    float maximumScore = getMaximumScore(scoringSettings);
    float penalties = 0.0f;
    float costs = 0.0f;
    return maximumScore - penalties - costs;
}

float BoardStatistics::getMaximumScore(settings::RouterScoringSettings scoringSettings) {
    return 0.0f;
}

float BoardStatistics::getNormalizedScore(settings::RouterScoringSettings scoringSettings) {
    float maxScore = getMaximumScore(scoringSettings);
    if (maxScore > 0.0f) {
        return std::max(0.0f, calculateScore(scoringSettings) / maxScore) * 1000.0f;
    }
    return 0.0f;
}

} // namespace scoring
} // namespace core
} // namespace freerouting