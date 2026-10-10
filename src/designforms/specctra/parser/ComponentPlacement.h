#pragma once

#include <array>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace freerouting::designforms::specctra::parser {

/**
 * The placements of the components of one library image, read from the placement scope.
 * Port of Java parser.ComponentPlacement.
 */
class ComponentPlacement
{
public:
    /** Java ComponentPlacement.ItemClearanceInfo. */
    struct ItemClearanceInfo
    {
        std::string name;
        std::string clearance_class;
    };

    /** Java ComponentPlacement.ComponentLocation. */
    struct ComponentLocation
    {
        std::string name;
        /** nullopt (Java null) if the component is not placed yet. */
        std::optional<std::array<double, 2>> coor;
        bool is_front = true;
        double rotation = 0;
        bool position_fixed = false;
        // Java TreeMaps keyed by item name
        std::map<std::string, ItemClearanceInfo> pin_infos;
        std::map<std::string, ItemClearanceInfo> keepout_infos;
        std::map<std::string, ItemClearanceInfo> via_keepout_infos;
        std::map<std::string, ItemClearanceInfo> place_keepout_infos;
        /** Java partNumber; empty = Java null. */
        std::string part_number;
    };

    explicit ComponentPlacement(std::string p_lib_name) : lib_name(std::move(p_lib_name)) {}

    /** The name of the library image of the components. Java libName. */
    const std::string lib_name;
    std::vector<ComponentLocation> locations;
};

} // namespace freerouting::designforms::specctra::parser
