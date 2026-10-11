#include "geometry/planar/ConvexTileShape.h"

#include "geometry/planar/IntBox.h"
#include "geometry/planar/IntOctagon.h"
#include "geometry/planar/Shape.h"
#include "geometry/planar/Simplex.h"

#include <string>
#include <type_traits>

namespace freerouting::geometry::planar {

namespace {

// Java dispatches TileShape.intersects on the runtime types of both operands.
template <typename T>
bool tile_intersects(const TileShape& p_tile, const T& p_other)
{
    if (const auto* box = dynamic_cast<const IntBox*>(&p_tile))
    {
        return box->intersects(p_other);
    }
    if (const auto* simplex = dynamic_cast<const Simplex*>(&p_tile))
    {
        return simplex->intersects(p_other);
    }
    if (const auto* oct = dynamic_cast<const IntOctagon*>(&p_tile))
    {
        if constexpr (std::is_same_v<T, Simplex>)
        {
            return p_other.intersects(*oct);
        }
        else
        {
            return oct->intersects(p_other);
        }
    }
    return false;
}

} // namespace

ConvexTileShape::ConvexTileShape(std::unique_ptr<TileShape> p_tile)
    : tile_(std::move(p_tile))
{
}

ConvexTileShape::ConvexTileShape(const ConvexTileShape& p_other)
    : tile_(p_other.tile_->clone())
{
}

IntBox ConvexTileShape::bounding_box() const { return tile_->bounding_box(); }

bool ConvexTileShape::intersects(const Shape& p_other) const
{
    if (const auto* other_tile = dynamic_cast<const ConvexTileShape*>(&p_other))
    {
        if (const auto* box = dynamic_cast<const IntBox*>(&other_tile->tile()))
        {
            return intersects(*box);
        }
        if (const auto* oct = dynamic_cast<const IntOctagon*>(&other_tile->tile()))
        {
            return intersects(*oct);
        }
        if (const auto* simplex = dynamic_cast<const Simplex*>(&other_tile->tile()))
        {
            return intersects(*simplex);
        }
        return false;
    }
    if (const auto* circle = dynamic_cast<const Circle*>(&p_other))
    {
        return intersects(*circle);
    }
    return p_other.intersects(*this);
}

bool ConvexTileShape::contains(const IntPoint& p_point) const { return tile_->contains(p_point); }
std::string ConvexTileShape::to_string() const
{
    const IntBox box = tile_->bounding_box();
    return "ConvexTileShape[" + std::to_string(tile_->border_line_count()) + " lines, bbox ("
        + std::to_string(box.ll.x) + "," + std::to_string(box.ll.y) + ")-(" + std::to_string(box.ur.x) + ","
        + std::to_string(box.ur.y) + ")]";
}
double ConvexTileShape::circumference() const { return tile_->circumference(); }
double ConvexTileShape::area() const { return tile_->area(); }
FloatPoint ConvexTileShape::centre_of_gravity() const { return tile_->centre_of_gravity(); }
bool ConvexTileShape::is_outside(const IntPoint& p_point) const { return tile_->is_outside(p_point); }
bool ConvexTileShape::contains_inside(const IntPoint& p_point) const { return tile_->contains_inside(p_point); }
bool ConvexTileShape::contains_on_border(const IntPoint& p_point) const { return tile_->contains_on_border(p_point); }
bool ConvexTileShape::contains(const FloatPoint& p_point) const { return tile_->contains(p_point); }
double ConvexTileShape::distance(const FloatPoint& p_point) const { return tile_->distance(p_point); }
double ConvexTileShape::border_distance(const FloatPoint& p_point) const { return tile_->border_distance(p_point); }
std::unique_ptr<TileShape> ConvexTileShape::bounding_tile() const { return tile_->clone(); }
double ConvexTileShape::smallest_radius() const { return tile_->smallest_radius(); }

std::unique_ptr<Shape> ConvexTileShape::enlarge(double p_offset) const
{
    return std::make_unique<ConvexTileShape>(tile_->offset(p_offset));
}

bool ConvexTileShape::intersects(const IntBox& p_box) const { return tile_intersects(*tile_, p_box); }
bool ConvexTileShape::intersects(const IntOctagon& p_oct) const { return tile_intersects(*tile_, p_oct); }
bool ConvexTileShape::intersects(const Simplex& p_simplex) const { return tile_intersects(*tile_, p_simplex); }

bool ConvexTileShape::intersects(const Circle& p_circle) const
{
    if (const auto* box = dynamic_cast<const IntBox*>(tile_.get()))
    {
        return p_circle.intersects(*box);
    }
    if (const auto* oct = dynamic_cast<const IntOctagon*>(tile_.get()))
    {
        return p_circle.intersects(*oct);
    }
    if (const auto* simplex = dynamic_cast<const Simplex*>(tile_.get()))
    {
        return p_circle.intersects(*simplex);
    }
    return false;
}

std::unique_ptr<ConvexShape> ConvexTileShape::offset(double p_distance) const
{
    return std::make_unique<ConvexTileShape>(tile_->offset(p_distance));
}

std::unique_ptr<ConvexShape> ConvexTileShape::shrink(double p_offset) const
{
    return std::make_unique<ConvexTileShape>(tile_->shrink(p_offset));
}

double ConvexTileShape::max_width() const { return tile_->max_width(); }
double ConvexTileShape::min_width() const { return tile_->min_width(); }

IntOctagon ConvexTileShape::bounding_shape(const ShapeBoundingDirections& p_dirs) const
{
    return tile_->bounding_shape(p_dirs);
}

} // namespace freerouting::geometry::planar
