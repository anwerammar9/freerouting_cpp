#include "designforms/specctra/parser/Library.h"

#include <regex>
#include <cmath>
#include <cctype>
#include <string>
#include <vector>

#include "board/BoardLibrary.h"
#include "board/Package.h"
#include "board/Padstacks.h"
#include "board/RoutingBoard.h"
#include "designforms/specctra/parser/CoordinateTransform.h"
#include "designforms/specctra/parser/DsnFile.h"
#include "designforms/specctra/parser/Keyword.h"
#include "designforms/specctra/parser/Layer.h"
#include "designforms/specctra/parser/Package.h"
#include "designforms/specctra/parser/Path.h"
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

/** Java Library.arePackagePinsIdentical. */
bool are_package_pins_identical(const board::Package* p_package, const std::vector<board::Package::Pin>& p_pins)
{
    if (p_package == nullptr)
    {
        return false; // Java: (pkg1 == null) == (p2 == null) with a non-null pin array
    }
    if (p_package->pin_count() != static_cast<int>(p_pins.size()))
    {
        return false;
    }
    for (size_t i = 0; i < p_pins.size(); ++i)
    {
        const board::Package::Pin* pin1 = p_package->get_pin(static_cast<int>(i));
        const board::Package::Pin& pin2 = p_pins[i];
        if (pin1 == nullptr)
        {
            return false;
        }
        if (pin1->name != pin2.name || pin1->padstack_id != pin2.padstack_id)
        {
            return false;
        }
        const fp::FloatPoint loc1 = pin1->relative_location.to_float();
        const fp::FloatPoint loc2 = pin2.relative_location.to_float();
        if (std::abs(loc1.x - loc2.x) > 0.001 || std::abs(loc1.y - loc2.y) > 0.001)
        {
            return false;
        }
        if (std::abs(pin1->rotation_in_degree - pin2.rotation_in_degree) > 0.001)
        {
            return false;
        }
    }
    return true;
}

bool equals_ignore_case(const std::string& p_a, const std::string& p_b)
{
    if (p_a.size() != p_b.size())
    {
        return false;
    }
    for (size_t i = 0; i < p_a.size(); ++i)
    {
        if (std::tolower(static_cast<unsigned char>(p_a[i])) != std::tolower(static_cast<unsigned char>(p_b[i])))
        {
            return false;
        }
    }
    return true;
}

/** Java Library.generateMissingKeepoutNames: if any keepout has no name, all get generated names. */
void generate_missing_keepout_names(const std::string& p_keepout_type, std::vector<Shape::ReadAreaScopeResult>& p_list)
{
    bool all_names_existing = true;
    for (const Shape::ReadAreaScopeResult& keepout : p_list)
    {
        if (keepout.area_name.empty())
        {
            all_names_existing = false;
            break;
        }
    }
    if (all_names_existing)
    {
        return;
    }
    int current_name_index = 1;
    for (Shape::ReadAreaScopeResult& keepout : p_list)
    {
        keepout.area_name = p_keepout_type + std::to_string(current_name_index);
        ++current_name_index;
    }
}

int java_round(double p_value) { return static_cast<int>(std::floor(p_value + 0.5)); }

std::vector<board::Package::Keepout> to_board_keepouts(
    const std::vector<Shape::ReadAreaScopeResult>& p_keepouts, const CoordinateTransform& p_transform)
{
    std::vector<board::Package::Keepout> result;
    for (const Shape::ReadAreaScopeResult& keepout : p_keepouts)
    {
        board::Package::Keepout board_keepout;
        board_keepout.name = keepout.area_name;
        board_keepout.area = to_board_area(Shape::transform_area_to_board_rel(keepout.shape_list, p_transform));
        board_keepout.layer = keepout.shape_list.front()->layer->no;
        result.push_back(std::move(board_keepout));
    }
    return result;
}

} // namespace

