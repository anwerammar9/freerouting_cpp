#pragma once

#include <string>

#include "board/Item.h"
#include "geometry/planar/Shape.h"

namespace freerouting::drc {

class ClearanceViolation
{
public:
    ClearanceViolation(board::Item* p_first_item,
                       board::Item* p_second_item,
                       geometry::planar::Shape* p_shape,
                       int p_layer,
                       double p_expected_clearance,
                       double p_actual_clearance);

    board::Item* get_first_item() const { return first_item_; }
    board::Item* get_second_item() const { return second_item_; }
    geometry::planar::Shape* get_shape() const { return shape_; }
    int get_layer() const { return layer_; }
    double get_expected_clearance() const { return expected_clearance_; }
    double get_actual_clearance() const { return actual_clearance_; }

    std::string to_string() const;

private:
    board::Item* first_item_;
    board::Item* second_item_;
    geometry::planar::Shape* shape_;
    int layer_;
    double expected_clearance_;
    double actual_clearance_;
};

} // namespace freerouting::drc