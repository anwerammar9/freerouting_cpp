// Tests for the drill item shapes: Java Pin.getShape / Pin.getPadstackLayer,
// Via.getShape and ShapeSearchTree.calculateTreeShapes(DrillItem).

#include "board/BoardLibrary.h"
#include "board/Component.h"
#include "board/Layer.h"
#include "board/Package.h"
#include "board/Pin.h"
#include "board/RoutingBoard.h"
#include "board/SearchTreeManager.h"
#include "board/ShapeSearchTree.h"
#include "geometry/planar/ConvexTileShape.h"
#include "geometry/planar/IntBox.h"
#include "rules/BoardRules.h"

#include <gtest/gtest.h>

#include <memory>
#include <vector>

namespace freerouting::board {
namespace {

using geometry::planar::ConvexShape;
using geometry::planar::ConvexTileShape;
using geometry::planar::IntBox;
using geometry::planar::IntPoint;
using geometry::planar::IntVector;

std::unique_ptr<RoutingBoard> make_board(int p_layer_count)
{
    std::vector<std::shared_ptr<Layer>> layers;
    for (int i = 0; i < p_layer_count; ++i)
    {
        layers.push_back(std::make_shared<Layer>("L" + std::to_string(i), true));
    }
    LayerStructure layer_structure(layers);
    auto rules = std::make_unique<rules::BoardRules>(layer_structure, 10);
    return std::make_unique<RoutingBoard>(IntBox(-100000, -100000, 100000, 100000), layer_structure, std::move(rules));
}

/** An SMD padstack on layer 0 with a 200 x 100 rectangle centred at the origin. */
Padstack* add_smd_padstack(RoutingBoard& p_board)
{
    std::vector<std::unique_ptr<geometry::planar::Shape>> shapes(p_board.get_layer_count());
    shapes[0] = std::make_unique<ConvexTileShape>(std::make_unique<IntBox>(-100, -50, 100, 50));
    return p_board.library().padstacks().add("smd", std::move(shapes), false, false);
}

Pin* add_pin(RoutingBoard& p_board, Padstack* p_padstack, const IntPoint& p_location, double p_rotation,
    bool p_on_front, const IntVector& p_rel_location, double p_pin_rotation = 0.0)
{
    std::vector<Package::Pin> pins;
    pins.push_back(Package::Pin{ "1", p_padstack->get_id(), p_rel_location, p_pin_rotation });
    Package* pkg = p_board.library().packages().add("PKG", std::move(pins));
    Component* component = p_board.components()->add("U1", p_location, p_rotation, p_on_front, pkg, pkg, false);
    auto* pin = new Pin(component->get_id(), 0, { 1 }, 1, p_board.generate_item_id(), FixedState::NOT_FIXED, &p_board);
    pin->set_padstack(p_padstack);
    return pin;
}

TEST(PinShapeTest, ShapeIsTranslatedByTheRelativeAndComponentLocation)
{
    auto board = make_board(2);
    Pin* pin = add_pin(*board, add_smd_padstack(*board), IntPoint(1000, 2000), 0.0, true, IntVector(300, 0));
    ASSERT_EQ(1, pin->tile_shape_count());
    const geometry::planar::Shape* shape = pin->get_shape(0);
    ASSERT_NE(nullptr, shape);
    EXPECT_EQ(IntPoint(1200, 1950), shape->bounding_box().ll);
    EXPECT_EQ(IntPoint(1400, 2050), shape->bounding_box().ur);
    EXPECT_EQ(IntPoint(1300, 2000), pin->get_center());
    delete pin;
}

TEST(PinShapeTest, ComponentRotationTurnsThePadAroundTheComponentOrigin)
{
    auto board = make_board(2);
    Pin* pin = add_pin(*board, add_smd_padstack(*board), IntPoint(0, 0), 90.0, true, IntVector(300, 0));
    const geometry::planar::Shape* shape = pin->get_shape(0);
    ASSERT_NE(nullptr, shape);
    // (300, 0) turns to (0, 300); the 200 x 100 pad becomes 100 x 200.
    EXPECT_EQ(IntPoint(-50, 200), shape->bounding_box().ll);
    EXPECT_EQ(IntPoint(50, 400), shape->bounding_box().ur);
    delete pin;
}

TEST(PinShapeTest, PinRotationTurnsOnlyThePad)
{
    auto board = make_board(2);
    Pin* pin = add_pin(*board, add_smd_padstack(*board), IntPoint(0, 0), 0.0, true, IntVector(300, 0), 90.0);
    const geometry::planar::Shape* shape = pin->get_shape(0);
    ASSERT_NE(nullptr, shape);
    EXPECT_EQ(IntPoint(250, -100), shape->bounding_box().ll);
    EXPECT_EQ(IntPoint(350, 100), shape->bounding_box().ur);
    delete pin;
}

TEST(PinShapeTest, BackSideMirrorsTheLocationAndUsesTheMirroredPadstackLayer)
{
    auto board = make_board(2);
    Padstack* padstack = add_smd_padstack(*board);
    Pin* pin = add_pin(*board, padstack, IntPoint(0, 0), 0.0, false, IntVector(300, 0));
    // The SMD padstack is on layer 0; on the back side the pin uses padstack
    // layer board_layer_count - index - first_layer - 1.
    EXPECT_EQ(2 - 0 - 0 - 1, pin->get_padstack_layer(0));
    delete pin;
}

TEST(PinShapeTest, TreeShapeIsTheBoundingOctagonOfThePad)
{
    auto board = make_board(2);
    Pin* pin = add_pin(*board, add_smd_padstack(*board), IntPoint(0, 0), 0.0, true, IntVector(0, 0));
    const ShapeSearchTree* tree = board->get_search_tree_manager()->get_default_tree();
    ASSERT_NE(nullptr, tree);
    ASSERT_EQ(1, pin->tree_shape_count(*tree));
    const geometry::planar::TileShape* tree_shape = pin->get_tree_shape(*tree, 0);
    ASSERT_NE(nullptr, tree_shape);
    // The default tree has no clearance compensation.
    EXPECT_EQ(IntPoint(-100, -50), tree_shape->bounding_box().ll);
    EXPECT_EQ(IntPoint(100, 50), tree_shape->bounding_box().ur);
    delete pin;
}

TEST(ViaShapeTest, ShapesAreThePadstackShapesAtTheCenter)
{
    auto board = make_board(2);
    std::vector<std::unique_ptr<geometry::planar::Shape>> shapes(2);
    shapes[0] = std::make_unique<ConvexTileShape>(std::make_unique<IntBox>(-40, -40, 40, 40));
    shapes[1] = std::make_unique<ConvexTileShape>(std::make_unique<IntBox>(-30, -30, 30, 30));
    Padstack* padstack = board->library().padstacks().add("via", std::move(shapes), true, false);
    Via via(padstack, IntPoint(500, 500), { 1 }, 1, board->generate_item_id(), 0, FixedState::NOT_FIXED, true,
        board.get());
    ASSERT_EQ(2, via.tile_shape_count());
    EXPECT_EQ(0, via.shape_layer(0));
    EXPECT_EQ(1, via.shape_layer(1));
    EXPECT_EQ(IntPoint(460, 460), via.get_shape(0)->bounding_box().ll);
    EXPECT_EQ(IntPoint(470, 470), via.get_shape(1)->bounding_box().ll);
}

} // namespace
} // namespace freerouting::board
