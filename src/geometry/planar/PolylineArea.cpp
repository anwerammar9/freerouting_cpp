#include "geometry/planar/PolylineArea.h"

#include <algorithm>
#include <functional>
#include <limits>
#include <sstream>

#include "geometry/planar/IntOctagon.h"
#include "geometry/planar/PolygonShape.h"
#include "logger/FRLogger.h"

namespace freerouting {
namespace geometry {
namespace planar {

namespace {

std::shared_ptr<PolylineShape> transform_polyline(
    const std::shared_ptr<PolylineShape>& p_shape,
    const std::function<std::shared_ptr<PolylineShape>(const PolygonShape&)>& p_transform,
    const char* p_operation)
{
    const auto* polygon = dynamic_cast<const PolygonShape*>(p_shape.get());
    if (polygon == nullptr)
    {
        logger::FRLogger::instance().warn(
            std::string("PolylineArea.") + p_operation + ": border shape is not a PolygonShape");
        return p_shape;
    }
    return p_transform(*polygon);
}

} // namespace

PolylineArea::PolylineArea(std::shared_ptr<PolylineShape> p_border_shape,
    std::vector<std::shared_ptr<PolylineShape>> p_hole_arr)
    : border_shape_(std::move(p_border_shape))
    , hole_arr_(std::move(p_hole_arr))
{
}

IntBox PolylineArea::bounding_box() const
{
    return border_shape_->bounding_box();
}

IntOctagon PolylineArea::bounding_octagon() const
{
    return border_shape_->bounding_octagon();
}

bool PolylineArea::contains(const IntPoint& p_point) const
{
    if (!border_shape_->contains(p_point))
    {
        return false;
    }
    for (const auto& hole : hole_arr_)
    {
        for (const auto& hole_piece : hole->split_to_convex())
        {
            if (hole_piece->contains_inside(p_point))
            {
                return false;
            }
        }
    }
    return true;
}

bool PolylineArea::is_empty() const
{
    return border_shape_->is_empty();
}

bool PolylineArea::is_bounded() const
{
    return border_shape_->is_bounded();
}

bool PolylineArea::is_contained_in(const IntBox& p_box) const
{
    return border_shape_->is_contained_in(p_box);
}

int PolylineArea::dimension() const
{
    return border_shape_->dimension();
}

const std::shared_ptr<PolylineShape>& PolylineArea::get_border() const
{
    return border_shape_;
}

const std::vector<std::shared_ptr<PolylineShape>>& PolylineArea::get_holes() const
{
    return hole_arr_;
}

std::unique_ptr<Area> PolylineArea::clone() const
{
    return std::make_unique<PolylineArea>(border_shape_, hole_arr_);
}

bool PolylineArea::intersects(const Shape& p_other) const
{
    if (is_empty())
    {
        return false;
    }
    if (!bounding_box().intersects(p_other.bounding_box()))
    {
        return false;
    }
    for (const auto& piece : split_to_convex())
    {
        for (int i = 0; i < piece->border_line_count(); ++i)
        {
            if (p_other.contains(piece->corner(i)))
            {
                return true;
            }
        }
    }
    return false;
}

bool PolylineArea::contains(const FloatPoint& p_point) const
{
    bool border_contains = false;
    for (const auto& piece : border_shape_->split_to_convex())
    {
        if (piece->contains(p_point))
        {
            border_contains = true;
            break;
        }
    }
    if (!border_contains)
    {
        return false;
    }
    for (const auto& hole : hole_arr_)
    {
        for (const auto& hole_piece : hole->split_to_convex())
        {
            if (hole_piece->contains(p_point))
            {
                return false;
            }
        }
    }
    return true;
}

std::optional<FloatPoint> PolylineArea::nearest_point_approx(const FloatPoint& p_from_point) const
{
    double min_distance_square = std::numeric_limits<double>::max();
    std::optional<FloatPoint> result;
    for (const auto& piece : split_to_convex())
    {
        std::optional<FloatPoint> nearest_point = piece->nearest_point_approx(p_from_point);
        if (!nearest_point.has_value())
        {
            continue;
        }
        double distance_square = nearest_point->distance_square(p_from_point);
        if (distance_square < min_distance_square)
        {
            min_distance_square = distance_square;
            result = *nearest_point;
        }
    }
    return result;
}

std::vector<FloatPoint> PolylineArea::corner_approx_arr() const
{
    std::vector<FloatPoint> result = border_shape_->corner_approx_arr();
    for (const auto& hole : hole_arr_)
    {
        std::vector<FloatPoint> hole_corners = hole->corner_approx_arr();
        result.insert(result.end(), hole_corners.begin(), hole_corners.end());
    }
    return result;
}

void PolylineArea::cutout_hole_piece(const TileShape& p_divide_piece, const TileShape& p_hole_piece,
    std::vector<std::unique_ptr<TileShape>>& p_result_pieces)
{
    std::vector<std::unique_ptr<TileShape>> result_pieces = p_divide_piece.cutout(p_hole_piece);
    for (auto& piece : result_pieces)
    {
        if (piece->dimension() == 2)
        {
            p_result_pieces.push_back(std::move(piece));
        }
    }
}

const std::vector<std::unique_ptr<TileShape>>& PolylineArea::split_to_convex() const
{
    if (precalculated_convex_pieces_ == nullptr)
    {
        auto result = std::make_shared<std::vector<std::unique_ptr<TileShape>>>();
        const std::vector<std::unique_ptr<TileShape>>& border_pieces = border_shape_->split_to_convex();

        if (border_pieces.empty())
        {
            precalculated_convex_pieces_ = result;
            return *precalculated_convex_pieces_;
        }

        result->reserve(border_pieces.size());
        for (const auto& border_piece : border_pieces)
        {
            result->push_back(border_piece->clone());
        }

        for (const auto& hole : hole_arr_)
        {
            if (hole->dimension() < 2)
            {
                logger::FRLogger::instance().warn(
                    "PolylineArea.split_to_convex: a hole of dimension < 2 is not supported");
                continue;
            }
            const std::vector<std::unique_ptr<TileShape>>& hole_pieces = hole->split_to_convex();
            if (hole_pieces.empty())
            {
                precalculated_convex_pieces_ = result;
                return *precalculated_convex_pieces_;
            }
            for (const auto& hole_piece : hole_pieces)
            {
                std::vector<std::unique_ptr<TileShape>> new_piece_list;
                for (const auto& divide_piece : *result)
                {
                    cutout_hole_piece(*divide_piece, *hole_piece, new_piece_list);
                }
                result->swap(new_piece_list);
            }
        }
        precalculated_convex_pieces_ = result;
    }
    return *precalculated_convex_pieces_;
}

std::unique_ptr<Area> PolylineArea::translate_by(const Vector& p_vector) const
{
    if (p_vector.is_zero())
    {
        return std::make_unique<PolylineArea>(*this);
    }
    std::vector<std::shared_ptr<PolylineShape>> holes;
    for (const auto& hole : hole_arr_)
    {
        holes.push_back(transform_polyline(hole, [&p_vector](const PolygonShape& p_shape) {
            return std::make_shared<PolygonShape>(p_shape.translate_by(p_vector));
        }, "translate_by"));
    }
    return std::make_unique<PolylineArea>(transform_polyline(border_shape_, [&p_vector](const PolygonShape& p_shape) {
            return std::make_shared<PolygonShape>(p_shape.translate_by(p_vector));
        }, "translate_by"), std::move(holes));
}

std::unique_ptr<Area> PolylineArea::turn_90_degree(int p_factor, const IntPoint& p_pole) const
{
    std::vector<std::shared_ptr<PolylineShape>> holes;
    for (const auto& hole : hole_arr_)
    {
        holes.push_back(transform_polyline(hole, [p_factor, &p_pole](const PolygonShape& p_shape) {
            return std::make_shared<PolygonShape>(p_shape.turn_90_degree(p_factor, p_pole));
        }, "turn_90_degree"));
    }
    return std::make_unique<PolylineArea>(transform_polyline(border_shape_, [p_factor, &p_pole](const PolygonShape& p_shape) {
            return std::make_shared<PolygonShape>(p_shape.turn_90_degree(p_factor, p_pole));
        }, "turn_90_degree"), std::move(holes));
}

std::unique_ptr<Area> PolylineArea::rotate_approx(double p_angle, const FloatPoint& p_pole) const
{
    std::vector<std::shared_ptr<PolylineShape>> holes;
    for (const auto& hole : hole_arr_)
    {
        holes.push_back(transform_polyline(hole, [p_angle, &p_pole](const PolygonShape& p_shape) {
            return std::make_shared<PolygonShape>(p_shape.rotate_approx(p_angle, p_pole));
        }, "rotate_approx"));
    }
    return std::make_unique<PolylineArea>(transform_polyline(border_shape_, [p_angle, &p_pole](const PolygonShape& p_shape) {
            return std::make_shared<PolygonShape>(p_shape.rotate_approx(p_angle, p_pole));
        }, "rotate_approx"), std::move(holes));
}

std::unique_ptr<Area> PolylineArea::mirror_vertical(const IntPoint& p_pole) const
{
    std::vector<std::shared_ptr<PolylineShape>> holes;
    for (const auto& hole : hole_arr_)
    {
        holes.push_back(transform_polyline(hole, [&p_pole](const PolygonShape& p_shape) {
            return std::make_shared<PolygonShape>(p_shape.mirror_vertical(p_pole));
        }, "mirror_vertical"));
    }
    return std::make_unique<PolylineArea>(transform_polyline(border_shape_, [&p_pole](const PolygonShape& p_shape) {
            return std::make_shared<PolygonShape>(p_shape.mirror_vertical(p_pole));
        }, "mirror_vertical"), std::move(holes));
}

std::unique_ptr<Area> PolylineArea::mirror_horizontal(const IntPoint& p_pole) const
{
    std::vector<std::shared_ptr<PolylineShape>> holes;
    for (const auto& hole : hole_arr_)
    {
        holes.push_back(transform_polyline(hole, [&p_pole](const PolygonShape& p_shape) {
            return std::make_shared<PolygonShape>(p_shape.mirror_horizontal(p_pole));
        }, "mirror_horizontal"));
    }
    return std::make_unique<PolylineArea>(transform_polyline(border_shape_, [&p_pole](const PolygonShape& p_shape) {
            return std::make_shared<PolygonShape>(p_shape.mirror_horizontal(p_pole));
        }, "mirror_horizontal"), std::move(holes));
}

std::string PolylineArea::to_string() const
{
    std::ostringstream ss;
    ss << "PolylineArea[border = " << border_shape_->to_string() << ", hole_count = " << hole_arr_.size()
       << "]";
    return ss.str();
}

} // namespace planar
} // namespace geometry
} // namespace freerouting