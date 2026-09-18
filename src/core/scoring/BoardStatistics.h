#pragma once

#include <string>
#include <vector>
#include <memory>
#include <cstdint>

#include "board/RoutingBoard.h"
#include "settings/RouterScoringSettings.h"
#include "core/FileFormat.h"
#include "geometry/planar/Point.h"

namespace freerouting {
namespace board {
class Item;
class Trace;
class Via;
class PolylineTrace;
class ConductionArea;
class Pin;
class ComponentOutline;
class DrillItem;
} // namespace board
} // namespace freerouting

namespace freerouting {
namespace core {
namespace scoring {

// Sub-struct mirrors Java BoardStatisticsBoard
struct BoardStatisticsBoard {
    geometry::planar::FloatPoint boundingBox = {0, 0};
    geometry::planar::FloatPoint size = {0, 0};
    float areaCm2 = 0;
};

// Sub-struct mirrors Java BoardStatisticsLayers
struct BoardStatisticsLayers {
    int totalCount = 0;
    int signalCount = 0;
};

// Sub-struct mirrors Java BoardStatisticsItems
struct BoardStatisticsItems {
    int totalCount = 0;
    int traceCount = 0;
    int viaCount = 0;
    int conductionAreaCount = 0;
    int drillItemCount = 0;
    int pinCount = 0;
    int componentOutlineCount = 0;
    int otherCount = 0;
};

// Sub-struct mirrors Java BoardStatisticsComponents
struct BoardStatisticsComponents {
    int totalCount = 0;
};

// Sub-struct mirrors Java BoardStatisticsPads
struct BoardStatisticsPads {
    int totalCount = 0;
};

// Sub-struct mirrors Java BoardStatisticsNets
struct BoardStatisticsNets {
    int totalCount = 0;
    int classCount = 0;
};

// Sub-struct mirrors Java BoardStatisticsConnections
struct BoardStatisticsConnections {
    int maximumCount = 0;
    int incompleteCount = 0;
};

// Sub-struct mirrors Java BoardStatisticsTraces
struct BoardStatisticsTraces {
    int totalCount = 0;
    float totalLength = 0;
    float totalLengthMm = 0;
    float averageLength = 0;
    int totalSegmentCount = 0;
    float totalHorizontalLength = 0;
    float totalVerticalLength = 0;
    float totalAngledLength = 0;
    float totalWeightedLength = 0;
};

// Sub-struct mirrors Java BoardStatisticsBends
struct BoardStatisticsBends {
    int totalCount = 0;
    int ninetyDegreeCount = 0;
    int fortyFiveDegreeCount = 0;
    int otherAngleCount = 0;
};

// Sub-struct mirrors Java BoardStatisticsVias
struct BoardStatisticsVias {
    int totalCount = 0;
    int throughHoleCount = 0;
    int blindCount = 0;
    int buriedCount = 0;
};

// Sub-struct mirrors Java BoardStatisticsClearanceViolations
struct BoardStatisticsClearanceViolations {
    int totalCount = 0;
};

// Sub-struct mirrors Java BoardStatisticsDifficulty
struct BoardStatisticsDifficulty {
    int pinCount = 0;
    int signalLayerCount = 0;
    int complexityC = 0;
    float boardAreaCm2 = 0;
    float difficultyD = 0;
};

// Sub-struct mirrors Java BoardStatisticsFanout
struct BoardStatisticsFanout {
    int totalSmdPins = 0;
    int pinsToEscape = 0;
    int escapedCount = 0;
};

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
    int totalCount() const { return items_.totalCount; }
    int traceCount() const { return items_.traceCount; }
    int viaCount() const { return items_.viaCount; }
    float board_area_cm2() const { return board_.areaCm2; }
    float total_length_mm() const { return traces_.totalLengthMm; }
    int difficulty_pin_count() const { return difficulty_.pinCount; }
    int difficulty_signal_layer_count() const { return difficulty_.signalLayerCount; }
    int difficulty_complexity_c() const { return difficulty_.complexityC; }
    float difficulty_d() const { return difficulty_.difficultyD; }
    int fanout_total_smd_pins() const { return fanout_.totalSmdPins; }
    int fanout_pins_to_escape() const { return fanout_.pinsToEscape; }
    int fanout_escaped_count() const { return fanout_.escapedCount; }
    const std::string& host() const { return host_; }
    int total_layers() const { return layers_.totalCount; }
    int net_count() const { return nets_.totalCount; }
    int net_class_count() const { return nets_.classCount; }
    int component_count() const { return components_.totalCount; }
    int trace_count() const { return traces_.totalCount; }
    int via_count() const { return vias_.totalCount; }

private:
    void init_from_board(board::RoutingBoard* board, const std::string& unit);

    BoardStatisticsBoard board_;
    BoardStatisticsLayers layers_;
    BoardStatisticsItems items_;
    BoardStatisticsComponents components_;
    BoardStatisticsPads pads_;
    BoardStatisticsNets nets_;
    BoardStatisticsConnections connections_;
    BoardStatisticsTraces traces_;
    BoardStatisticsBends bends_;
    BoardStatisticsVias vias_;
    BoardStatisticsClearanceViolations clearanceViolations_;
    BoardStatisticsDifficulty difficulty_;
    BoardStatisticsFanout fanout_;
    std::string host_;
    std::string unit_;
};

} // namespace scoring
} // namespace core
} // namespace freerouting