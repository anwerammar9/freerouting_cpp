/**
 * @file CompleteExpansionRoom_test.cpp
 * @brief Unit tests for CompleteExpansionRoom interface
 */

#include "autoroute/CompleteExpansionRoom.h"
#include <gtest/gtest.h>
#include <memory>
#include <vector>

namespace freerouting {
namespace autoroute {

// Mock implementations for testing
class MockTargetItemExpansionDoor : public TargetItemExpansionDoor {
public:
    // Mock implementation
};

class MockSearchTreeObject : public SearchTreeObject {
public:
    // Mock implementation
};

namespace graphics {
    class GraphicsContext {
    public:
        // Mock implementation
    };
}

class MockCompleteExpansionRoom : public CompleteExpansionRoom {
public:
    MockCompleteExpansionRoom() {
        // Initialize mock target doors
        target_doors_.push_back(std::make_shared<MockTargetItemExpansionDoor>());
        target_doors_.push_back(std::make_shared<MockTargetItemExpansionDoor>());
        
        // Initialize mock object
        object_ = std::make_shared<MockSearchTreeObject>();
    }
    
    // Implement CompleteExpansionRoom interface
    std::vector<std::shared_ptr<TargetItemExpansionDoor>> get_target_doors() const override {
        return target_doors_;
    }
    
    std::shared_ptr<SearchTreeObject> get_object() const override {
        return object_;
    }
    
    void draw(void* graphics, const graphics::GraphicsContext& graphics_context, double intensity) const override {
        draw_called_ = true;
        last_intensity_ = intensity;
    }
    
    // Test helpers
    bool was_draw_called() const { return draw_called_; }
    double get_last_intensity() const { return last_intensity_; }
    
    // Mock ExpansionRoom interface methods (minimal implementation)
    // These would be inherited from ExpansionRoom
    int get_dimension() const override { return 2; }
    void reset() override { reset_called_ = true; }
    bool was_reset_called() const { return reset_called_; }
    
private:
    std::vector<std::shared_ptr<TargetItemExpansionDoor>> target_doors_;
    std::shared_ptr<SearchTreeObject> object_;
    mutable bool draw_called_ = false;
    mutable double last_intensity_ = 0.0;
    bool reset_called_ = false;
};

class CompleteExpansionRoomTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_room_ = std::make_unique<MockCompleteExpansionRoom>();
    }
    
    std::unique_ptr<MockCompleteExpansionRoom> test_room_;
};

// Test interface contract
TEST_F(CompleteExpansionRoomTest, InterfaceContract) {
    // Test that all virtual methods are properly defined
    EXPECT_NE(test_room_, nullptr);
    
    // Test basic functionality
    auto target_doors = test_room_->get_target_doors();
    EXPECT_EQ(target_doors.size(), 2);
    
    auto object = test_room_->get_object();
    EXPECT_NE(object, nullptr);
    
    // Test draw functionality
    EXPECT_FALSE(test_room_->was_draw_called());
    
    graphics::GraphicsContext context;
    test_room_->draw(nullptr, context, 0.5);
    
    EXPECT_TRUE(test_room_->was_draw_called());
    EXPECT_EQ(test_room_->get_last_intensity(), 0.5);
}

// Test target doors functionality
TEST_F(CompleteExpansionRoomTest, TargetDoorsFunctionality) {
    auto target_doors = test_room_->get_target_doors();
    
    // Should return the expected number of doors
    EXPECT_EQ(target_doors.size(), 2);
    
    // Should return valid door pointers
    for (const auto& door : target_doors) {
        EXPECT_NE(door, nullptr);
    }
    
    // Should be consistent across calls
    auto target_doors2 = test_room_->get_target_doors();
    EXPECT_EQ(target_doors.size(), target_doors2.size());
}

