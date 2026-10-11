#include "designforms/specctra/BoardCountsDump.h"

#include <cstdio>
#include <sstream>
#include <typeinfo>

#include "board/BoardLibrary.h"
#include "board/BoardOutline.h"
#include "board/Component.h"
#include "board/ComponentObstacleArea.h"
#include "board/ComponentOutline.h"
#include "board/ConductionArea.h"
#include "board/Layer.h"
#include "board/ObstacleArea.h"
#include "board/Padstacks.h"
#include "board/Pin.h"
#include "board/PolylineTrace.h"
#include "board/Via.h"
#include "board/ViaObstacleArea.h"
#include "rules/BoardRules.h"
#include "rules/ClearanceMatrix.h"
#include "rules/Net.h"
#include "rules/ViaInfo.h"

namespace freerouting::designforms::specctra {

namespace {

std::string indent(int p_level) { return std::string(static_cast<std::size_t>(p_level) * 2, ' '); }

/** Java getClass().getSimpleName() of the board item classes. */
std::string simple_class_name(const board::Item& p_item)
{
    const std::type_info& type = typeid(p_item);
    if (type == typeid(board::Pin)) return "Pin";
    if (type == typeid(board::Via)) return "Via";
    if (type == typeid(board::PolylineTrace)) return "PolylineTrace";
    if (type == typeid(board::ConductionArea)) return "ConductionArea";
    if (type == typeid(board::ObstacleArea)) return "ObstacleArea";
    if (type == typeid(board::ViaObstacleArea)) return "ViaObstacleArea";
    if (type == typeid(board::ComponentObstacleArea)) return "ComponentObstacleArea";
    if (type == typeid(board::ComponentOutline)) return "ComponentOutline";
    if (type == typeid(board::BoardOutline)) return "BoardOutline";
    return "Item";
}

/** Java FixedState.name(); the C++ NOT_FIXED is Java's UNFIXED. */
std::string fixed_state_name(board::FixedState p_state)
{
    switch (p_state)
    {
    case board::FixedState::NOT_FIXED:
        return "UNFIXED";
    case board::FixedState::SHOVE_FIXED:
        return "SHOVE_FIXED";
    case board::FixedState::USER_FIXED:
        return "USER_FIXED";
    case board::FixedState::SYSTEM_FIXED:
        return "SYSTEM_FIXED";
    case board::FixedState::FROM_DESIGN:
        return "FROM_DESIGN";
    }
    return "UNFIXED";
}

struct Counts
{
    int pins = 0;
    int vias = 0;
    int traces = 0;
    int conduction_areas = 0;
    int obstacle_areas = 0;
    int via_obstacle_areas = 0;
    int component_obstacle_areas = 0;
    int component_outlines = 0;
    int board_outline = 0;

    void add(const board::Item& p_item)
    {
        pins += dynamic_cast<const board::Pin*>(&p_item) != nullptr;
        vias += dynamic_cast<const board::Via*>(&p_item) != nullptr;
        traces += dynamic_cast<const board::PolylineTrace*>(&p_item) != nullptr;
        conduction_areas += dynamic_cast<const board::ConductionArea*>(&p_item) != nullptr;
        via_obstacle_areas += dynamic_cast<const board::ViaObstacleArea*>(&p_item) != nullptr;
        component_obstacle_areas += dynamic_cast<const board::ComponentObstacleArea*>(&p_item) != nullptr;
        component_outlines += dynamic_cast<const board::ComponentOutline*>(&p_item) != nullptr;
        obstacle_areas += typeid(p_item) == typeid(board::ObstacleArea);
    }

