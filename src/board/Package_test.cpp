#include <gtest/gtest.h>

#include "board/Package.h"

namespace freerouting::board {
namespace {

std::vector<Package::Pin> two_pins()
{
    std::vector<Package::Pin> pins;
    pins.push_back(Package::Pin{ "1", 1, geometry::planar::IntVector(-100, 0), 0.0 });
    pins.push_back(Package::Pin{ "2", 1, geometry::planar::IntVector(100, 0), 90.0 });
    return pins;
}

TEST(PackageTest, PinLookupByIndexAndName)
{
    Packages packages;
    const Package* pkg = packages.add("R0805", two_pins());
    EXPECT_EQ(2, pkg->pin_count());
    EXPECT_EQ("2", pkg->get_pin(1)->name);
    EXPECT_EQ(90.0, pkg->get_pin(1)->rotation_in_degree);
    EXPECT_EQ(nullptr, pkg->get_pin(2));
    EXPECT_EQ(1, pkg->get_pin_index("2"));
    EXPECT_EQ(-1, pkg->get_pin_index("3"));
}

TEST(PackagesTest, IdsAreOneBased)
{
    Packages packages;
    Package* first = packages.add("A", two_pins());
    Package* second = packages.add("B", two_pins());
    EXPECT_EQ(1, first->get_id());
    EXPECT_EQ(2, second->get_id());
    EXPECT_EQ(first, packages.get(1));
    EXPECT_EQ(second, packages.get(2));
    EXPECT_EQ(nullptr, packages.get(0));
    EXPECT_EQ(nullptr, packages.get(3));
}

TEST(PackagesTest, GetByNamePrefersTheRequestedSideAndIgnoresCase)
{
    Packages packages;
    Package* front = packages.add("SOT23", two_pins(), {}, {}, {}, {}, {}, {}, true);
    Package* back = packages.add("SOT23", two_pins(), {}, {}, {}, {}, {}, {}, false);
    EXPECT_EQ(front, packages.get("sot23", true));
    EXPECT_EQ(back, packages.get("SOT23", false));
}

TEST(PackagesTest, GetFallsBackToOtherSideAndStripsVariantSuffix)
{
    Packages packages;
    Package* front_only = packages.add("MountingHole", two_pins(), {}, {}, {}, {}, {}, {}, true);
    EXPECT_EQ(front_only, packages.get("MountingHole", false));
    EXPECT_EQ(front_only, packages.get("MountingHole::1", true));
    EXPECT_EQ(nullptr, packages.get("Other", true));
}

} // namespace
} // namespace freerouting::board
