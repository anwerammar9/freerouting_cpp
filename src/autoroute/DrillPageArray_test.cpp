/**
 * @file DrillPageArray_test.cpp
 * @brief Unit tests for DrillPageArray class
 */

#include "autoroute/DrillPageArray.h"
#include <gtest/gtest.h>
#include <memory>
#include <vector>

namespace freerouting {
namespace autoroute {

// Mock implementations for testing
class MockRoutingBoard : public RoutingBoard {
public:
    IntBox bounding_box;
    
    MockRoutingBoard(const IntBox& bounds) : bounding_box(bounds) {}
    
    std::shared_ptr<RoutingBoard> deep_copy() const override {
        return std::make_shared<MockRoutingBoard>(bounding_box);
    }
};

class MockDrillPage : public DrillPage {
public:
    std::shared_ptr<TileShape> shape;
    bool invalidated = false;
    bool reset_called = false;
    bool draw_called = false;
    
    MockDrillPage(std::shared_ptr<IntBox> bounds, std::shared_ptr<RoutingBoard> board)
        : DrillPage(bounds, board) {
        // Mock implementation
    }
    
    void invalidate() override {
        invalidated = true;
    }
    
    void reset() override {
        reset_called = true;
        invalidated = false;
    }
    
    void draw(void* graphics, const graphics::GraphicsContext& context, double intensity) const override {
        draw_called = true;
    }
};

class MockTileShape : public TileShape {
public:
    std::shared_ptr<IntBox> bounding_box_val;
    int dimension_val = 2;
    
    MockTileShape(std::shared_ptr<IntBox> bbox, int dim = 2) 
        : bounding_box_val(bbox), dimension_val(dim) {}
    
    std::shared_ptr<IntBox> bounding_box() const override {
        return bounding_box_val;
    }
    
    std::shared_ptr<TileShape> intersection(std::shared_ptr<TileShape> other) const override {
        // Mock intersection - return this with reduced dimension if overlapping
        if (other) {
            auto result = std::make_shared<MockTileShape>(bounding_box_val, 1);
            return result;
        }
        return nullptr;
    }
    
    int get_dimension() const override {
        return dimension_val;
    }
};

namespace graphics {
    class GraphicsContext {
    public:
        // Mock implementation
    };
}

class DrillPageArrayTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a test board with bounds (0,0) to (100,100)
        bounds = std::make_shared<IntBox>(0, 0, 100, 100);
        board = std::make_shared<MockRoutingBoard>(*bounds);
        max_page_width = 30;
    }
    
    std::shared_ptr<IntBox> bounds;
    std::shared_ptr<MockRoutingBoard> board;
    int max_page_width;
};

// Test constructor with valid parameters
TEST_F(DrillPageArrayTest, ConstructorWithValidParameters) {
    EXPECT_NO_THROW({
        DrillPageArray array(board, max_page_width);
    });
    
    DrillPageArray array(board, max_page_width);
    
    // Test basic properties
    EXPECT_EQ(array.get_bounds().ll.x, 0);
    EXPECT_EQ(array.get_bounds().ll.y, 0);
    EXPECT_EQ(array.get_bounds().ur.x, 100);
    EXPECT_EQ(array.get_bounds().ur.y, 100);
    
    // Should have calculated reasonable page dimensions
    EXPECT_GT(array.get_column_count(), 0);
    EXPECT_GT(array.get_row_count(), 0);
    EXPECT_GT(array.get_page_width(), 0);
    EXPECT_GT(array.get_page_height(), 0);
}

// Test page dimensions calculation
TEST_F(DrillPageArrayTest, PageDimensionsCalculation) {
    DrillPageArray array(board, max_page_width);
    
    // With bounds (0,0) to (100,100) and max page width 30:
    // Expected: 4 columns (100/30 = 3.33 -> ceil = 4)
    // Expected: 4 rows (100/30 = 3.33 -> ceil = 4)
    // Expected page width: 100/4 = 25
    // Expected page height: 100/4 = 25
    
    EXPECT_EQ(array.get_column_count(), 4);
    EXPECT_EQ(array.get_row_count(), 4);
    EXPECT_EQ(array.get_page_width(), 25);
    EXPECT_EQ(array.get_page_height(), 25);
}