    std::string to_json(const board::RoutingBoard& p_board) const
    {
        const rules::BoardRules& rules = p_board.get_rules();
        std::ostringstream out;
        out << "{\"nets\": " << rules.get_nets().max_net_no() << ", \"net_classes\": " << rules.get_net_classes().count()
            << ", \"via_infos\": " << rules.get_via_infos().count() << ", \"via_rules\": " << rules.get_via_rules().size()
            << ", \"padstacks\": " << p_board.library().padstacks().count()
            << ", \"packages\": " << p_board.library().packages().count()
            << ", \"components\": " << p_board.components()->component_count() << ", \"pins\": " << pins
            << ", \"vias\": " << vias << ", \"traces\": " << traces << ", \"conduction_areas\": " << conduction_areas
            << ", \"obstacle_areas\": " << obstacle_areas << ", \"via_obstacle_areas\": " << via_obstacle_areas
            << ", \"component_obstacle_areas\": " << component_obstacle_areas
            << ", \"component_outlines\": " << component_outlines << ", \"board_outline\": " << board_outline << "}";
        return out.str();
    }
};

const char* const kEmptyCounts =
    "{\"nets\": 0, \"net_classes\": 0, \"via_infos\": 0, \"via_rules\": 0, "
    "\"padstacks\": 0, \"packages\": 0, \"components\": 0, \"pins\": 0, \"vias\": 0, "
    "\"traces\": 0, \"conduction_areas\": 0, \"obstacle_areas\": 0, "
    "\"via_obstacle_areas\": 0, \"component_obstacle_areas\": 0, "
    "\"component_outlines\": 0, \"board_outline\": 0}";

std::string render_item(const board::Item& p_item)
{
    std::ostringstream r;
    r << "{\"id\": " << p_item.get_id() << ", \"type\": " << BoardCountsDump::json_string(simple_class_name(p_item))
      << ", \"layers\": [" << p_item.first_layer() << ", " << p_item.last_layer() << "], \"nets\": [";
    for (int i = 0; i < p_item.net_count(); ++i)
    {
        r << (i > 0 ? ", " : "") << p_item.get_net_no(i);
    }
    r << "], \"fixed\": " << BoardCountsDump::json_string(fixed_state_name(p_item.get_fixed_state()));
    if (const auto* pin = dynamic_cast<const board::Pin*>(&p_item))
    {
        r << ", \"component\": " << p_item.get_component_no() << ", \"pin\": " << pin->get_pin_no();
    }
    int corners = 0;
    if (const auto* trace = dynamic_cast<const board::PolylineTrace*>(&p_item))
    {
        corners = trace->corner_count();
    }
    r << ", \"corners\": " << corners << "}";
    return r.str();
}

void append_base(std::string& p_out, const std::string& p_result, const std::string& p_error)
{
    p_out += indent(1) + "\"result\": \"" + p_result + "\",\n";
    p_out += indent(1) + "\"error\": " + BoardCountsDump::json_string(p_error) + ",\n";
    p_out += indent(1) + "\"layers\": [],\n";
    p_out += indent(1) + "\"counts\": " + kEmptyCounts + ",\n";
    p_out += indent(1) + "\"clearance_classes\": [],\n";
    p_out += indent(1) + "\"clearance_matrix\": [],\n";
    p_out += indent(1) + "\"items_omitted\": false,\n";
    p_out += indent(1) + "\"items\": [],\n";
    p_out += indent(1) + "\"warnings\": 0";
}

void append_board(std::string& p_out, const board::RoutingBoard& p_board, std::size_t p_warning_count,
    const std::string& p_result, bool p_omit_items)
{
    p_out += indent(1) + "\"result\": \"" + p_result + "\",\n";
    p_out += indent(1) + "\"error\": null,\n";

    std::string layers = "[";
    const board::LayerStructure& layer_structure = p_board.get_layer_structure();
    for (int i = 0; i < layer_structure.layer_count(); ++i)
    {
        const auto& layer = layer_structure.get_layer(i);
        layers += (i > 0 ? ", " : "");
        layers += "{\"name\": " + BoardCountsDump::json_string(layer->name)
            + ", \"signal\": " + (layer->is_signal ? "true" : "false") + "}";
    }
    layers += "]";

    const rules::ClearanceMatrix& matrix = p_board.get_rules().get_clearance_matrix();
    const int class_count = matrix.get_class_count();
    std::string classes = "[";
    for (int i = 0; i < class_count; ++i)
    {
        classes += (i > 0 ? ", " : "") + BoardCountsDump::json_string(matrix.get_name(i));
    }
    classes += "]";

    std::ostringstream matrix_json;
    matrix_json << "[";
    const int layer_count = matrix.get_layer_count();
    for (int ci = 0; ci < class_count; ++ci)
    {
        matrix_json << (ci > 0 ? ", " : "") << "[";
        for (int cj = 0; cj < class_count; ++cj)
        {
            matrix_json << (cj > 0 ? ", " : "") << "[";
            for (int li = 0; li < layer_count; ++li)
            {
                matrix_json << (li > 0 ? ", " : "") << matrix.get_value(ci, cj, li, false);
            }
            matrix_json << "]";
        }
        matrix_json << "]";
    }
    matrix_json << "]";

    // one pass over get_items() in item-list order (descending id): render items + total the counts
    Counts counts;
    std::string items;
    if (p_omit_items)
    {
        items = "[]";
        for (const board::Item* item : p_board.get_items())
        {
            counts.add(*item);
        }
    }
    else
    {
        items = "[";
        bool first = true;
        for (const board::Item* item : p_board.get_items())
        {
            if (!first)
            {
                items += ",\n";
            }
            first = false;
            items += render_item(*item);
            counts.add(*item);
        }
        items += "]";
    }
    counts.board_outline = p_board.get_outline() != nullptr ? 1 : 0;

    p_out += indent(1) + "\"layers\": " + layers + ",\n";
    p_out += indent(1) + "\"counts\": " + counts.to_json(p_board) + ",\n";
    p_out += indent(1) + "\"clearance_classes\": " + classes + ",\n";
    p_out += indent(1) + "\"clearance_matrix\": " + matrix_json.str() + ",\n";
    p_out += indent(1) + "\"items_omitted\": " + (p_omit_items ? "true" : "false") + ",\n";
    p_out += indent(1) + "\"items\": " + items + ",\n";
    p_out += indent(1) + "\"warnings\": " + std::to_string(p_warning_count);
}

} // namespace

std::string BoardCountsDump::json_string(const std::string& p_text)
{
    std::string out = "\"";
    for (char c : p_text)
    {
        switch (c)
        {
        case '"':
            out += "\\\"";
            break;
        case '\\':
            out += "\\\\";
            break;
        case '\n':
            out += "\\n";
            break;
        case '\r':
            out += "\\r";
            break;
        case '\t':
            out += "\\t";
            break;
        case '\b':
            out += "\\b";
            break;
        case '\f':
            out += "\\f";
            break;
        default:
            if (static_cast<unsigned char>(c) < 0x20)
            {
                char buffer[8];
                std::snprintf(buffer, sizeof(buffer), "\\u%04x", static_cast<int>(c));
                out += buffer;
            }
            else
            {
                out += c;
            }
        }
    }
    return out + "\"";
}

std::string BoardCountsDump::render(const BoardReadResult& p_result, const std::string& p_dsn_path, bool p_omit_items)
{
    std::string out = "{\n";
    out += indent(1) + "\"file\": " + json_string(p_dsn_path) + ",\n";
    if (const auto* success = std::get_if<board_read_result::Success>(&p_result); success && success->board)
    {
        append_board(out, *success->board, success->warnings.size(), "success", p_omit_items);
    }
    else if (const auto* missing = std::get_if<board_read_result::OutlineMissing>(&p_result); missing && missing->board)
    {
        append_board(out, *missing->board, missing->warnings.size(), "outline_missing", p_omit_items);
    }
    else if (const auto* parse_error = std::get_if<board_read_result::ParseError>(&p_result))
    {
        append_base(out, "parse_error", parse_error->location + ": " + parse_error->detail);
    }
    else if (const auto* io_error = std::get_if<board_read_result::IoError>(&p_result))
    {
        append_base(out, "io_error", io_error->cause);
    }
    else
    {
        // Java would throw on a null board; report it like an unexpected result.
        append_base(out, "io_error", "unexpected BoardReadResult: no board");
    }
    out += "}\n";
    return out;
}

} // namespace freerouting::designforms::specctra
