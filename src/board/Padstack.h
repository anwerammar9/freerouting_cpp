#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "geometry/planar/Shape.h"
#include "geometry/planar/IntBox.h"

namespace freerouting::geometry::planar {
class Direction;
} // namespace freerouting::geometry::planar

namespace freerouting::board {

class Padstack
{
public:
    Padstack(const std::string& p_name);

    const std::string& get_name() const { return name_; }

    int get_from_layer() const { return from_layer_; }
    void set_from_layer(int p_layer) { from_layer_ = p_layer; }

    int get_to_layer() const { return to_layer_; }
    void set_to_layer(int p_layer) { to_layer_ = p_layer; }

    int get_drill() const { return drill_; }
    void set_drill(int p_drill)
    {
        drill_ = p_drill;
        // Unlike Java, where the padstack shapes are immutable after
        // construction, the C++ padstack is filled in step by step, so the
        // drill radius cache has to be dropped whenever the padstack changes.
        cached_drill_radius_.reset();
    }

    bool is_placed_absolute() const { return placed_absolute_; }
    void set_placed_absolute(bool p_value) { placed_absolute_ = p_value; }

    void add_shape(int p_layer, geometry::planar::Shape* p_shape);
    const geometry::planar::Shape* get_shape(int p_layer) const;

    int board_layer_count() const { return static_cast<int>(shapes_.size()); }

    /**
     * Calculates the allowed trace exit directions of the pad shape on layer
     * p_layer. If the pad is squarish (within p_factor), all 4 directions are
     * allowed, otherwise only the short-side directions. Mirrors Java
     * core.Padstack.get_trace_exit_directions().
     */
    std::vector<std::unique_ptr<geometry::planar::Direction>> get_trace_exit_directions(
        int p_layer, double p_factor) const;

    geometry::planar::IntBox get_bounding_box() const;

    /**
     * Returns the drill radius of this padstack in board units. Port of Java
     * core.Padstack.getDrillRadius(): the drill diameter is parsed from the
     * padstack name if it carries one, otherwise 45 percent of the smallest
     * shape radius is used. The result is cached, like the Java field.
     */
    double get_drill_radius() const;

    std::string to_string() const;

private:
    /**
     * Smallest half-width over all non-null shapes, 0 if the padstack has no
     * shape. Port of Java core.Padstack.getSmallestRadius().
     */
    double get_smallest_radius() const;

    std::string name_;
    int from_layer_ = 0;
    int to_layer_ = 0;
    int drill_ = 0;
    bool placed_absolute_ = false;
    std::vector<geometry::planar::Shape*> shapes_;
    mutable std::optional<double> cached_drill_radius_;
};

} // namespace freerouting::board