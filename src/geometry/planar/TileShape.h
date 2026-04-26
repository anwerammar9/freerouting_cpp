#pragma once

#include <memory>

namespace freerouting {
namespace geometry {
namespace planar {

class IntBox;

class TileShape
{
public:
    virtual ~TileShape() = default;
    virtual bool is_empty() const { return true; }
    virtual std::unique_ptr<TileShape> translate_by(int dx, int dy) const { return nullptr; }
};

} // namespace planar
} // namespace geometry
} // namespace freerouting