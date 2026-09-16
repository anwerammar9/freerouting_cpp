#include "autoroute/ExpansionRoom.h"
#include <gtest/gtest.h>
#include <memory>
#include <vector>

namespace freerouting {
namespace autoroute {

class CompleteExpansionRoomTest : public ::testing::Test {
protected:
    void SetUp() override {
        layer_ = 1;
        room_id_ = 42;
        room_ = std::make_unique<CompleteExpansionRoom>(layer_, room_id_);
    }

    int layer_;
    int room_id_;
    std::unique_ptr<CompleteExpansionRoom> room_;
};

TEST_F(CompleteExpansionRoomTest, BasicConstruction) {
    EXPECT_NE(room_, nullptr);
    EXPECT_EQ(room_->get_layer(), layer_);
    EXPECT_EQ(room_->get_room_id(), room_id_);
}

TEST_F(CompleteExpansionRoomTest, GetShapeDefaultIsInfinite) {
    geometry::planar::IntBox shape = room_->get_shape();
    EXPECT_EQ(shape, geometry::planar::IntBox::INFINITE);
}

TEST_F(CompleteExpansionRoomTest, DoorsStartEmpty) {
    auto doors = room_->get_doors();
    EXPECT_TRUE(doors.empty());
}

TEST_F(CompleteExpansionRoomTest, TargetDoorsStartEmpty) {
    EXPECT_TRUE(room_->target_doors().empty());
}

TEST_F(CompleteExpansionRoomTest, AddTargetDoor) {
    ExpansionRoom* room_a = room_.get();
    auto other_room = std::make_unique<CompleteFreeSpaceExpansionRoom>(layer_, 99,
        geometry::planar::IntBox(0, 0, 100, 100));

    ExpansionDoor door(room_a, other_room.get(), 2);
    room_->add_target_door(&door);
    EXPECT_EQ(room_->target_doors().size(), 1u);
}

TEST_F(CompleteExpansionRoomTest, AddNullTargetDoorIsNoop) {
    room_->add_target_door(nullptr);
    EXPECT_TRUE(room_->target_doors().empty());
}

TEST_F(CompleteExpansionRoomTest, DoorsCalculatedFlag) {
    EXPECT_FALSE(room_->all_doors_calculated());
    room_->set_doors_calculated(true);
    EXPECT_TRUE(room_->all_doors_calculated());
    room_->set_doors_calculated(false);
    EXPECT_FALSE(room_->all_doors_calculated());
}

TEST_F(CompleteExpansionRoomTest, MultipleRoomsAreIndependent) {
    auto room2 = std::make_unique<CompleteExpansionRoom>(layer_, 99);
    EXPECT_EQ(room_->get_room_id(), room_id_);
    EXPECT_EQ(room2->get_room_id(), 99);
    EXPECT_NE(room_->get_room_id(), room2->get_room_id());
}

TEST_F(CompleteExpansionRoomTest, ConstAccessors) {
    const CompleteExpansionRoom* croom = room_.get();
    EXPECT_EQ(croom->get_layer(), layer_);
    EXPECT_EQ(croom->get_room_id(), room_id_);
    EXPECT_EQ(croom->all_doors_calculated(), false);
}

} // namespace autoroute
} // namespace freerouting