// Test different max page widths
TEST_F(DrillPageArrayTest, DifferentMaxPageWidths) {
    std::vector<int> page_widths = {10, 20, 25, 50, 100};
    
    for (int width : page_widths) {
        DrillPageArray array(board, width);
        
        EXPECT_GT(array.get_column_count(), 0);
        EXPECT_GT(array.get_row_count(), 0);
        EXPECT_GT(array.get_page_width(), 0);
        EXPECT_GT(array.get_page_height(), 0);
        
        // Page dimensions should not exceed max width
        EXPECT_LE(array.get_page_width(), width);
        EXPECT_LE(array.get_page_height(), width);
    }
}

// Test page access
TEST_F(DrillPageArrayTest, PageAccess) {
    DrillPageArray array(board, max_page_width);
    
    // Test valid page access
    for (int row = 0; row < array.get_row_count(); ++row) {
        for (int col = 0; col < array.get_column_count(); ++col) {
            auto page = array.get_page(row, col);
            EXPECT_NE(page, nullptr);
        }
    }
    
    // Test invalid page access
    EXPECT_EQ(array.get_page(-1, 0), nullptr);
    EXPECT_EQ(array.get_page(0, -1), nullptr);
    EXPECT_EQ(array.get_page(array.get_row_count(), 0), nullptr);
    EXPECT_EQ(array.get_page(0, array.get_column_count()), nullptr);
}

// Test overlapping pages functionality
TEST_F(DrillPageArrayTest, OverlappingPages) {
    DrillPageArray array(board, max_page_width);
    
    // Create a shape that overlaps with some pages
    auto shape_bounds = std::make_shared<IntBox>(10, 10, 40, 40);
    auto shape = std::make_shared<MockTileShape>(shape_bounds);
    
    auto overlapping = array.overlapping_pages(shape);
    
    // Should find some overlapping pages
    EXPECT_GT(overlapping.size(), 0);
    EXPECT_LE(overlapping.size(), array.get_row_count() * array.get_column_count());
}

// Test invalidate functionality
TEST_F(DrillPageArrayTest, InvalidateFunctionality) {
    DrillPageArray array(board, max_page_width);
    
    // Create a shape that overlaps with some pages
    auto shape_bounds = std::make_shared<IntBox>(10, 10, 40, 40);
    auto shape = std::make_shared<MockTileShape>(shape_bounds);
    
    // Get overlapping pages
    auto overlapping = array.overlapping_pages(shape);
    
    // Invalidate the shape
    EXPECT_NO_THROW({
        array.invalidate(shape);
    });
    
    // Should not crash
    SUCCEED();
}

// Test reset functionality
TEST_F(DrillPageArrayTest, ResetFunctionality) {
    DrillPageArray array(board, max_page_width);
    
    // Should be able to reset without crashing
    EXPECT_NO_THROW({
        array.reset();
    });
    
    // All pages should still be accessible
    for (int row = 0; row < array.get_row_count(); ++row) {
        for (int col = 0; col < array.get_column_count(); ++col) {
            auto page = array.get_page(row, col);
            EXPECT_NE(page, nullptr);
        }
    }
}

// Test draw functionality
TEST_F(DrillPageArrayTest, DrawFunctionality) {
    DrillPageArray array(board, max_page_width);
    graphics::GraphicsContext context;
    
    // Should be able to draw without crashing
    EXPECT_NO_THROW({
        array.draw(nullptr, context, 0.5);
    });
    
    // Should not crash
    SUCCEED();
}

// Test edge cases with small bounds
TEST_F(DrillPageArrayTest, SmallBounds) {
    auto small_bounds = std::make_shared<IntBox>(0, 0, 10, 10);
    auto small_board = std::make_shared<MockRoutingBoard>(*small_bounds);
    
    DrillPageArray array(small_board, max_page_width);
    
    // Should handle small bounds correctly
    EXPECT_GE(array.get_column_count(), 1);
    EXPECT_GE(array.get_row_count(), 1);
    EXPECT_GE(array.get_page_width(), 1);
    EXPECT_GE(array.get_page_height(), 1);
}

