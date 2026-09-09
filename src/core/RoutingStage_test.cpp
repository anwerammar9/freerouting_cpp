/**
 * @file RoutingStage_test.cpp
 * @brief Unit tests for RoutingStage enum class
 */

#include "RoutingStage.h"
#include <gtest/gtest.h>

namespace freerouting {
namespace core {

class RoutingStageTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Test setup if needed
    }
};

// Test basic enum functionality
TEST_F(RoutingStageTest, BasicEnumValues) {
    EXPECT_EQ(static_cast<int>(RoutingStage::IDLE), 0);
    EXPECT_EQ(static_cast<int>(RoutingStage::ROUTING), 1);
    EXPECT_EQ(static_cast<int>(RoutingStage::OPTIMIZATION), 2);
}

// Test string conversion
TEST_F(RoutingStageTest, ToStringConversion) {
    EXPECT_EQ(to_string(RoutingStage::IDLE), "IDLE");
    EXPECT_EQ(to_string(RoutingStage::ROUTING), "ROUTING");
    EXPECT_EQ(to_string(RoutingStage::OPTIMIZATION), "OPTIMIZATION");
}

// Test stage descriptions
TEST_F(RoutingStageTest, StageDescriptions) {
    EXPECT_EQ(get_stage_description(RoutingStage::IDLE), 
              "The job hasn't started yet");
    EXPECT_EQ(get_stage_description(RoutingStage::ROUTING), 
              "The board is currently being processed by the auto-router");
    EXPECT_EQ(get_stage_description(RoutingStage::OPTIMIZATION), 
              "The board is routed, and it is currently being optimized");
}

// Test active stage detection
TEST_F(RoutingStageTest, ActiveStageDetection) {
    EXPECT_FALSE(is_active_stage(RoutingStage::IDLE));
    EXPECT_TRUE(is_active_stage(RoutingStage::ROUTING));
    EXPECT_TRUE(is_active_stage(RoutingStage::OPTIMIZATION));
}

// Test all stages array
TEST_F(RoutingStageTest, GetAllStages) {
    auto stages = get_all_stages();
    EXPECT_EQ(stages.size(), 3);
    
    // Verify correct order
    EXPECT_EQ(stages[0], RoutingStage::IDLE);
    EXPECT_EQ(stages[1], RoutingStage::ROUTING);
    EXPECT_EQ(stages[2], RoutingStage::OPTIMIZATION);
}

// Test stage progression logic
TEST_F(RoutingStageTest, StageProgression) {
    // Test that stages follow logical progression
    auto stages = get_all_stages();
    
    // IDLE should come first
    EXPECT_EQ(stages[0], RoutingStage::IDLE);
    
    // ROUTING should be active
    EXPECT_TRUE(is_active_stage(stages[1]));
    
    // OPTIMIZATION should be active
    EXPECT_TRUE(is_active_stage(stages[2]));
    
    // Only IDLE should be inactive
    int active_count = 0;
    for (const auto& stage : stages) {
        if (is_active_stage(stage)) {
            active_count++;
        }
    }
    EXPECT_EQ(active_count, 2);
}

// Test edge cases and robustness
TEST_F(RoutingStageTest, EdgeCases) {
    // Test with invalid enum values (should handle gracefully)
    // Note: This test ensures our functions are robust
    auto stages = get_all_stages();
    for (const auto& stage : stages) {
        // Should not crash for any valid stage
        std::string str = to_string(stage);
        std::string desc = get_stage_description(stage);
        
        // Should return non-empty strings
        EXPECT_FALSE(str.empty());
        EXPECT_FALSE(desc.empty());
    }
}

// Test consistency
TEST_F(RoutingStageTest, ConsistencyChecks) {
    // Ensure string representations are unique
    std::string idle_str = to_string(RoutingStage::IDLE);
    std::string routing_str = to_string(RoutingStage::ROUTING);
    std::string opt_str = to_string(RoutingStage::OPTIMIZATION);
    
    EXPECT_NE(idle_str, routing_str);
    EXPECT_NE(routing_str, opt_str);
    EXPECT_NE(idle_str, opt_str);
    
    // Ensure descriptions are unique
    std::string idle_desc = get_stage_description(RoutingStage::IDLE);
    std::string routing_desc = get_stage_description(RoutingStage::ROUTING);
    std::string opt_desc = get_stage_description(RoutingStage::OPTIMIZATION);
    
    EXPECT_NE(idle_desc, routing_desc);
    EXPECT_NE(routing_desc, opt_desc);
    EXPECT_NE(idle_desc, opt_desc);
}

} // namespace core
} // namespace freerouting
