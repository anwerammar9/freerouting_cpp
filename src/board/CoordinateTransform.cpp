#include "board/CoordinateTransform.h"

namespace freerouting::board {

CoordinateTransform::CoordinateTransform(double p_user_unit_factor,
                                          Unit p_user_unit,
                                          double p_board_unit_factor,
                                          Unit p_board_unit)
    : user_unit_(p_user_unit)
    , board_unit_(p_board_unit)
    , user_unit_factor_(p_user_unit_factor)
    , board_unit_factor_(p_board_unit_factor)
    , scale_factor_(p_board_unit_factor / p_user_unit_factor)
{
}

double CoordinateTransform::board_to_user(int64_t p_coord) const
{
    return static_cast<double>(p_coord) / scale_factor_;
}

int64_t CoordinateTransform::user_to_board(double p_coord) const
{
    return static_cast<int64_t>(std::round(p_coord * scale_factor_));
}

double CoordinateTransform::board_to_user(double p_coord) const
{
    return p_coord / scale_factor_;
}

int64_t CoordinateTransform::user_to_board_int(double p_coord) const
{
    return static_cast<int64_t>(std::round(p_coord * scale_factor_));
}

} // namespace freerouting::board