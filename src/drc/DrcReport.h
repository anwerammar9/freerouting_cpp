#pragma once

#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <iomanip>
#include <sstream>

#include "drc/DrcViolation.h"

namespace freerouting {
namespace drc {

/**
 * Represents a complete DRC report in KiCad's JSON schema format.
 * Based on https://schemas.kicad.org/drc.v1.json
 */
class DrcReport
{
public:
    /**
     * JSON schema URL
     */
    static constexpr const char* SCHEMA = "https://schemas.kicad.org/drc.v1.json";

    /**
     * Coordinate units used in the report (e.g., "mm", "mil")
     */
    std::string coordinate_units;

    /**
     * Date and time when the report was generated
     */
    std::string date;

    /**
     * Version of KiCad that generated the report (this is "N/A" for Freerouting)
     */
    static constexpr const char* KICAD_VERSION = "N/A";

    /**
     * Version of Freerouting that generated the report
     */
    std::string freerouting_version;

    /**
     * Source file name
     */
    std::string source;

    /**
     * List of unconnected items
     */
    std::vector<std::shared_ptr<DrcViolation>> unconnected_items;

    /**
     * List of violations found
     */
    std::vector<std::shared_ptr<DrcViolation>> violations;

    /**
     * Schematic parity issues (empty for now)
     */
    std::vector<std::shared_ptr<void>> schematic_parity;

    DrcReport(const std::string& p_coordinate_units,
             const std::string& p_source,
             const std::string& p_version);

    /**
     * Add a violation to the report
     */
    void add_violation(const std::shared_ptr<DrcViolation>& p_violation);

    /**
     * Add an unconnected item to the report
     */
    void add_unconnected_item(const std::shared_ptr<DrcViolation>& p_item);

private:
    /**
     * Generate current date/time in ISO 8601 format
     */
    static std::string get_current_datetime();
};

} // namespace drc
} // namespace freerouting
