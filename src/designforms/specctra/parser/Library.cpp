#include "designforms/specctra/parser/Library.h"

#include <regex>
#include <string>
#include <vector>

#include "board/BoardLibrary.h"
#include "board/Padstacks.h"
#include "board/RoutingBoard.h"
#include "designforms/specctra/parser/CoordinateTransform.h"
#include "designforms/specctra/parser/DsnFile.h"
#include "designforms/specctra/parser/Keyword.h"
#include "designforms/specctra/parser/Layer.h"
#include "designforms/specctra/parser/ReadScopeParameter.h"
#include "designforms/specctra/parser/Shape.h"
#include "geometry/planar/ConvexTileShape.h"
#include "geometry/planar/PolygonShape.h"
#include "geometry/planar/Simplex.h"
#include "logger/FRLogger.h"

namespace freerouting::designforms::specctra::parser {

namespace fp = geometry::planar;

namespace {

void warn(const std::string& p_message) { logger::FRLogger::instance().warn(p_message); }

/** Java name.replaceAll("\\.\\d+", ""). */
std::string clean_padstack_name(const std::string& p_name)
{
    static const std::regex kSuffix("\\.\\d+");
    return std::regex_replace(p_name, kSuffix, "");
}

int shape_dimension(const fp::Shape& p_shape)
{
    if (const auto* circle = dynamic_cast<const fp::Circle*>(&p_shape))
    {
        return circle->dimension();
    }
    if (const auto* tile = dynamic_cast<const fp::ConvexTileShape*>(&p_shape))
    {
        return tile->tile().dimension();
    }
    return 2;
}

std::unique_ptr<fp::Shape> clone_pad_shape(const fp::Shape& p_shape)
{
    if (const auto* circle = dynamic_cast<const fp::Circle*>(&p_shape))
    {
        return std::make_unique<fp::Circle>(*circle);
    }
    if (const auto* tile = dynamic_cast<const fp::ConvexTileShape*>(&p_shape))
    {
        return as_board_shape(tile->tile().clone());
    }
    return nullptr;
}

/**
 * Java readPadstackScope, per shape: the board shape of a pad as convex shape (Java ConvexShape;
 * the C++ Circle is not a ConvexShape but plays that role). Non convex shapes become the single
 * convex piece of their convex hull; shapes that are not areas are enlarged by 1 once.
 */
std::unique_ptr<fp::Shape> to_convex_pad_shape(
    const Shape& p_pad_shape, const CoordinateTransform& p_transform, const std::string& p_padstack_name,
    IJFlexScanner& p_scanner)
{
    std::unique_ptr<fp::Shape> current_shape = p_pad_shape.transform_to_board_rel(p_transform);
    std::unique_ptr<fp::Shape> convex_shape;
    if (dynamic_cast<const fp::Circle*>(current_shape.get()) != nullptr
        || dynamic_cast<const fp::ConvexTileShape*>(current_shape.get()) != nullptr)
    {
        convex_shape = std::move(current_shape);
    }
    else if (const auto* polygon = dynamic_cast<const fp::PolygonShape*>(current_shape.get()))
    {
        const fp::PolygonShape hull = polygon->convex_hull();
        const std::vector<std::unique_ptr<fp::TileShape>>& convex_shapes = hull.split_to_convex();
        if (convex_shapes.size() != 1)
        {
            warn("Library.read_padstack_scope: convex shape expected at '" + p_scanner.get_scope_identifier() + "'");
        }
        if (convex_shapes.empty())
        {
            return nullptr; // Java would throw on convexShapes[0]
        }
        std::unique_ptr<fp::TileShape> piece = convex_shapes.front()->clone();
        if (dynamic_cast<const fp::Simplex*>(piece.get()) != nullptr)
        {
            piece = piece->simplify();
        }
        convex_shape = as_board_shape(std::move(piece));
    }
    else
    {
        warn("Library.read_padstack_scope: convex shape expected at '" + p_scanner.get_scope_identifier() + "'");
        return nullptr;
    }

    if (shape_dimension(*convex_shape) < 2)
    {
        warn("Library.read_padstack_scope: the shape of padstack '" + p_padstack_name
            + "' is not an area. We will enlarge it as a workaround, but it may result in unintended consequences.");
        // enlarge the shape a little bit, so that it is an area
        if (const auto* circle = dynamic_cast<const fp::Circle*>(convex_shape.get()))
        {
            convex_shape = std::make_unique<fp::Circle>(circle->offset(1));
        }
        else if (const auto* tile = dynamic_cast<const fp::ConvexTileShape*>(convex_shape.get()))
        {
            convex_shape = tile->offset(1);
        }
        if (convex_shape == nullptr || shape_dimension(*convex_shape) < 2)
        {
            return nullptr;
        }
    }
    return convex_shape;
}

} // namespace

bool Library::read_padstack_scope(IJFlexScanner& p_scanner, const LayerStructure& p_layer_structure,
    const CoordinateTransform& p_transform, board::Padstacks& p_board_padstacks)
{
    std::string padstack_name;
    bool is_drillable = true;
    bool placed_absolute = false;
    std::vector<std::unique_ptr<Shape>> shape_list;

    Token next_token = p_scanner.next_token();
    if (next_token.is_string())
    {
        padstack_name = clean_padstack_name(next_token.string_value);
        p_scanner.set_scope_identifier(padstack_name);
    }
    else
    {
        warn("Library.read_padstack_scope: unexpected padstack identifier at '" + p_scanner.get_scope_identifier() + "'");
        return false;
    }
    while (!next_token.is(Keyword::CLOSED_BRACKET))
    {
        const Token prev_token = next_token;
        next_token = p_scanner.next_token();
        if (next_token.is_eof())
        {
            return false; // Java: IOException / endless loop on null; stop here
        }
        if (prev_token.is(Keyword::OPEN_BRACKET))
        {
            if (next_token.is(Keyword::SHAPE))
            {
                std::unique_ptr<Shape> current_shape = Shape::read_scope(p_scanner, &p_layer_structure);
                if (current_shape != nullptr)
                {
                    shape_list.push_back(std::move(current_shape));
                }
                // overread the closing bracket and unknown scopes
                Token current_next_token = p_scanner.next_token();
                while (current_next_token.is(Keyword::OPEN_BRACKET))
                {
                    ScopeKeyword::skip_scope(p_scanner);
                    current_next_token = p_scanner.next_token();
                }
                if (!current_next_token.is(Keyword::CLOSED_BRACKET))
                {
                    warn("Library.read_padstack_scope: closing bracket expected at '"
                        + p_scanner.get_scope_identifier() + "'");
                    return false;
                }
            }
            else if (next_token.is(Keyword::ATTACH))
            {
                is_drillable = DsnFile::read_on_off_scope(p_scanner);
            }
            else if (next_token.is(Keyword::ABSOLUTE))
            {
                placed_absolute = DsnFile::read_on_off_scope(p_scanner);
            }
            else
            {
                ScopeKeyword::skip_scope(p_scanner);
            }
        }
    }
    if (p_board_padstacks.get(padstack_name) != nullptr)
    {
        return true; // padstack exists already
    }
    if (shape_list.empty())
    {
        warn("Library.read_padstack_scope: shape not found for padstack with name '" + padstack_name + "'");
        return true;
    }
    const int layer_count = static_cast<int>(p_layer_structure.layers.size());
    std::vector<std::unique_ptr<fp::Shape>> padstack_shapes(layer_count);
    for (const std::unique_ptr<Shape>& pad_shape : shape_list)
    {
        std::unique_ptr<fp::Shape> padstack_shape =
            to_convex_pad_shape(*pad_shape, p_transform, padstack_name, p_scanner);
        if (pad_shape->layer == Layer::pcb() || pad_shape->layer == Layer::signal())
        {
            // Java Arrays.fill: the same shape on every layer (a copy per layer in C++)
            for (int i = 0; i < layer_count; ++i)
            {
                padstack_shapes[i] = padstack_shape != nullptr ? clone_pad_shape(*padstack_shape) : nullptr;
            }
        }
        else
        {
            const int shape_layer = p_layer_structure.get_no(pad_shape->layer->name);
            if (shape_layer < 0 || shape_layer >= layer_count)
            {
                warn("Library.read_padstack_scope: layer number found at '" + p_scanner.get_scope_identifier() + "'");
                return false;
            }
            padstack_shapes[shape_layer] = std::move(padstack_shape);
        }
    }
    p_board_padstacks.add(padstack_name, std::move(padstack_shapes), is_drillable, placed_absolute);
    return true;
}

bool Library::read_scope(ReadScopeParameter& p_par) const
{
    board::RoutingBoard* board = p_par.get_board();
    if (board == nullptr || p_par.layer_structure == nullptr || !p_par.coordinate_transform.has_value())
    {
        // Java would throw a NullPointerException: the structure scope did not create a board.
        warn("Library.read_scope: board not initialized");
        return false;
    }
    board->library().padstacks() = board::Padstacks(board->get_layer_count());
    Token next_token;
    for (;;)
    {
        const Token prev_token = next_token;
        next_token = p_par.scanner.next_token();
        if (next_token.is_eof())
        {
            warn("Library.read_scope: unexpected end of file at '" + p_par.scanner.get_scope_identifier() + "'");
            return false;
        }
        if (next_token.is(Keyword::CLOSED_BRACKET))
        {
            break; // end of scope
        }
        if (prev_token.is(Keyword::OPEN_BRACKET))
        {
            if (next_token.is(Keyword::PADSTACK))
            {
                if (!read_padstack_scope(p_par.scanner, *p_par.layer_structure, *p_par.coordinate_transform,
                        board->library().padstacks()))
                {
                    return false;
                }
            }
            else
            {
                // (image ...) -> Package.readScope and the package creation follow with T111.
                ScopeKeyword::skip_scope(p_par.scanner);
            }
        }
    }
    return true;
}

} // namespace freerouting::designforms::specctra::parser
