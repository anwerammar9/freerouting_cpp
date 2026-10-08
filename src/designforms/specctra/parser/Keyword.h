#pragma once

#include <string>

namespace freerouting::designforms::specctra::parser {

class ScopeKeyword;

/**
 * Keywords of the Specctra DSN file format. Port of Java io.specctra.parser.Keyword.
 * The instances are unique, so keywords are compared by address like in Java.
 */
class Keyword
{
public:
    virtual ~Keyword() = default;

    /** The name of this keyword (used for debugging). Java Keyword.getName. */
    const std::string& get_name() const { return name_; }

    static const Keyword* const ABSOLUTE;
    static const Keyword* const ACTIVE;
    static const Keyword* const AGAINST_PREFERRED_DIRECTION_TRACE_COSTS;
    static const Keyword* const ATTACH;
    static const Keyword* const AUTOROUTE;
    static const Keyword* const AUTOROUTE_SETTINGS;
    static const Keyword* const BACK;
    static const Keyword* const BOUNDARY;
    static const Keyword* const CIRCUIT;
    static const Keyword* const CIRCLE;
    static const Keyword* const CLASS;
    static const Keyword* const CLASS_CLASS;
    static const Keyword* const CLASSES;
    static const ScopeKeyword* const COMPONENT_SCOPE;
    static const Keyword* const CONSTANT;
    static const Keyword* const CONTROL;
    static const Keyword* const CLEARANCE;
    static const Keyword* const CLEARANCE_CLASS;
    static const Keyword* const CLOSED_BRACKET;
    static const Keyword* const FANOUT;
    static const Keyword* const FLIP_STYLE;
    static const Keyword* const FIX;
    static const Keyword* const FORTYFIVE_DEGREE;
    static const Keyword* const FROMTO;
    static const Keyword* const FRONT;
    static const Keyword* const GENERATED_BY_FREEROUTING;
    static const Keyword* const HORIZONTAL;
    static const Keyword* const HOST_CAD;
    static const Keyword* const HOST_VERSION;
    static const Keyword* const IMAGE;
    static const Keyword* const JUMPER;
    static const Keyword* const KEEPOUT;
    static const Keyword* const LAYER;
    static const Keyword* const LAYER_RULE;
    static const Keyword* const LENGTH;
    static const ScopeKeyword* const LIBRARY_SCOPE;
    static const Keyword* const LOCK_TYPE;
    static const Keyword* const LOGICAL_PART;
    static const Keyword* const LOGICAL_PART_MAPPING;
    static const Keyword* const NET;
    static const Keyword* const NETWORK_OUT;
    static const ScopeKeyword* const NETWORK_SCOPE;
    static const Keyword* const NINETY_DEGREE;
    static const Keyword* const NONE;
    static const Keyword* const NORMAL;
    static const Keyword* const OFF;
    static const Keyword* const ON;
    static const Keyword* const OPEN_BRACKET;
    static const Keyword* const ORDER;
    static const Keyword* const OUTLINE;
    static const Keyword* const PADSTACK;
    static const ScopeKeyword* const PART_LIBRARY_SCOPE;
    static const ScopeKeyword* const PARSER_SCOPE;
    static const ScopeKeyword* const PCB_SCOPE;
    static const Keyword* const PIN;
    static const Keyword* const PINS;
    static const Keyword* const PLACE;
    static const ScopeKeyword* const PLACE_CONTROL;
    static const Keyword* const PLACE_KEEPOUT;
    static const ScopeKeyword* const PLACEMENT_SCOPE;
    static const ScopeKeyword* const PLANE_SCOPE;
    static const Keyword* const PLANE_VIA_COSTS;
    static const Keyword* const PREFERRED_DIRECTION;
    static const Keyword* const PREFERRED_DIRECTION_TRACE_COSTS;
    static const Keyword* const SNAP_ANGLE;
    static const Keyword* const POLYGON;
    static const Keyword* const PN;
    static const Keyword* const POLYGON_PATH;
    static const Keyword* const POLYLINE_PATH;
    static const Keyword* const POSITION;
    static const Keyword* const POSTROUTE;
    static const Keyword* const POWER;
    static const Keyword* const PULL_TIGHT;
    static const Keyword* const RECTANGLE;
    static const ScopeKeyword* const RESOLUTION_SCOPE;
    static const Keyword* const ROTATE;
    static const Keyword* const ROTATE_FIRST;
    static const Keyword* const ROUTES;
    static const Keyword* const RULE;
    static const Keyword* const RULES;
    static const Keyword* const SESSION;
    static const Keyword* const SHAPE;
    static const Keyword* const SHOVE_FIXED;
    static const Keyword* const SIDE;
    static const Keyword* const SIGNAL;
    static const Keyword* const SPARE;
    static const Keyword* const START_PASS_NO;
    static const Keyword* const START_RIPUP_COSTS;
    static const Keyword* const STRING_QUOTE;
    static const ScopeKeyword* const STRUCTURE_SCOPE;
    static const Keyword* const TYPE;
    static const Keyword* const USE_LAYER;
    static const Keyword* const USE_NET;
    static const Keyword* const USE_VIA;
    static const Keyword* const VERTICAL;
    static const Keyword* const VIA;
    static const Keyword* const VIAS;
    static const Keyword* const VIA_AT_SMD;
    static const Keyword* const VIA_COSTS;
    static const Keyword* const VIA_KEEPOUT;
    static const Keyword* const VIA_RULE;
    static const Keyword* const WIDTH;
    static const Keyword* const WINDOW;
    static const Keyword* const WIRE;
    static const ScopeKeyword* const WIRING_SCOPE;
    static const Keyword* const WRITE_RESOLUTION;

protected:
    explicit Keyword(std::string p_name) : name_(std::move(p_name)) {}

private:
    std::string name_;
};

} // namespace freerouting::designforms::specctra::parser
