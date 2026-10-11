#pragma once

#include <array>
#include <memory>
#include <optional>
#include <vector>

#include "board/Trace.h"
#include "geometry/planar/Polyline.h"

namespace freerouting {
namespace datastructures {
class Stoppable;
} // namespace datastructures

namespace geometry::planar {
class IntOctagon;
class Line;
class Point;
} // namespace geometry::planar

namespace board {

class PullTightAlgo;

/**
 * Trace whose geometry is described by a polyline. Mirrors the Java class
 * app.freerouting.board.PolylineTrace.
 */
class PolylineTrace : public Trace
{
public:
    PolylineTrace(const std::shared_ptr<geometry::planar::Polyline>& p_polyline,
                  int p_layer, int p_half_width,
                  const std::vector<int>& p_net_no_arr,
                  int p_clearance_type, int p_id_no, int p_group_no,
                  FixedState p_fixed_state);

    const geometry::planar::Point& first_corner() const override;
    const geometry::planar::Point& last_corner() const override;
    const std::vector<std::shared_ptr<geometry::planar::Point>>& corners() const override;
    int corner_count() const override;
    const geometry::planar::Point& corner(int p_index) const override;

    double get_length() const override;

    int tile_shape_count() const override;
    geometry::planar::IntOctagon get_tile_shape(int p_index) const override;

    const geometry::planar::Polyline& polyline() const { return *polyline_; }

    void change(const std::shared_ptr<geometry::planar::Polyline>& p_new_polyline);

    void clear_derived_data() override;

    /**
     * Checks if other traces can be combined with this trace. Returns true if
     * something was combined. This trace will be the combined trace, so that
     * only other traces may be deleted. Mirrors Java PolylineTrace.combine().
     */
    bool combine() override;

    /**
     * Splits this trace and overlapping traces, and combines this trace.
     * Returns true if something was changed. Mirrors Java
     * PolylineTrace.normalize(IntOctagon).
     */
    bool normalize(const geometry::planar::IntOctagon* p_clip_shape) override;

    /**
     * Splits this trace into two at p_point. Returns the 2 pieces, or empty
     * if nothing was split. Mirrors Java PolylineTrace.split(Point).
     */
    std::vector<Trace*> split(const geometry::planar::Point& p_point) override;

    /**
     * Splits this trace and intersecting traces at intersection points.
     * Mirrors Java PolylineTrace.split(IntOctagon).
     */
    std::vector<PolylineTrace*> split(
        const geometry::planar::IntOctagon* p_clip_shape) override;

    /**
     * Tries to pull this trace tight without creating clearance violations.
     * Returns true if the trace was changed. Mirrors Java
     * PolylineTrace.pull_tight(boolean, int, Stoppable).
     */
    bool pull_tight(bool p_own_net_only, int p_pull_tight_accuracy,
                    datastructures::Stoppable* p_stoppable);

    /**
     * Tries to shorten this trace with p_pull_tight_algo. Returns true if the
     * trace was changed. Mirrors Java PolylineTrace.pull_tight(PullTightAlgo).
     * The pin-connection swap is a TODO.
     */
    bool pull_tight(PullTightAlgo* p_pull_tight_algo);

private:
    /**
     * Splits this trace at line p_line_no by inserting p_new_end_line as the
     * concluding line of the first piece and the start line of the second.
     * Returns nullopt if nothing was split. Mirrors Java
     * PolylineTrace.split(int, Line).
     */
    std::optional<std::array<PolylineTrace*, 2>> split_at_line(
        int p_line_no, const geometry::planar::Line& p_new_end_line);

    /**
     * Checks if the intersection of the p_line_no-th line with p_line is
     * inside the pad of a pin. Mirrors Java
     * PolylineTrace.split_inside_drill_pad_prohibited().
     */
    bool split_inside_drill_pad_prohibited(int p_line_no,
                                           const geometry::planar::Line& p_line) const;

    /**
     * Looks if this trace can be combined at its first point with another
     * trace. Mirrors Java PolylineTrace.combine_at_start().
     */
    bool combine_at_start(bool p_ignore_areas);

    /**
     * Looks if this trace can be combined at its last point with another
     * trace. Mirrors Java PolylineTrace.combine_at_end().
     */
    bool combine_at_end(bool p_ignore_areas);

    bool normalize_impl(const geometry::planar::IntOctagon* p_clip_shape,
                        int p_normalization_depth);

    void ensure_corners() const;

    /**
     * Returns an owned copy of the corner at p_index (-1 = last corner), like
     * the Java code, which copies the result of firstCorner()/lastCorner()
     * instead of keeping a reference to the cached corner.
     */
    geometry::planar::IntPoint own_corner(int p_index) const;

    std::shared_ptr<geometry::planar::Polyline> polyline_;
    mutable std::vector<std::shared_ptr<geometry::planar::Point>> corner_points_;
};

} // namespace board
} // namespace freerouting