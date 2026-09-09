#pragma once

#include <cmath>

#include "board/Unit.h"

namespace freerouting::board {

class CoordinateTransform
{
public:
    CoordinateTransform(double p_user_unit_factor,
                        Unit p_user_unit,
                        double p_board_unit_factor,
                        Unit p_board_unit);

    double board_to_user(int64_t p_coord) const;
    int64_t user_to_board(double p_coord) const;

    double board_to_user(double p_coord) const;
    int64_t user_to_board_int(double p_coord) const;

    double get_scale_factor() const { return scale_factor_; }

    Unit get_user_unit() const { return user_unit_; }
    Unit get_board_unit() const { return board_unit_; }

private:
    Unit user_unit_;
    Unit board_unit_;
    double user_unit_factor_;
    double board_unit_factor_;
    double scale_factor_;
};

} // namespace freerouting::board