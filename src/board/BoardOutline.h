#pragma once

#include <memory>
#include <vector>

#include "board/Item.h"

namespace freerouting {
namespace board {

class TileShape;

class BoardOutline : public Item
{
public:
    BoardOutline();

    BoardOutline(const std::vector<std::shared_ptr<class PolylineShape>>& p_shapes,
               int p_clearance_class_no);

    const std::vector<std::shared_ptr<class PolylineShape>>& get_shapes() const { return shapes_; }

    std::shared_ptr<TileShape> get_tile_shape() const;

    std::shared_ptr<class Area> get_keepout_area() const { return keepout_area_; }
    void set_keepout_area(const std::shared_ptr<class Area>& p_area);

    bool is_keepout_outside_outline() const { return keepout_outside_outline_; }
    void set_keepout_outside_outline(bool p_value);

    /**
     * Returns the number of clearance violations recorded for this outline.
     * Mirrors Java BoardOutline.clearance_violation_count.
     */
    int clearance_violation_count() const { return clearance_violation_count_; }
    void set_clearance_violation_count(int p_count) { clearance_violation_count_ = p_count; }

    std::string to_string() const override;

private:
    std::vector<std::shared_ptr<class PolylineShape>> shapes_;
    std::shared_ptr<class Area> keepout_area_;
    std::vector<std::shared_ptr<TileShape>> keepout_lines_;
    bool keepout_outside_outline_ = true;
    int clearance_violation_count_ = 0;
};

} // namespace board
} // namespace freerouting