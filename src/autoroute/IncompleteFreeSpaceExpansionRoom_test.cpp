/**
 * @file IncompleteFreeSpaceExpansionRoom_test.cpp
 * @brief Unit tests for IncompleteFreeSpaceExpansionRoom class
 */

#include "autoroute/IncompleteFreeSpaceExpansionRoom.h"
#include <gtest/gtest.h>
#include <memory>

namespace freerouting {
namespace autoroute {

// Mock implementations for testing
class MockTileShape : public TileShape {
public:
    // Mock implementation
};

class MockTargetItemExpansionDoor : public TargetItemExpansionDoor {
public:
    // Mock implementation
};

class IncompleteFreeSpaceExpansionRoomTest : public ::testing::Test {
protected:
    void SetUp() override {
        shape = std::make_shared<MockTileShape>();
        contained_shape = std::make_shared<MockTileShape>();
        layer = 1;
    }
    
    std::shared_ptr<MockTileShape> shape;
    std::shared_ptr<MockTileShape> contained_shape;
    int layer;
};

// Test constructor with valid parameters
TEST_F(IncompleteFreeSpaceExpansionRoomTest, ConstructorWithValidParameters) {
    EXPECT_NO_THROW({
        IncompleteFreeSpaceExpansionRoom room(shape, layer, contained_shape);
    });
    
    IncompleteFreeSpaceExpansionRoom room(shape, layer, contained_shape);
    
    // Test that contained shape is set correctly
    EXPECT_EQ(room.get_contained_shape(), contained_shape);
    
    // Test that target doors is empty
    auto target_doors = room.get_target_doors();
    EXPECT_TRUE(target_doors.empty());
}

// Test constructor with null shape
TEST_F(IncompleteFreeSpaceExpansionRoomTest, ConstructorWithNullShape) {
    EXPECT_NO_THROW({
        IncompleteFreeSpaceExpansionRoom room(nullptr, layer, contained_shape);
    });
    
    IncompleteFreeSpaceExpansionRoom room(nullptr, layer, contained_shape);
    
    // Test that contained shape is still set correctly
    EXPECT_EQ(room.get_contained_shape(), contained_shape);
    
    // Test that target doors is empty
    auto target_doors = room.get_target_doors();
    EXPECT_TRUE(target_doors.empty());
}

// Test constructor with null contained shape
TEST_F(IncompleteFreeSpaceExpansionRoomTest, ConstructorWithNullContainedShape) {
    EXPECT_NO_THROW({
        IncompleteFreeSpaceExpansionRoom room(shape, layer, nullptr);
    });
    
    IncompleteFreeSpaceExpansionRoom room(shape, layer, nullptr);
    
    // Test that contained shape is null
    EXPECT_EQ(room.get_contained_shape(), nullptr);
    
    // Test that target doors is empty
    auto target_doors = room.get_target_doors();
    EXPECT_TRUE(target_doors.empty());
}

// Test constructor with both null shapes
TEST_F(IncompleteFreeSpaceExpansionRoomTest, ConstructorWithBothNullShapes) {
    EXPECT_NO_THROW({
        IncompleteFreeSpaceExpansionRoom room(nullptr, layer, nullptr);
    });
    
    IncompleteFreeSpaceExpansionRoom room(nullptr, layer, nullptr);
    
    // Test that contained shape is null
    EXPECT_EQ(room.get_contained_shape(), nullptr);
    
    // Test that target doors is empty
    auto target_doors = room.get_target_doors();
    EXPECT_TRUE(target_doors.empty());
}

// Test get_contained_shape functionality
TEST_F(IncompleteFreeSpaceExpansionRoomTest, GetContainedShapeFunctionality) {
    IncompleteFreeSpaceExpansionRoom room(shape, layer, contained_shape);
    
    // Should return the contained shape
    EXPECT_EQ(room.get_contained_shape(), contained_shape);
    
    // Should be consistent across calls
    EXPECT_EQ(room.get_contained_shape(), room.get_contained_shape());
}

// Test set_contained_shape functionality
TEST_F(IncompleteFreeSpaceExpansionRoomTest, SetContainedShapeFunctionality) {
    IncompleteFreeSpaceExpansionRoom room(shape, layer, contained_shape);
    
    // Initially should have the original contained shape
    EXPECT_EQ(room.get_contained_shape(), contained_shape);
    
    // Set a new contained shape
    auto new_contained_shape = std::make_shared<MockTileShape>();
    room.set_contained_shape(new_contained_shape);
    
    // Should now have the new contained shape
    EXPECT_EQ(room.get_contained_shape(), new_contained_shape);
    EXPECT_NE(room.get_contained_shape(), contained_shape);
}

// Test set_contained_shape to null
TEST_F(IncompleteFreeSpaceExpansionRoomTest, SetContainedShapeToNull) {
    IncompleteFreeSpaceExpansionRoom room(shape, layer, contained_shape);
    
    // Initially should have the original contained shape
    EXPECT_EQ(room.get_contained_shape(), contained_shape);
    
    // Set contained shape to null
    room.set_contained_shape(nullptr);
    
    // Should now be null
    EXPECT_EQ(room.get_contained_shape(), nullptr);
}

// Test get_target_doors functionality
TEST_F(IncompleteFreeSpaceExpansionRoomTest, GetTargetDoorsFunctionality) {
    IncompleteFreeSpaceExpansionRoom room(shape, layer, contained_shape);
    
    // Should always return empty collection for incomplete rooms
    auto target_doors = room.get_target_doors();
    EXPECT_TRUE(target_doors.empty());
    EXPECT_EQ(target_doors.size(), 0);
    
    // Should be consistent across calls
    auto target_doors2 = room.get_target_doors();
    EXPECT_TRUE(target_doors2.empty());
    EXPECT_EQ(target_doors.size(), target_doors2.size());
}

// Test target doors after changing contained shape
TEST_F(IncompleteFreeSpaceExpansionRoomTest, TargetDoorsAfterChangingContainedShape) {
    IncompleteFreeSpaceExpansionRoom room(shape, layer, contained_shape);
    
    // Should start with empty target doors
    auto target_doors = room.get_target_doors();
    EXPECT_TRUE(target_doors.empty());
    
    // Change contained shape
    auto new_contained_shape = std::make_shared<MockTileShape>();
    room.set_contained_shape(new_contained_shape);
    
    // Should still have empty target doors
    target_doors = room.get_target_doors();
    EXPECT_TRUE(target_doors.empty());
}

// Test multiple rooms
TEST_F(IncompleteFreeSpaceExpansionRoomTest, MultipleRooms) {
    // Create multiple rooms with different parameters
    IncompleteFreeSpaceExpansionRoom room1(shape, layer, contained_shape);
    IncompleteFreeSpaceExpansionRoom room2(shape, layer + 1, contained_shape);
    IncompleteFreeSpaceExpansionRoom room3(shape, layer + 2, nullptr);
    
    // Each should have their own contained shape
    EXPECT_EQ(room1.get_contained_shape(), contained_shape);
    EXPECT_EQ(room2.get_contained_shape(), contained_shape);
    EXPECT_EQ(room3.get_contained_shape(), nullptr);
    
    // All should have empty target doors
    EXPECT_TRUE(room1.get_target_doors().empty());
    EXPECT_TRUE(room2.get_target_doors().empty());
    EXPECT_TRUE(room3.get_target_doors().empty());
}

// Test const correctness
TEST_F(IncompleteFreeSpaceExpansionRoomTest, ConstCorrectness) {
    const IncompleteFreeSpaceExpansionRoom room(shape, layer, contained_shape);
    
    // Should be able to call const methods
    EXPECT_EQ(room.get_contained_shape(), contained_shape);
    
    auto target_doors = room.get_target_doors();
    EXPECT_TRUE(target_doors.empty());
}

// Test edge cases
TEST_F(IncompleteFreeSpaceExpansionRoomTest, EdgeCases) {
    // Test with different layer values
    std::vector<int> layers = {-1, 0, 1, 100, 999};
    
    for (int test_layer : layers) {
        EXPECT_NO_THROW({
            IncompleteFreeSpaceExpansionRoom room(shape, test_layer, contained_shape);
            EXPECT_EQ(room.get_contained_shape(), contained_shape);
            EXPECT_TRUE(room.get_target_doors().empty());
        });
    }
}

// Test inheritance from FreeSpaceExpansionRoom
TEST_F(IncompleteFreeSpaceExpansionRoomTest, InheritanceFromFreeSpaceExpansionRoom) {
    IncompleteFreeSpaceExpansionRoom room(shape, layer, contained_shape);
    
    // Should be able to use as FreeSpaceExpansionRoom pointer
    FreeSpaceExpansionRoom* base_ptr = &room;
    EXPECT_NE(base_ptr, nullptr);
    
    // Should have the expected behavior from base class
    // (This would depend on FreeSpaceExpansionRoom implementation)
    // For now, just test that the inheritance works
}

} // namespace autoroute
} // namespace freerouting
