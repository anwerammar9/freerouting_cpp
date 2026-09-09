#pragma once

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "board/Item.h"
#include "geometry/planar/Point.h"

namespace freerouting {
namespace geometry::planar {
class IntOctagon;
} // namespace geometry::planar

namespace board {

class PolylineTrace;
class ShapeSearchTree;

/**
 * Abstract base class for traces in the plane. Mirrors the Java class
 * app.freerouting.board.Trace. The C++ port keeps the trace a single-layer
 * item; the Connectable interface is not ported yet.
 */
class Trace : public Item
{
public:
    Trace(int p_layer, int p_half_width,
          const std::vector<int>& p_net_no_arr,
          int p_clearance_type, int p_id_no, int p_group_no,
          FixedState p_fixed_state);

    int get_layer() const override { return layer_; }
    int first_layer() const override { return layer_; }
    int last_layer() const override { return layer_; }
    void set_layer(int p_layer);

    int get_half_width() const { return half_width_; }

    /**
     * Returns the half width of this trace compensated by the clearance
     * compensation value of p_search_tree. Mirrors Java Trace.get_compensated_half_width.
     */
    int get_compensated_half_width(const ShapeSearchTree& p_search_tree) const;

    bool is_trace() const override { return true; }
    bool is_routable() const override;

    virtual const geometry::planar::Point& first_corner() const = 0;
    virtual const geometry::planar::Point& last_corner() const = 0;
    virtual const std::vector<std::shared_ptr<geometry::planar::Point>>& corners() const = 0;
    virtual int corner_count() const = 0;
    virtual const geometry::planar::Point& corner(int p_index) const = 0;

    virtual double get_length() const = 0;

    std::string to_string() const override;

    /**
     * Returns all items having a connection point at p_point on the layer of
     * this trace. If p_ignore_net is false, only contacts to items sharing a
     * net with this trace are calculated. This is the normal case.
     */
    std::set<Item*> get_normal_contacts(const geometry::planar::Point& p_point, bool p_ignore_net) const;

    std::set<Item*> get_start_contacts() const;
    std::set<Item*> get_end_contacts() const;
    std::vector<Item*> get_normal_contacts() const override;

    // ---- Trace-specific overrides of Item virtuals ----

    bool is_obstacle(const Item* p_other) const override;
    const geometry::planar::Point* normal_contact_point(const Item* p_other) const override;
    bool is_tail() const override;
    bool is_overlap() const override;
    bool is_shove_fixed() const override;
    bool is_drillable(int p_net_no) const override;
    std::vector<std::shared_ptr<geometry::planar::Point>> get_ratsnest_corners() const override;
    bool validate() override;

    /** Returns the endpoint of this trace nearest to p_from_point. */
    const geometry::planar::Point& nearest_end_point(const geometry::planar::Point& p_from_point) const;

    /**
     * Looks up touching pins at the first and last corner of the trace.
     * Mirrors Java Trace.touching_pins_at_end_corners().
     */
    std::set<class Pin*> touching_pins_at_end_corners() const;

    /**
     * Splits this trace at p_point. Base stub returns empty; PolylineTrace
     * overrides. Mirrors Java Trace.split(Point).
     */
    virtual std::vector<Trace*> split(const geometry::planar::Point& p_point);

    /**
     * Splits this trace and intersecting same-net traces at intersection
     * points. If p_clip_shape != nullptr, splitting is restricted to it.
     * Returns the resulting pieces (just this trace if nothing was split).
     * Mirrors Java Trace.split(IntOctagon).
     */
    virtual std::vector<PolylineTrace*> split(const geometry::planar::IntOctagon* p_clip_shape);

    /** Returns true if this trace is reachable via more than one path. */
    bool is_cycle() const;

    /**
     * Normalizes the trace geometry within the optional optimization area.
     * Returns true if something was changed. Base stub returns false.
     */
    virtual bool normalize(const geometry::planar::IntOctagon* p_opt_area);

    /**
     * Looks if this trace can be combined with other traces. Returns true if
     * something was combined. Base stub returns false. Mirrors Java
     * Trace.combine().
     */
    virtual bool combine();

protected:
    /**
     * Java Trace.isWithinTolerance: Manhattan distance connectivity test with
     * tolerance half_width + 1.
     */
    static bool is_within_tolerance(const geometry::planar::Point& p_point,
                                    const geometry::planar::Point& p_other,
                                    int p_tolerance);

    /**
     * Creates a copy of p_point preserving its concrete point type.
     */
    static std::shared_ptr<geometry::planar::Point> create_point_copy(const geometry::planar::Point& p_point);

private:
    int half_width_;
    int layer_;
};

} // namespace board
} // namespace freerouting