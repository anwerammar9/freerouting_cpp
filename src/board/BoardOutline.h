#pragma once

#include <memory>
#include <string>
#include <vector>

#include "board/Item.h"
#include "geometry/planar/PolylineShape.h"
#include "geometry/planar/Shape.h"

namespace freerouting {
namespace board {

/**
 * The outline of the board: an obstacle for all items except areas. Its tree
 * shapes are thin shapes along the outline lines, or the area outside the
 * outline if the keepout outside the outline is generated. Port of Java
 * board.model.structure.BoardOutline.
 */
class BoardOutline : public Item
{
public:
    /** Java BoardOutline.HALF_WIDTH. */
    static constexpr int kHalfWidth = 100;

    BoardOutline(std::vector<std::shared_ptr<geometry::planar::PolylineShape>> p_shapes, int p_clearance_class_no,
        int p_id_no, BasicBoard* p_board);

    int tile_shape_count() const override;
    int shape_layer(int p_index) const override;
    bool is_obstacle(const Item* p_other) const override;
    int first_layer() const override { return 0; }
    int last_layer() const override;
    bool is_routable() const override { return false; }

    int shape_count() const { return static_cast<int>(shapes_.size()); }
    /** The shape with index p_index, or nullptr. Java getShape. */
    const geometry::planar::PolylineShape* get_shape(int p_index) const;
    const std::vector<std::shared_ptr<geometry::planar::PolylineShape>>& get_shapes() const { return shapes_; }

    /** The board bounding box minus the outline shapes, created on demand. Java getKeepoutArea. */
    const geometry::planar::Area* get_keepout_area() const;
    bool keepout_outside_outline_generated() const { return keepout_outside_outline_; }
    /** Switches the keepout outside the outline and reinserts the outline into the trees. Java generateKeepoutOutside. */
    void generate_keepout_outside(bool p_value);
    /** The number of border lines of all outline shapes. Java lineCount. */
    int line_count() const;
    int get_half_width() const { return kHalfWidth; }
    bool contains(const geometry::planar::IntPoint& p_point) const;
    bool contains(const geometry::planar::FloatPoint& p_point) const;

    /** Number of clearance violations recorded for this outline (C++ DRC helper). */
    int clearance_violation_count() const { return clearance_violation_count_; }
    void set_clearance_violation_count(int p_count) { clearance_violation_count_ = p_count; }

    std::string to_string() const override { return "BoardOutline"; }

protected:
    std::vector<std::shared_ptr<geometry::planar::TileShape>> calculate_tree_shapes(
        const ShapeSearchTree& p_search_tree) const override;

private:
    std::vector<std::shared_ptr<geometry::planar::PolylineShape>> shapes_;
    mutable std::shared_ptr<geometry::planar::Area> keepout_area_;
    bool keepout_outside_outline_ = false;
    int clearance_violation_count_ = 0;
};

} // namespace board
} // namespace freerouting
