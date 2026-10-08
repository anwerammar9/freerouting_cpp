#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "designforms/specctra/parser/IJFlexScanner.h"
#include "designforms/specctra/parser/Layer.h"
#include "geometry/planar/Shape.h"

namespace freerouting::designforms::specctra::parser {

class CoordinateTransform;
class Rectangle;

/**
 * A shape in a Specctra DSN file. Port of Java parser.Shape.
 *
 * transform_to_board / transform_to_board_rel return board geometry. Box,
 * octagon and simplex results are wrapped in geometry::planar::ConvexTileShape,
 * because the C++ TileShape is not a geometry Shape (Java: IntBox is a Shape).
 */
class Shape
{
public:
    explicit Shape(std::shared_ptr<Layer> p_layer) : layer(std::move(p_layer)) {}
    virtual ~Shape() = default;

    /** Transforms this DSN shape to a board shape. Java transformToBoard. */
    virtual std::unique_ptr<geometry::planar::Shape> transform_to_board(
        const CoordinateTransform& p_transform) const = 0;
    /** Transforms the relative (vector) coordinates of this shape. Java transformToBoardRel. */
    virtual std::unique_ptr<geometry::planar::Shape> transform_to_board_rel(
        const CoordinateTransform& p_transform) const = 0;
    /** The smallest axis parallel rectangle containing this shape, or nullptr. Java boundingBox. */
    virtual std::unique_ptr<Rectangle> bounding_box() const = 0;

    /**
     * Reads a shape scope. If p_layer_structure is nullptr, only the layers
     * pcb and signal are expected. Returns nullptr on error. Java Shape.readScope.
     */
    static std::unique_ptr<Shape> read_scope(IJFlexScanner& p_scanner, const LayerStructure* p_layer_structure);
    /** Reads a shape scope whose keyword token was already read. Java readScopeFromKeyword. */
    static std::unique_ptr<Shape> read_scope_from_keyword(
        IJFlexScanner& p_scanner, const Token& p_keyword, const LayerStructure* p_layer_structure);

    static std::unique_ptr<Shape> read_rectangle_scope(IJFlexScanner& p_scanner, const LayerStructure* p_layers);
    static std::unique_ptr<Shape> read_polygon_scope(IJFlexScanner& p_scanner, const LayerStructure* p_layers);
    static std::unique_ptr<Shape> read_circle_scope(IJFlexScanner& p_scanner, const LayerStructure* p_layers);
    static std::unique_ptr<Shape> read_polygon_path_scope(IJFlexScanner& p_scanner, const LayerStructure* p_layers);
    static std::unique_ptr<Shape> read_polyline_path_scope(IJFlexScanner& p_scanner, const LayerStructure* p_layers);

    /** Result of read_area_scope. Java Shape.ReadAreaScopeResult. */
    struct ReadAreaScopeResult
    {
        /** May be generated later on, if empty. */
        std::string area_name;
        /** The first shape is the border, the others are holes (windows). */
        std::vector<std::unique_ptr<Shape>> shape_list;
        std::string clearance_class_name;
    };

    /**
     * Reads a shape which may contain holes. Returns std::nullopt on error.
     * Java Shape.readAreaScope.
     */
    static std::optional<ReadAreaScopeResult> read_area_scope(
        IJFlexScanner& p_scanner, const LayerStructure* p_layer_structure, bool p_skip_window_scopes);

    /**
     * Transforms a shape with holes to the board; the first shape is the
     * border. Returns nullptr on error. Java Shape.transformAreaToBoard(Rel).
     */
    static std::unique_ptr<geometry::planar::Shape> transform_area_to_board(
        const std::vector<std::unique_ptr<Shape>>& p_area, const CoordinateTransform& p_transform);
    static std::unique_ptr<geometry::planar::Shape> transform_area_to_board_rel(
        const std::vector<std::unique_ptr<Shape>>& p_area, const CoordinateTransform& p_transform);

    std::shared_ptr<Layer> layer;
};

/** Java (IntBox) wrapped as a board Shape. */
std::unique_ptr<geometry::planar::Shape> as_board_shape(std::unique_ptr<geometry::planar::TileShape> p_tile);

} // namespace freerouting::designforms::specctra::parser