// Test object functionality
TEST_F(CompleteExpansionRoomTest, ObjectFunctionality) {
    auto object = test_room_->get_object();
    
    // Should return a valid object
    EXPECT_NE(object, nullptr);
    
    // Should be consistent across calls
    auto object2 = test_room_->get_object();
    EXPECT_EQ(object, object2);
}

// Test draw functionality
TEST_F(CompleteExpansionRoomTest, DrawFunctionality) {
    graphics::GraphicsContext context;
    
    // Test with different intensities
    std::vector<double> intensities = {0.0, 0.25, 0.5, 0.75, 1.0};
    
    for (double intensity : intensities) {
        // Reset the draw flag
        test_room_ = std::make_unique<MockCompleteExpansionRoom>();
        
        // Call draw
        test_room_->draw(nullptr, context, intensity);
        
        // Should have been called with correct intensity
        EXPECT_TRUE(test_room_->was_draw_called());
        EXPECT_EQ(test_room_->get_last_intensity(), intensity);
    }
}

// Test interface inheritance
TEST_F(CompleteExpansionRoomTest, InterfaceInheritance) {
    // Test that we can use the interface polymorphically
    CompleteExpansionRoom* interface_ptr = test_room_.get();
    
    EXPECT_NE(interface_ptr, nullptr);
    
    // Should be able to call interface methods
    auto target_doors = interface_ptr->get_target_doors();
    EXPECT_EQ(target_doors.size(), 2);
    
    auto object = interface_ptr->get_object();
    EXPECT_NE(object, nullptr);
    
    // Test draw through interface
    graphics::GraphicsContext context;
    interface_ptr->draw(nullptr, context, 0.8);
    EXPECT_TRUE(test_room_->was_draw_called());
    EXPECT_EQ(test_room_->get_last_intensity(), 0.8);
}

// Test const correctness
TEST_F(CompleteExpansionRoomTest, ConstCorrectness) {
    const CompleteExpansionRoom* const_ptr = test_room_.get();
    
    // Should be able to call const methods
    auto target_doors = const_ptr->get_target_doors();
    EXPECT_EQ(target_doors.size(), 2);
    
    auto object = const_ptr->get_object();
    EXPECT_NE(object, nullptr);
    
    // Should be able to call draw (const method)
    graphics::GraphicsContext context;
    const_ptr->draw(nullptr, context, 0.3);
    EXPECT_TRUE(test_room_->was_draw_called());
    EXPECT_EQ(test_room_->get_last_intensity(), 0.3);
}

// Test edge cases
TEST_F(CompleteExpansionRoomTest, EdgeCases) {
    // Test with null graphics (should not crash)
    graphics::GraphicsContext context;
    EXPECT_NO_THROW({
        test_room_->draw(nullptr, context, 0.5);
    });
    
    // Test with extreme intensities
    EXPECT_NO_THROW({
        test_room_->draw(nullptr, context, -1.0);
    });
    
    EXPECT_NO_THROW({
        test_room_->draw(nullptr, context, 2.0);
    });
    
    // Should still have been called
    EXPECT_TRUE(test_room_->was_draw_called());
}

// Test multiple rooms
TEST_F(CompleteExpansionRoomTest, MultipleRooms) {
    // Create multiple rooms
    auto room1 = std::make_unique<MockCompleteExpansionRoom>();
    auto room2 = std::make_unique<MockCompleteExpansionRoom>();
    
    // Each should have its own target doors
    auto doors1 = room1->get_target_doors();
    auto doors2 = room2->get_target_doors();
    
    EXPECT_EQ(doors1.size(), 2);
    EXPECT_EQ(doors2.size(), 2);
    
    // Each should have its own object
    auto obj1 = room1->get_object();
    auto obj2 = room2->get_object();
    
    EXPECT_NE(obj1, nullptr);
    EXPECT_NE(obj2, nullptr);
    EXPECT_NE(obj1, obj2); // Should be different objects
}

} // namespace autoroute
} // namespace freerouting
