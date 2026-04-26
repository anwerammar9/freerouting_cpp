#include "board/BoardOutline.h"

namespace freerouting {
namespace board {

BoardOutline::BoardOutline()
{
}

BoardOutline::BoardOutline(const std::vector<std::shared_ptr<PolylineShape>>& p_shapes,
                       int p_clearance_class_no)
    : shapes_(p_shapes)
{
}

std::shared_ptr<TileShape> BoardOutline::get_tile_shape() const
{
    if (shapes_.empty())
    {
        return nullptr;
    }

    std::shared_ptr<TileShape> result = shapes_[0]->get_tile_shape();
    for (size_t i = 1; i < shapes_.size(); ++i)
    {
        if (shapes_[i])
        {
            result = result->intersection(shapes_[i]->get_tile_shape().get());
        }
    }
    return result;
}

void BoardOutline::set_keepout_area(const std::shared_ptr<class Area>& p_area)
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