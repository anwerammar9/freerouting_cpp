#pragma once

#include <string>
#include <memory>

#include "drc/DrcPosition.h"

namespace freerouting {
namespace drc {

/**
 * Represents a single item involved in a DRC violation, matching KiCad's JSON schema.
 */
class DrcViolationItem
{
public:
    /**
     * Human-readable description of the item
     */
    std::string description;

    /**
     * Position of the item
     */
    DrcPosition pos;

    /**
     * Unique identifier of the item
     */
    std::string uuid;

    DrcViolationItem(const std::string& p_description,
                    const DrcPosition& p_pos,
                    const std::string& p_uuid)
        : description(p_description)
        , pos(p_pos)
        , uuid(p_uuid)
    {
    }

    /**
     * Serializes this item to JSON (description, pos {x, y}, uuid).
     */
    std::string to_json() const;
};

} // namespace drc
} // namespace freerouting
