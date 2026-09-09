#include "board/Padstack.h"

#include <algorithm>

#include "geometry/planar/Direction.h"
#include "geometry/planar/IntOctagon.h"

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

std::vector<std::unique_ptr<geometry::planar::Direction>> Padstack::get_trace_exit_directions(
    int p_layer, double p_factor) const
{
    using namespace geometry::planar;
    std::vector<std::unique_ptr<Direction>> result;
    if (p_layer < 0 || p_layer >= static_cast<int>(shapes_.size()))
    {
        return result;
    }
    const geometry::planar::Shape* curr_shape = shapes_[p_layer];
    if (curr_shape == nullptr)
    {
        return result;
    }
    // Currently implemented only for box shapes, mirroring Java.
    if (dynamic_cast<const geometry::planar::IntBox*>(curr_shape) == nullptr
        && dynamic_cast<const geometry::planar::IntOctagon*>(curr_shape) == nullptr)
    {
        return result;
    }
    const geometry::planar::IntBox curr_box = curr_shape->bounding_box();

    const bool all_dirs = std::max(curr_box.width(), curr_box.height())
        < p_factor * std::min(curr_box.width(), curr_box.height());

    if (all_dirs || curr_box.width() >= curr_box.height())
    {
        result.push_back(std::make_unique<IntDirection>(IntDirection::RIGHT));
        result.push_back(std::make_unique<IntDirection>(IntDirection::LEFT));
    }
    if (all_dirs || curr_box.width() <= curr_box.height())
    {
        result.push_back(std::make_unique<IntDirection>(IntDirection::UP));
        result.push_back(std::make_unique<IntDirection>(IntDirection::DOWN));
    }
    return result;
}

std::string Padstack::to_string() const
{
    return "Padstack[" + name_ + "]";
}

} // namespace freerouting::board