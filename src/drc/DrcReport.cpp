#include "drc/DrcReport.h"
#include "constants/Constants.h"

#include <chrono>
#include <iomanip>
#include <sstream>
#include <iomanip>

namespace freerouting {
namespace drc {

std::string DrcReport::get_current_datetime()
{
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
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
