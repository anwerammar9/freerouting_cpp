#include "geometry/planar/OrthogonalBoundingDirections.h"
#include "geometry/planar/ConvexShape.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/Simplex.h"
#include "geometry/planar/Shape.h"
#include "geometry/planar/PolygonShape.h"

namespace freerouting {
namespace geometry {
namespace planar {

const OrthogonalBoundingDirections OrthogonalBoundingDirections::INSTANCE;

IntOctagon OrthogonalBoundingDirections::bounds(const ConvexShape& p_shape) const
{
    return p_shape.bounding_shape(*this);
}

IntOctagon OrthogonalBoundingDirections::bounds(const IntBox& p_box) const
{
    return p_box.to_IntOctagon();
}

IntOctagon OrthogonalBoundingDirections::bounds(const IntOctagon& p_oct) const
{
    return p_oct.bounding_box().to_IntOctagon();
}

IntOctagon OrthogonalBoundingDirections::bounds(const Simplex& p_simplex) const
{
    return p_simplex.bounding_box().to_IntOctagon();
}

IntOctagon OrthogonalBoundingDirections::bounds(const Circle& p_circle) const
{
    return p_circle.bounding_box().to_IntOctagon();
}

IntOctagon OrthogonalBoundingDirections::bounds(const PolygonShape& p_polygon) const
{
    return p_polygon.bounding_box().to_IntOctagon();
}

} // namespace planar
} // namespace geometry
} // namespace freerouting