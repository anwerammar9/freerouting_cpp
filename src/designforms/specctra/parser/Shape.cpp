#include "designforms/specctra/parser/Shape.h"

#include <array>

#include "designforms/specctra/parser/Circle.h"
#include "designforms/specctra/parser/CoordinateTransform.h"
#include "designforms/specctra/parser/DsnFile.h"
#include "designforms/specctra/parser/Keyword.h"
#include "designforms/specctra/parser/Path.h"
#include "designforms/specctra/parser/Polygon.h"
#include "designforms/specctra/parser/Rectangle.h"
#include "designforms/specctra/parser/ScopeKeyword.h"
#include "geometry/planar/ConvexTileShape.h"
#include "geometry/planar/PolygonShape.h"
#include "geometry/planar/CircleArea.h"
#include "geometry/planar/PolylineArea.h"
#include "geometry/planar/TileShape.h"
#include "logger/FRLogger.h"

namespace freerouting::designforms::specctra::parser {

namespace fp = geometry::planar;

namespace {

void warn(const std::string& p_message) { logger::FRLogger::instance().warn(p_message); }

/** Java Shape.getLayer: pcb, signal or a layer of the structure; nullptr if unknown. */
std::shared_ptr<Layer> get_layer(const LayerStructure* p_layer_structure, const std::string& p_layer_name)
{
    if (p_layer_name == Keyword::PCB_SCOPE->get_name())
    {
        return Layer::pcb();
    }
    if (p_layer_name == Keyword::SIGNAL->get_name())
    {
        return Layer::signal();
    }
    if (p_layer_structure == nullptr)
    {
        warn("Shape.read_circle_scope: layerStructure != null expected");
        return nullptr;
    }
    const int layer_index = p_layer_structure->get_no(p_layer_name);
    if (layer_index < 0 || layer_index >= static_cast<int>(p_layer_structure->layers.size()))
    {
        warn("Shape.read_circle_scope: layer with name '" + p_layer_name + "' not found in layer structure.");
        return nullptr;
    }
    return p_layer_structure->layers[layer_index];
}

/**
 * Java casts the board shape of an area border/hole to PolylineShape; Java
 * IntBox/IntOctagon/Simplex are PolylineShapes, the C++ tile shapes are
 * converted to a PolygonShape with the same corners.
 */
std::shared_ptr<fp::PolylineShape> to_polyline_shape_impl(std::unique_ptr<fp::Shape> p_shape)
{
    if (p_shape == nullptr)
    {
        return nullptr;
    }
    if (auto* polyline_shape = dynamic_cast<fp::PolylineShape*>(p_shape.get()))
    {
        p_shape.release();
        return std::shared_ptr<fp::PolylineShape>(polyline_shape);
    }
    if (const auto* tile = dynamic_cast<const fp::ConvexTileShape*>(p_shape.get()))
    {
        std::vector<fp::IntPoint> corners;
        for (int i = 0; i < tile->tile().border_line_count(); ++i)
        {
            corners.push_back(tile->tile().corner(i));
        }
        return std::make_shared<fp::PolygonShape>(corners);
    }
    return nullptr;
}

std::unique_ptr<fp::Shape> transform_area(const std::vector<std::unique_ptr<Shape>>& p_area,
    const CoordinateTransform& p_transform, bool p_relative)
{
    const int hole_count = static_cast<int>(p_area.size()) - 1;
    if (hole_count <= -1)
    {
        warn("Shape.transform_area_to_board: area.size() > 0 expected");
        return nullptr;
    }
    for (const auto& shape : p_area)
    {
        if (shape == nullptr)
        {
            // Java throws a NullPointerException for a shape that could not be read.
            warn("Shape.transform_area_to_board: shape expected");
            return nullptr;
        }
    }
    auto transform = [&](const Shape& p_shape) {
        return p_relative ? p_shape.transform_to_board_rel(p_transform) : p_shape.transform_to_board(p_transform);
    };
    std::unique_ptr<fp::Shape> boundary_shape = transform(*p_area[0]);
    if (hole_count == 0)
    {
        return boundary_shape;
    }
    // Area with holes
    std::shared_ptr<fp::PolylineShape> border = to_polyline_shape_impl(std::move(boundary_shape));
    if (border == nullptr)
    {
        warn("Shape.transform_area_to_board: PolylineShape expected");
        return nullptr;
    }
    std::vector<std::shared_ptr<fp::PolylineShape>> holes;
    for (int i = 1; i <= hole_count; ++i)
    {
        std::shared_ptr<fp::PolylineShape> hole = to_polyline_shape_impl(transform(*p_area[i]));
        if (hole == nullptr)
        {
            warn("Shape.transform_area_to_board: PolylineShape expected");
            return nullptr;
        }
        holes.push_back(std::move(hole));
    }
    return std::make_unique<fp::PolylineArea>(std::move(border), std::move(holes));
}

/** Collects the numbers of a token list; false (with a warning) at the first non-number. */
bool to_numbers(const std::vector<Token>& p_tokens, size_t p_from, std::vector<double>& p_result)
{
    for (size_t i = p_from; i < p_tokens.size(); ++i)
    {
        if (!p_tokens[i].is_number())
        {
            return false;
        }
        p_result.push_back(p_tokens[i].number());
    }
    return true;
}

} // namespace

std::unique_ptr<fp::Shape> as_board_shape(std::unique_ptr<fp::TileShape> p_tile)
{
    return std::make_unique<fp::ConvexTileShape>(std::move(p_tile));
}

std::unique_ptr<Shape> Shape::read_scope(IJFlexScanner& p_scanner, const LayerStructure* p_layer_structure)
{
    Token next_token = p_scanner.next_token();
    if (next_token.is(Keyword::OPEN_BRACKET))
    {
        // overread the open bracket
        next_token = p_scanner.next_token();
    }
    return read_scope_from_keyword(p_scanner, next_token, p_layer_structure);
}

std::unique_ptr<Shape> Shape::read_scope_from_keyword(
    IJFlexScanner& p_scanner, const Token& p_keyword, const LayerStructure* p_layer_structure)
{
    if (p_keyword.is(Keyword::RECTANGLE))
    {
        return read_rectangle_scope(p_scanner, p_layer_structure);
    }
    if (p_keyword.is(Keyword::POLYGON))
    {
        return read_polygon_scope(p_scanner, p_layer_structure);
    }
    if (p_keyword.is(Keyword::CIRCLE))
    {
        return read_circle_scope(p_scanner, p_layer_structure);
    }
    if (p_keyword.is(Keyword::POLYGON_PATH))
    {
        return read_polygon_path_scope(p_scanner, p_layer_structure);
    }
    if (p_keyword.is(Keyword::POLYLINE_PATH))
    {
        return read_polyline_path_scope(p_scanner, p_layer_structure);
    }
    ScopeKeyword::skip_scope(p_scanner);
    return nullptr;
}

std::unique_ptr<Shape> Shape::read_polyline_path_scope(IJFlexScanner& p_scanner, const LayerStructure* p_layers)
{
    const std::string layer_name = p_scanner.next_string();
    std::shared_ptr<Layer> layer = get_layer(p_layers, layer_name);
    std::vector<Token> corner_list;
    // read the width and the corners of the path
    for (;;)
    {
        Token next_token = p_scanner.next_token();
        if (next_token.is(Keyword::CLOSED_BRACKET))
        {
            break;
        }
        if (next_token.is_eof())
        {
            // Java loops forever on null tokens; stop at the end of the file instead.
            break;
        }
        corner_list.push_back(std::move(next_token));
    }
    if (corner_list.size() < 5)
    {
        warn("PolylinePath.read_scope: too few numbers in scope at '" + p_scanner.get_scope_identifier() + "'");
        return nullptr;
    }
    if (!corner_list[0].is_number())
    {
        warn("PolylinePath.read_scope: number expected at '" + p_scanner.get_scope_identifier() + "'");
        return nullptr;
    }
    std::vector<double> corners;
    if (!to_numbers(corner_list, 1, corners))
    {
        warn("Shape.read_polygon_path_scope: number expected at '" + p_scanner.get_scope_identifier() + "'");
        return nullptr;
    }
    return std::make_unique<PolylinePath>(layer, corner_list[0].number(), std::move(corners));
}

std::optional<Shape::ReadAreaScopeResult> Shape::read_area_scope(
    IJFlexScanner& p_scanner, const LayerStructure* p_layer_structure, bool p_skip_window_scopes)
{
    ReadAreaScopeResult result;
    bool result_ok = true;
    Token next_token = p_scanner.next_token();
    if (next_token.is_string())
    {
        p_scanner.set_scope_identifier(next_token.string_value);
        result.area_name = next_token.string_value;
    }
    std::unique_ptr<Shape> curr_shape = read_scope(p_scanner, p_layer_structure);
    if (curr_shape == nullptr)
    {
        warn("Shape.read_area_scope: could not read shape at '" + p_scanner.get_scope_identifier() + "'");
        result_ok = false;
    }
    result.shape_list.push_back(std::move(curr_shape));
    next_token = Token::end_of_file();
    for (;;)
    {
        const Token prev_token = next_token;
        next_token = p_scanner.next_token();
        if (next_token.is_eof())
        {
            warn("Shape.read_area_scope: unexpected end of file at '" + p_scanner.get_scope_identifier() + "'");
            return std::nullopt;
        }
        if (next_token.is(Keyword::CLOSED_BRACKET))
        {
            // end of scope
            break;
        }
        if (prev_token.is(Keyword::OPEN_BRACKET))
        {
            // a new scope is expected
            if (next_token.is(Keyword::WINDOW) && !p_skip_window_scopes)
            {
                result.shape_list.push_back(read_scope(p_scanner, p_layer_structure));
                // overread closing bracket
                next_token = p_scanner.next_token();
                if (!next_token.is(Keyword::CLOSED_BRACKET))
                {
                    warn("Shape.read_area_scope: closed bracket expected at '" + p_scanner.get_scope_identifier() + "'");
                    return std::nullopt;
                }
            }
            else if (next_token.is(Keyword::CLEARANCE_CLASS))
            {
                result.clearance_class_name = DsnFile::read_string_scope(p_scanner).value_or("");
            }
            else
            {
                // skip unknown scope
                ScopeKeyword::skip_scope(p_scanner);
            }
        }
    }
    if (!result_ok)
    {
        return std::nullopt;
    }
    return result;
}

std::unique_ptr<Shape> Shape::read_rectangle_scope(IJFlexScanner& p_scanner, const LayerStructure* p_layers)
{
    const std::string layer_name = p_scanner.next_string();
    std::shared_ptr<Layer> rect_layer = get_layer(p_layers, layer_name);
    if (rect_layer == nullptr)
    {
        rect_layer = get_layer(p_layers, Keyword::SIGNAL->get_name());
    }
    std::array<double, 4> rect_coor{};
    // fill the rectangle
    for (int i = 0; i < 4; ++i)
    {
        const Token next_token = p_scanner.next_token();
        if (!next_token.is_number())
        {
            warn("Shape.read_rectangle_scope: number expected at '" + p_scanner.get_scope_identifier() + "'");
            return nullptr;
        }
        rect_coor[i] = next_token.number();
    }
    // overread the closing bracket
    if (!p_scanner.next_token().is(Keyword::CLOSED_BRACKET))
    {
        warn("Shape.read_rectangle_scope ) expected at '" + p_scanner.get_scope_identifier() + "'");
        return nullptr;
    }
    if (rect_layer == nullptr)
    {
        return nullptr;
    }
    return std::make_unique<Rectangle>(rect_layer, rect_coor);
}

std::unique_ptr<Shape> Shape::read_polygon_scope(IJFlexScanner& p_scanner, const LayerStructure* p_layers)
{
    std::shared_ptr<Layer> polygon_layer;
    bool layer_ok = true;
    Token next_token = p_scanner.next_token();
    if (next_token.is(Keyword::PCB_SCOPE))
    {
        polygon_layer = Layer::pcb();
    }
    else if (next_token.is(Keyword::SIGNAL))
    {
        polygon_layer = Layer::signal();
    }
    else
    {
        if (p_layers == nullptr)
        {
            warn("Shape.read_polygon_scope: only layer types pcb or signal expected at '"
                + p_scanner.get_scope_identifier() + "'");
            return nullptr;
        }
        if (!next_token.is_string())
        {
            warn("Shape.read_polygon_scope: layer name string expected at '" + p_scanner.get_scope_identifier() + "'");
            return nullptr;
        }
        const int layer_index = p_layers->get_no(next_token.string_value);
        if (layer_index < 0 || layer_index >= static_cast<int>(p_layers->layers.size()))
        {
            warn("Shape.read_polygon_scope: layer name '" + next_token.string_value
                + "' not found in layer structure  at '" + p_scanner.get_scope_identifier() + "'");
            layer_ok = false;
        }
        else
        {
            polygon_layer = p_layers->layers[layer_index];
        }
    }
    // overread the aperture width
    p_scanner.next_token();
    std::vector<Token> coor_list;
    // read the coordinates of the polygon
    for (;;)
    {
        next_token = p_scanner.next_token();
        if (next_token.is_eof())
        {
            warn("Shape.read_polygon_scope: unexpected end of file at '" + p_scanner.get_scope_identifier() + "'");
            return nullptr;
        }
        if (next_token.is(Keyword::OPEN_BRACKET))
        {
            // unknown scope
            ScopeKeyword::skip_scope(p_scanner);
            next_token = p_scanner.next_token();
        }
        if (next_token.is(Keyword::CLOSED_BRACKET))
        {
            break;
        }
        coor_list.push_back(std::move(next_token));
    }
    if (!layer_ok)
    {
        return nullptr;
    }
    std::vector<double> coor_arr;
    if (!to_numbers(coor_list, 0, coor_arr))
    {
        warn("Shape.read_polygon_scope: number expected at '" + p_scanner.get_scope_identifier() + "'");
        return nullptr;
    }
    return std::make_unique<Polygon>(polygon_layer, std::move(coor_arr));
}

std::unique_ptr<Shape> Shape::read_circle_scope(IJFlexScanner& p_scanner, const LayerStructure* p_layers)
{
    const std::string layer_name = p_scanner.next_string();
    std::shared_ptr<Layer> circle_layer = get_layer(p_layers, layer_name);
    if (circle_layer == nullptr)
    {
        warn("Circle.read_circle_scope: layer with name '" + layer_name + "' not found in layer structure at '"
            + p_scanner.get_scope_identifier() + "'");
    }
    // fill the coordinates
    std::array<double, 3> circle_coor{};
    int curr_index = 0;
    for (;;)
    {
        const Token next_token = p_scanner.next_token();
        if (next_token.is(Keyword::CLOSED_BRACKET))
        {
            break;
        }
        if (curr_index > 2)
        {
            warn("Shape.read_circle_scope: closed bracket expected at '" + p_scanner.get_scope_identifier() + "'");
            return nullptr;
        }
        if (!next_token.is_number())
        {
            warn("Shape.read_circle_scope: number expected at '" + p_scanner.get_scope_identifier() + "'");
            return nullptr;
        }
        circle_coor[curr_index] = next_token.number();
        ++curr_index;
    }
    if (circle_layer == nullptr)
    {
        return nullptr;
    }
    return std::make_unique<Circle>(circle_layer, circle_coor);
}

std::unique_ptr<Shape> Shape::read_polygon_path_scope(IJFlexScanner& p_scanner, const LayerStructure* p_layers)
{
    const std::string layer_name = p_scanner.next_string();
    std::shared_ptr<Layer> layer = get_layer(p_layers, layer_name);
    std::vector<Token> corner_list;
    // read the width and the corners of the path
    for (;;)
    {
        Token next_token = p_scanner.next_token();
        if (next_token.is(Keyword::OPEN_BRACKET))
        {
            // unknown scope
            ScopeKeyword::skip_scope(p_scanner);
            next_token = p_scanner.next_token();
        }
        if (next_token.is(Keyword::CLOSED_BRACKET))
        {
            break;
        }
        if (next_token.is_eof())
        {
            // Java loops forever on null tokens; stop at the end of the file instead.
            break;
        }
        corner_list.push_back(std::move(next_token));
    }
    // corner_list contains width + coordinate pairs
    if (corner_list.size() < 5)
    {
        // Single-point paths are not valid traces, skip them
        logger::FRLogger::instance().debug("Shape.read_polygon_path_scope: skipping path with too few coordinates "
                                           "(need at least 2 points) at '" + p_scanner.get_scope_identifier() + "'");
        return nullptr;
    }
    if (layer == nullptr)
    {
        return nullptr;
    }
    if (!corner_list[0].is_number())
    {
        warn("Shape.read_polygon_path_scope: number expected at '" + p_scanner.get_scope_identifier() + "'");
        return nullptr;
    }
    std::vector<double> coordinate_arr;
    if (!to_numbers(corner_list, 1, coordinate_arr))
    {
        warn("Shape.read_polygon_path_scope: number expected at '" + p_scanner.get_scope_identifier() + "'");
        return nullptr;
    }
    return std::make_unique<PolygonPath>(layer, corner_list[0].number(), std::move(coordinate_arr));
}

std::unique_ptr<fp::Shape> Shape::transform_area_to_board(
    const std::vector<std::unique_ptr<Shape>>& p_area, const CoordinateTransform& p_transform)
{
    return transform_area(p_area, p_transform, false);
}

std::unique_ptr<fp::Shape> Shape::transform_area_to_board_rel(
    const std::vector<std::unique_ptr<Shape>>& p_area, const CoordinateTransform& p_transform)
{
    return transform_area(p_area, p_transform, true);
}

std::shared_ptr<fp::PolylineShape> to_polyline_shape(std::unique_ptr<fp::Shape> p_shape)
{
    return to_polyline_shape_impl(std::move(p_shape));
}

std::shared_ptr<fp::Area> to_board_area(std::unique_ptr<fp::Shape> p_shape)
{
    if (p_shape == nullptr)
    {
        return nullptr;
    }
    if (auto* area = dynamic_cast<fp::Area*>(p_shape.get()))
    {
        p_shape.release();
        return std::shared_ptr<fp::Area>(area);
    }
    if (const auto* circle = dynamic_cast<const fp::Circle*>(p_shape.get()))
    {
        return std::make_shared<fp::CircleArea>(*circle); // Java Circle is an Area
    }
    std::shared_ptr<fp::PolylineShape> border = to_polyline_shape_impl(std::move(p_shape));
    if (border == nullptr)
    {
        return nullptr;
    }
    return std::make_shared<fp::PolylineArea>(std::move(border));
}

} // namespace freerouting::designforms::specctra::parser
