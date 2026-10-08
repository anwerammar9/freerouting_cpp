#include <gtest/gtest.h>

#include "board/BoardLibrary.h"
#include "geometry/planar/ConvexTileShape.h"
#include "geometry/planar/IntBox.h"

namespace freerouting::board {
namespace {

Padstack* add_via(BoardLibrary& p_library, const std::string& p_name, int p_from, int p_to)
{
    std::vector<std::unique_ptr<geometry::planar::Shape>> shapes(p_library.padstacks().board_layer_count());
    for (int i = p_from; i <= p_to; ++i)
    {
        shapes[i] = std::make_unique<geometry::planar::ConvexTileShape>(
            std::make_unique<geometry::planar::IntBox>(-10, -10, 10, 10));
    }
    return p_library.padstacks().add(p_name, std::move(shapes), true, false);
}

TEST(BoardLibraryTest, ViaPadstacksAreUniqueByName)
{
    BoardLibrary library(4);
    Padstack* through = add_via(library, "via_0_3", 0, 3);
    EXPECT_TRUE(library.add_via_padstack(through));
    EXPECT_FALSE(library.add_via_padstack(through));
    EXPECT_EQ(1, library.via_padstack_count());
    EXPECT_EQ(through, library.get_via_padstack(0));
    EXPECT_EQ(through, library.get_via_padstack("via_0_3"));
    EXPECT_EQ(nullptr, library.get_via_padstack(1));
}

TEST(BoardLibraryTest, MirroredViaPadstackSwapsTheLayerRange)
{
    BoardLibrary library(4);
    Padstack* through = add_via(library, "through", 0, 3);
    Padstack* top_blind = add_via(library, "blind_0_1", 0, 1);
    Padstack* bottom_blind = add_via(library, "blind_2_3", 2, 3);
    library.add_via_padstack(through);
    library.add_via_padstack(top_blind);
    library.add_via_padstack(bottom_blind);

    EXPECT_EQ(through, library.get_mirrored_via_padstack(through));
    EXPECT_EQ(bottom_blind, library.get_mirrored_via_padstack(top_blind));
    EXPECT_EQ(top_blind, library.get_mirrored_via_padstack(bottom_blind));
}

} // namespace
} // namespace freerouting::board
