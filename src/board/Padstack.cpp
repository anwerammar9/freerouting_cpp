#include "board/Padstack.h"

#include <algorithm>

namespace freerouting::board {

Padstack::Padstack(const std::string& p_name)
    : name_(p_name)
    , from_layer_(0)
    , to_layer_(0)
    , drill_(0)
{
}

void Padstack::add_shape(int p_layer, geometry::planar::Shape* p_shape)
{
    if (p_layer >= static_cast<int>(shapes_.size()))
    {
        shapes_.resize(p_layer + 1, nullptr);
    }
    shapes_[p_layer] = p_shape;
}

const geometry::planar::Shape* Padstack::get_shape(int p_layer) const
{
    if (p_layer >= 0 && p_layer < static_cast<int>(shapes_.size()))
    {
        return shapes_[p_layer];
    }
    return nullptr;
}

geometry::planar::IntBox Padstack::get_bounding_box() const
{
    int64_t min_x = 0, max_x = 0, min_y = 0, max_y = 0;
    bool first = true;

    for (const auto* shape : shapes_)
    {
        if (shape)
        {
            auto box = shape->bounding_box();
            if (first)
            {
                min_x = box.ll.x;
                max_x = box.ur.x;
                min_y = box.ll.y;
                max_y = box.ur.y;
                first = false;
            }
            else
            {
                min_x = std::min(min_x, box.ll.x);
                max_x = std::max(max_x, box.ur.x);
                min_y = std::min(min_y, box.ll.y);
                max_y = std::max(max_y, box.ur.y);
            }
        }
    }

    if (first)
    {
        return geometry::planar::IntBox::EMPTY;
    }
    return geometry::planar::IntBox(min_x, min_y, max_x, max_y);
}

std::string Padstack::to_string() const
{
    return "Padstack[" + name_ + "]";
}

} // namespace freerouting::board