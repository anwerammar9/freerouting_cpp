// Tests for the pin center, name and relative location port of Java Pin.

#include "board/Pin.h"

#include "board/Component.h"
#include "board/Layer.h"
#include "board/Padstack.h"
#include "board/RoutingBoard.h"
#include "board/BoardLibrary.h"
#include "board/Package.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/PolygonShape.h"
#include "rules/BoardRules.h"

#include <gtest/gtest.h>

#include <memory>
#include <optional>
#include <vector>

namespace freerouting {
namespace board {

namespace {

std::shared_ptr<RoutingBoard> make_routing_board()
{
    std::vector<std::shared_ptr<Layer>> layers;
    layers.push_back(std::make_shared<Layer>("Top", true));
    LayerStructure layer_structure(layers);
    auto board_rules = std::make_unique<rules::BoardRules>(layer_structure, 10);
    return std::make_shared<RoutingBoard>(geometry::planar::IntBox(0, 0, 100000, 100000),
        layer_structure, std::move(board_rules), nullptr);
}

void add_component(RoutingBoard* p_board, const std::string& p_name, int p_no,
    const geometry::planar::IntPoint& p_location, int p_pin_no, int p_rel_x, int p_rel_y,
    double p_rotation = 0.0, bool p_on_front = true)
{
    (void)p_no;
    std::vector<Package::Pin> pins;
    pins.push_back(Package::Pin{ std::to_string(p_pin_no), 0, geometry::planar::IntVector(p_rel_x, p_rel_y), 0.0 });
    Package* lib_package = p_board->library().packages().add(p_name, std::move(pins));
    p_board->components()->add(p_name, p_location, p_rotation, p_on_front, lib_package, lib_package, false);
}

Pin* make_pin(RoutingBoard* p_board, int p_component_no, int p_pin_no)
{
    return new Pin(p_component_no, p_pin_no, { 1 }, 1, p_board->generate_item_id(),
        FixedState::NOT_FIXED, p_board);
}

} // namespace

TEST(PinCenterTest, CenterIsCalculatedFromTheComponentLocation)
{
    auto board = make_routing_board();
    add_component(board.get(), "U1", 1, geometry::planar::IntPoint(2000, 3000), 3, 100, 50);
    Pin* pin = make_pin(board.get(), 1, 0);

    // Component IDs are 1-based like in Java, so the pin refers to component 1.
    EXPECT_EQ(pin->center_is_set(), false);
    EXPECT_EQ(pin->relative_location(), geometry::planar::IntPoint(100, 50));
    EXPECT_EQ(pin->get_center(), geometry::planar::IntPoint(2100, 3050));
    // The center is cached after the first calculation.
    EXPECT_EQ(pin->center_is_set(), true);
}

TEST(PinCenterTest, CenterWithoutComponentStaysAtTheDrillItemCenter)
{
    auto board = make_routing_board();
    Pin* pin = make_pin(board.get(), 0, 0);
    pin->set_center(geometry::planar::IntPoint(500, 600));

    EXPECT_EQ(pin->get_center(), geometry::planar::IntPoint(500, 600));
    EXPECT_EQ(pin->name(), std::nullopt);
}

TEST(PinCenterTest, RelativeLocationIsMirroredOnTheBackSide)
{
    auto board = make_routing_board();
    add_component(board.get(), "U1", 1, geometry::planar::IntPoint(0, 0), 1, 100, 50, 0.0, false);
    Pin* pin = make_pin(board.get(), 1, 0);

    // flip_style_rotate_first is false, so the mirror is applied first.
    EXPECT_EQ(pin->relative_location(), geometry::planar::IntPoint(-100, 50));
}

TEST(PinCenterTest, RelativeLocationIsMirroredAfterTheRotationOnRotateFirst)
{
    auto board = make_routing_board();
    board->set_flip_style_rotate_first(true);
    add_component(board.get(), "U1", 1, geometry::planar::IntPoint(0, 0), 1, 100, 50, 0.0, false);
    Pin* pin = make_pin(board.get(), 1, 0);

    // With rotate first the component is turned first and mirrored afterwards.
    EXPECT_EQ(pin->relative_location(), geometry::planar::IntPoint(-100, 50));
}

TEST(PinCenterTest, RelativeLocationIsRotatedByTheComponent)
{
    auto board = make_routing_board();
    add_component(board.get(), "U1", 1, geometry::planar::IntPoint(0, 0), 1, 100, 50, 90.0);
    Pin* pin = make_pin(board.get(), 1, 0);

    // A 90 degree rotation turns (100, 50) into (-50, 100).
    EXPECT_EQ(pin->relative_location(), geometry::planar::IntPoint(-50, 100));
}

TEST(PinCenterTest, NameIsThePackagePinNumber)
{
    auto board = make_routing_board();
    add_component(board.get(), "U1", 1, geometry::planar::IntPoint(0, 0), 7, 100, 50);
    Pin* pin = make_pin(board.get(), 1, 0);

    const std::optional<std::string> name = pin->name();
    ASSERT_TRUE(name.has_value());
    EXPECT_EQ(*name, "7");
}

} // namespace board
} // namespace freerouting