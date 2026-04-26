#include "geometry/planar/TileShape.h"
#include "geometry/planar/IntBox.h"

namespace freerouting {
namespace geometry {
namespace planar {

// TileShape stub implementation using IntBox
class RegularTileShape : public TileShape
{
public:
    RegularTileShape() = default;
    
    explicit RegularTileShape(const IntBox& p_box) : box_(p_box) {}
    
    bool is_empty() const override { return box_.is_empty(); }
    
    std::unique_ptr<TileShape> translate_by(int dx, int dy) const override {
        return std::make_unique<RegularTileShape>(box_.translate(dx, dy));
    }
    
    const IntBox& box() const { return box_; }

private:
    IntBox box_;
};

} // namespace planar
} // namespace geometry
} // namespace freerouting