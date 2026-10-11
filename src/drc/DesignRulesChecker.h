#pragma once

#include <memory>
#include <string>
#include <vector>

#include "board/BasicBoard.h"
#include "board/Item.h"
#include "board/Via.h"
#include "board/Pin.h"
#include "board/RoutingBoard.h"
#include "settings/ProfileSettings.h"
#include "interactive/NetIncompletes.h"
#include "interactive/RatsNest.h"
#include "drc/ClearanceViolation.h"
#include "drc/UnconnectedItems.h"
#include "drc/ZoneIslandViolation.h"
#include "drc/DrcSummaryResponse.h"
#include "drc/DrcReport.h"
#include "drc/DrcViolation.h"
#include "drc/DrcViolationItem.h"
#include "drc/DrcPosition.h"
#include "constants/Constants.h"

namespace freerouting {
namespace drc {

/**
 * Design Rules Checker that centralizes DRC functionality. This class is
 * responsible for detecting clearance violations and other design rule issues.
 *
 * Port of Java app.freerouting.drc.DesignRulesChecker. The ratsnest /
 * incomplete-connection state mirrors Java NetIncompletes[] and reuses
 * interactive::NetIncompletes (a separate module), because the layering rules
 * forbid board depending on drc.
 */
class DesignRulesChecker
{
public:
    DesignRulesChecker(board::RoutingBoard* p_board,
                       const settings::DesignRulesCheckerSettings& p_drc_settings);

    /**
     * Collects all clearance violations on the board. Mirrors Java
     * DesignRulesChecker.getAllClearanceViolations (item.clearanceViolations
     * is ported here as a static helper, because board must not depend on drc).
     */
    std::vector<std::shared_ptr<ClearanceViolation>> get_all_clearance_violations();

    /**
     * Collects all unconnected items on the board.
     */
    std::vector<UnconnectedItems> get_all_unconnected_items();

    /**
     * Checks for copper pour (conduction area) fragmentation. Mirrors Java
     * getZoneIslandViolations. NOTE: the C++ Area model stores a single border
     * per conduction area, so the island decomposition returns at most one
     * island per area (the common Java case reports no violations either);
     * exotic multi-contour fills are not decomposed.
     */
    std::vector<ZoneIslandViolation> get_zone_island_violations();

    /**
     * Generates a DRC report in KiCad JSON schema format.
     */
    std::unique_ptr<DrcReport> generate_report(const std::string& p_source_file,
                                               const std::string& p_coordinate_unit);

    /**
     * Generates a JSON string of the DRC report. Mirrors Java generateReportJson.
     */
    std::string generate_report_json(const std::string& p_source_file,
                                     const std::string& p_coordinate_unit);

    /**
     * Generates a concise diagnostic summary of DRC violations with component
     * context, spatial congestion clustering and auto-correction hints.
     * Mirrors Java DesignRulesChecker.generateSummary.
     */
    DrcSummaryResponse generate_summary();

    // ==================== Incompletes / ratsnest state ====================

    /**
     * Initializes the incomplete connection calculations for all nets.
     * Mirrors Java DesignRulesChecker.calculateAllIncompletes.
     */
    void calculate_all_incompletes();

    /**
     * Recalculates the incomplete connections for the specified net.
     */
    void recalculate_net_incompletes(int p_net_no);

    /** Returns the total number of incomplete connections across all nets. */
    int get_incomplete_count();

    /** Returns the number of incomplete connections for a specific net. */
    int get_incomplete_count(int p_net_no);

    /** Returns the total number of nets that violate length restrictions. */
    int get_length_violation_count();

    /** Returns the magnitude of the length violation for the specified net. */
    double get_length_violation(int p_net_no);

    /**
     * Recalculates length matching violations for all nets. Returns true if
     * the status of any length violation has changed.
     */
    bool recalculate_length_violations();

    /**
     * Retrieves all airlines (incomplete connections) for the entire board.
     * The Java drc.AirLine is mapped to interactive::RatsNest::AirLine.
     */
    std::vector<std::shared_ptr<interactive::RatsNest::AirLine>> get_all_airlines();

