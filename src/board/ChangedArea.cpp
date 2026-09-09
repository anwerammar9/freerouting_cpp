#include "board/ChangedArea.h"

#include <cstdint>

#include "geometry/planar/TileShape.h"

namespace freerouting::board {

ChangedArea::ChangedArea(int p_layer_count)
    : layer_count_(p_layer_count)
    , right_(p_layer_count, 0)
    , top_(p_layer_count, 0)
    , left_(p_layer_count, INT64_MAX)
    , bottom_(p_layer_count, INT64_MAX)
{
}

void ChangedArea::join(int64_t p_x, int64_t p_y, int p_layer)
{
    if (p_layer < 0 || p_layer >= layer_count_)
        return;
    
    right_[p_layer] = std::max(right_[p_layer], p_x);
    top_[p_layer] = std::max(top_[p_layer], p_y);
    left_[p_layer] = std::min(left_[p_layer], p_x);
    bottom_[p_layer] = std::min(bottom_[p_layer], p_y);
}

void ChangedArea::join(int64_t p_right, int64_t p_top, int64_t p_left, int64_t p_bottom, int p_layer)
{
    if (p_layer < 0 || p_layer >= layer_count_)
        return;
    
    right_[p_layer] = std::max(right_[p_layer], p_right);
    top_[p_layer] = std::max(top_[p_layer], p_top);
    left_[p_layer] = std::min(left_[p_layer], p_left);
    bottom_[p_layer] = std::min(bottom_[p_layer], p_bottom);
}

bool ChangedArea::is_empty() const
{
    for (int i = 0; i < layer_count_; ++i)
    {
        if (right_[i] > left_[i] && top_[i] > bottom_[i])
            return false;
    }
    return true;
}

void ChangedArea::clear()
{
    for (int i = 0; i < layer_count_; ++i)
    {
        set_empty(i);
    }
}

geometry::planar::IntOctagon ChangedArea::get_area(int p_layer) const
{
    if (p_layer < 0 || p_layer >= layer_count_)
    {
        return geometry::planar::IntOctagon::EMPTY;
    }
    if (right_[p_layer] <= left_[p_layer] || top_[p_layer] <= bottom_[p_layer])
    {
        return geometry::planar::IntOctagon::EMPTY;
    }
    const int64_t lx = left_[p_layer];
    const int64_t ly = bottom_[p_layer];
    const int64_t rx = right_[p_layer];
    const int64_t uy = top_[p_layer];
    // Box corners expressed as an octagon: ulx = lx - uy, lrx = rx - ly,
    // llx = lx + ly, urx = rx + uy.
    return geometry::planar::TileShape::get_instance(
        static_cast<int>(lx), static_cast<int>(ly), static_cast<int>(rx), static_cast<int>(uy),
        static_cast<int>(lx - uy), static_cast<int>(rx - ly), static_cast<int>(lx + ly),
        static_cast<int>(rx + uy));
}

void ChangedArea::set_empty(int p_layer)
{
    if (p_layer < 0 || p_layer >= layer_count_)
    {
        return;
    }
    right_[p_layer] = 0;
    top_[p_layer] = 0;
    left_[p_layer] = INT64_MAX;
    bottom_[p_layer] = INT64_MAX;
}

} // namespace freerouting::board