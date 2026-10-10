#include "geometry/planar/CircleArea.h"

#include "geometry/planar/TileShape.h"

namespace freerouting::geometry::planar {

const std::vector<std::unique_ptr<TileShape>>& CircleArea::split_to_convex() const
{
    if (!convex_pieces_)
    {
        convex_pieces_ = std::make_shared<std::vector<std::unique_ptr<TileShape>>>(circle_.split_to_convex());
    }
    return *convex_pieces_;
}

} // namespace freerouting::geometry::planar