    /**
     * Gets the NetIncompletes object for a specific net, or nullptr.
     */
    interactive::NetIncompletes* get_net_incompletes(int p_net_no);

    /**
     * Total number of connections that would complete all nets (minimum
     * spanning tree count). Computed by calculate_all_incompletes.
     */
    int max_connections = 0;

private:
    /**
     * Calculates all clearance violations of p_item against other items on the
     * board. Mirrors Java Item.clearanceViolations.
     */
    static std::vector<std::shared_ptr<ClearanceViolation>> calculate_clearance_violations_for_item(
        board::Item* p_item, board::RoutingBoard& p_board);

    /**
     * Finds a representative item from a connected set, preferring Pins over
     * other items. Mirrors Java findRepresentativeItem.
     */
    static board::Item* find_representative_item(const std::vector<board::Item*>& p_connected_set);

    /**
     * Converts an internal ClearanceViolation to a DrcViolation for the report.
     */
    std::shared_ptr<DrcViolation> convert_to_drc_violation(const ClearanceViolation* p_violation,
                                                           const std::string& p_coordinate_unit);

    /**
     * Converts internal UnconnectedItems to a DrcViolation for the report.
     */
    std::shared_ptr<DrcViolation> convert_to_drc_violation(const UnconnectedItems* p_unconnected_item,
                                                           const std::string& p_coordinate_unit);

    /**
     * Converts a ZoneIslandViolation to a DrcViolation for the report.
     */
    std::shared_ptr<DrcViolation> convert_to_drc_violation(const ZoneIslandViolation* p_zone_violation,
                                                           const std::string& p_coordinate_unit);

    /**
     * Returns the net name for p_net_no, or a fallback if unknown.
     */
    std::string get_net_name(int p_net_no) const;

    /**
     * Mirrors the Java isHole helper: Via and Pin items are classified as
     * holeClearance in the report.
     */
    static bool is_hole(const board::Item* p_item);

    /**
     * Gets a human-readable description of an item. Mirrors Java
     * getItemDescription.
     */
    std::string get_item_description(const board::Item* p_item) const;

    /**
     * Gets a detailed description of a trace including net, layer and length
     * in the target coordinate unit.
     */
    std::string get_detailed_trace_description(const board::Item* p_item,
                                               const std::string& p_coordinate_unit) const;

    /**
     * Formats an item with component/pin context, mirroring Java
     * formatItemWithContext. Core Package::Pin has no name field, so pins
     * fall back to their number (documented deviation).
     */
    std::string format_item_with_context(const board::Item* p_item) const;

    /**
     * Converts a coordinate value from the board's internal coordinate system
     * to the specified unit. Mirrors Java convertCoordinate.
     */
    double convert_coordinate(double p_board_coordinate, const std::string& p_coordinate_unit) const;

    /**
     * Returns the board's communication resolution expressed in mm
     * (= UnitHelper::scale(resolution, unit, MM)), mirroring Java
     * board.communication.getResolution(Unit.MM).
     */
    double get_resolution_mm() const;

    /**
     * Clusters the centres of gravity of the clearance violations within a
     * 5.0 mm neighbourhood. Mirrors Java detectCongestionZones.
     */
    std::vector<CongestionZone> detect_congestion_zones(
        const std::vector<std::shared_ptr<ClearanceViolation>>& p_violations);

    /**
     * Mirrors Java generateActionableHints.
     */
    std::vector<std::string> generate_actionable_hints(
        const std::vector<std::shared_ptr<ClearanceViolation>>& p_violations,
        const std::vector<UnconnectedItems>& p_unconnecteds,
        const std::vector<CongestionZone>& p_zones);

    board::RoutingBoard* routing_board_;
    settings::DesignRulesCheckerSettings drc_settings_;

    std::vector<std::shared_ptr<interactive::NetIncompletes>> net_incompletes_;
};

} // namespace drc
} // namespace freerouting