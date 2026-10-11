#include <gtest/gtest.h>

#include "board/Padstacks.h"
#include "geometry/planar/ConvexTileShape.h"
#include "geometry/planar/IntBox.h"

namespace freerouting::board {
namespace {

using geometry::planar::ConvexShape;
using geometry::planar::ConvexTileShape;
using geometry::planar::IntBox;

std::unique_ptr<geometry::planar::Shape> box_shape(int p_half)
{
    return std::make_unique<ConvexTileShape>(std::make_unique<IntBox>(-p_half, -p_half, p_half, p_half));
}

TEST(PadstackTest, FromAndToLayerAreTheFirstAndLastLayerWithAShape)
{
    std::vector<std::unique_ptr<geometry::planar::Shape>> shapes(4);
    shapes[1] = box_shape(10);
    shapes[2] = box_shape(20);
    const Padstack padstack("inner", 7, std::move(shapes), true, false);
    EXPECT_EQ(7, padstack.get_id());
    EXPECT_TRUE(padstack.is_attach_allowed());
    EXPECT_FALSE(padstack.is_placed_absolute());
    EXPECT_EQ(1, padstack.get_from_layer());
    EXPECT_EQ(2, padstack.get_to_layer());
    EXPECT_EQ(4, padstack.board_layer_count());
    EXPECT_EQ(nullptr, padstack.get_shape(0));
    ASSERT_NE(nullptr, padstack.get_shape(2));
    EXPECT_EQ(40, padstack.get_shape(2)->bounding_box().width());
}

TEST(PadstacksTest, AddAssignsOneBasedIdsAndGetIgnoresCase)
{
    Padstacks padstacks(2);
    std::vector<std::unique_ptr<geometry::planar::Shape>> shapes(2);
    shapes[0] = box_shape(10);
    Padstack* smd = padstacks.add("Rect[T]Pad_100x100", std::move(shapes), false, false);
    std::vector<std::unique_ptr<geometry::planar::Shape>> unnamed(2);
    unnamed[0] = box_shape(5);
    unnamed[1] = box_shape(5);
    Padstack* generated = padstacks.add(std::move(unnamed));

    EXPECT_EQ(1, smd->get_id());
    EXPECT_EQ(2, generated->get_id());
    EXPECT_EQ("padstack#2", generated->get_name());
    EXPECT_EQ(smd, padstacks.get("rect[t]pad_100x100"));
    EXPECT_EQ(generated, padstacks.get(2));
    EXPECT_EQ(nullptr, padstacks.get(0));
    EXPECT_EQ(nullptr, padstacks.get(3));
    EXPECT_EQ(nullptr, padstacks.get("missing"));
}

} // namespace
} // namespace freerouting::board
