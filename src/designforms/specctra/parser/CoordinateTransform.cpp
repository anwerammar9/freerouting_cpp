#include "designforms/specctra/parser/CoordinateTransform.h"

#include <array>

#include "designforms/specctra/parser/Circle.h"
#include "designforms/specctra/parser/Layer.h"
#include "designforms/specctra/parser/Polygon.h"
#include "designforms/specctra/parser/Rectangle.h"
#include "geometry/planar/ConvexTileShape.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/PolylineShape.h"
#include "geometry/planar/Shape.h"
#include "geometry/planar/TileShape.h"
#include "geometry/planar/Vector.h"
#include "logger/FRLogger.h"

namespace freerouting::designforms::specctra::parser {

namespace fp = geometry::planar;

std::vector<double> CoordinateTransform::board_to_dsn(const std::vector<fp::FloatPoint>& p_points) const
{
    std::vector<double> result(2 * p_points.size());
    for (std::size_t i = 0; i < p_points.size(); ++i)
    {
        result[2 * i] = board_to_dsn(p_points[i].x) + base_x_;
        result[2 * i + 1] = board_to_dsn(p_points[i].y) + base_y_;
    }
    return result;
}

std::vector<double> CoordinateTransform::board_to_dsn_rel(const std::vector<fp::FloatPoint>& p_points) const
{
    std::vector<double> result(2 * p_points.size());
    for (std::size_t i = 0; i < p_points.size(); ++i)
    {
        result[2 * i] = board_to_dsn(p_points[i].x);
        result[2 * i + 1] = board_to_dsn(p_points[i].y);
    }
    return result;
}

std::vector<double> CoordinateTransform::board_to_dsn(const fp::IntBox& p_box) const
{
    // Java parity: ll/ur are divided by the scale factor directly (same as boardToDsn).
    return { p_box.ll.x / scale_factor_ + base_x_, p_box.ll.y / scale_factor_ + base_y_,
        p_box.ur.x / scale_factor_ + base_x_, p_box.ur.y / scale_factor_ + base_y_ };
}

std::vector<double> CoordinateTransform::board_to_dsn_rel(const fp::IntBox& p_box) const
{
    return { p_box.ll.x / scale_factor_, p_box.ll.y / scale_factor_, p_box.ur.x / scale_factor_,
        p_box.ur.y / scale_factor_ };
}

std::vector<double> CoordinateTransform::board_to_dsn(const fp::Vector& p_vector) const
{
    const fp::FloatPoint value = p_vector.to_float();
    return { board_to_dsn(value.x), board_to_dsn(value.y) };
}

std::vector<double> CoordinateTransform::board_to_dsn(const std::vector<fp::Line>& p_lines) const
{
    std::vector<double> result(4 * p_lines.size());
    for (std::size_t i = 0; i < p_lines.size(); ++i)
    {
        const fp::FloatPoint a = p_lines[i].a.to_float();
        const fp::FloatPoint b = p_lines[i].b.to_float();
        result[4 * i] = board_to_dsn(a.x) + base_x_;
        result[4 * i + 1] = board_to_dsn(a.y) + base_y_;
        result[4 * i + 2] = board_to_dsn(b.x) + base_x_;
        result[4 * i + 3] = board_to_dsn(b.y) + base_y_;
    }
    return result;
}

std::unique_ptr<Shape> CoordinateTransform::board_to_dsn(
    const fp::IntBox& p_box, std::shared_ptr<Layer> p_layer) const
{
    const std::vector<double> coordinates = board_to_dsn(p_box);
    return std::make_unique<Rectangle>(std::move(p_layer),
        std::array<double, 4>{ coordinates[0], coordinates[1], coordinates[2], coordinates[3] });
}

std::unique_ptr<Shape> CoordinateTransform::board_to_dsn_rel(
    const fp::IntBox& p_box, std::shared_ptr<Layer> p_layer) const
{
    const std::vector<double> coordinates = board_to_dsn_rel(p_box);
    return std::make_unique<Rectangle>(std::move(p_layer),
        std::array<double, 4>{ coordinates[0], coordinates[1], coordinates[2], coordinates[3] });
}

std::unique_ptr<Shape> CoordinateTransform::board_to_dsn(
    const fp::Shape& p_board_shape, std::shared_ptr<Layer> p_layer) const
{
    // Java checks IntBox first (IntBox is a PolylineShape in Java); in the C++
    // port a board IntBox arrives wrapped in a ConvexTileShape.
    if (const auto* tile = dynamic_cast<const fp::ConvexTileShape*>(&p_board_shape))
    {
        if (tile->tile().is_IntBox())
        {
            return board_to_dsn(dynamic_cast<const fp::IntBox&>(tile->tile()), std::move(p_layer));
        }
        // IntOctagon/Simplex are PolylineShapes in Java: write their corners as a polygon.
        const fp::TileShape& shape = tile->tile();
        std::vector<fp::FloatPoint> corners;
        corners.reserve(static_cast<std::size_t>(shape.border_line_count()));
        for (int i = 0; i < shape.border_line_count(); ++i)
        {
            corners.push_back(shape.corner(i).to_float());
        }
        return std::make_unique<Polygon>(std::move(p_layer), board_to_dsn(corners));
    }
    if (const auto* polyline = dynamic_cast<const fp::PolylineShape*>(&p_board_shape))
    {
        return std::make_unique<Polygon>(std::move(p_layer), board_to_dsn(polyline->corner_approx_arr()));
    }
    if (const auto* circle = dynamic_cast<const fp::Circle*>(&p_board_shape))
    {
        const double diameter = 2 * board_to_dsn(circle->radius);
        const std::vector<double> center = board_to_dsn(circle->center.to_float());
        return std::make_unique<Circle>(
            std::move(p_layer), std::array<double, 3>{ diameter, center[0], center[1] });
    }
    logger::FRLogger::instance().warn("CoordinateTransform.board_to_dsn not yet implemented for boardShape");
    return nullptr;
}

std::unique_ptr<Shape> CoordinateTransform::board_to_dsn_rel(
    const fp::Shape& p_board_shape, std::shared_ptr<Layer> p_layer) const
{
    if (const auto* tile = dynamic_cast<const fp::ConvexTileShape*>(&p_board_shape))
    {
        if (tile->tile().is_IntBox())
        {
            return board_to_dsn_rel(dynamic_cast<const fp::IntBox&>(tile->tile()), std::move(p_layer));
        }
        const fp::TileShape& shape = tile->tile();
        std::vector<fp::FloatPoint> corners;
        corners.reserve(static_cast<std::size_t>(shape.border_line_count()));
        for (int i = 0; i < shape.border_line_count(); ++i)
        {
            corners.push_back(shape.corner(i).to_float());
        }
        return std::make_unique<Polygon>(std::move(p_layer), board_to_dsn_rel(corners));
    }
    if (const auto* polyline = dynamic_cast<const fp::PolylineShape*>(&p_board_shape))
    {
        return std::make_unique<Polygon>(std::move(p_layer), board_to_dsn_rel(polyline->corner_approx_arr()));
    }
    if (const auto* circle = dynamic_cast<const fp::Circle*>(&p_board_shape))
    {
        // Java parity: the diameter uses boardToDsn (not rel), only the centre is relative.
        const double diameter = 2 * board_to_dsn(circle->radius);
        const std::vector<double> center = board_to_dsn_rel(circle->center.to_float());
        return std::make_unique<Circle>(
            std::move(p_layer), std::array<double, 3>{ diameter, center[0], center[1] });
    }
    logger::FRLogger::instance().warn("CoordinateTransform.board_to_dsn not yet implemented for boardShape");
    return nullptr;
}

} // namespace freerouting::designforms::specctra::parser
