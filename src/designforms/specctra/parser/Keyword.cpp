#include "designforms/specctra/parser/Keyword.h"

#include "designforms/specctra/parser/Component.h"
#include "designforms/specctra/parser/Library.h"
#include "designforms/specctra/parser/PartLibrary.h"
#include "designforms/specctra/parser/PlaceControl.h"
#include "designforms/specctra/parser/Plane.h"
#include "designforms/specctra/parser/Resolution.h"
#include "designforms/specctra/parser/ScopeKeyword.h"
#include "designforms/specctra/parser/Structure.h"

namespace freerouting::designforms::specctra::parser {

namespace {

// Keyword is constructed only here, like the Java private-constructor enumeration.
struct KeywordInstance : Keyword
{
    explicit KeywordInstance(std::string p_name) : Keyword(std::move(p_name)) {}
};

const KeywordInstance k_ABSOLUTE("absolute");
const KeywordInstance k_ACTIVE("active");
const KeywordInstance k_AGAINST_PREFERRED_DIRECTION_TRACE_COSTS("against_preferred_direction_trace_costs");
const KeywordInstance k_ATTACH("attach");
const KeywordInstance k_AUTOROUTE("autoroute");
const KeywordInstance k_AUTOROUTE_SETTINGS("autoroute_settings");
const KeywordInstance k_BACK("back");
const KeywordInstance k_BOUNDARY("boundary");
const KeywordInstance k_CIRCUIT("circuit");
const KeywordInstance k_CIRCLE("circle");
const KeywordInstance k_CLASS("class");
const KeywordInstance k_CLASS_CLASS("class_class");
const KeywordInstance k_CLASSES("classes");
const Component k_COMPONENT_SCOPE; // Java: new Component()
const KeywordInstance k_CONSTANT("constant");
const KeywordInstance k_CONTROL("control");
const KeywordInstance k_CLEARANCE("clearance");
const KeywordInstance k_CLEARANCE_CLASS("clearance_class");
const KeywordInstance k_CLOSED_BRACKET(")");
const KeywordInstance k_FANOUT("fanout");
const KeywordInstance k_FLIP_STYLE("flip_style");
const KeywordInstance k_FIX("fix");
const KeywordInstance k_FORTYFIVE_DEGREE("fortyfive_degree");
const KeywordInstance k_FROMTO("fromto");
const KeywordInstance k_FRONT("front");
const KeywordInstance k_GENERATED_BY_FREEROUTING("generated_by_freerouting");
const KeywordInstance k_HORIZONTAL("horizontal");
const KeywordInstance k_HOST_CAD("host_cad");
const KeywordInstance k_HOST_VERSION("host_version");
const KeywordInstance k_IMAGE("image");
const KeywordInstance k_JUMPER("jumper");
const KeywordInstance k_KEEPOUT("keepout");
const KeywordInstance k_LAYER("layer");
const KeywordInstance k_LAYER_RULE("layer_rule");
const KeywordInstance k_LENGTH("length");
const Library k_LIBRARY_SCOPE; // Java: new Library()
const KeywordInstance k_LOCK_TYPE("lock_type");
const KeywordInstance k_LOGICAL_PART("logical_part");
const KeywordInstance k_LOGICAL_PART_MAPPING("logical_part_mapping");
const KeywordInstance k_NET("net");
const KeywordInstance k_NETWORK_OUT("network_out");
const ScopeKeyword k_NETWORK_SCOPE("network"); // Java: new Network()
const KeywordInstance k_NINETY_DEGREE("ninety_degree");
const KeywordInstance k_NONE("none");
const KeywordInstance k_NORMAL("normal");
const KeywordInstance k_OFF("off");
const KeywordInstance k_ON("on");
const KeywordInstance k_OPEN_BRACKET("(");
const KeywordInstance k_ORDER("order");
const KeywordInstance k_OUTLINE("outline");
const KeywordInstance k_PADSTACK("padstack");
const PartLibrary k_PART_LIBRARY_SCOPE; // Java: new PartLibrary()
const ScopeKeyword k_PARSER_SCOPE("parser"); // Java: new Parser()
const ScopeKeyword k_PCB_SCOPE("pcb"); // Java: new ScopeKeyword
const KeywordInstance k_PIN("pin");
const KeywordInstance k_PINS("pins");
const KeywordInstance k_PLACE("place");
const PlaceControl k_PLACE_CONTROL; // Java: new PlaceControl()
const KeywordInstance k_PLACE_KEEPOUT("place_keepout");
const ScopeKeyword k_PLACEMENT_SCOPE("placement"); // Java: new Placement()
const Plane k_PLANE_SCOPE; // Java: new Plane()
const KeywordInstance k_PLANE_VIA_COSTS("plane_via_costs");
const KeywordInstance k_PREFERRED_DIRECTION("preferred_direction");
const KeywordInstance k_PREFERRED_DIRECTION_TRACE_COSTS("preferred_direction_trace_costs");
const KeywordInstance k_SNAP_ANGLE("snap_angle");
const KeywordInstance k_POLYGON("polygon");
const KeywordInstance k_PN("PN");
const KeywordInstance k_POLYGON_PATH("polygon_path");
const KeywordInstance k_POLYLINE_PATH("polyline_path");
const KeywordInstance k_POSITION("position");
const KeywordInstance k_POSTROUTE("postroute");
const KeywordInstance k_POWER("power");
const KeywordInstance k_PULL_TIGHT("pull_tight");
const KeywordInstance k_RECTANGLE("rectangle");
const Resolution k_RESOLUTION_SCOPE;
const KeywordInstance k_ROTATE("rotate");
const KeywordInstance k_ROTATE_FIRST("rotate_first");
const KeywordInstance k_ROUTES("routes");
const KeywordInstance k_RULE("rule");
const KeywordInstance k_RULES("rules");
const KeywordInstance k_SESSION("session");
const KeywordInstance k_SHAPE("shape");
const KeywordInstance k_SHOVE_FIXED("shove_fixed");
const KeywordInstance k_SIDE("side");
const KeywordInstance k_SIGNAL("signal");
const KeywordInstance k_SPARE("spare");
const KeywordInstance k_START_PASS_NO("start_pass_no");
const KeywordInstance k_START_RIPUP_COSTS("start_ripup_costs");
const KeywordInstance k_STRING_QUOTE("string_quote");
const Structure k_STRUCTURE_SCOPE; // Java: new Structure()
const KeywordInstance k_TYPE("type");
const KeywordInstance k_USE_LAYER("use_layer");
const KeywordInstance k_USE_NET("use_net");
const KeywordInstance k_USE_VIA("use_via");
const KeywordInstance k_VERTICAL("vertical");
const KeywordInstance k_VIA("via");
const KeywordInstance k_VIAS("vias");
const KeywordInstance k_VIA_AT_SMD("via_at_smd");
const KeywordInstance k_VIA_COSTS("via_costs");
const KeywordInstance k_VIA_KEEPOUT("via_keepout");
const KeywordInstance k_VIA_RULE("via_rule");
const KeywordInstance k_WIDTH("width");
const KeywordInstance k_WINDOW("window");
const KeywordInstance k_WIRE("wire");
const ScopeKeyword k_WIRING_SCOPE("wiring"); // Java: new Wiring()
const KeywordInstance k_WRITE_RESOLUTION("write_resolution");

} // namespace

const Keyword* const Keyword::ABSOLUTE = &k_ABSOLUTE;
const Keyword* const Keyword::ACTIVE = &k_ACTIVE;
const Keyword* const Keyword::AGAINST_PREFERRED_DIRECTION_TRACE_COSTS = &k_AGAINST_PREFERRED_DIRECTION_TRACE_COSTS;
const Keyword* const Keyword::ATTACH = &k_ATTACH;
const Keyword* const Keyword::AUTOROUTE = &k_AUTOROUTE;
const Keyword* const Keyword::AUTOROUTE_SETTINGS = &k_AUTOROUTE_SETTINGS;
const Keyword* const Keyword::BACK = &k_BACK;
const Keyword* const Keyword::BOUNDARY = &k_BOUNDARY;
const Keyword* const Keyword::CIRCUIT = &k_CIRCUIT;
const Keyword* const Keyword::CIRCLE = &k_CIRCLE;
const Keyword* const Keyword::CLASS = &k_CLASS;
const Keyword* const Keyword::CLASS_CLASS = &k_CLASS_CLASS;
const Keyword* const Keyword::CLASSES = &k_CLASSES;
const ScopeKeyword* const Keyword::COMPONENT_SCOPE = &k_COMPONENT_SCOPE;
const Keyword* const Keyword::CONSTANT = &k_CONSTANT;
const Keyword* const Keyword::CONTROL = &k_CONTROL;
const Keyword* const Keyword::CLEARANCE = &k_CLEARANCE;
const Keyword* const Keyword::CLEARANCE_CLASS = &k_CLEARANCE_CLASS;
const Keyword* const Keyword::CLOSED_BRACKET = &k_CLOSED_BRACKET;
const Keyword* const Keyword::FANOUT = &k_FANOUT;
const Keyword* const Keyword::FLIP_STYLE = &k_FLIP_STYLE;
const Keyword* const Keyword::FIX = &k_FIX;
const Keyword* const Keyword::FORTYFIVE_DEGREE = &k_FORTYFIVE_DEGREE;
const Keyword* const Keyword::FROMTO = &k_FROMTO;
const Keyword* const Keyword::FRONT = &k_FRONT;
const Keyword* const Keyword::GENERATED_BY_FREEROUTING = &k_GENERATED_BY_FREEROUTING;
const Keyword* const Keyword::HORIZONTAL = &k_HORIZONTAL;
const Keyword* const Keyword::HOST_CAD = &k_HOST_CAD;
const Keyword* const Keyword::HOST_VERSION = &k_HOST_VERSION;
const Keyword* const Keyword::IMAGE = &k_IMAGE;
const Keyword* const Keyword::JUMPER = &k_JUMPER;
const Keyword* const Keyword::KEEPOUT = &k_KEEPOUT;
const Keyword* const Keyword::LAYER = &k_LAYER;
const Keyword* const Keyword::LAYER_RULE = &k_LAYER_RULE;
const Keyword* const Keyword::LENGTH = &k_LENGTH;
const ScopeKeyword* const Keyword::LIBRARY_SCOPE = &k_LIBRARY_SCOPE;
const Keyword* const Keyword::LOCK_TYPE = &k_LOCK_TYPE;
const Keyword* const Keyword::LOGICAL_PART = &k_LOGICAL_PART;
const Keyword* const Keyword::LOGICAL_PART_MAPPING = &k_LOGICAL_PART_MAPPING;
const Keyword* const Keyword::NET = &k_NET;
const Keyword* const Keyword::NETWORK_OUT = &k_NETWORK_OUT;
const ScopeKeyword* const Keyword::NETWORK_SCOPE = &k_NETWORK_SCOPE;
const Keyword* const Keyword::NINETY_DEGREE = &k_NINETY_DEGREE;
const Keyword* const Keyword::NONE = &k_NONE;
const Keyword* const Keyword::NORMAL = &k_NORMAL;
const Keyword* const Keyword::OFF = &k_OFF;
const Keyword* const Keyword::ON = &k_ON;
const Keyword* const Keyword::OPEN_BRACKET = &k_OPEN_BRACKET;
const Keyword* const Keyword::ORDER = &k_ORDER;
const Keyword* const Keyword::OUTLINE = &k_OUTLINE;
const Keyword* const Keyword::PADSTACK = &k_PADSTACK;
const ScopeKeyword* const Keyword::PART_LIBRARY_SCOPE = &k_PART_LIBRARY_SCOPE;
const ScopeKeyword* const Keyword::PARSER_SCOPE = &k_PARSER_SCOPE;
const ScopeKeyword* const Keyword::PCB_SCOPE = &k_PCB_SCOPE;
const Keyword* const Keyword::PIN = &k_PIN;
const Keyword* const Keyword::PINS = &k_PINS;
const Keyword* const Keyword::PLACE = &k_PLACE;
const ScopeKeyword* const Keyword::PLACE_CONTROL = &k_PLACE_CONTROL;
const Keyword* const Keyword::PLACE_KEEPOUT = &k_PLACE_KEEPOUT;
const ScopeKeyword* const Keyword::PLACEMENT_SCOPE = &k_PLACEMENT_SCOPE;
const ScopeKeyword* const Keyword::PLANE_SCOPE = &k_PLANE_SCOPE;
const Keyword* const Keyword::PLANE_VIA_COSTS = &k_PLANE_VIA_COSTS;
const Keyword* const Keyword::PREFERRED_DIRECTION = &k_PREFERRED_DIRECTION;
const Keyword* const Keyword::PREFERRED_DIRECTION_TRACE_COSTS = &k_PREFERRED_DIRECTION_TRACE_COSTS;
const Keyword* const Keyword::SNAP_ANGLE = &k_SNAP_ANGLE;
const Keyword* const Keyword::POLYGON = &k_POLYGON;
const Keyword* const Keyword::PN = &k_PN;
const Keyword* const Keyword::POLYGON_PATH = &k_POLYGON_PATH;
const Keyword* const Keyword::POLYLINE_PATH = &k_POLYLINE_PATH;
const Keyword* const Keyword::POSITION = &k_POSITION;
const Keyword* const Keyword::POSTROUTE = &k_POSTROUTE;
const Keyword* const Keyword::POWER = &k_POWER;
const Keyword* const Keyword::PULL_TIGHT = &k_PULL_TIGHT;
const Keyword* const Keyword::RECTANGLE = &k_RECTANGLE;
const ScopeKeyword* const Keyword::RESOLUTION_SCOPE = &k_RESOLUTION_SCOPE;
const Keyword* const Keyword::ROTATE = &k_ROTATE;
const Keyword* const Keyword::ROTATE_FIRST = &k_ROTATE_FIRST;
const Keyword* const Keyword::ROUTES = &k_ROUTES;
const Keyword* const Keyword::RULE = &k_RULE;
const Keyword* const Keyword::RULES = &k_RULES;
const Keyword* const Keyword::SESSION = &k_SESSION;
const Keyword* const Keyword::SHAPE = &k_SHAPE;
const Keyword* const Keyword::SHOVE_FIXED = &k_SHOVE_FIXED;
const Keyword* const Keyword::SIDE = &k_SIDE;
const Keyword* const Keyword::SIGNAL = &k_SIGNAL;
const Keyword* const Keyword::SPARE = &k_SPARE;
const Keyword* const Keyword::START_PASS_NO = &k_START_PASS_NO;
const Keyword* const Keyword::START_RIPUP_COSTS = &k_START_RIPUP_COSTS;
const Keyword* const Keyword::STRING_QUOTE = &k_STRING_QUOTE;
const ScopeKeyword* const Keyword::STRUCTURE_SCOPE = &k_STRUCTURE_SCOPE;
const Keyword* const Keyword::TYPE = &k_TYPE;
const Keyword* const Keyword::USE_LAYER = &k_USE_LAYER;
const Keyword* const Keyword::USE_NET = &k_USE_NET;
const Keyword* const Keyword::USE_VIA = &k_USE_VIA;
const Keyword* const Keyword::VERTICAL = &k_VERTICAL;
const Keyword* const Keyword::VIA = &k_VIA;
const Keyword* const Keyword::VIAS = &k_VIAS;
const Keyword* const Keyword::VIA_AT_SMD = &k_VIA_AT_SMD;
const Keyword* const Keyword::VIA_COSTS = &k_VIA_COSTS;
const Keyword* const Keyword::VIA_KEEPOUT = &k_VIA_KEEPOUT;
const Keyword* const Keyword::VIA_RULE = &k_VIA_RULE;
const Keyword* const Keyword::WIDTH = &k_WIDTH;
const Keyword* const Keyword::WINDOW = &k_WINDOW;
const Keyword* const Keyword::WIRE = &k_WIRE;
const ScopeKeyword* const Keyword::WIRING_SCOPE = &k_WIRING_SCOPE;
const Keyword* const Keyword::WRITE_RESOLUTION = &k_WRITE_RESOLUTION;

} // namespace freerouting::designforms::specctra::parser
