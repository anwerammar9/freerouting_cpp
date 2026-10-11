#pragma once

#include "board/Via.h"
#include "geometry/planar/FortyfiveDegreeDirection.h"

#include <optional>
#include <string>
#include <vector>

namespace freerouting::board {

class Component;

class Pin : public DrillItem
{
public:
    Pin(int p_component_no,
        int p_pin_no,
        const std::vector<int>& p_net_nos,
        int p_clearance_class,
        int p_id_no,
        FixedState p_fixed_state,
        BasicBoard* p_board);

    bool is_pin() const override { return true; }

    /**
     * Returns the center point of this pin. If the center has not been
     * calculated yet, it is derived from the location of the component of the
     * pin and its relative location in the component package, and corrected to
     * lie inside the pin shape if necessary. Port of Java Pin.get_center().
     */
    const geometry::planar::IntPoint& get_center() const override;

    /**
     * Returns the name of this pin inside the package of its component, or
     * nothing, if the component of the pin is not found on the board.
     * Port of Java Pin.name().
     */
    std::optional<std::string> name() const;

    /**
     * Calculates the location of this pin relative to the location of its
     * component, including the mirroring of a component on the back side and
     * the rotation of the component. Port of Java Pin.relative_location().
     */
    geometry::planar::IntPoint relative_location() const;

    /**
     * Returns true, if vias through the pads of this pin are allowed, false,
     * otherwise. Currently, drills are allowed to SMD-pins.
     * Mirrors Java Pin.drill_allowed().
     */
    bool drill_allowed() const { return first_layer() == last_layer(); }

    /** Java Pin.isPlacedOnFront: the side of the component, true without a component. */
    bool is_placed_on_front() const override;

    int get_pin_no() const { return pin_no_; }
    Pin* get_changed_to() const { return changed_to_; }
    void set_changed_to(Pin* p_pin) { changed_to_ = p_pin; }

    /**
     * Returns the smallest width of the pin shape on p_layer. Port of Java
     * Pin.getMinWidth(int). Returns 0 if the padstack has no shape on that
     * layer, exactly like the Java warning path.
     */
    double min_width(int p_layer) const;

    /**
     * Returns the biggest width of the pin shape on p_layer. Port of Java
     * Pin.getMaxWidth(int). Returns 0 if the padstack has no shape on that
     * layer, exactly like the Java warning path.
     */
    double get_max_width(int p_layer) const;

    /**
     * Returns the neckdown half width for traces on p_layer. The neckdown
     * width is used, when the pin width is smaller than the trace width to
     * enter or leave the pin with a trace. Port of Java
     * Pin.getTraceNeckdownHalfwidth(int).
     */
    int get_trace_neckdown_halfwidth(int p_layer) const;

    /** One allowed trace exit direction of a pin with its minimum line length. */
    struct TraceExitRestriction
    {
        /** The direction of the trace leaving the pin. */
        geometry::planar::IntDirection direction;
        /** The minimum length of the trace into that direction. */
        double min_trace_length;
    };

    /**
     * Calculates the allowed trace exit directions of the shape of this pin on
     * p_layer together with the minimal trace line lengths into their
     * directions. Currently implemented only for box shapes, where traces are
     * allowed to exit the pad only on the small sides. Port of Java
     * Pin.getTraceExitRestrictions(int).
     */
    std::vector<TraceExitRestriction> get_trace_exit_restrictions(int p_layer) const;

    /** Returns true, if this pin has exit restrictions on some layer. */
    bool has_trace_exit_restrictions() const;

    /**
     * Returns the exit corner of this pin which is nearest to p_from_point, or
     * nothing, if the pin has no trace exit restriction or the turn distance of
     * the board rules is negative. Port of Java Pin.nearestTraceExitCorner().
     */
    std::optional<geometry::planar::FloatPoint> nearest_trace_exit_corner(
        const geometry::planar::FloatPoint& p_from_point, int p_trace_half_width, int p_layer) const;

    std::string to_string() const override;

    /**
     * The layer of the padstack shape for the shape with index p_index. The
     * padstack of a component on the back side is used mirrored, unless it is
     * placed absolute. Port of Java Pin.getPadstackLayer.
     */
    int get_padstack_layer(int p_index) const;

protected:
    /** Port of Java Pin.getShape: rotates, mirrors and translates the padstack shapes. */
    std::vector<std::unique_ptr<geometry::planar::Shape>> calculate_shapes() const override;

private:
    /**
     * Returns the component of this pin or nullptr, if this pin does not
     * belong to a component of the board. Mirrors the repeated
     * board.components.get(getComponentId()) lookups of the Java Pin class.
     */
    Component* get_component() const;

    int pin_no_;
    Pin* changed_to_;
};

} // namespace freerouting::board