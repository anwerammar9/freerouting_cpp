/**
 * @file TargetItemExpansionDoor_test.cpp
 * @brief Unit tests for TargetItemExpansionDoor class
 */

#include "autoroute/TargetItemExpansionDoor.h"
#include <gtest/gtest.h>
#include <memory>

namespace freerouting {
namespace autoroute {

// Mock implementations for testing
class MockItem : public Item {
public:
    std::shared_ptr<ItemAutorouteInfo> get_autoroute_info() const override {
        return autoroute_info_;
    }
    
    std::shared_ptr<TileShape> get_tree_shape(std::shared_ptr<ShapeSearchTree> search_tree, int entry_no) const override {
        return std::make_shared<TileShape>(); // Mock implementation
    }
    
    void set_autoroute_info(std::shared_ptr<ItemAutorouteInfo> info) {
        autoroute_info_ = info;
    }
    
private:
    std::shared_ptr<ItemAutorouteInfo> autoroute_info_;
};

class MockCompleteExpansionRoom : public CompleteExpansionRoom {
public:
    std::shared_ptr<TileShape> get_shape() const override {
        return shape_;
    }
    
    void set_shape(std::shared_ptr<TileShape> shape) {
        shape_ = shape;
    }
    
private:
    std::shared_ptr<TileShape> shape_;
};

class MockShapeSearchTree : public ShapeSearchTree {
public:
    // Mock implementation
};

class MockItemAutorouteInfo : public ItemAutorouteInfo {
public:
    bool is_start_info() const override {
        return is_start_;
    }
    
    void set_is_start(bool is_start) {
        is_start_ = is_start;
    }
    
private:
    bool is_start_ = false;
};

class TargetItemExpansionDoorTest : public ::testing::Test {
protected:
    void SetUp() override {
        item = std::make_shared<MockItem>();
        room = std::make_shared<MockCompleteExpansionRoom>();
        search_tree = std::make_shared<MockShapeSearchTree>();
        autoroute_info = std::make_shared<MockItemAutorouteInfo>();
        
        item->set_autoroute_info(autoroute_info);
        
        tree_entry_no = 1;
    }
    
