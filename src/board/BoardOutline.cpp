#include "board/BoardOutline.h"
#include "geometry/planar/PolylineShape.h"
#include "geometry/planar/TileShape.h"

namespace freerouting {
namespace board {

BoardOutline::BoardOutline()
    : Item(std::vector<int>{}, 1, 0, 0, FixedState::SYSTEM_FIXED, nullptr)
{
}

BoardOutline::BoardOutline(const std::vector<std::shared_ptr<geometry::planar::PolylineShape>>& p_shapes,
                           int p_clearance_class_no)
    : Item(std::vector<int>{}, p_clearance_class_no, 0, 0, FixedState::SYSTEM_FIXED, nullptr)
    , shapes_(p_shapes)
{
}

std::shared_ptr<geometry::planar::TileShape> BoardOutline::get_tile_shape() const
{
    if (shapes_.empty())
    {
        return nullptr;
    }
    const auto& convex_shapes = shapes_[0]->split_to_convex();
    if (convex_shapes.empty())
    {
        return nullptr;
    }
    return convex_shapes[0]->clone();
}

void BoardOutline::set_keepout_area(const std::shared_ptr<Area>& p_area)
{
    keepout_area_ = p_area;
}

void BoardOutline::set_keepout_outside_outline(bool p_value)
{
    keepout_outside_outline_ = p_value;
}

std::string BoardOutline::to_string() const
{
    return "BoardOutline";
}

} // namespace board
} // namespace freerouting