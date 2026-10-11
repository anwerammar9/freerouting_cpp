#include "designforms/specctra/parser/Path.h"

#include <algorithm>
#include <climits>
#include <cmath>

#include "datastructures/JavaFormat.h"
#include "designforms/specctra/parser/CoordinateTransform.h"
#include "designforms/specctra/parser/Rectangle.h"
#include "geometry/planar/IntOctagon.h"
#include "geometry/planar/PolygonShape.h"
#include "geometry/planar/TileShape.h"
#include "logger/FRLogger.h"

namespace freerouting::designforms::specctra::parser {

namespace fp = geometry::planar;

namespace {

/** Java (int) Math.round(double): floor(value + 0.5) truncated to int. */
int java_round(double p_value) { return static_cast<int>(std::floor(p_value + 0.5)); }

/** Java FloatPoint.boundingOctagon(FloatPoint[]) (an unnormalized IntOctagon). */
fp::IntOctagon bounding_octagon(const std::vector<fp::FloatPoint>& p_points)
{
    double min_x = INT_MAX, min_y = INT_MAX, max_x = INT_MIN, max_y = INT_MIN;
    double min_ulx = INT_MAX, max_lrx = INT_MIN, min_llx = INT_MAX, max_urx = INT_MIN;
    for (const fp::FloatPoint& current : p_points)
    {
        min_x = std::min(min_x, current.x);
        min_y = std::min(min_y, current.y);
        max_x = std::max(max_x, current.x);
        max_y = std::max(max_y, current.y);
        double tmp = current.x - current.y;
        min_ulx = std::min(min_ulx, tmp);
        max_lrx = std::max(max_lrx, tmp);
        tmp = current.x + current.y;
        min_llx = std::min(min_llx, tmp);
        max_urx = std::max(max_urx, tmp);
    }
    return fp::IntOctagon(static_cast<int64_t>(std::floor(min_x)), static_cast<int64_t>(std::floor(min_y)),
        static_cast<int64_t>(std::ceil(max_x)), static_cast<int64_t>(std::ceil(max_y)),
        static_cast<int64_t>(std::floor(min_ulx)), static_cast<int64_t>(std::ceil(max_lrx)),
        static_cast<int64_t>(std::floor(min_llx)), static_cast<int64_t>(std::ceil(max_urx)));
}

std::unique_ptr<fp::Shape> path_to_board(const std::vector<fp::FloatPoint>& p_corners, double p_offset)
{
    if (p_corners.size() <= 2)
    {
        return as_board_shape(std::make_unique<fp::IntOctagon>(bounding_octagon(p_corners).enlarge(p_offset)));
    }
    std::vector<fp::IntPoint> rounded_corners;
    for (const fp::FloatPoint& corner : p_corners)
    {
        rounded_corners.push_back(corner.round());
    }
    auto result = std::make_unique<fp::PolygonShape>(rounded_corners);
    if (p_offset > 0)
    {
        return as_board_shape(result->bounding_tile()->enlarge(p_offset));
    }
    return result;
}

} // namespace

std::unique_ptr<fp::Shape> PolygonPath::transform_to_board(const CoordinateTransform& p_transform) const
{
    std::vector<fp::FloatPoint> corners;
    for (size_t i = 0; i + 1 < coordinate_arr.size(); i += 2)
    {
        corners.push_back(p_transform.dsn_to_board(coordinate_arr[i], coordinate_arr[i + 1]));
    }
    return path_to_board(corners, p_transform.dsn_to_board(width) / 2);
}

std::unique_ptr<fp::Shape> PolygonPath::transform_to_board_rel(const CoordinateTransform& p_transform) const
{
    std::vector<fp::FloatPoint> corners;
    for (size_t i = 0; i + 1 < coordinate_arr.size(); i += 2)
    {
        corners.push_back(p_transform.dsn_to_board_rel(coordinate_arr[i], coordinate_arr[i + 1]));
    }
    return path_to_board(corners, p_transform.dsn_to_board(width) / 2);
}

std::unique_ptr<Rectangle> PolygonPath::bounding_box() const
{
    const double offset = width / 2;
    std::array<double, 4> bounds{ static_cast<double>(INT_MAX), static_cast<double>(INT_MAX),
        static_cast<double>(INT_MIN), static_cast<double>(INT_MIN) };
    for (size_t i = 0; i < coordinate_arr.size(); ++i)
    {
        if (i % 2 == 0)
        {
            bounds[0] = std::min(bounds[0], coordinate_arr[i] - offset);
            // Java parity: the offset is added outside of max()
            bounds[2] = std::max(bounds[2], coordinate_arr[i]) + offset;
        }
        else
        {
            bounds[1] = std::min(bounds[1], coordinate_arr[i] - offset);
            bounds[3] = std::max(bounds[3], coordinate_arr[i] + offset);
        }
    }
    return std::make_unique<Rectangle>(layer, bounds);
}

std::unique_ptr<fp::Shape> PolylinePath::transform_to_board(const CoordinateTransform& /*p_transform*/) const
{
    logger::FRLogger::instance().warn("PolylinePath.transform_to_board not implemented");
    return nullptr;
}

std::unique_ptr<fp::Shape> PolylinePath::transform_to_board_rel(const CoordinateTransform& /*p_transform*/) const
{
    logger::FRLogger::instance().warn("PolylinePath.transform_to_board_rel not implemented");
    return nullptr;
}

std::unique_ptr<Rectangle> PolylinePath::bounding_box() const
{
    logger::FRLogger::instance().warn("PolylinePath.boundingBox not implemented");
    return nullptr;
}

void PolygonPath::write_scope(
    datastructures::IndentFileWriter& p_file, const datastructures::IdentifierType& p_identifier) const
{
    p_file.start_scope();
    p_file << "path ";
    p_identifier.write(this->layer->name, p_file.stream());
    p_file << " " << datastructures::java_double_to_string(this->width);
    const std::size_t corner_count = coordinate_arr.size() / 2;
    for (std::size_t i = 0; i < corner_count; ++i)
    {
        p_file.new_line();
        p_file << datastructures::java_double_to_string(coordinate_arr[2 * i]) << " "
               << datastructures::java_double_to_string(coordinate_arr[2 * i + 1]);
    }
    p_file.end_scope();
}

void PolygonPath::write_scope_int(
    datastructures::IndentFileWriter& p_file, const datastructures::IdentifierType& p_identifier) const
{
    p_file.start_scope();
    p_file << "path ";
    p_identifier.write(this->layer->name, p_file.stream());
    // Java parity: the width is written unrounded even in the integer variant.
    p_file << " " << datastructures::java_double_to_string(this->width);
    const std::size_t corner_count = coordinate_arr.size() / 2;
    for (std::size_t i = 0; i < corner_count; ++i)
    {
        p_file.new_line();
        p_file << static_cast<int>(java_round(coordinate_arr[2 * i])) << " "
               << static_cast<int>(java_round(coordinate_arr[2 * i + 1]));
    }
    p_file.end_scope();
}

void PolylinePath::write_scope(
    datastructures::IndentFileWriter& p_file, const datastructures::IdentifierType& p_identifier) const
{
    p_file.start_scope();
    p_file << "polyline_path ";
    p_identifier.write(this->layer->name, p_file.stream());
    p_file << " " << datastructures::java_double_to_string(this->width);
    const std::size_t line_count = coordinate_arr.size() / 4;
    for (std::size_t i = 0; i < line_count; ++i)
    {
        p_file.new_line();
        for (std::size_t j = 0; j < 4; ++j)
        {
            p_file << datastructures::java_double_to_string(coordinate_arr[4 * i + j]) << " ";
        }
    }
    p_file.end_scope();
}

void PolylinePath::write_scope_int(
    datastructures::IndentFileWriter& p_file, const datastructures::IdentifierType& p_identifier) const
{
    p_file.start_scope();
    p_file << "polyline_path ";
    p_identifier.write(this->layer->name, p_file.stream());
    // Java parity: the width is written unrounded even in the integer variant.
    p_file << " " << datastructures::java_double_to_string(this->width);
    const std::size_t line_count = coordinate_arr.size() / 4;
    for (std::size_t i = 0; i < line_count; ++i)
    {
        p_file.new_line();
        for (std::size_t j = 0; j < 4; ++j)
        {
            p_file << static_cast<int>(java_round(coordinate_arr[4 * i + j])) << " ";
        }
    }
    p_file.end_scope();
}

} // namespace freerouting::designforms::specctra::parser
