#include "drc/DrcReport.h"
#include "constants/Constants.h"
#include "drc/drc_json_internal.h"

#include <chrono>
#include <iomanip>
#include <sstream>

namespace freerouting {
namespace drc {

namespace detail = freerouting::drc::detail;

std::string DrcReport::get_current_datetime()
{
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}

std::string DrcViolationItem::to_json() const
{
    std::string json;
    json.push_back('{');
    detail::json_append_string(json, "description");
    json += ":";
    detail::json_append_string(json, description);
    json += ",";
    detail::json_append_string(json, "pos");
    json += ":";
    json.push_back('{');
    detail::json_append_string(json, "x");
    json += ":";
    detail::json_append_double(json, pos.x);
    json += ",";
    detail::json_append_string(json, "y");
    json += ":";
    detail::json_append_double(json, pos.y);
    json += "},";
    detail::json_append_string(json, "uuid");
    json += ":";
    detail::json_append_string(json, uuid);
    json += "}";
    return json;
}

std::string DrcViolation::to_json() const
{
    std::string json;
    json.push_back('{');
    detail::json_append_string(json, "description");
    json += ":";
    detail::json_append_string(json, description);
    json += ",";
    detail::json_append_string(json, "items");
    json += ":[";
    for (size_t i = 0; i < items.size(); ++i)
    {
        if (i > 0)
        {
            json += ",";
        }
        json += items[i]->to_json();
    }
    json += "],";
    detail::json_append_string(json, "severity");
    json += ":";
    detail::json_append_string(json, severity);
    json += ",";
    detail::json_append_string(json, "type");
    json += ":";
    detail::json_append_string(json, type);
    json += "}";
    return json;
}

std::string DrcReport::to_json() const
{
    std::string json;
    json.push_back('{');
    detail::json_append_string(json, "$schema");
    json += ":";
    detail::json_append_string(json, SCHEMA);
    json += ",";
    detail::json_append_string(json, "coordinateUnits");
    json += ":";
    detail::json_append_string(json, coordinate_units);
    json += ",";
    detail::json_append_string(json, "date");
    json += ":";
    detail::json_append_string(json, date);
    json += ",";
    detail::json_append_string(json, "kicadVersion");
    json += ":";
    detail::json_append_string(json, KICAD_VERSION);
    json += ",";
    detail::json_append_string(json, "freeroutingVersion");
    json += ":";
    detail::json_append_string(json, freerouting_version);
    json += ",";
    detail::json_append_string(json, "source");
    json += ":";
    detail::json_append_string(json, source);
    json += ",";
    detail::json_append_string(json, "unconnectedItems");
    json += ":[";
    for (size_t i = 0; i < unconnected_items.size(); ++i)
    {
        if (i > 0)
        {
            json += ",";
        }
        json += unconnected_items[i]->to_json();
    }
    json += "],";
    detail::json_append_string(json, "violations");
    json += ":[";
    for (size_t i = 0; i < violations.size(); ++i)
    {
        if (i > 0)
        {
            json += ",";
        }
        json += violations[i]->to_json();
    }
    json += "],";
    detail::json_append_string(json, "schematicParity");
    json += ":[]";
    if (quality_score.has_value())
    {
        json += ",";
        detail::json_append_string(json, "qualityScore");
        json += ":";
        detail::json_append_double(json, *quality_score);
    }
    if (optimizer_score.has_value())
    {
        json += ",";
        detail::json_append_string(json, "optimizerScore");
        json += ":";
        detail::json_append_double(json, *optimizer_score);
    }
    json += "}";
    return json;
}

DrcReport::DrcReport(const std::string& p_coordinate_units,
                     const std::string& p_source,
                     const std::string& p_version)
    : coordinate_units(p_coordinate_units)
    , date(get_current_datetime())
    , freerouting_version(p_version)
    , source(p_source)
{
}

void DrcReport::add_violation(const std::shared_ptr<DrcViolation>& p_violation)
{
    violations.push_back(p_violation);
}

void DrcReport::add_unconnected_item(const std::shared_ptr<DrcViolation>& p_item)
{
    unconnected_items.push_back(p_item);
}

} // namespace drc
} // namespace freerouting
