#include "core/scoring/BoardStatistics.h"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <set>
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
#include "constants/Constants.h"
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

namespace {

/**
 * Counts the occurrences of p_target in p_text. Mirrors Java
 * BoardStatistics.countOccurrences().
 */
int count_occurrences(const std::string& p_text, const std::string& p_target)
{
    int count = 0;
    size_t index = 0;
    while ((index = p_text.find(p_target, index)) != std::string::npos)
    {
        ++count;
        index += p_target.length();
    }
    return count;
}

/**
 * Returns true, if the pin is connected to all connectable items of its first
 * net. Mirrors the isEmpty(getUnconnectedSet(netNumber)) check in the Java
 * fanout calculation.
 */
bool pin_is_fully_connected(board::Pin* p_pin, board::RoutingBoard* p_board)
{
    const std::vector<int>& net_nos = p_pin->get_net_nos();
    if (net_nos.empty())
    {
        return false;
    }
    int net_no = net_nos[0];
    if (net_no <= 0)
    {
        return false;
    }
    board::ItemSet unconnected;
    for (board::Item* curr_item : p_board->get_connectable_items(net_no))
    {
        unconnected.insert(curr_item);
    }
    const board::ItemSet connected = p_pin->get_connected_set(net_no, false);
    for (board::Item* curr_item : connected)
    {
        unconnected.erase(curr_item);
    }
    return unconnected.empty();
}

/**
 * Returns true, if the pin has a valid trace or via escape. Mirrors Java
 * BoardStatistics.isPinEscaped(). The C++ board model has no per-item DRC
 * clearance-violation collection yet, so the Java checks against
 * trace/via.clearanceViolations() are approximated away (they would only
 * reject a pin whose only contacts already violate clearance).
 */
bool pin_is_escaped(board::Pin* p_pin, board::RoutingBoard* p_board)
{
    (void)p_board;
    for (board::Item* contact : p_pin->get_normal_contacts())
    {
        if (dynamic_cast<board::Trace*>(contact) != nullptr)
        {
            return true;
        }
        if (auto* via = dynamic_cast<board::Via*>(contact))
        {
            for (board::Item* via_contact : via->get_normal_contacts())
            {
                if (dynamic_cast<board::Trace*>(via_contact) != nullptr
                    || dynamic_cast<board::ConductionArea*>(via_contact) != nullptr)
                {
                    return true;
                }
            }
        }
        else if (dynamic_cast<board::ConductionArea*>(contact) != nullptr)
        {
            return true;
        }
    }
    return false;
}

} // namespace

BoardStatistics::BoardStatistics() {}

BoardStatistics::~BoardStatistics() {}

BoardStatistics::BoardStatistics(board::RoutingBoard* board) : BoardStatistics(board, "") {}

BoardStatistics::BoardStatistics(board::RoutingBoard* board, const std::string& unit) { init_from_board(board, unit); }

BoardStatistics::BoardStatistics(const std::string& data, FileFormat format)
{
    if (data.empty())
    {
        return;
    }
    if (format == FileFormat::SES)
    {
        // Layers are collected from all "(path <layer>" occurrences.
        std::set<std::string> layers;
        size_t pos = 0;
        while ((pos = data.find("(path ", pos)) != std::string::npos)
        {
            size_t word_start = pos + std::string("(path ").length();
            size_t word_end = data.find_first_of(" \n\r\t", word_start);
            if (word_end != std::string::npos)
            {
                layers.insert(data.substr(word_start, word_end - word_start));
            }
            pos = word_start;
        }
        layers_.totalCount = static_cast<int>(layers.size());
        components_.totalCount = count_occurrences(data, "(component");
        nets_.totalCount = count_occurrences(data, "(net");
        traces_.totalCount = count_occurrences(data, "(wire");
        vias_.totalCount = count_occurrences(data, "(via");
    }
    else if (format == FileFormat::DSN)
    {
        // Extract the host from the "(hostCad ...)" and "(hostVersion ...)"
        // tokens anywhere in the content (in real DSN files they live inside
        // the "(parser ...)" scope, but searching the whole content keeps the
        // parser simple). Mirrors the Java line-based scan in the
        // BoardStatistics file constructor.
        auto extract_quoted_value = [](const std::string& p_content, const std::string& p_key)
            -> std::string {
            size_t key_idx = p_content.find(p_key);
            if (key_idx == std::string::npos)
            {
                return "";
            }
            size_t value_start = key_idx + p_key.length();
            size_t first_quote = p_content.find('"', value_start);
            if (first_quote == std::string::npos)
            {
                return "";
            }
            first_quote += 1;
            size_t last_quote = p_content.find('"', first_quote);
            if (last_quote == std::string::npos)
            {
                return "";
            }
            return p_content.substr(first_quote, last_quote - first_quote);
        };
        std::string host_cad = extract_quoted_value(data, "(hostCad");
        std::string host_version = extract_quoted_value(data, "(hostVersion");

        if (!host_cad.empty() && !host_version.empty())
        {
            host_ = host_cad + ", " + host_version;
        }
        else if (!host_cad.empty())
        {
            host_ = host_cad;
        }

        layers_.totalCount = count_occurrences(data, "(layer");
        components_.totalCount = count_occurrences(data, "(component");
        nets_.classCount = count_occurrences(data, "(class");
        nets_.totalCount = count_occurrences(data, "(net");
        traces_.totalCount = count_occurrences(data, "(wire");
        vias_.totalCount = count_occurrences(data, "(via");
    }
}

