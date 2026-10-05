#include "board/Pin.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <optional>
#include <vector>

#include "board/Component.h"
#include "board/Padstack.h"
#include "board/RoutingBoard.h"
#include "core/Package.h"
#include "geometry/planar/Direction.h"
#include "geometry/planar/FortyfiveDegreeDirection.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/LineSegment.h"
#include "geometry/planar/Shape.h"
#include "geometry/planar/TileShape.h"
#include "logger/FRLogger.h"

namespace freerouting::board {

Pin::Pin(int p_component_no,
             int p_pin_no,
             const std::vector<int>& p_net_nos,
             int p_clearance_class,
             int p_id_no,
             FixedState p_fixed_state,
             BasicBoard* p_board)
    : DrillItem(geometry::planar::IntPoint(0, 0), p_net_nos, p_clearance_class,
                p_id_no, p_component_no, p_fixed_state, p_board),
      pin_no_(p_pin_no),
      changed_to_(nullptr)
{
    changed_to_ = this;
    // Java Pin passes a null center to the DrillItem constructor, so that the
    // pin center is calculated from the component placement on the first
    // get_center() call.
    center_is_set_ = false;
}

/**
 * Returns the component of this pin or nullptr, if this pin does not belong
 * to a component on the board. Mirrors the repeated
 * board.components.get(getComponentId()) lookups of the Java Pin class.
 */
Component* Pin::get_component() const
{
    // Divergence: Java board.components.get() looks up the component by its
    // 1 based component id, while the compiled C++ Components class looks up
    // a component by its index in the board component list. A pin therefore
    // refers to its component by that index in the C++ port.
    const auto* routing_board = dynamic_cast<const RoutingBoard*>(board_);
    if (routing_board == nullptr || routing_board->components() == nullptr || get_component_no() < 0)
    {
        return nullptr;
    }
    return routing_board->components()->get(get_component_no());
}

geometry::planar::IntPoint Pin::relative_location() const
{
    // Port of Java Pin.relative_location(). The Java IntVector of a package pin
    // is represented by an IntPoint in C++.
    geometry::planar::IntPoint rel_location(0, 0);
    const Component* component = get_component();
    if (component == nullptr)
    {
        logger::FRLogger::instance().warn("Pin.relative_location: component not found");
        return rel_location;
    }
    const core::Package* lib_package = component->get_package();
    if (lib_package == nullptr)
    {
        logger::FRLogger::instance().warn("Pin.relative_location: package not found");
        return rel_location;
    }
    const core::Package::Pin* package_pin = lib_package->get_pin(pin_no_);
    if (package_pin == nullptr)
    {
        logger::FRLogger::instance().warn("Pin.relative_location: pin no out of range");
        return rel_location;
    }
    const geometry::planar::IntPoint package_rel_location(package_pin->x, package_pin->y);
    rel_location = package_rel_location;
    const double component_rotation = component->get_rotation();
    const auto* routing_board = dynamic_cast<const RoutingBoard*>(board_);
    const bool flip_style_rotate_first =
        routing_board != nullptr && routing_board->get_flip_style_rotate_first();
    if (!component->is_placed_on_front() && !flip_style_rotate_first)
    {
        rel_location = geometry::planar::IntPoint(-package_rel_location.x, package_rel_location.y);
    }
    if (std::fmod(component_rotation, 90.0) == 0.0)
    {
        const int component_ninety_degree_factor = static_cast<int>(component_rotation) / 90;
        if (component_ninety_degree_factor != 0)
        {
            rel_location =
                rel_location.turn_90_degree(component_ninety_degree_factor, geometry::planar::IntPoint(0, 0));
        }
    }
    else
    {
        // rotation may be not exact. Java subtracts the origin point from the
        // rounded float vector here, which is a no-op.
        geometry::planar::FloatPoint location_approx = rel_location.to_float();
        location_approx = location_approx.rotate(component_rotation * std::acos(-1.0) / 180.0,
            geometry::planar::FloatPoint(0, 0));
        rel_location = location_approx.round();
    }
    if (!component->is_placed_on_front() && flip_style_rotate_first)
    {
        rel_location = geometry::planar::IntPoint(-rel_location.x, rel_location.y);
    }
    return rel_location;
}

std::optional<std::string> Pin::name() const
{
    // Port of Java Pin.name().
    const Component* component = get_component();
    if (component == nullptr)
    {
        logger::FRLogger::instance().warn("Pin.name: component not found");
        return std::nullopt;
    }
    const core::Package* lib_package = component->get_package();
    if (lib_package == nullptr)
    {
        logger::FRLogger::instance().warn("Pin.name: package not found");
        return std::nullopt;
    }
    const core::Package::Pin* package_pin = lib_package->get_pin(pin_no_);
    if (package_pin == nullptr)
    {
        logger::FRLogger::instance().warn("Pin.name: pin no out of range");
        return std::nullopt;
    }
    return std::to_string(package_pin->number);
}

const geometry::planar::IntPoint& Pin::get_center() const
{
    // Port of Java Pin.get_center(). Java raises a NullPointerException, if
    // the component of the pin is not found; the C++ port returns the
    // unchanged center of the DrillItem base class.
    if (center_is_set_)
    {
        return center_;
    }
    const Component* component = get_component();
    if (component == nullptr)
    {
        logger::FRLogger::instance().warn("Pin.get_center: component not found");
        return center_;
    }
    const geometry::planar::IntPoint rel_location = relative_location();
    geometry::planar::IntPoint pin_center(component->get_location().x + rel_location.x,
        component->get_location().y + rel_location.y);

    // check that the pin center is inside the pin shape and correct it eventually
    const Padstack* padstack = get_padstack();
    if (padstack == nullptr)
    {
        logger::FRLogger::instance().warn("Pin.get_center: padstack not found");
    }
    else
    {
        const int from_layer = first_layer();
        const int to_layer = last_layer();
        const geometry::planar::Shape* current_shape = nullptr;
        for (int i = from_layer; i <= to_layer; i++)
        {
            current_shape = padstack->get_shape(i - from_layer);
            if (current_shape != nullptr)
            {
                break;
            }
        }
        if (current_shape == nullptr)
        {
            logger::FRLogger::instance().warn("Pin.get_center: at least 1 shape != null expected");
        }
        else if (!current_shape->contains_inside(pin_center))
        {
            // Divergence: the C++ Pin has no precalculated shape in board
            // coordinates, so the correction uses the untransformed padstack
            // shape of the pin.
            pin_center = current_shape->centre_of_gravity().round();
        }
    }
    center_ = pin_center;
    center_is_set_ = true;
    return center_;
}

double Pin::min_width(int p_layer) const
{
    // Port of Java Pin.getMinWidth(int): the padstack layer index is relative
    // to the first layer of the pin.
    const Padstack* padstack = get_padstack();
    if (padstack == nullptr)
    {
        logger::FRLogger::instance().warn("Pin.min_width: padstack is null");
        return 0;
    }
    const int padstack_layer = p_layer - first_layer();
    const geometry::planar::Shape* padstack_shape = padstack->get_shape(padstack_layer);
    if (padstack_shape == nullptr)
    {
        logger::FRLogger::instance().warn("Pin.min_width: padstackShape is null");
        return 0;
    }
    return padstack_shape->bounding_box().min_width();
}

double Pin::get_max_width(int p_layer) const
{
    // Port of Java Pin.getMaxWidth(int): the padstack layer index is relative
    // to the first layer of the pin.
    const Padstack* padstack = get_padstack();
    if (padstack == nullptr)
    {
        logger::FRLogger::instance().warn("Pin.get_max_width: padstack is null");
        return 0;
    }
    const int padstack_layer = p_layer - first_layer();
    const geometry::planar::Shape* padstack_shape = padstack->get_shape(padstack_layer);
    if (padstack_shape == nullptr)
    {
        logger::FRLogger::instance().warn("Pin.get_max_width: padstackShape is null");
        return 0;
    }
    return padstack_shape->bounding_box().max_width();
}

int Pin::get_trace_neckdown_halfwidth(int p_layer) const
{
    // Port of Java Pin.getTraceNeckdownHalfwidth(int).
    const double result = std::max(0.5 * min_width(p_layer) - 1, 1.0);
    return static_cast<int>(result);
}

std::vector<Pin::TraceExitRestriction> Pin::get_trace_exit_restrictions(int p_layer) const
{
    // Port of Java Pin.getTraceExitRestrictions(int).
    std::vector<TraceExitRestriction> result;
    const Padstack* pin_padstack = get_padstack();
    if (pin_padstack == nullptr)
    {
        return result;
    }

    // Java Pin.getPadstackLayer(int) resolves the padstack layer from the
    // component placement, because the padstack of a component is stored
    // mirrored for components on the bottom side.
    const int padstack_layer = p_layer - first_layer();

    double pad_xy_factor = 1.5;
    // Setting 1.5 to a higher factor may hinder the shove algorithm of the
    // autorouter between the pins of SMD components, because the channels can
    // get blocked by the shove_fixed stubs.

    auto* routing_board = dynamic_cast<const RoutingBoard*>(board_);
    Component* component = nullptr;
    if (routing_board != nullptr && routing_board->components() != nullptr
        && get_component_no() > 0)
    {
        component = routing_board->components()->get(get_component_no());
        if (component != nullptr && component->get_package() != nullptr
            && component->get_package()->get_pin_count() <= 3)
        {
            pad_xy_factor *= 2; // allow connection to the longer side also for shorter pads
        }
    }

    std::vector<std::unique_ptr<geometry::planar::Direction>> padstack_exit_directions =
        pin_padstack->get_trace_exit_directions(padstack_layer, pad_xy_factor);
    if (padstack_exit_directions.empty() || component == nullptr)
    {
        return result;
    }

    const geometry::planar::Shape* current_shape = pin_padstack->get_shape(padstack_layer);
    auto* pad_shape = dynamic_cast<const geometry::planar::TileShape*>(current_shape);
    if (pad_shape == nullptr)
    {
        return result;
    }

    const double component_rotation = component->get_rotation();
    const geometry::planar::IntPoint pin_center = center_;
    const geometry::planar::FloatPoint center_approx = pin_center.to_float();

    for (const auto& padstack_exit_direction : padstack_exit_directions)
    {
        const core::Package* lib_package = component->get_package();
        if (lib_package == nullptr)
        {
            continue;
        }
        const core::Package::Pin* package_pin = lib_package->get_pin(pin_no_);
        if (package_pin == nullptr)
        {
            continue;
        }
        const double current_rotation_in_degree = component_rotation + package_pin->rotation_in_degree;
        std::unique_ptr<geometry::planar::Direction> current_exit_direction;
        if (std::fmod(current_rotation_in_degree, 45.0) == 0.0)
        {
            int fortyfive_degree_factor = static_cast<int>(current_rotation_in_degree) / 45;
            current_exit_direction = padstack_exit_direction->turn_45_degree(fortyfive_degree_factor);
        }
        else
        {
            double current_angle_in_radian =
                current_rotation_in_degree * std::acos(-1.0) / 180.0 + padstack_exit_direction->angle_approx();
            current_exit_direction = geometry::planar::Direction::get_instance_approx(current_angle_in_radian);
        }
        if (current_exit_direction == nullptr)
        {
            continue;
        }
        auto* int_exit_direction = dynamic_cast<const geometry::planar::IntDirection*>(
            current_exit_direction.get());
        if (int_exit_direction == nullptr)
        {
            continue;
        }

        // Calculate the minimum line length from the pin center into the exit direction
        int intersecting_border_line_no =
            pad_shape->intersecting_border_line_no(pin_center, *int_exit_direction);
        if (intersecting_border_line_no < 0)
        {
            logger::FRLogger::instance().warn("Pin.get_trace_exit_restrictions: border line not found");
            continue;
        }
        geometry::planar::Line current_exit_line =
            geometry::planar::Line::get_instance(pin_center, *int_exit_direction);
        geometry::planar::FloatPoint nearest_border_point =
            current_exit_line.intersection_approx(pad_shape->border_line(intersecting_border_line_no));
        result.push_back(TraceExitRestriction{*int_exit_direction,
            center_approx.distance(nearest_border_point)});
    }
    return result;
}

bool Pin::has_trace_exit_restrictions() const
{
    for (int layer = first_layer(); layer <= last_layer(); ++layer)
    {
        if (!get_trace_exit_restrictions(layer).empty())
        {
            return true;
        }
    }
    return false;
}

std::optional<geometry::planar::FloatPoint> Pin::nearest_trace_exit_corner(
    const geometry::planar::FloatPoint& p_from_point, int p_trace_half_width, int p_layer) const
{
    // Port of Java Pin.nearestTraceExitCorner().
    const std::vector<TraceExitRestriction> trace_exit_restrictions =
        get_trace_exit_restrictions(p_layer);
    if (trace_exit_restrictions.empty())
    {
        return std::nullopt;
    }
    const Padstack* pin_padstack = get_padstack();
    if (pin_padstack == nullptr)
    {
        return std::nullopt;
    }
    auto* pin_shape = dynamic_cast<const geometry::planar::TileShape*>(
        pin_padstack->get_shape(p_layer - first_layer()));
    if (pin_shape == nullptr)
    {
        return std::nullopt;
    }
    auto* routing_board = dynamic_cast<const RoutingBoard*>(board_);
    if (routing_board == nullptr)
    {
        return std::nullopt;
    }
    const double edge_to_turn_dist = routing_board->get_rules().get_pin_edge_to_turn_dist();
    if (edge_to_turn_dist < 0)
    {
        return std::nullopt;
    }
    std::unique_ptr<geometry::planar::TileShape> offset_pin_shape =
        pin_shape->offset(edge_to_turn_dist + p_trace_half_width);
    if (offset_pin_shape == nullptr)
    {
        return std::nullopt;
    }

    // Calculate the nearest legal pin exit point to p_from_point
    const geometry::planar::IntPoint pin_center = center_;
    double min_exit_corner_distance = std::numeric_limits<double>::max();
    std::optional<geometry::planar::FloatPoint> nearest_exit_corner;
    for (const TraceExitRestriction& current_exit_restriction : trace_exit_restrictions)
    {
        int current_intersecting_border_line_no =
            offset_pin_shape->intersecting_border_line_no(pin_center, current_exit_restriction.direction);
        if (current_intersecting_border_line_no < 0)
        {
            continue;
        }
        geometry::planar::Line current_pin_exit_ray =
            geometry::planar::Line::get_instance(pin_center, current_exit_restriction.direction);
        geometry::planar::FloatPoint current_exit_corner =
            current_pin_exit_ray.intersection_approx(
                offset_pin_shape->border_line(current_intersecting_border_line_no));
        double current_exit_corner_distance = current_exit_corner.distance_square(p_from_point);
        if (current_exit_corner_distance < min_exit_corner_distance)
        {
            min_exit_corner_distance = current_exit_corner_distance;
            nearest_exit_corner = current_exit_corner;
        }
    }
    return nearest_exit_corner;
}

std::string Pin::to_string() const
{
    return "Pin[id=" + std::to_string(id_no_) + ", no=" + std::to_string(pin_no_) + "]";
}

} // namespace freerouting::board
