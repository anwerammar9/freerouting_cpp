#pragma once

#include <string>

#include "board/RoutingBoard.h"
#include "settings/RouterScoringSettings.h"
#include "core/FileFormat.h"

namespace freerouting {
namespace core {
namespace scoring {

class BoardStatistics
{
public:
    BoardStatistics();
    ~BoardStatistics();

    BoardStatistics(board::RoutingBoard* board);
    BoardStatistics(board::RoutingBoard* board, const std::string& unit);
    BoardStatistics(const std::string& data, FileFormat format);

    float calculateScore(settings::RouterScoringSettings scoringSettings);
    float getMaximumScore(settings::RouterScoringSettings scoringSettings);
    float getNormalizedScore(settings::RouterScoringSettings scoringSettings);
    std::string toString() const;

    // Accessors for sub-sections
    int totalCount() const { return 0; }
    int traceCount() const { return 0; }
    int viaCount() const { return 0; }

private:
    void init_from_board(board::RoutingBoard* board, const std::string& unit);
};

} // namespace scoring
} // namespace core
} // namespace freerouting