void BoardStatistics::init_from_board(board::RoutingBoard* board, const std::string& unit) {
    if (board == nullptr) return;

    const auto bb = board->get_bounding_box();

    // Host info
    auto* comm = board->communication();
    if (comm != nullptr && comm->get_parser_info().has_host_cad()) {
        host_ = comm->get_parser_info().host_cad + "," + comm->get_parser_info().host_version;
    } else {
        host_ = "Freerouting," + std::string(constants::FREEROUTING_VERSION);
    }

    // Unit: Java defaults to MM when no preferred unit is given, and otherwise
    // keeps the board communication unit.
    board::Unit board_unit = comm != nullptr ? comm->get_unit() : board::Unit::MIL;
    int resolution = comm != nullptr ? comm->get_resolution() : 1;
    std::string requested_unit_str = unit;
    if (requested_unit_str.empty()) {
        requested_unit_str = "mm";
    }
    board::Unit requested_unit = board::UnitHelper::from_string(requested_unit_str);
    unit_ = board::UnitHelper::to_string(requested_unit);

    double board_unit_to_mm_factor =
        board::UnitHelper::scale(1.0, board_unit, board::Unit::MM) / (resolution > 0 ? resolution : 1);

    auto& layer_structure = board->get_layer_structure();

    // Board
    board_.boundingBox = bb.ur.to_float();
    board_.size = geometry::planar::FloatPoint(
        static_cast<float>(std::abs(bb.ll.x - bb.ur.x)),
        static_cast<float>(std::abs(bb.ll.y - bb.ur.y))
    );
    board_.areaCm2 = static_cast<float>(
        board_.size.x * board_unit_to_mm_factor * board_.size.y * board_unit_to_mm_factor
        / 100.0);

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
        } else if (dynamic_cast<board::Pin*>(curr_item) != nullptr) {
            items_.pinCount++;
            items_.drillItemCount++;
        } else if (dynamic_cast<board::DrillItem*>(curr_item) != nullptr) {
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
    traces_.totalLengthMm = 0;
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
    // Normalise trace length to millimetres so that calculateScore() uses a
    // physically meaningful cost regardless of the DSN coordinate resolution.
    traces_.totalLengthMm = static_cast<float>(traces_.totalLength * board_unit_to_mm_factor);

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

    // Difficulty. Mirrors Java BoardStatistics constructor: C = max(1, pins *
    // signal layers) and D equals C so via/bend/DRC penalties stay comparable
    // across small and large boards.
    difficulty_.pinCount = items_.pinCount;
    difficulty_.signalLayerCount = layers_.signalCount;
    difficulty_.complexityC = std::max(1, difficulty_.pinCount * difficulty_.signalLayerCount);
    difficulty_.boardAreaCm2 = board_.areaCm2;
    difficulty_.difficultyD = static_cast<float>(difficulty_.complexityC);

    // Fanout statistics for surface-mount pins.
    int total = 0;
    int escaped = 0;
    int already_connected = 0;
    for (board::Pin* curr_pin : board->get_pins()) {
        // SMD pins have a padstack on exactly one layer.
        if (curr_pin->first_layer() != curr_pin->last_layer()) {
            continue;
        }
        if (curr_pin->get_net_nos().empty()) {
            continue;
        }
        ++total;
        if (pin_is_fully_connected(curr_pin, board)) {
            ++already_connected;
        }
        if (pin_is_escaped(curr_pin, board)) {
            ++escaped;
        }
    }
    fanout_.totalSmdPins = total;
    fanout_.pinsToEscape = total - already_connected;
    fanout_.escapedCount = escaped;

    // Convert board and trace length values from the board unit to the
    // preferred unit. Mirrors the unit-conversion block in the Java
    // constructor.
    if (requested_unit != board_unit) {
        double factor = board::UnitHelper::scale(1.0, board_unit, requested_unit);
        board_.boundingBox.x *= static_cast<float>(factor);
        board_.boundingBox.y *= static_cast<float>(factor);
        board_.size.x *= static_cast<float>(factor);
        board_.size.y *= static_cast<float>(factor);
        traces_.totalLength *= static_cast<float>(factor);
        traces_.totalWeightedLength *= static_cast<float>(factor);
        traces_.averageLength *= static_cast<float>(factor);
        traces_.totalHorizontalLength *= static_cast<float>(factor);
        traces_.totalVerticalLength *= static_cast<float>(factor);
        traces_.totalAngledLength *= static_cast<float>(factor);
    }
}

