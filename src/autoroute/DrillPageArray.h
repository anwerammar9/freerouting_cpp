#pragma once

#include <memory>
#include <vector>

#include "geometry/planar/IntBox.h"

namespace freerouting::board {
class RoutingBoard;
}

namespace freerouting::autoroute {

class DrillPage;

/**
 * Describes the 2 dimensional array of pages of ExpansionDrills used in the
 * maze search algorithm. The pages are rectangles of about equal width and
 * height covering the bounding box of the board area. Mirrors Java
 * app.freerouting.autoroute.drill.DrillPageArray.
 *
 * The C++ room model is IntBox based, so shapes are passed as IntBox instead
 * of the Java TileShape (an IntBox is the bounding box of any TileShape).
 */
class DrillPageArray
{
public:
    /**
     * Creates a new instance of DrillPageArray covering the bounding box of
     * p_board with pages of a size near p_max_page_width.
     */
    DrillPageArray(board::RoutingBoard* p_board, int p_max_page_width);

    /**
     * Invalidates all drill pages intersecting with p_shape so that they are
     * recalculated at the next call of get_drills(). Mirrors Java
     * DrillPageArray.invalidate().
     */
    void invalidate(const geometry::planar::IntBox& p_shape);

    /**
     * Collects all drill pages with a 2-dimensional overlap with p_shape.
     * Mirrors Java DrillPageArray.overlappingPages().
     */
    std::vector<DrillPage*> overlapping_pages(const geometry::planar::IntBox& p_shape) const;

    /** Resets all drill pages for autorouting the next connection. */
    void reset();

    // Accessors for testing and monitoring.
    const geometry::planar::IntBox& get_bounds() const { return bounds_; }
    int get_column_count() const { return column_count_; }
    int get_row_count() const { return row_count_; }
    int get_page_width() const { return page_width_; }
    int get_page_height() const { return page_height_; }

    /** Returns the drill page at p_row, p_col or nullptr if out of range. */
    DrillPage* get_page(int p_row, int p_col) const;

private:
    static int calc_column_count(const geometry::planar::IntBox& p_bounds, int p_max_page_width);
    static int calc_row_count(const geometry::planar::IntBox& p_bounds, int p_max_page_width);
    static int calc_page_size(int p_length, int p_segment_count);

    const geometry::planar::IntBox bounds_;
    const int column_count_;
    const int row_count_;
    const int page_width_;
    const int page_height_;
    std::vector<std::vector<std::unique_ptr<DrillPage>>> pages_;
};

} // namespace freerouting::autoroute