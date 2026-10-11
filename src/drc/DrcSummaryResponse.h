#pragma once

#include <optional>
#include <string>
#include <vector>

namespace freerouting {
namespace drc {

/**
 * High-level diagnostic DRC summary designed for fast LLM/agent comprehension
 * and concise API reporting. Mirrors Java app.freerouting.drc.DrcSummaryResponse.
 */
class DiagnosticViolation
{
public:
    DiagnosticViolation() = default;

    DiagnosticViolation(std::string p_type,
                        std::string p_severity,
                        std::string p_layer,
                        std::string p_explanation,
                        std::optional<double> p_expected_clearance_mm,
                        std::optional<double> p_actual_clearance_mm,
                        std::optional<double> p_shortfall_mm,
                        std::vector<std::string> p_items)
        : type(std::move(p_type))
        , severity(std::move(p_severity))
        , layer(std::move(p_layer))
        , explanation(std::move(p_explanation))
        , expected_clearance_mm(p_expected_clearance_mm)
        , actual_clearance_mm(p_actual_clearance_mm)
        , shortfall_mm(p_shortfall_mm)
        , items(std::move(p_items))
    {
    }

    std::string type;
    std::string severity;
    std::string layer;
    std::string explanation;
    std::optional<double> expected_clearance_mm;
    std::optional<double> actual_clearance_mm;
    std::optional<double> shortfall_mm;
    std::vector<std::string> items;

    void to_json_string(std::string& o_json) const;
};

/** Represents a spatial congestion cluster where multiple violations occur near each other. */
class CongestionZone
{
public:
    CongestionZone() = default;

    CongestionZone(double p_center_x_mm,
                   double p_center_y_mm,
                   double p_radius_mm,
                   int p_violation_count,
                   std::string p_description)
        : center_x_mm(p_center_x_mm)
        , center_y_mm(p_center_y_mm)
        , radius_mm(p_radius_mm)
        , violation_count(p_violation_count)
        , description(std::move(p_description))
    {
    }

    double center_x_mm = 0.0;
    double center_y_mm = 0.0;
    double radius_mm = 0.0;
    int violation_count = 0;
    std::string description;

    void to_json_string(std::string& o_json) const;
};

/** The DRC summary response: aggregated counts, violations, congestion zones and hints. */
class DrcSummaryResponse
{
public:
    int clearance_violations_count = 0;
    int unconnected_nets_count = 0;
    std::vector<DiagnosticViolation> violations;
    std::vector<CongestionZone> congestion_zones;
    std::vector<std::string> hints;

    /** Serializes this summary to a JSON object (snake_case keys, Java parity). */
    void to_json_string(std::string& o_json) const;
    std::string to_json() const;
};

} // namespace drc
} // namespace freerouting