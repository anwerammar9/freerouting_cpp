#pragma once

#include <memory>
#include <vector>
#include "geometry/planar/IntBox.h"
#include "geometry/planar/TileShape.h"

namespace freerouting {
namespace autoroute {

class RoutingBoard;
class DrillPage;

namespace graphics {
    class GraphicsContext;
}

/**
 * @brief Describes the 2-dimensional array of pages of ExpansionDrills used in the maze search algorithm
 * 
 * The pages are rectangles of about equal width and height covering the bounding box
 * of the board area. This class manages the spatial partitioning for efficient
 * drill page operations during autorouting.
 */
class DrillPageArray {
public:
    /**
     * @brief Creates a new instance of DrillPageArray
     * 
     * @param p_board The routing board containing the area to partition
     * @param p_max_page_width Maximum width/height for individual pages
     */
    DrillPageArray(std::shared_ptr<RoutingBoard> p_board, int p_max_page_width);

    /**
     * @brief Invalidates all drill pages intersecting with p_shape
     * 
     * This forces recalculation of affected pages at the next call of get_drills()
     * 
     * @param p_shape The shape that intersects with pages to invalidate
     */
    void invalidate(std::shared_ptr<TileShape> p_shape);

    /**
     * @brief Collects all drill pages with a 2-dimensional overlap with p_shape
     * 
     * @param p_shape The shape to find overlapping pages for
     * @return Collection of overlapping drill pages
     */
    std::vector<std::shared_ptr<DrillPage>> overlapping_pages(std::shared_ptr<TileShape> p_shape) const;

    /**
     * @brief Resets all drill pages for autorouting the next connection
     */
    void reset();

    /**
     * @brief Test draw of all drills
     * 
     * @param p_graphics Graphics context for drawing
     * @param p_graphics_context Additional graphics context information
     * @param p_intensity Drawing intensity (0.0 to 1.0)
     */
    void draw(void* p_graphics, 
             const graphics::GraphicsContext& p_graphics_context, 
             double p_intensity) const;

    // Accessors for testing and monitoring
    const IntBox& get_bounds() const { return bounds_; }
    int get_column_count() const { return column_count_; }
    int get_row_count() const { return row_count_; }
    int get_page_width() const { return page_width_; }
    int get_page_height() const { return page_height_; }

    /**
     * @brief Get the drill page at specific coordinates
     * 
     * @param row Row index (0-based)
     * @param col Column index (0-based)
     * @return Pointer to the drill page, or nullptr if out of bounds
     */
    std::shared_ptr<DrillPage> get_page(int row, int col) const;

private:
    const IntBox bounds_;
    const int column_count_;
    const int row_count_;
    const int page_width_;
    const int page_height_;
    std::vector<std::vector<std::shared_ptr<DrillPage>>> page_arr_;

    /**
     * @brief Calculate page dimensions based on bounds and max page width
     * 
     * @param bounds The bounding box
     * @param max_page_width Maximum page width
     * @return Pair of (page_width, page_height)
     */
    std::pair<int, int> calculate_page_dimensions(const IntBox& bounds, int max_page_width) const;

    /**
     * @brief Get page indices for a coordinate
     * 
     * @param coord Coordinate value
     * @param page_size Size of pages in that dimension
     * @return Page index
     */
    int get_page_index(int coord, int page_size, int min_coord) const;

    /**
     * @brief Check if page coordinates are valid
     * 
     * @param row Row index
     * @param col Column index
     * @return true if coordinates are within bounds
     */
    bool is_valid_page(int row, int col) const;
};

} // namespace autoroute
} // namespace freerouting