void Library::create_board_packages(std::vector<Package>& p_package_list, ReadScopeParameter& p_par,
    board::RoutingBoard& p_board)
{
    const CoordinateTransform& transform = *p_par.coordinate_transform;
    p_board.library().packages() = board::Packages();
    for (Package& current_package : p_package_list)
    {
        std::vector<board::Package::Pin> pins;
        for (const Package::PinInfo& pin_info : current_package.pin_info_arr)
        {
            const int rel_x = java_round(transform.dsn_to_board(pin_info.rel_coor[0]));
            const int rel_y = java_round(transform.dsn_to_board(pin_info.rel_coor[1]));
            const std::string cleaned_lookup_name = clean_padstack_name(pin_info.padstack_name);
            const board::Padstack* board_padstack = p_board.library().padstacks().get(cleaned_lookup_name);
            if (board_padstack == nullptr)
            {
                warn("Library.read_scope: board padstack '" + pin_info.padstack_name + "' (cleaned: '"
                    + cleaned_lookup_name + "') not found at '" + p_par.scanner.get_scope_identifier() + "'");
                return; // Java returns false from readScope here; the caller already ended the scope
            }
            pins.push_back(board::Package::Pin{
                pin_info.pin_name, board_padstack->get_id(), fp::IntVector(rel_x, rel_y), pin_info.rotation});
        }
        std::vector<std::shared_ptr<fp::Shape>> outlines;
        std::vector<double> outline_widths;
        std::vector<bool> outline_is_closed;
        for (const std::unique_ptr<Shape>& current_shape : current_package.outline)
        {
            outlines.push_back(std::shared_ptr<fp::Shape>(current_shape->transform_to_board_rel(transform)));
            if (const auto* path = dynamic_cast<const Path*>(current_shape.get()))
            {
                outline_widths.push_back(path->width);
                const std::vector<double>& coords = path->coordinate_arr;
                bool closed = false;
                if (coords.size() >= 4)
                {
                    closed = coords[0] == coords[coords.size() - 2] && coords[1] == coords[coords.size() - 1];
                }
                outline_is_closed.push_back(closed);
            }
            else
            {
                outline_widths.push_back(0.0);
                outline_is_closed.push_back(true); // non-path shapes (polygons, rects) are closed
            }
        }
        generate_missing_keepout_names("keepout_", current_package.keepouts);
        generate_missing_keepout_names("via_keepout_", current_package.via_keepouts);
        generate_missing_keepout_names("place_keepout_", current_package.place_keepouts);
        std::vector<board::Package::Keepout> keepouts = to_board_keepouts(current_package.keepouts, transform);
        std::vector<board::Package::Keepout> via_keepouts = to_board_keepouts(current_package.via_keepouts, transform);
        std::vector<board::Package::Keepout> place_keepouts =
            to_board_keepouts(current_package.place_keepouts, transform);

        static const std::regex kVariantSuffix("::\\d+$");
        const std::string base_package_name = std::regex_replace(current_package.name, kVariantSuffix, "");
        for (int suffix = 0;; ++suffix)
        {
            const std::string test_name = suffix == 0 ? base_package_name : base_package_name + "::" + std::to_string(suffix);
            const board::Package* existing_package = p_board.library().packages().get(test_name, current_package.is_front);
            if (existing_package == nullptr || !equals_ignore_case(existing_package->get_name(), test_name))
            {
                p_board.library().packages().add(test_name, pins, outlines, outline_widths, outline_is_closed, keepouts,
                    via_keepouts, place_keepouts, current_package.is_front);
                break;
            }
            if (are_package_pins_identical(existing_package, pins))
            {
                break;
            }
        }
    }
}

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
    std::vector<Package> package_list;
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
            else if (next_token.is(Keyword::IMAGE))
            {
                std::optional<Package> current_package = Package::read_scope(p_par.scanner, *p_par.layer_structure);
                if (!current_package.has_value())
                {
                    return false;
                }
                package_list.push_back(std::move(*current_package));
            }
            else
            {
                ScopeKeyword::skip_scope(p_par.scanner);
            }
        }
    }
    create_board_packages(package_list, p_par, *board);
    return true;
}

} // namespace freerouting::designforms::specctra::parser
