/**
 * @file ItemRouteResult_test.cpp
 * @brief Unit tests for ItemRouteResult class
 */

#include "autoroute/ItemRouteResult.h"
#include <gtest/gtest.h>
#include <vector>
#include <algorithm>

namespace freerouting {
namespace autoroute {

class ItemRouteResultTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Test setup if needed
    }
};

// Test default constructor
TEST_F(ItemRouteResultTest, DefaultConstructor) {
    ItemRouteResult result(123);
    
    EXPECT_EQ(result.item_id(), 123);
    EXPECT_FALSE(result.improved());
    EXPECT_EQ(result.improvement_percentage(), 0.0f);
    EXPECT_EQ(result.via_count(), 0);
    EXPECT_EQ(result.trace_length(), 0.0);
    EXPECT_EQ(result.incomplete_count(), 1);
    EXPECT_EQ(result.via_count_reduced(), 0);
    EXPECT_EQ(result.length_reduced(), 0.0);
    EXPECT_EQ(result.incomplete_count_before(), 0);
}

// Test constructor with full metrics
TEST_F(ItemRouteResultTest, FullConstructor) {
    ItemRouteResult result(456, 5, 3, 100.0, 80.0, 2, 1);
    
    EXPECT_EQ(result.item_id(), 456);
    EXPECT_TRUE(result.improved());  // Incomplete count reduced
    EXPECT_GT(result.improvement_percentage(), 0.0f);
    EXPECT_EQ(result.via_count(), 3);
    EXPECT_EQ(result.trace_length(), 80.0);
    EXPECT_EQ(result.incomplete_count(), 1);
    EXPECT_EQ(result.via_count_reduced(), 2);
    EXPECT_EQ(result.length_reduced(), 20.0);
    EXPECT_EQ(result.incomplete_count_before(), 2);
}

// Test improvement detection - incomplete count reduction
TEST_F(ItemRouteResultTest, ImprovementDetectionIncompleteCount) {
    // Improved: incomplete count reduced
    ItemRouteResult improved(1, 5, 5, 100.0, 100.0, 3, 1);
    EXPECT_TRUE(improved.improved());
    
    // Not improved: incomplete count increased
    ItemRouteResult not_improved(2, 5, 5, 100.0, 100.0, 1, 3);
    EXPECT_FALSE(not_improved.improved());
    
    // Not improved: incomplete count same
    ItemRouteResult same(3, 5, 5, 100.0, 100.0, 2, 2);
    EXPECT_FALSE(same.improved());
}

// Test improvement detection - via count reduction
TEST_F(ItemRouteResultTest, ImprovementDetectionViaCount) {
    // Improved: via count reduced (incomplete count same)
    ItemRouteResult improved(1, 5, 3, 100.0, 100.0, 2, 2);
    EXPECT_TRUE(improved.improved());
    
    // Not improved: via count increased
    ItemRouteResult not_improved(2, 3, 5, 100.0, 100.0, 2, 2);
    EXPECT_FALSE(not_improved.improved());
    
    // Not improved: via count same
    ItemRouteResult same(3, 5, 5, 100.0, 100.0, 2, 2);
    EXPECT_FALSE(same.improved());
}

// Test improvement detection - trace length reduction
TEST_F(ItemRouteResultTest, ImprovementDetectionTraceLength) {
    // Improved: trace length reduced (incomplete and via count same)
    ItemRouteResult improved(1, 5, 5, 100.0, 80.0, 2, 2);
    EXPECT_TRUE(improved.improved());
    
    // Not improved: trace length increased
    ItemRouteResult not_improved(2, 5, 5, 80.0, 100.0, 2, 2);
    EXPECT_FALSE(not_improved.improved());
    
    // Not improved: trace length same
    ItemRouteResult same(3, 5, 5, 100.0, 100.0, 2, 2);
    EXPECT_FALSE(same.improved());
}

// Test improvement percentage calculation
TEST_F(ItemRouteResultTest, ImprovementPercentageCalculation) {
    // Test with both via and trace reduction
    ItemRouteResult result1(1, 10, 5, 200.0, 100.0, 2, 2);
    EXPECT_GT(result1.improvement_percentage(), 0.0f);
    EXPECT_LT(result1.improvement_percentage(), 1.0f);
    
    // Test with no improvement
    ItemRouteResult result2(2, 5, 5, 100.0, 100.0, 2, 2);
    EXPECT_EQ(result2.improvement_percentage(), 0.0f);
    
    // Test with zero before values (should be 0)
    ItemRouteResult result3(3, 0, 0, 0.0, 0.0, 2, 2);
    EXPECT_EQ(result3.improvement_percentage(), 0.0f);
}

// Test comparison operators
TEST_F(ItemRouteResultTest, ComparisonOperators) {
    // Create results with different metrics
    ItemRouteResult result1(1, 5, 3, 100.0, 80.0, 2, 1);  // Best: incomplete=1, via=3, length=80
    ItemRouteResult result2(2, 5, 3, 100.0, 80.0, 2, 2);  // Middle: incomplete=2, via=3, length=80
    ItemRouteResult result3(3, 5, 7, 100.0, 80.0, 2, 2);  // Worst: incomplete=2, via=7, length=80
    
    // Test less than operator
    EXPECT_TRUE(result1 < result2);
    EXPECT_TRUE(result2 < result3);
    EXPECT_TRUE(result1 < result3);
    
    // Test three-way comparison
    EXPECT_EQ(result1 <=> result2, std::strong_ordering::less);
    EXPECT_EQ(result2 <=> result3, std::strong_ordering::less);
    EXPECT_EQ(result1 <=> result3, std::strong_ordering::less);
    
    // Test equal results
    ItemRouteResult result4(4, 5, 3, 100.0, 80.0, 2, 2);
    EXPECT_EQ(result2 <=> result4, std::strong_ordering::equal);
    EXPECT_FALSE(result2 < result4);
    EXPECT_FALSE(result4 < result2);
}

