#pragma once

#include <string>
#include <vector>
#include <memory>

#include "drc/DrcViolationItem.h"

namespace freerouting {
namespace drc {

/**
 * Represents a single DRC violation, matching KiCad's JSON schema.
 */
class DrcViolation
{
public:
    /**
     * Human-readable description of the violation
     */
    std::string description;

    /**
     * Items involved in the violation
     */
    std::vector<std::shared_ptr<DrcViolationItem>> items;

    /**
     * Severity of the violation ("error", "warning", "ignore")
     */
    std::string severity;

    /**
     * Type of violation (e.g., "clearance", "via_dangling", etc.)
     * "unconnected_items" - This entry represents a specific Net that is not fully continuous.
     * "clearance" - This occurs when two pieces of copper from different nets are closer than the minimum safety distance.
     * "track_dangling" - A segment of copper track that is connected at one end but stops in the middle of nowhere at the other end.
     * "via_dangling" - A via that doesn't actually connect two valid layers.
     * "hole_clearance" - This is a violation of the distance between the edge of a drilled hole and nearby copper of a different net.
     */
    std::string type;

    DrcViolation(const std::string& p_type,
                const std::string& p_description,
                const std::string& p_severity,
                const std::vector<std::shared_ptr<DrcViolationItem>>& p_items)
        : description(p_description)
        , items(p_items)
        , severity(p_severity)
        , type(p_type)
    {
    }

    /**
     * Serializes this violation to JSON (description, items, severity, type).
     */
    std::string to_json() const;
};

} // namespace drc
} // namespace freerouting
