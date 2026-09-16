#include <gtest/gtest.h>
#include "autoroute/ExpansionDrill.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/Point.h"

namespace fr = freerouting;
using fr::geometry::planar::IntBox;
using fr::geometry::planar::IntPoint;
using fr::autoroute::ExpansionDrill;

TEST(ExpansionDrillTest, Construction)
{
    IntBox shape(IntPoint(0, 0), IntPoint(100, 100));
    IntPoint location(50, 50);
    ExpansionDrill drill(shape, location, 0, 3);

    EXPECT_EQ(drill.first_layer, 0);
    EXPECT_EQ(drill.last_layer, 3);
    EXPECT_EQ(drill.location.x, 50);
    EXPECT_EQ(drill.location.y, 50);
    EXPECT_EQ(drill.get_dimension(), 2);
    EXPECT_EQ(drill.room_arr.size(), 4u);
}

TEST(ExpansionDrillTest, GetShape)
{
    IntBox shape(IntPoint(10, 20), IntPoint(30, 40));
    IntPoint location(20, 30);
    ExpansionDrill drill(shape, location, 1, 2);

    EXPECT_EQ(drill.get_shape(), shape);
}

TEST(ExpansionDrillTest, MazeSearchElements)
{
    IntBox shape(IntPoint(0, 0), IntPoint(10, 10));
    IntPoint location(5, 5);
    ExpansionDrill drill(shape, location, 0, 2);

    EXPECT_EQ(drill.maze_search_element_count(), 3);
    EXPECT_NE(drill.get_maze_search_element(0), nullptr);
    EXPECT_NE(drill.get_maze_search_element(1), nullptr);
    EXPECT_NE(drill.get_maze_search_element(2), nullptr);
    EXPECT_EQ(drill.get_maze_search_element(3), nullptr);
    EXPECT_EQ(drill.get_maze_search_element(-1), nullptr);
}

TEST(ExpansionDrillTest, Reset)
{
    IntBox shape(IntPoint(0, 0), IntPoint(10, 10));
    IntPoint location(5, 5);
    ExpansionDrill drill(shape, location, 0, 1);

    drill.reset();
    EXPECT_NE(drill.get_maze_search_element(0), nullptr);
    EXPECT_NE(drill.get_maze_search_element(1), nullptr);
}

TEST(ExpansionDrillTest, OtherRoomAlwaysNull)
{
    IntBox shape(IntPoint(0, 0), IntPoint(10, 10));
    IntPoint location(5, 5);
    ExpansionDrill drill(shape, location, 0, 0);

    EXPECT_EQ(drill.other_room(nullptr), nullptr);
}
