#pragma once

#include <memory>
#include <vector>
#include <string>
#include <unordered_set>

#include "board/BasicBoard.h"
#include "board/Item.h"
#include "board/Via.h"
#include "board/Connectable.h"
#include "board/RoutingBoard.h"
#include "board/Pin.h"
#include "settings/ProfileSettings.h"
#include "drc/ClearanceViolation.h"
#include "drc/UnconnectedItems.h"
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
 */
class DesignRulesChecker
{
public:
    DesignRulesChecker(board::RoutingBoard* p_board,
                      const settings::DesignRulesCheckerSettings& p_drc_settings);

    /**
     * Collects all clearance violations on the board.
     *
     * @return Collection of all clearance violations found
     */
    std::vector<ClearanceViolation*> get_all_clearance_violations();

    /**
     * Collects all unconnected items on the board.
     *
     * @return Collection of all unconnected items found
     */
    std::vector<UnconnectedItems*> get_all_unconnected_items();

    /**
     * Generates a DRC report in KiCad JSON format.
     *
     * @param p_source_file Name of the source file
     * @param p_coordinate_unit Unit for coordinates (e.g., "mm", "mil")
     * @return DRC report in KiCad JSON format
     */
    std::unique_ptr<DrcReport> generate_report(const std::string& p_source_file,
                                               const std::string& p_coordinate_unit);

private:
    board::RoutingBoard* routing_board_;
    settings::DesignRulesCheckerSettings drc_settings_;

    /**
     * Finds a representative item from a connected set, preferring Pins over other
     * items.
     *
     * @param p_connected_set The set of connected items
     * @return A representative item, or null if the set is empty
     */
    board::Item* find_representative_item(const std::vector<board::Item*>& p_connected_set);

    /**
     * Converts an internal ClearanceViolation to a DrcViolation for the report.
     *
     * @param p_violation The internal violation to convert
     * @param p_coordinate_unit Unit for coordinates
     * @return DRC violation for the report
     */
    std::shared_ptr<DrcViolation> convert_to_drc_violation(const ClearanceViolation* p_violation,
                                                           const std::string& p_coordinate_unit);

    /**
     * Converts an internal UnconnectedItems to a DrcViolation for the report.
     *
     * @param p_unconnected_item The internal unconnected items to convert
     * @param p_coordinate_unit Unit for coordinates
     * @return DRC violation for the report
     */
    std::shared_ptr<DrcViolation> convert_to_drc_violation(const UnconnectedItems* p_unconnected_item,
                                                           const std::string& p_coordinate_unit);
};

} // namespace drc
} // namespace freerouting
