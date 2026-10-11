#include "drc/DrcSummaryResponse.h"

#include "drc/drc_json_internal.h"

namespace freerouting {
namespace drc {

namespace detail = freerouting::drc::detail;

void DiagnosticViolation::to_json_string(std::string& o_json) const
{
    o_json.push_back('{');
    detail::json_append_string(o_json, "type");
    o_json += ":";
    detail::json_append_string(o_json, type);
    o_json += ",";
    detail::json_append_string(o_json, "severity");
    o_json += ":";
    detail::json_append_string(o_json, severity);
    o_json += ",";
    detail::json_append_string(o_json, "layer");
    o_json += ":";
    detail::json_append_string(o_json, layer);
    o_json += ",";
    detail::json_append_string(o_json, "explanation");
    o_json += ":";
    detail::json_append_string(o_json, explanation);
    o_json += ",";
    detail::json_append_string(o_json, "expected_clearance_mm");
    o_json += ":";
    if (expected_clearance_mm.has_value())
    {
        detail::json_append_double(o_json, *expected_clearance_mm);
    }
    else
    {
        o_json += "null";
    }
    o_json += ",";
    detail::json_append_string(o_json, "actual_clearance_mm");
    o_json += ":";
    if (actual_clearance_mm.has_value())
    {
        detail::json_append_double(o_json, *actual_clearance_mm);
    }
    else
    {
        o_json += "null";
    }
    o_json += ",";
    detail::json_append_string(o_json, "shortfall_mm");
    o_json += ":";
    if (shortfall_mm.has_value())
    {
        detail::json_append_double(o_json, *shortfall_mm);
    }
    else
    {
        o_json += "null";
    }
    o_json += ",";
    detail::json_append_string(o_json, "items");
    o_json += ":[";
    for (size_t i = 0; i < items.size(); ++i)
    {
        if (i > 0)
        {
            o_json += ",";
        }
        detail::json_append_string(o_json, items[i]);
    }
    o_json += "]}";
}

void CongestionZone::to_json_string(std::string& o_json) const
{
    o_json.push_back('{');
    detail::json_append_string(o_json, "center_x_mm");
    o_json += ":";
    detail::json_append_double(o_json, center_x_mm);
    o_json += ",";
    detail::json_append_string(o_json, "center_y_mm");
    o_json += ":";
    detail::json_append_double(o_json, center_y_mm);
    o_json += ",";
    detail::json_append_string(o_json, "radius_mm");
    o_json += ":";
    detail::json_append_double(o_json, radius_mm);
    o_json += ",";
    detail::json_append_string(o_json, "violation_count");
    o_json += ":";
    o_json += std::to_string(violation_count);
    o_json += ",";
    detail::json_append_string(o_json, "description");
    o_json += ":";
    detail::json_append_string(o_json, description);
    o_json += "}";
}

void DrcSummaryResponse::to_json_string(std::string& o_json) const
{
    o_json.push_back('{');
    detail::json_append_string(o_json, "clearance_violations_count");
    o_json += ":";
    o_json += std::to_string(clearance_violations_count);
    o_json += ",";
    detail::json_append_string(o_json, "unconnected_nets_count");
    o_json += ":";
    o_json += std::to_string(unconnected_nets_count);
    o_json += ",";
    detail::json_append_string(o_json, "violations");
    o_json += ":[";
    for (size_t i = 0; i < violations.size(); ++i)
    {
        if (i > 0)
        {
            o_json += ",";
        }
        violations[i].to_json_string(o_json);
    }
    o_json += "],";
    detail::json_append_string(o_json, "congestion_zones");
    o_json += ":[";
    for (size_t i = 0; i < congestion_zones.size(); ++i)
    {
        if (i > 0)
        {
            o_json += ",";
        }
        congestion_zones[i].to_json_string(o_json);
    }
    o_json += "],";
    detail::json_append_string(o_json, "hints");
    o_json += ":[";
    for (size_t i = 0; i < hints.size(); ++i)
    {
        if (i > 0)
        {
            o_json += ",";
        }
        detail::json_append_string(o_json, hints[i]);
    }
    o_json += "]}";
}

std::string DrcSummaryResponse::to_json() const
{
    std::string result;
    to_json_string(result);
    return result;
}

} // namespace drc
} // namespace freerouting