// Test improved_over method
TEST_F(ItemRouteResultTest, ImprovedOverMethod) {
    ItemRouteResult better(1, 5, 3, 100.0, 80.0, 2, 1);
    ItemRouteResult worse(2, 5, 7, 100.0, 120.0, 2, 3);
    ItemRouteResult equal(3, 5, 3, 100.0, 80.0, 2, 1);
    
    EXPECT_TRUE(better.improved_over(worse));
    EXPECT_FALSE(worse.improved_over(better));
    EXPECT_FALSE(better.improved_over(equal));
    EXPECT_FALSE(equal.improved_over(better));
}

// Test sorting functionality
TEST_F(ItemRouteResultTest, SortingFunctionality) {
    std::vector<ItemRouteResult> results;
    
    // Add results in random order
    results.emplace_back(3, 5, 7, 100.0, 120.0, 2, 3);  // Worst
    results.emplace_back(1, 5, 3, 100.0, 80.0, 2, 1);   // Best
    results.emplace_back(2, 5, 3, 100.0, 80.0, 2, 2);   // Middle
    results.emplace_back(4, 5, 3, 100.0, 80.0, 2, 2);   // Middle (equal to 2)
    
    // Sort the results
    std::sort(results.begin(), results.end());
    
    // Check sorting order
    EXPECT_EQ(results[0].item_id(), 1);  // Best
    EXPECT_EQ(results[1].item_id(), 2);  // Middle
    EXPECT_EQ(results[2].item_id(), 4);  // Middle (equal to 2)
    EXPECT_EQ(results[3].item_id(), 3);  // Worst
}

// Test update_improved method
TEST_F(ItemRouteResultTest, UpdateImprovedMethod) {
    ItemRouteResult result(1, 5, 3, 100.0, 80.0, 2, 1);
    
    // Initially should be improved
    EXPECT_TRUE(result.improved());
    
    // Update to not improved
    result.update_improved(false);
    EXPECT_FALSE(result.improved());
    
    // Update back to improved
    result.update_improved(true);
    EXPECT_TRUE(result.improved());
}

// Test edge cases
TEST_F(ItemRouteResultTest, EdgeCases) {
    // Test with negative values (should work but may not be realistic)
    EXPECT_NO_THROW({
        ItemRouteResult result(1, -1, -2, -100.0, -80.0, -1, -2);
    });
    
    // Test with very large values
    EXPECT_NO_THROW({
        ItemRouteResult result(1, 1000000, 500000, 1e6, 5e5, 1000, 500);
    });
    
    // Test with zero values
    ItemRouteResult zero_result(1, 0, 0, 0.0, 0.0, 0, 0);
    EXPECT_EQ(zero_result.via_count(), 0);
    EXPECT_EQ(zero_result.trace_length(), 0.0);
    EXPECT_EQ(zero_result.incomplete_count(), 0);
    EXPECT_EQ(zero_result.via_count_reduced(), 0);
    EXPECT_EQ(zero_result.length_reduced(), 0.0);
}

// Test const correctness
TEST_F(ItemRouteResultTest, ConstCorrectness) {
    const ItemRouteResult result(1, 5, 3, 100.0, 80.0, 2, 1);
    
    // Should be able to call const methods
    EXPECT_EQ(result.item_id(), 1);
    EXPECT_TRUE(result.improved());
    EXPECT_GT(result.improvement_percentage(), 0.0f);
    EXPECT_EQ(result.via_count(), 3);
    EXPECT_EQ(result.trace_length(), 80.0);
    EXPECT_EQ(result.incomplete_count(), 1);
    EXPECT_EQ(result.via_count_reduced(), 2);
    EXPECT_EQ(result.length_reduced(), 20.0);
    EXPECT_EQ(result.incomplete_count_before(), 2);
    
    // Should be able to use comparison operators
    const ItemRouteResult other(2, 5, 3, 100.0, 80.0, 2, 1);
    EXPECT_TRUE(result < other || other < result || result == other);
}

// Test copy and move semantics
TEST_F(ItemRouteResultTest, CopyAndMoveSemantics) {
    ItemRouteResult original(1, 5, 3, 100.0, 80.0, 2, 1);
    
    // Test copy constructor
    ItemRouteResult copied(original);
    EXPECT_EQ(copied.item_id(), original.item_id());
    EXPECT_EQ(copied.improved(), original.improved());
    EXPECT_EQ(copied.improvement_percentage(), original.improvement_percentage());
    
    // Test copy assignment
    ItemRouteResult assigned(2, 1, 1, 1.0, 1.0, 1, 1);
    assigned = original;
    EXPECT_EQ(assigned.item_id(), original.item_id());
    EXPECT_EQ(assigned.improved(), original.improved());
    
    // Test move constructor
    ItemRouteResult moved(std::move(original));
    EXPECT_EQ(moved.item_id(), 1);
    EXPECT_TRUE(moved.improved());
}

} // namespace autoroute
} // namespace freerouting
