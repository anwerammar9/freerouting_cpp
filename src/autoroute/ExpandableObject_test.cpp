/**
 * @file ExpandableObject_test.cpp
 * @brief Unit tests for ExpandableObject interface
 */

#include "autoroute/ExpandableObject.h"
#include <gtest/gtest.h>
#include <memory>

namespace freerouting {
namespace autoroute {

// Mock implementation of ExpandableObject for testing
class MockExpandableObject : public ExpandableObject {
public:
    MockExpandableObject() : dimension_(10), element_count_(5) {}
    
    std::shared_ptr<TileShape> get_shape() const override {
        return nullptr; // Mock implementation
    }
    
    int get_dimension() const override {
        return dimension_;
    }
    
    CompleteExpansionRoom* other_room(CompleteExpansionRoom* p_room) override {
        return nullptr; // Mock implementation
    }
    
    int maze_search_element_count() const override {
        return element_count_;
    }
    
    MazeSearchElement* get_maze_search_element(int p_no) override {
        if (p_no >= 0 && p_no < element_count_) {
            return nullptr; // Mock implementation
        }
        return nullptr;
    }
    
    void reset() override {
        reset_called_ = true;
    }
    
    // Test helpers
    bool was_reset_called() const { return reset_called_; }
    void set_dimension(int dim) { dimension_ = dim; }
    void set_element_count(int count) { element_count_ = count; }
    
private:
    int dimension_;
    int element_count_;
    bool reset_called_ = false;
};

class ExpandableObjectTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_object_ = std::make_unique<MockExpandableObject>();
    }
    
    std::unique_ptr<MockExpandableObject> test_object_;
};

// Test interface contract
TEST_F(ExpandableObjectTest, InterfaceContract) {
    // Test that all virtual methods are properly defined
    EXPECT_NE(test_object_, nullptr);
    
    // Test basic functionality
    EXPECT_EQ(test_object_->get_dimension(), 10);
    EXPECT_EQ(test_object_->maze_search_element_count(), 5);
    
    // Test reset functionality
    EXPECT_FALSE(test_object_->was_reset_called());
    test_object_->reset();
    EXPECT_TRUE(test_object_->was_reset_called());
}

// Test dimension functionality
TEST_F(ExpandableObjectTest, DimensionFunctionality) {
    // Test setting and getting dimension
    test_object_->set_dimension(20);
    EXPECT_EQ(test_object_->get_dimension(), 20);
    
    test_object_->set_dimension(0);
    EXPECT_EQ(test_object_->get_dimension(), 0);
    
    test_object_->set_dimension(-5);
    EXPECT_EQ(test_object_->get_dimension(), -5);
}

// Test element count functionality
TEST_F(ExpandableObjectTest, ElementCountFunctionality) {
    // Test setting and getting element count
    test_object_->set_element_count(100);
    EXPECT_EQ(test_object_->maze_search_element_count(), 100);
    
    test_object_->set_element_count(0);
    EXPECT_EQ(test_object_->maze_search_element_count(), 0);
    
    test_object_->set_element_count(1);
    EXPECT_EQ(test_object_->maze_search_element_count(), 1);
}

// Test maze search element access
TEST_F(ExpandableObjectTest, MazeSearchElementAccess) {
    // Test accessing elements within bounds
    test_object_->set_element_count(3);
    
    for (int i = 0; i < 3; ++i) {
        // Should not crash (returns nullptr in mock)
        EXPECT_EQ(test_object_->get_maze_search_element(i), nullptr);
    }
    
    // Test accessing elements out of bounds
    EXPECT_EQ(test_object_->get_maze_search_element(-1), nullptr);
    EXPECT_EQ(test_object_->get_maze_search_element(3), nullptr);
    EXPECT_EQ(test_object_->get_maze_search_element(100), nullptr);
}

// Test shape functionality
TEST_F(ExpandableObjectTest, ShapeFunctionality) {
    // Test getting shape (returns nullptr in mock)
    auto shape = test_object_->get_shape();
    EXPECT_EQ(shape, nullptr);
}

// Test other room functionality
TEST_F(ExpandableObjectTest, OtherRoomFunctionality) {
    // Test getting other room (returns nullptr in mock)
    CompleteExpansionRoom* room = nullptr;
    auto other = test_object_->other_room(room);
    EXPECT_EQ(other, nullptr);
}

// Test reset functionality
TEST_F(ExpandableObjectTest, ResetFunctionality) {
    // Initial state
    EXPECT_FALSE(test_object_->was_reset_called());
    
    // Call reset
    test_object_->reset();
    
    // Should have been called
    EXPECT_TRUE(test_object_->was_reset_called());
    
    // Call reset again
    test_object_->reset();
    
    // Should still be true
    EXPECT_TRUE(test_object_->was_reset_called());
}

// Test interface polymorphism
TEST_F(ExpandableObjectTest, InterfacePolymorphism) {
    // Test that we can use the interface polymorphically
    ExpandableObject* interface_ptr = test_object_.get();
    
    EXPECT_NE(interface_ptr, nullptr);
    EXPECT_EQ(interface_ptr->get_dimension(), 10);
    EXPECT_EQ(interface_ptr->maze_search_element_count(), 5);
    
    interface_ptr->reset();
    EXPECT_TRUE(test_object_->was_reset_called());
}

// Test const correctness
TEST_F(ExpandableObjectTest, ConstCorrectness) {
    const ExpandableObject* const_ptr = test_object_.get();
    
    // Should be able to call const methods
    EXPECT_EQ(const_ptr->get_dimension(), 10);
    EXPECT_EQ(const_ptr->maze_search_element_count(), 5);
    
    // Should be able to get shape (const method)
    auto shape = const_ptr->get_shape();
    EXPECT_EQ(shape, nullptr);
}

} // namespace autoroute
} // namespace freerouting