std::string BoardStatistics::toString() const {
    std::ostringstream oss;
    oss << "{host: " << host_ << ", unit: " << unit_
        << ", board: {size: " << board_.size.x << "," << board_.size.y
        << ", area_cm2: " << board_.areaCm2 << "}"
        << ", layers: {total: " << layers_.totalCount << ", signal: " << layers_.signalCount << "}"
        << ", items: {total: " << items_.totalCount << ", traces: " << items_.traceCount
        << ", vias: " << items_.viaCount << "}"
        << ", traces: {count: " << traces_.totalCount << ", length: " << traces_.totalLength
        << ", length_mm: " << traces_.totalLengthMm << ", avgLength: " << traces_.averageLength
        << ", segments: " << traces_.totalSegmentCount << ", horizontal: " << traces_.totalHorizontalLength
        << ", vertical: " << traces_.totalVerticalLength << ", angled: " << traces_.totalAngledLength << "}"
        << ", connections: {max: " << connections_.maximumCount << ", incomplete: " << connections_.incompleteCount << "}"
        << ", bends: {total: " << bends_.totalCount << ", 90deg: " << bends_.ninetyDegreeCount
        << ", 45deg: " << bends_.fortyFiveDegreeCount << "}"
        << ", vias: {total: " << vias_.totalCount << ", throughHole: " << vias_.throughHoleCount << "}"
        << ", clearanceViolations: {total: " << clearanceViolations_.totalCount << "}"
        << ", difficulty: {pins: " << difficulty_.pinCount << ", layers: " << difficulty_.signalLayerCount
        << ", C: " << difficulty_.complexityC << ", D: " << difficulty_.difficultyD << "}"
        << ", fanout: {smd: " << fanout_.totalSmdPins << ", to_escape: " << fanout_.pinsToEscape
        << ", escaped: " << fanout_.escapedCount << "}"
        << "}";
    return oss.str();
}

float BoardStatistics::calculateScore(settings::RouterScoringSettings scoringSettings) {
    float maximumScore = getMaximumScore(scoringSettings);
    float penalties = connections_.incompleteCount * scoringSettings.unroutedNetPenalty
        + clearanceViolations_.totalCount * scoringSettings.clearanceViolationPenalty
        + bends_.totalCount * scoringSettings.bendPenalty;
    // Use the mm-normalised trace length so that the trace-cost term is
    // comparable to the unroutedNetPenalty regardless of the DSN internal
    // coordinate resolution (mirrors Java BoardStatistics.calculateScore).
    float traceLengthForCost =
        traces_.totalLengthMm != 0.0f ? traces_.totalLengthMm : traces_.totalLength;
    float costs = static_cast<float>(traceLengthForCost * scoringSettings.defaultPreferredDirectionTraceCost
        + vias_.totalCount * scoringSettings.via_costs);
    return maximumScore - penalties - costs;
}

float BoardStatistics::getMaximumScore(settings::RouterScoringSettings scoringSettings) {
    return static_cast<float>(connections_.maximumCount * scoringSettings.unroutedNetPenalty);
}

float BoardStatistics::getNormalizedScore(settings::RouterScoringSettings scoringSettings) {
    // Legacy score normalized to a range from zero to one thousand. Mirrors
    // Java getLegacyNormalizedScore().
    float maximumScore = getMaximumScore(scoringSettings);
    if (maximumScore <= 0.0f) {
        // Avoid division by zero and negative maximum scores (e.g. boards with
        // no connections, or boards where all nets are single-pin nets).
        return 0.0f;
    }
    return std::max(0.0f, calculateScore(scoringSettings) / maximumScore) * 1000.0f;
}

} // namespace scoring
} // namespace core
} // namespace freerouting