    std::shared_ptr<MockItem> item;
    std::shared_ptr<MockCompleteExpansionRoom> room;
    std::shared_ptr<MockShapeSearchTree> search_tree;
    std::shared_ptr<MockItemAutorouteInfo> autoroute_info;
    int tree_entry_no;
};

// Test constructor with valid parameters
TEST_F(TargetItemExpansionDoorTest, ConstructorWithValidParameters) {
    EXPECT_NO_THROW({
        TargetItemExpansionDoor door(item, tree_entry_no, room, search_tree);
    });
    
    TargetItemExpansionDoor door(item, tree_entry_no, room, search_tree);
    
    // Test basic properties
    EXPECT_EQ(door.get_item(), item);
    EXPECT_EQ(door.get_tree_entry_no(), tree_entry_no);
    EXPECT_EQ(door.get_room(), room);
    EXPECT_EQ(door.get_dimension(), 2);
    EXPECT_EQ(door.maze_search_element_count(), 1);
}

// Test constructor with null room
TEST_F(TargetItemExpansionDoorTest, ConstructorWithNullRoom) {
    EXPECT_NO_THROW({
        TargetItemExpansionDoor door(item, tree_entry_no, nullptr, search_tree);
    });
    
    TargetItemExpansionDoor door(item, tree_entry_no, nullptr, search_tree);
    
    // Should still work with null room
    EXPECT_EQ(door.get_item(), item);
    EXPECT_EQ(door.get_tree_entry_no(), tree_entry_no);
    EXPECT_EQ(door.get_room(), nullptr);
    EXPECT_EQ(door.get_dimension(), 2);
    EXPECT_EQ(door.maze_search_element_count(), 1);
}

// Test is_destination_door functionality
TEST_F(TargetItemExpansionDoorTest, IsDestinationDoorFunctionality) {
    // Test with start door
    autoroute_info->set_is_start(true);
    TargetItemExpansionDoor start_door(item, tree_entry_no, room, search_tree);
    EXPECT_FALSE(start_door.is_destination_door());
    
    // Test with destination door
    autoroute_info->set_is_start(false);
    TargetItemExpansionDoor dest_door(item, tree_entry_no, room, search_tree);
    EXPECT_TRUE(dest_door.is_destination_door());
}

// Test ExpandableObject interface implementation
TEST_F(TargetItemExpansionDoorTest, ExpandableObjectInterface) {
    TargetItemExpansionDoor door(item, tree_entry_no, room, search_tree);
    
    // Test get_shape
    auto shape = door.get_shape();
    EXPECT_NE(shape, nullptr);
    
    // Test get_dimension
    EXPECT_EQ(door.get_dimension(), 2);
    
    // Test other_room (should return nullptr)
    auto other_room = door.other_room(room);
    EXPECT_EQ(other_room, nullptr);
    
    // Test maze search elements
    EXPECT_EQ(door.maze_search_element_count(), 1);
    auto maze_element = door.get_maze_search_element(0);
    EXPECT_NE(maze_element, nullptr);
    
    // Should return the same element for any index
    auto maze_element2 = door.get_maze_search_element(5);
    EXPECT_EQ(maze_element, maze_element2);
}

// Test reset functionality
TEST_F(TargetItemExpansionDoorTest, ResetFunctionality) {
    TargetItemExpansionDoor door(item, tree_entry_no, room, search_tree);
    
    // Get the maze search element
    auto maze_element = door.get_maze_search_element(0);
    EXPECT_NE(maze_element, nullptr);
    
    // Reset the door
    EXPECT_NO_THROW({
        door.reset();
    });
    
    // Should not crash and maze element should still be accessible
    auto maze_element_after_reset = door.get_maze_search_element(0);
    EXPECT_EQ(maze_element, maze_element_after_reset);
}

// Test multiple doors
TEST_F(TargetItemExpansionDoorTest, MultipleDoors) {
    // Create multiple doors with different parameters
    TargetItemExpansionDoor door1(item, 1, room, search_tree);
    TargetItemExpansionDoor door2(item, 2, room, search_tree);
    TargetItemExpansionDoor door3(item, 3, nullptr, search_tree);
    
    // Each should have independent properties
    EXPECT_EQ(door1.get_tree_entry_no(), 1);
    EXPECT_EQ(door2.get_tree_entry_no(), 2);
    EXPECT_EQ(door3.get_tree_entry_no(), 3);
    
    EXPECT_NE(door1.get_room(), nullptr);
    EXPECT_NE(door2.get_room(), nullptr);
    EXPECT_EQ(door3.get_room(), nullptr);
    
    // All should have the same dimension
    EXPECT_EQ(door1.get_dimension(), 2);
    EXPECT_EQ(door2.get_dimension(), 2);
    EXPECT_EQ(door3.get_dimension(), 2);
}

// Test door with different tree entry numbers
TEST_F(TargetItemExpansionDoorTest, DifferentTreeEntryNumbers) {
    std::vector<int> entry_numbers = {0, 1, 5, 10, 100};
    
    for (int entry_no : entry_numbers) {
        TargetItemExpansionDoor door(item, entry_no, room, search_tree);
        
        EXPECT_EQ(door.get_tree_entry_no(), entry_no);
        EXPECT_EQ(door.get_dimension(), 2);
        EXPECT_EQ(door.maze_search_element_count(), 1);
        
        // Should not crash for any valid entry number
        EXPECT_NO_THROW({
            auto shape = door.get_shape();
            auto maze_element = door.get_maze_search_element(0);
        });
    }
}

// Test edge cases
TEST_F(TargetItemExpansionDoorTest, EdgeCases) {
    // Test with zero tree entry number
    TargetItemExpansionDoor door_zero(item, 0, room, search_tree);
    EXPECT_EQ(door_zero.get_tree_entry_no(), 0);
    
    // Test with negative tree entry number
    TargetItemExpansionDoor door_negative(item, -1, room, search_tree);
    EXPECT_EQ(door_negative.get_tree_entry_no(), -1);
    
    // Test with very large tree entry number
    TargetItemExpansionDoor door_large(item, 1000000, room, search_tree);
    EXPECT_EQ(door_large.get_tree_entry_no(), 1000000);
    
    // All should work without issues
    EXPECT_EQ(door_zero.get_dimension(), 2);
    EXPECT_EQ(door_negative.get_dimension(), 2);
    EXPECT_EQ(door_large.get_dimension(), 2);
}

// Test const correctness
TEST_F(TargetItemExpansionDoorTest, ConstCorrectness) {
    const TargetItemExpansionDoor door(item, tree_entry_no, room, search_tree);
    
    // Should be able to call const methods
    EXPECT_EQ(door.get_item(), item);
    EXPECT_EQ(door.get_tree_entry_no(), tree_entry_no);
    EXPECT_EQ(door.get_room(), room);
    EXPECT_EQ(door.get_dimension(), 2);
    EXPECT_EQ(door.maze_search_element_count(), 1);
    
    // Should be able to get shape and maze elements
    auto shape = door.get_shape();
    EXPECT_NE(shape, nullptr);
    
    auto maze_element = door.get_maze_search_element(0);
    EXPECT_NE(maze_element, nullptr);
    
    // Should be able to check if it's destination door
    bool is_dest = door.is_destination_door();
    EXPECT_TRUE(is_dest == true || is_dest == false); // Just ensure it doesn't crash
}

// Test interface polymorphism
TEST_F(TargetItemExpansionDoorTest, InterfacePolymorphism) {
    TargetItemExpansionDoor door(item, tree_entry_no, room, search_tree);
    
    // Test that we can use the interface polymorphically
    ExpandableObject* interface_ptr = &door;
    
    EXPECT_NE(interface_ptr, nullptr);
    
    // Should be able to call interface methods
    EXPECT_EQ(interface_ptr->get_dimension(), 2);
    EXPECT_EQ(interface_ptr->maze_search_element_count(), 1);
    
    auto shape = interface_ptr->get_shape();
    EXPECT_NE(shape, nullptr);
    
    auto maze_element = interface_ptr->get_maze_search_element(0);
    EXPECT_NE(maze_element, nullptr);
    
    // Should be able to reset through interface
    EXPECT_NO_THROW({
        interface_ptr->reset();
    });
}

// Test door lifecycle
TEST_F(TargetItemExpansionDoorTest, DoorLifecycle) {
    // Create door
    TargetItemExpansionDoor door(item, tree_entry_no, room, search_tree);
    
    // Initial state
    EXPECT_EQ(door.get_tree_entry_no(), tree_entry_no);
    EXPECT_EQ(door.maze_search_element_count(), 1);
    
    // Use the door
    auto shape = door.get_shape();
    auto maze_element = door.get_maze_search_element(0);
    
    EXPECT_NE(shape, nullptr);
    EXPECT_NE(maze_element, nullptr);
    
    // Reset the door
    door.reset();
    
    // Should still be functional after reset
    auto shape_after_reset = door.get_shape();
    auto maze_element_after_reset = door.get_maze_search_element(0);
    
    EXPECT_NE(shape_after_reset, nullptr);
    EXPECT_NE(maze_element_after_reset, nullptr);
}

} // namespace autoroute
} // namespace freerouting
