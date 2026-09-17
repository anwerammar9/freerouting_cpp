#include "core/scoring/BoardStatistics.h"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <vector>

#include "board/BasicBoard.h"
#include "board/Component.h"
#include "board/ConductionArea.h"
#include "board/Item.h"
#include "board/Pin.h"
#include "board/PolylineTrace.h"
#include "board/RoutingBoard.h"
#include "board/Trace.h"
#include "board/Via.h"
#include "board/ComponentOutline.h"
#include "geometry/planar/Point.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/Polyline.h"
#include "rules/BoardRules.h"
#include "rules/Net.h"
#include "rules/NetClass.h"
#include "interactive/RatsNest.h"
#include "logger/FRLogger.h"

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
    if (board == nullptr) return;

    auto& bb = board->get_bounding_box();

    // Host info
    auto* comm = board->communication();
    if (comm != nullptr && comm->get_parser_info().has_host_cad()) {
        host_ = comm->get_parser_info().host_cad + "," + comm->get_parser_info().host_version;
    } else {
        host_ = "Freerouting";
    }

    // Unit
    if (unit.empty()) {
        unit_ = board::UnitHelper::to_string(comm != nullptr ? comm->get_unit() : board::Unit::MIL);
    } else {
        unit_ = unit;
    }

    auto& layer_structure = board->get_layer_structure();

    // Board
    board_.boundingBox = bb.ur.to_float();
    board_.size = geometry::planar::FloatPoint(
        static_cast<float>(std::abs(bb.ll.x - bb.ur.x)),
        static_cast<float>(std::abs(bb.ll.y - bb.ur.y))
    );

    // Layers
    layers_.totalCount = board->get_layer_count();
    layers_.signalCount = layer_structure.signal_layer_count();

    // Items - iterate through all items on the board
    items_.totalCount = 0;
    items_.traceCount = 0;
    items_.viaCount = 0;
    items_.conductionAreaCount = 0;
    items_.drillItemCount = 0;
    items_.pinCount = 0;
    items_.componentOutlineCount = 0;
    items_.otherCount = 0;

    for (int i = 0; i < board->get_item_count(); ++i) {
        board::Item* curr_item = board->get_item(i);
        if (curr_item == nullptr) continue;
        items_.totalCount++;
        if (dynamic_cast<board::Trace*>(curr_item) != nullptr) {
            items_.traceCount++;
        } else if (dynamic_cast<board::Via*>(curr_item) != nullptr) {
            items_.viaCount++;
            items_.drillItemCount++;
        } else if (dynamic_cast<board::ConductionArea*>(curr_item) != nullptr) {
            items_.conductionAreaCount++;
        } else if (dynamic_cast<board::DrillItem*>(curr_item) != nullptr) {
            items_.drillItemCount++;
        } else if (dynamic_cast<board::Pin*>(curr_item) != nullptr) {
            items_.pinCount++;
            items_.drillItemCount++;
        } else if (dynamic_cast<board::ComponentOutline*>(curr_item) != nullptr) {
            items_.componentOutlineCount++;
        } else {
            items_.otherCount++;
        }
    }

    // Components
    if (board->components() != nullptr) {
        components_.totalCount = board->components()->component_count();
    } else {
        components_.totalCount = 0;
    }

    // Pads
    pads_.totalCount = static_cast<int>(board->get_pins().size());

    // Nets
    nets_.totalCount = board->get_rules().get_nets().max_net_no();
    nets_.classCount = board->get_rules().get_net_classes().count();

    // Traces
    traces_.totalCount = 0;
    traces_.totalLength = 0;
    traces_.averageLength = 0;
    traces_.totalSegmentCount = 0;
    traces_.totalHorizontalLength = 0;
    traces_.totalVerticalLength = 0;
    traces_.totalAngledLength = 0;
    traces_.totalWeightedLength = 0;

    const auto& all_traces = board->get_traces();
    traces_.totalCount = static_cast<int>(all_traces.size());
    if (traces_.totalCount > 0) {
        for (board::Trace* curr_trace : all_traces) {
            traces_.totalLength += static_cast<float>(curr_trace->get_length());
        }
        traces_.averageLength = traces_.totalLength / traces_.totalCount;
    }

    for (board::Trace* curr_trace : all_traces) {
        if (auto* poly_trace = dynamic_cast<board::PolylineTrace*>(curr_trace)) {
            const auto& polyline = poly_trace->polyline();
            int cornerCount = polyline.corner_count();
            if (cornerCount > 1) {
                traces_.totalSegmentCount += cornerCount - 1;
            }

            for (const auto& line : polyline.arr) {
                auto a = line.a.to_float();
                auto b = line.b.to_float();
                float length = static_cast<float>(std::sqrt(std::pow(a.x - b.x, 2) + std::pow(a.y - b.y, 2)));

                if (std::abs(a.x - b.x) < 0.0001f) {
                    traces_.totalVerticalLength += length;
                } else if (std::abs(a.y - b.y) < 0.0001f) {
                    traces_.totalHorizontalLength += length;
                } else {
                    traces_.totalAngledLength += length;
                }
            }
        }
    }

    // Weighted trace length
    int default_clearance_class = rules::BoardRules::default_clearance_class();
    for (board::Trace* curr_trace : all_traces) {
        board::FixedState fixed_state = curr_trace->get_fixed_state();
        if (fixed_state == board::FixedState::NOT_FIXED || fixed_state == board::FixedState::SHOVE_FIXED) {
            double weighted_trace_length = curr_trace->get_length() *
                (curr_trace->get_half_width() + board->get_rules().get_clearance_matrix().get_value(
                    curr_trace->get_clearance_class(), default_clearance_class, curr_trace->get_layer(), false));
            if (fixed_state == board::FixedState::SHOVE_FIXED) {
                weighted_trace_length /= 2;
            }
            traces_.totalWeightedLength += static_cast<float>(weighted_trace_length);
        }
    }

    // Connections (RatsNest)
    {
        interactive::RatsNest ratsnest(board);
        connections_.maximumCount = ratsnest.max_connections;
        connections_.incompleteCount = ratsnest.incomplete_count();
    }

    // Bends
    bends_.totalCount = 0;
    bends_.ninetyDegreeCount = 0;
    bends_.fortyFiveDegreeCount = 0;
    bends_.otherAngleCount = 0;
    for (board::Trace* curr_trace : all_traces) {
        if (auto* poly_trace = dynamic_cast<board::PolylineTrace*>(curr_trace)) {
            const auto& polyline = poly_trace->polyline();
            int cornerCount = polyline.corner_count();
            if (cornerCount >= 3) {
                for (int i = 1; i < cornerCount - 1; ++i) {
                    auto prev = polyline.corner(i - 1).to_float();
                    auto curr = polyline.corner(i).to_float();
                    auto next = polyline.corner(i + 1).to_float();

                    double dx1 = curr.x - prev.x;
                    double dy1 = curr.y - prev.y;
                    double dx2 = next.x - curr.x;
                    double dy2 = next.y - curr.y;

                    double angle = std::abs(std::atan2(dy2, dx2) - std::atan2(dy1, dx1));
                    angle = std::abs(angle * 180.0 / M_PI);
                    angle = std::min(angle, 360.0 - angle);
                    if (angle > 180) angle = 360 - angle;

                    if (std::abs(angle - 90) < 1.0) {
                        bends_.ninetyDegreeCount++;
                    } else if (std::abs(angle - 45) < 1.0 || std::abs(angle - 135) < 1.0) {
                        bends_.fortyFiveDegreeCount++;
                    } else {
                        bends_.otherAngleCount++;
                    }
                }
                bends_.totalCount += cornerCount - 2;
            }
        }
    }

    // Vias
    const auto& vias = board->get_vias();
    vias_.totalCount = static_cast<int>(vias.size());
    vias_.throughHoleCount = 0;
    vias_.blindCount = 0;
    vias_.buriedCount = 0;
    for (const board::Via* curr_via : vias) {
        int first_layer = curr_via->first_layer();
        int last_layer = curr_via->last_layer();
        if (first_layer == 0 && last_layer == layers_.totalCount - 1) {
            vias_.throughHoleCount++;
        } else if (first_layer == 0 || last_layer == layers_.totalCount - 1) {
            vias_.blindCount++;
        } else {
            vias_.buriedCount++;
        }
    }

    // Clearance violations
    const board::BoardOutline* outline = board->get_outline();
    if (outline != nullptr) {
        clearanceViolations_.totalCount = outline->clearance_violation_count();
    } else {
        clearanceViolations_.totalCount = 0;
    }
}

