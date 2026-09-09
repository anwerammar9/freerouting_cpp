#include "geometry/planar/FortyfiveDegreeBoundingDirections.h"
#include "geometry/planar/ConvexShape.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/Simplex.h"
#include "geometry/planar/Shape.h"
#include "geometry/planar/PolygonShape.h"

namespace freerouting {
namespace geometry {
namespace planar {

const FortyfiveDegreeBoundingDirections FortyfiveDegreeBoundingDirections::INSTANCE;

IntOctagon FortyfiveDegreeBoundingDirections::bounds(const ConvexShape& p_shape) const
{
    return p_shape.bounding_shape(*this);
}

IntOctagon FortyfiveDegreeBoundingDirections::bounds(const IntBox& p_box) const
{
    return p_box.to_IntOctagon();
}

IntOctagon FortyfiveDegreeBoundingDirections::bounds(const IntOctagon& p_oct) const
{
    return p_oct;
}

IntOctagon FortyfiveDegreeBoundingDirections::bounds(const Simplex& p_simplex) const
{
    return p_simplex.bounding_octagon();
}

IntOctagon FortyfiveDegreeBoundingDirections::bounds(const Circle& p_circle) const
{
    return p_circle.bounding_octagon();
}

IntOctagon FortyfiveDegreeBoundingDirections::bounds(const PolygonShape& p_polygon) const
{
    return p_polygon.bounding_octagon();
}

} // namespace planar
} // namespace geometry
} // namespace freerouting