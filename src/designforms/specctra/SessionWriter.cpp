#include "designforms/specctra/SessionWriter.h"

#include <cmath>
#include <cstdio>
#include <fstream>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "board/BoardLibrary.h"
#include "board/Communication.h"
#include "board/Component.h"
#include "board/ConductionArea.h"
#include "board/Layer.h"
#include "board/Package.h"
#include "board/Padstack.h"
#include "board/Pin.h"
#include "board/PolylineTrace.h"
#include "board/RoutingBoard.h"
#include "board/Via.h"
#include "datastructures/IdentifierType.h"
#include "datastructures/IndentFileWriter.h"
#include "designforms/specctra/parser/CoordinateTransform.h"
#include "designforms/specctra/parser/Layer.h"
#include "designforms/specctra/parser/Parser.h"
#include "designforms/specctra/parser/Resolution.h"
#include "designforms/specctra/parser/Shape.h"
#include "geometry/planar/CircleArea.h"
#include "geometry/planar/PolylineArea.h"
#include "geometry/planar/Polyline.h"
#include "logger/FRLogger.h"
#include "rules/BoardRules.h"
#include "rules/Net.h"

namespace freerouting {
namespace designforms {
namespace specctra {

namespace {

namespace fp = geometry::planar;
using datastructures::IdentifierType;
using datastructures::IndentFileWriter;
using parser::CoordinateTransform;

/** Java (int) Math.round(double). */
int java_round(double p_value) { return static_cast<int>(std::floor(p_value + 0.5)); }

/** Java String.replace(CharSequence, CharSequence): replaces every occurrence. */
std::string replace_all(std::string p_text, const std::string& p_from, const std::string& p_to)
{
    std::size_t pos = 0;
    while ((pos = p_text.find(p_from, pos)) != std::string::npos)
    {
        p_text.replace(pos, p_from.size(), p_to);
        pos += p_to.size();
    }
    return p_text;
}

void write_identifier(const IdentifierType& p_identifier_type, const std::string& p_name, IndentFileWriter& p_file)
{
    p_identifier_type.write(p_name, p_file.stream());
}

std::shared_ptr<parser::Layer> session_layer(const board::RoutingBoard& p_board, int p_layer_no)
{
    const auto& board_layer = p_board.get_layer_structure().get_layer(p_layer_no);
    return std::make_shared<parser::Layer>(board_layer->name, p_layer_no, board_layer->is_signal);
}

/** Writes all components with the package p_package. Java SesWriter.writeComponents. */
void write_component(const IdentifierType& p_identifier_type, const CoordinateTransform& p_transform,
    IndentFileWriter& p_file, const board::Component& p_component)
{
    p_file.new_line();
    p_file << "(place ";
    write_identifier(p_identifier_type, p_component.get_name(), p_file);
    const std::vector<double> location = p_transform.board_to_dsn(p_component.get_location().to_float());
    p_file << " " << java_round(location[0]) << " " << java_round(location[1]);
    p_file << (p_component.is_placed_on_front() ? " front " : " back ");
    p_file << SessionWriter::format_placement_rotation(p_component.get_rotation());
    if (p_component.is_position_fixed())
    {
        p_file.new_line();
        p_file << " (lock_type position)";
    }
    p_file << ")";
}

void write_components(const board::RoutingBoard& p_board, const IdentifierType& p_identifier_type,
    const CoordinateTransform& p_transform, IndentFileWriter& p_file, const board::Package* p_package)
{
    const std::vector<board::Item*> board_items = p_board.get_items();
    bool component_found = false;
    const board::Components& components = *p_board.components();
    for (int i = 1; i <= components.component_count(); ++i)
    {
        const board::Component* curr_component = components.get(i);
        if (curr_component == nullptr || curr_component->get_package() != p_package)
        {
            continue;
        }
        // check that not all items of the component are deleted
        bool undeleted_item_found = false;
        for (const board::Item* curr_item : board_items)
        {
            if (curr_item->get_component_no() == curr_component->get_id())
            {
                undeleted_item_found = true;
                break;
            }
        }
        if (undeleted_item_found)
        {
            if (!component_found)
            {
                p_file.start_scope();
                p_file << "component ";
                write_identifier(p_identifier_type, p_package->get_name(), p_file);
                component_found = true;
            }
            write_component(p_identifier_type, p_transform, p_file, *curr_component);
        }
    }
    if (component_found)
    {
        p_file.end_scope();
    }
}

void write_placement(const board::RoutingBoard& p_board, const IdentifierType& p_identifier_type,
    const CoordinateTransform& p_transform, IndentFileWriter& p_file)
{
    p_file.start_scope();
    p_file << "placement";
    parser::Resolution::write_scope(p_file, *p_board.communication());
    const board::Packages& packages = p_board.library().packages();
    for (int i = 1; i <= packages.count(); ++i)
    {
        write_components(p_board, p_identifier_type, p_transform, p_file, packages.get(i));
    }
    p_file.end_scope();
}

/** Writes "<component>-<pin>" of p_pin. */
void write_pin_name(const board::RoutingBoard& p_board, const IdentifierType& p_identifier_type,
    IndentFileWriter& p_file, const board::Pin& p_pin)
{
    const board::Component* component = p_board.components()->get(p_pin.get_component_no());
    if (component == nullptr)
    {
        logger::FRLogger::instance().warn("SesWriter.writeWasIs: component not found");
        return;
    }
    write_identifier(p_identifier_type, component->get_name(), p_file);
    p_file << "-";
    const board::Package::Pin* package_pin = component->get_package()->get_pin(p_pin.get_pin_no());
    write_identifier(p_identifier_type, package_pin != nullptr ? package_pin->name : std::string(), p_file);
}

void write_was_is(const board::RoutingBoard& p_board, const IdentifierType& p_identifier_type, IndentFileWriter& p_file)
{
    p_file.start_scope();
    p_file << "was_is";
    for (const board::Pin* curr_pin : p_board.get_pins())
    {
        const board::Pin* swapped_with = curr_pin->get_changed_to();
        if (swapped_with != curr_pin && swapped_with != nullptr)
        {
            p_file.new_line();
            p_file << "(pins ";
            write_pin_name(p_board, p_identifier_type, p_file, *curr_pin);
            p_file << " ";
            write_pin_name(p_board, p_identifier_type, p_file, *swapped_with);
            p_file << ")";
        }
    }
    p_file.end_scope();
}

void write_padstack(const board::Padstack& p_padstack, const board::RoutingBoard& p_board,
    const IdentifierType& p_identifier_type, const CoordinateTransform& p_transform, IndentFileWriter& p_file)
{
    // determine the layer range covered by this padstack
    const int layer_count = p_board.get_layer_count();
    int first_layer_no = 0;
    while (first_layer_no < layer_count && p_padstack.get_shape(first_layer_no) == nullptr)
    {
        ++first_layer_no;
    }
    int last_layer_no = layer_count - 1;
    while (last_layer_no >= 0 && p_padstack.get_shape(last_layer_no) == nullptr)
    {
        --last_layer_no;
    }
    if (first_layer_no >= layer_count || last_layer_no < 0)
    {
        logger::FRLogger::instance().warn("SesWriter.writePadstack: padstack shape not found");
        return;
    }
    p_file.start_scope();
    p_file << "padstack ";
    write_identifier(p_identifier_type, p_padstack.get_name(), p_file);
    for (int i = first_layer_no; i <= last_layer_no; ++i)
    {
        const fp::Shape* curr_board_shape = p_padstack.get_shape(i);
        if (curr_board_shape == nullptr)
        {
            continue;
        }
        std::unique_ptr<parser::Shape> curr_shape =
            p_transform.board_to_dsn_rel(*curr_board_shape, session_layer(p_board, i));
        p_file.start_scope();
        p_file << "shape";
        if (curr_shape != nullptr)
        {
            curr_shape->write_scope_int(p_file, p_identifier_type);
        }
        p_file.end_scope();
    }
    if (!p_padstack.is_attach_allowed())
    {
        p_file.new_line();
        p_file << "(attach off)";
    }
    p_file.end_scope();
}

void write_library(const board::RoutingBoard& p_board, const IdentifierType& p_identifier_type,
    const CoordinateTransform& p_transform, IndentFileWriter& p_file)
{
    p_file.start_scope();
    p_file << "library_out ";
    std::set<std::string> written_padstack_names;
    const board::BoardLibrary& library = p_board.library();
    for (int i = 0; i < library.via_padstack_count(); ++i)
    {
        const board::Padstack* via_padstack = library.get_via_padstack(i);
        if (via_padstack == nullptr || !written_padstack_names.insert(via_padstack->get_name()).second)
        {
            continue;
        }
        write_padstack(*via_padstack, p_board, p_identifier_type, p_transform, p_file);
    }
    p_file.end_scope();
}

void write_fixed_state(IndentFileWriter& p_file, board::FixedState p_fixed_state)
{
    if (static_cast<int>(p_fixed_state) <= static_cast<int>(board::FixedState::SHOVE_FIXED))
    {
        return;
    }
    p_file.new_line();
    p_file << "(type ";
    p_file << (p_fixed_state == board::FixedState::SYSTEM_FIXED ? "fix)" : "protect)");
}

void write_path(const std::string& p_layer_name, int p_width, const std::vector<int>& p_coors,
    const IdentifierType& p_identifier_type, IndentFileWriter& p_file)
{
    p_file.start_scope();
    p_file << "path ";
    write_identifier(p_identifier_type, p_layer_name, p_file);
    p_file << " " << p_width;
    const std::size_t corner_count = p_coors.size() / 2;
    for (std::size_t i = 0; i < corner_count; ++i)
    {
        p_file.new_line();
        p_file << p_coors[2 * i] << " " << p_coors[2 * i + 1];
    }
    p_file.end_scope();
}

/**
 * Java SesWriter.snappedEndpoint: the center of a contacted drill item to use for a wire end point
 * that lies inside the pad but not at its center, or nullopt to keep the corner.
 */
std::optional<fp::FloatPoint> snapped_endpoint(const board::PolylineTrace& p_wire, bool p_start_side)
{
    const fp::Polyline& polyline = p_wire.polyline();
    const fp::FloatPoint corner =
        p_start_side ? polyline.corner_approx(0) : polyline.corner_approx(polyline.corner_count() - 1);
    const board::ItemSet contacts = p_start_side ? p_wire.get_start_contacts() : p_wire.get_end_contacts();
    const int layer = p_wire.get_layer();
    for (const board::Item* contact : contacts)
    {
        const auto* drill = dynamic_cast<const board::DrillItem*>(contact);
        if (drill == nullptr)
        {
            continue;
        }
        if (layer < drill->first_layer() || layer > drill->last_layer())
        {
            continue;
        }
        const fp::Shape* pad_shape = drill->get_shape(layer - drill->first_layer());
        if (pad_shape == nullptr)
        {
            continue;
        }
        const fp::FloatPoint center = drill->get_center().to_float();
        const double center_distance = corner.distance(center);
        if (center_distance <= 0.5)
        {
            // Already at the center (within rounding) — nothing to fix.
            return std::nullopt;
        }
        if (center_distance <= pad_shape->border_distance(center))
        {
            return center;
        }
    }
    return std::nullopt;
}

void write_wire(const board::PolylineTrace& p_wire, const board::RoutingBoard& p_board,
    const IdentifierType& p_identifier_type, const CoordinateTransform& p_transform, IndentFileWriter& p_file)
{
    const int layer_index = p_wire.get_layer();
    const auto& board_layer = p_board.get_layer_structure().get_layer(layer_index);
    const int wire_width = java_round(p_transform.board_to_dsn(2.0 * p_wire.get_half_width()));
    p_file.start_scope();
    p_file << "wire";
    // Java polyline().corners() are exact (IntPoint or RationalPoint); their float approximations.
    const std::vector<fp::FloatPoint> corners = p_wire.polyline().corner_approx_arr();
    std::vector<int> coors;
    coors.reserve(2 * corners.size());
    int prev_x = 0;
    int prev_y = 0;
    for (std::size_t i = 0; i < corners.size(); ++i)
    {
        fp::FloatPoint corner_point = corners[i];
        if (i == 0 || i == corners.size() - 1)
        {
            if (std::optional<fp::FloatPoint> snapped = snapped_endpoint(p_wire, i == 0))
            {
                corner_point = *snapped;
            }
        }
        const std::vector<double> curr_float_coors = p_transform.board_to_dsn(corner_point);
        const int curr_x = java_round(curr_float_coors[0]);
        const int curr_y = java_round(curr_float_coors[1]);
        if (i == 0 || curr_x != prev_x || curr_y != prev_y)
        {
            coors.push_back(curr_x);
            coors.push_back(curr_y);
            prev_x = curr_x;
            prev_y = curr_y;
        }
    }
    write_path(board_layer->name, wire_width, coors, p_identifier_type, p_file);
    write_fixed_state(p_file, p_wire.get_fixed_state());
    p_file.end_scope();
}

void write_via(const board::Via& p_via, const IdentifierType& p_identifier_type,
    const CoordinateTransform& p_transform, IndentFileWriter& p_file)
{
    p_file.start_scope();
    p_file << "via ";
    write_identifier(p_identifier_type, p_via.get_padstack()->get_name(), p_file);
    const std::vector<double> location = p_transform.board_to_dsn(p_via.get_center().to_float());
    p_file << " " << java_round(location[0]) << " " << java_round(location[1]);
    write_fixed_state(p_file, p_via.get_fixed_state());
    p_file.end_scope();
}

void write_conduction_area(const board::ConductionArea& p_conduction_area, const board::RoutingBoard& p_board,
    const IdentifierType& p_identifier_type, const CoordinateTransform& p_transform, IndentFileWriter& p_file)
{
    if (p_conduction_area.net_count() != 1)
    {
        logger::FRLogger::instance().warn("SesWriter.writeConductionArea: unexpected net count");
        return;
    }
    const std::shared_ptr<fp::Area> curr_area = p_conduction_area.get_area();
    const std::shared_ptr<parser::Layer> conduction_layer = session_layer(p_board, p_conduction_area.get_layer());
    // Java: an area that is a Shape is its own boundary without holes, otherwise border + holes.
    const fp::Shape* boundary_shape = nullptr;
    std::vector<const fp::Shape*> holes;
    if (const auto* polyline_area = dynamic_cast<const fp::PolylineArea*>(curr_area.get()))
    {
        boundary_shape = polyline_area->get_border().get();
        for (const auto& hole : polyline_area->get_holes())
        {
            holes.push_back(hole.get());
        }
    }
    else if (const auto* circle_area = dynamic_cast<const fp::CircleArea*>(curr_area.get()))
    {
        boundary_shape = &circle_area->circle();
    }
    else
    {
        boundary_shape = curr_area.get();
    }
    p_file.start_scope();
    p_file << "wire ";
    if (boundary_shape != nullptr)
    {
        if (std::unique_ptr<parser::Shape> dsn_shape = p_transform.board_to_dsn(*boundary_shape, conduction_layer))
        {
            dsn_shape->write_scope_int(p_file, p_identifier_type);
        }
    }
    for (const fp::Shape* hole : holes)
    {
        if (std::unique_ptr<parser::Shape> dsn_hole = p_transform.board_to_dsn(*hole, conduction_layer))
        {
            dsn_hole->write_hole_scope(p_file, p_identifier_type);
        }
    }
    p_file.end_scope();
}

void write_net(int p_net_no, const board::RoutingBoard& p_board, const IdentifierType& p_identifier_type,
    const CoordinateTransform& p_transform, IndentFileWriter& p_file)
{
    bool header_written = false;
    for (const board::Item* curr_item : p_board.get_connectable_items(p_net_no))
    {
        if (curr_item->get_fixed_state() == board::FixedState::SYSTEM_FIXED)
        {
            continue;
        }
        const auto* wire = dynamic_cast<const board::PolylineTrace*>(curr_item);
        const auto* via = dynamic_cast<const board::Via*>(curr_item);
        const auto* conduction_area = dynamic_cast<const board::ConductionArea*>(curr_item);
        const bool is_conduction_area = conduction_area != nullptr
            && p_board.get_layer_structure().get_layer(curr_item->first_layer())->is_signal;
        if (!header_written && (wire != nullptr || via != nullptr || is_conduction_area))
        {
            p_file.start_scope();
            p_file << "net ";
            const rules::Net* curr_net = p_board.get_rules().get_nets().get(p_net_no);
            if (curr_net == nullptr)
            {
                logger::FRLogger::instance().warn("SesWriter.writeNet: net not found");
            }
            else
            {
                write_identifier(p_identifier_type, curr_net->get_name(), p_file);
            }
            header_written = true;
        }
        if (wire != nullptr)
        {
            write_wire(*wire, p_board, p_identifier_type, p_transform, p_file);
        }
        else if (via != nullptr)
        {
            write_via(*via, p_identifier_type, p_transform, p_file);
        }
        else if (is_conduction_area)
        {
            write_conduction_area(*conduction_area, p_board, p_identifier_type, p_transform, p_file);
        }
    }
    if (header_written)
    {
        p_file.end_scope();
    }
}

void write_network(const board::RoutingBoard& p_board, const IdentifierType& p_identifier_type,
    const CoordinateTransform& p_transform, IndentFileWriter& p_file)
{
    p_file.start_scope();
    p_file << "network_out ";
    for (int i = 1; i <= p_board.get_rules().get_nets().max_net_no(); ++i)
    {
        write_net(i, p_board, p_identifier_type, p_transform, p_file);
    }
    p_file.end_scope();
}

void write_routes(const board::RoutingBoard& p_board, const IdentifierType& p_identifier_type,
    const CoordinateTransform& p_transform, IndentFileWriter& p_file)
{
    p_file.start_scope();
    p_file << "routes ";
    parser::Resolution::write_scope(p_file, *p_board.communication());
    parser::Parser::write_scope(p_file, p_board.communication()->get_parser_info(), p_identifier_type, true);
    write_library(p_board, p_identifier_type, p_transform, p_file);
    write_network(p_board, p_identifier_type, p_transform, p_file);
    p_file.end_scope();
}

void write_session_scope(const board::RoutingBoard& p_board, const IdentifierType& p_identifier_type,
    IndentFileWriter& p_file, const std::string& p_session_name, const std::string& p_design_name)
{
    const board::Communication& communication = *p_board.communication();
    // Java: coordinateTransform.dsnToBoard(1) / resolution
    const double scale_factor = communication.get_transform_scale_factor() / communication.get_resolution();
    const CoordinateTransform coordinate_transform(scale_factor, 0, 0);
    p_file.start_scope(false);
    p_file << "session ";
    write_identifier(p_identifier_type, p_session_name, p_file);
    p_file.new_line();
    p_file << "(base_design ";
    write_identifier(p_identifier_type, p_design_name, p_file);
    p_file << ")";
    write_placement(p_board, p_identifier_type, coordinate_transform, p_file);
    write_was_is(p_board, p_identifier_type, p_file);
    write_routes(p_board, p_identifier_type, coordinate_transform, p_file);
    p_file.end_scope();
}

} // namespace

SessionWriter::SessionWriter(const std::string& p_filename)
    : filename_(p_filename)
{
}

bool SessionWriter::write_session(const board::RoutingBoard& p_board, const std::string& p_board_name)
{
    std::ofstream out(filename_);
    if (!out.is_open())
    {
        logger::FRLogger::instance().error("SessionWriter: Failed to open file for writing: " + filename_);
        return false;
    }
    write(p_board, out, p_board_name);
    logger::FRLogger::instance().info("SessionWriter: SES file written successfully: " + filename_);
    return out.good();
}

void SessionWriter::write(const board::RoutingBoard& p_board, std::ostream& p_out, const std::string& p_design_name)
{
    IndentFileWriter output_file(p_out);
    const std::string session_name = replace_all(p_design_name, ".dsn", ".ses");
    const std::vector<std::string> reserved_chars = {"(", ")", " ", ";", "-", "_", "/", "~", "{", "}"};
    const IdentifierType identifier_type(reserved_chars, p_board.communication()->get_parser_info().string_quote);
    write_session_scope(p_board, identifier_type, output_file, session_name, p_design_name);
    p_out.flush();
}

std::string SessionWriter::format_placement_rotation(double p_degrees)
{
    // Java Math.rint rounds half to even, like std::nearbyint in the default rounding mode.
    const double rounded = std::nearbyint(p_degrees * 1000.0) / 1000.0;
    char buffer[64];
    if (std::abs(rounded - std::nearbyint(rounded)) < 1e-9)
    {
        std::snprintf(buffer, sizeof(buffer), "%.0f", rounded);
        return buffer;
    }
    std::snprintf(buffer, sizeof(buffer), "%.3f", rounded);
    std::string formatted = buffer;
    if (formatted.find('.') != std::string::npos)
    {
        while (!formatted.empty() && formatted.back() == '0')
        {
            formatted.pop_back();
        }
        if (!formatted.empty() && formatted.back() == '.')
        {
            formatted.pop_back();
        }
    }
    return formatted;
}

} // namespace specctra
} // namespace designforms
} // namespace freerouting