std::string BoardStatistics::toString() const {
    std::ostringstream oss;
    oss << "{host: " << host_ << ", unit: " << unit_
        << ", board: {size: " << board_.size.x << "," << board_.size.y << "}"
        << ", layers: {total: " << layers_.totalCount << ", signal: " << layers_.signalCount << "}"
        << ", items: {total: " << items_.totalCount << ", traces: " << items_.traceCount
        << ", vias: " << items_.viaCount << "}"
        << ", traces: {count: " << traces_.totalCount << ", length: " << traces_.totalLength
        << ", avgLength: " << traces_.averageLength << ", segments: " << traces_.totalSegmentCount
        << ", horizontal: " << traces_.totalHorizontalLength << ", vertical: " << traces_.totalVerticalLength
        << ", angled: " << traces_.totalAngledLength << "}"
        << ", connections: {max: " << connections_.maximumCount << ", incomplete: " << connections_.incompleteCount << "}"
        << ", bends: {total: " << bends_.totalCount << ", 90deg: " << bends_.ninetyDegreeCount
        << ", 45deg: " << bends_.fortyFiveDegreeCount << "}"
        << ", vias: {total: " << vias_.totalCount << ", throughHole: " << vias_.throughHoleCount << "}"
        << ", clearanceViolations: {total: " << clearanceViolations_.totalCount << "}"
        << "}";
    return oss.str();
}

float BoardStatistics::calculateScore(settings::RouterScoringSettings scoringSettings) {
    float maximumScore = getMaximumScore(scoringSettings);
    float penalties = connections_.incompleteCount * scoringSettings.unroutedNetPenalty
        + clearanceViolations_.totalCount * scoringSettings.clearanceViolationPenalty
        + bends_.totalCount * scoringSettings.bendPenalty;
    float costs = static_cast<float>(traces_.totalLength * scoringSettings.defaultPreferredDirectionTraceCost
        + vias_.totalCount * scoringSettings.via_costs);
    return maximumScore - penalties - costs;
}

float BoardStatistics::getMaximumScore(settings::RouterScoringSettings scoringSettings) {
    return static_cast<float>(connections_.maximumCount * scoringSettings.unroutedNetPenalty);
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