#pragma once

#include <array>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "designforms/specctra/parser/Shape.h"

namespace freerouting::designforms::specctra::parser {

class IJFlexScanner;
class LayerStructure;

/** A component image as read from the library scope. Port of the read part of Java parser.Package. */
class Package
{
public:
    /** Java Package.PinInfo. */
    struct PinInfo
    {
        std::string padstack_name;
        std::string pin_name;
        std::array<double, 2> rel_coor{};
        double rotation = 0;
    };

    std::string name;
    std::vector<PinInfo> pin_info_arr;
    std::vector<std::unique_ptr<Shape>> outline;
    std::vector<Shape::ReadAreaScopeResult> keepouts;
    std::vector<Shape::ReadAreaScopeResult> via_keepouts;
    std::vector<Shape::ReadAreaScopeResult> place_keepouts;
    /** If false, the package is placed on the back side of the board. */
    bool is_front = true;

    /** Reads an (image <name> ...) scope; nullopt (Java null) on error. Java Package.readScope. */
    static std::optional<Package> read_scope(IJFlexScanner& p_scanner, const LayerStructure& p_layer_structure);

private:
    static std::optional<PinInfo> read_pin_info(IJFlexScanner& p_scanner);
    static double read_rotation(IJFlexScanner& p_scanner);
    static bool read_placement_side(IJFlexScanner& p_scanner);
};

} // namespace freerouting::designforms::specctra::parser