// Test edge cases with large bounds
TEST_F(DrillPageArrayTest, LargeBounds) {
    auto large_bounds = std::make_shared<IntBox>(0, 0, 1000, 1000);
    auto large_board = std::make_shared<MockRoutingBoard>(*large_bounds);
    
    DrillPageArray array(large_board, 50);
    
    // Should handle large bounds correctly
    EXPECT_GT(array.get_column_count(), 0);
    EXPECT_GT(array.get_row_count(), 0);
    EXPECT_GT(array.get_page_width(), 0);
    EXPECT_GT(array.get_page_height(), 0);
}

// Test edge cases with very small max page width
TEST_F(DrillPageArrayTest, SmallMaxPageWidth) {
    DrillPageArray array(board, 1); // Very small max page width
    
    // Should create many small pages
    EXPECT_EQ(array.get_page_width(), 1);
    EXPECT_EQ(array.get_page_height(), 1);
    EXPECT_EQ(array.get_column_count(), 100);
    EXPECT_EQ(array.get_row_count(), 100);
}

// Test edge cases with very large max page width
TEST_F(DrillPageArrayTest, LargeMaxPageWidth) {
    DrillPageArray array(board, 1000); // Very large max page width
    
    // Should create few large pages
    EXPECT_LE(array.get_column_count(), 1);
    EXPECT_LE(array.get_row_count(), 1);
    EXPECT_GE(array.get_page_width(), 100);
    EXPECT_GE(array.get_page_height(), 100);
}

// Test shape that doesn't overlap
TEST_F(DrillPageArrayTest, NonOverlappingShape) {
    DrillPageArray array(board, max_page_width);
    
    // Create a shape outside the bounds
    auto shape_bounds = std::make_shared<IntBox>(200, 200, 300, 300);
    auto shape = std::make_shared<MockTileShape>(shape_bounds);
    
    auto overlapping = array.overlapping_pages(shape);
    
    // Should find no overlapping pages
    EXPECT_EQ(overlapping.size(), 0);
}

// Test shape that covers entire bounds
TEST_F(DrillPageArrayTest, FullBoundsShape) {
    DrillPageArray array(board, max_page_width);
    
    // Create a shape that covers the entire board
    auto shape = std::make_shared<MockTileShape>(bounds);
    
    auto overlapping = array.overlapping_pages(shape);
    
    // Should find all pages overlapping
    EXPECT_EQ(overlapping.size(), array.get_row_count() * array.get_column_count());
}

// Test multiple operations
TEST_F(DrillPageArrayTest, MultipleOperations) {
    DrillPageArray array(board, max_page_width);
    
    // Perform multiple operations
    for (int i = 0; i < 10; ++i) {
        auto shape_bounds = std::make_shared<IntBox>(i*10, i*10, (i+1)*10, (i+1)*10);
        auto shape = std::make_shared<MockTileShape>(shape_bounds);
        
        // Should not crash
        array.invalidate(shape);
        auto overlapping = array.overlapping_pages(shape);
        array.reset();
        
        // Should still work
        EXPECT_GT(array.get_row_count(), 0);
        EXPECT_GT(array.get_column_count(), 0);
    }
}

// Test const correctness
TEST_F(DrillPageArrayTest, ConstCorrectness) {
    const DrillPageArray array(board, max_page_width);
    
    // Should be able to call const methods
    EXPECT_EQ(array.get_bounds().ll.x, 0);
    EXPECT_EQ(array.get_column_count(), 4);
    EXPECT_EQ(array.get_row_count(), 4);
    EXPECT_EQ(array.get_page_width(), 25);
    EXPECT_EQ(array.get_page_height(), 25);
    
    // Should be able to access pages
    auto page = array.get_page(0, 0);
    EXPECT_NE(page, nullptr);
    
    // Should be able to find overlapping pages
    auto shape = std::make_shared<MockTileShape>(bounds);
    auto overlapping = array.overlapping_pages(shape);
    EXPECT_GT(overlapping.size(), 0);
}

} // namespace autoroute
} // namespace freerouting
