#include "autoroute/ExpansionRoom.h"
#include <gtest/gtest.h>

namespace freerouting {
namespace autoroute {

TEST(IncompleteFreeSpaceExpansionRoomTest, Construction) {
    CompleteFreeSpaceExpansionRoom source_room(1, 10, geometry::planar::IntBox(0, 0, 200, 200));
    geometry::planar::IntBox shape(50, 50, 150, 150);

    IncompleteFreeSpaceExpansionRoom room(&source_room, 1, shape);

    EXPECT_EQ(room.get_layer(), 1);
    EXPECT_EQ(room.get_shape(), shape);
    // Port of Java IncompleteFreeSpaceExpansionRoom.getId(): a stable hash of
    // the room shape and the room layer.
    EXPECT_EQ(room.get_room_id(), static_cast<int>(31 * shape.get_id() + 1));
}

TEST(IncompleteFreeSpaceExpansionRoomTest, RoomIdIsStableAndDependsOnShapeAndLayer) {
    geometry::planar::IntBox shape(50, 50, 150, 150);
    geometry::planar::IntBox other_shape(50, 50, 150, 160);
    CompleteFreeSpaceExpansionRoom source_room(1, 10, geometry::planar::IntBox(0, 0, 200, 200));

    IncompleteFreeSpaceExpansionRoom room(&source_room, 1, shape);
    IncompleteFreeSpaceExpansionRoom same_room(&source_room, 1, shape);
    IncompleteFreeSpaceExpansionRoom other_layer(&source_room, 2, shape);
    IncompleteFreeSpaceExpansionRoom other_shape_room(&source_room, 1, other_shape);

    EXPECT_EQ(room.get_room_id(), same_room.get_room_id());
    EXPECT_NE(room.get_room_id(), other_layer.get_room_id());
    EXPECT_NE(room.get_room_id(), other_shape_room.get_room_id());
}

TEST(IncompleteFreeSpaceExpansionRoomTest, DoorsStartEmpty) {
    CompleteFreeSpaceExpansionRoom source_room(0, 0, geometry::planar::IntBox(0, 0, 100, 100));
    geometry::planar::IntBox shape(10, 10, 50, 50);

    IncompleteFreeSpaceExpansionRoom room(&source_room, 0, shape);
    EXPECT_TRUE(room.get_doors().empty());
}

TEST(IncompleteFreeSpaceExpansionRoomTest, AddAndRemoveDoor) {
    CompleteFreeSpaceExpansionRoom room_a(0, 1, geometry::planar::IntBox(0, 0, 100, 100));
    CompleteFreeSpaceExpansionRoom room_b(0, 2, geometry::planar::IntBox(100, 100, 200, 200));
    IncompleteFreeSpaceExpansionRoom incomplete(&room_a, 0, geometry::planar::IntBox(10, 10, 50, 50));

    ExpansionDoor door(&room_a, &room_b, 2);
    incomplete.add_door(&door);
    EXPECT_EQ(incomplete.get_doors().size(), 1u);

    bool removed = incomplete.remove_door(&door);
    EXPECT_TRUE(removed);
    EXPECT_TRUE(incomplete.get_doors().empty());
}

TEST(IncompleteFreeSpaceExpansionRoomTest, GetShapeReturnsBoundingBox) {
    geometry::planar::IntBox shape(25, 25, 75, 75);
    CompleteFreeSpaceExpansionRoom source_room(0, 0, geometry::planar::IntBox(0, 0, 200, 200));
    IncompleteFreeSpaceExpansionRoom room(&source_room, 0, shape);

    EXPECT_EQ(room.get_shape(), shape);
}

} // namespace autoroute
} // namespace freerouting
