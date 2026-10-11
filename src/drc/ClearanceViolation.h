#pragma once

#include <memory>
#include <string>

#include "board/Item.h"
#include "geometry/planar/TileShape.h"

namespace freerouting::drc {

class ClearanceViolation
{
public:
    ClearanceViolation(board::Item* p_first_item,
                       board::Item* p_second_item,
                       std::shared_ptr<geometry::planar::TileShape> p_shape,
                       int p_layer,
                       double p_expected_clearance,
                       double p_actual_clearance);

    board::Item* get_first_item() const { return first_item_; }
    board::Item* get_second_item() const { return second_item_; }
    const geometry::planar::TileShape* get_shape() const { return shape_.get(); }
    int get_layer() const { return layer_; }
    double get_expected_clearance() const { return expected_clearance_; }
    double get_actual_clearance() const { return actual_clearance_; }

    std::string to_string() const;

private:
    board::Item* first_item_;
    board::Item* second_item_;
    std::shared_ptr<geometry::planar::TileShape> shape_;
    int layer_;
    double expected_clearance_;
    double actual_clearance_;
};

} // namespace freerouting::drc