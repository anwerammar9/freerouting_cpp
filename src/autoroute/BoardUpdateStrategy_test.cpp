/**
 * @file BoardUpdateStrategy_test.cpp
 * @brief Unit tests for BoardUpdateStrategy enum class
 */

#include "autoroute/BoardUpdateStrategy.h"
#include <gtest/gtest.h>

namespace freerouting {
namespace autoroute {

class BoardUpdateStrategyTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Test setup if needed
    }
};

// Test basic enum functionality
TEST_F(BoardUpdateStrategyTest, BasicEnumValues) {
    EXPECT_EQ(static_cast<int>(BoardUpdateStrategy::GREEDY), 0);
    EXPECT_EQ(static_cast<int>(BoardUpdateStrategy::GLOBAL_OPTIMAL), 1);
    EXPECT_EQ(static_cast<int>(BoardUpdateStrategy::HYBRID), 2);
}

// Test string conversion
TEST_F(BoardUpdateStrategyTest, ToStringConversion) {
    EXPECT_EQ(to_string(BoardUpdateStrategy::GREEDY), "GREEDY");
    EXPECT_EQ(to_string(BoardUpdateStrategy::GLOBAL_OPTIMAL), "GLOBAL_OPTIMAL");
    EXPECT_EQ(to_string(BoardUpdateStrategy::HYBRID), "HYBRID");
}

// Test strategy descriptions
TEST_F(BoardUpdateStrategyTest, StrategyDescriptions) {
    EXPECT_EQ(get_strategy_description(BoardUpdateStrategy::GREEDY), 
              "Update immediately with local optimal choices");
    EXPECT_EQ(get_strategy_description(BoardUpdateStrategy::GLOBAL_OPTIMAL), 
              "Wait for global optimal solution before updating");
    EXPECT_EQ(get_strategy_description(BoardUpdateStrategy::HYBRID), 
              "Combination of greedy and global optimal approaches");
}

// Test immediate update detection
TEST_F(BoardUpdateStrategyTest, ImmediateUpdateDetection) {
    EXPECT_TRUE(uses_immediate_updates(BoardUpdateStrategy::GREEDY));
    EXPECT_FALSE(uses_immediate_updates(BoardUpdateStrategy::GLOBAL_OPTIMAL));
    EXPECT_FALSE(uses_immediate_updates(BoardUpdateStrategy::HYBRID));
}

// Test global optimization detection
TEST_F(BoardUpdateStrategyTest, GlobalOptimizationDetection) {
    EXPECT_FALSE(uses_global_optimization(BoardUpdateStrategy::GREEDY));
    EXPECT_TRUE(uses_global_optimization(BoardUpdateStrategy::GLOBAL_OPTIMAL));
    EXPECT_TRUE(uses_global_optimization(BoardUpdateStrategy::HYBRID));
}

// Test all strategies array
TEST_F(BoardUpdateStrategyTest, GetAllStrategies) {
    auto strategies = get_all_strategies();
    EXPECT_EQ(strategies.size(), 3);
    
    // Verify correct order
    EXPECT_EQ(strategies[0], BoardUpdateStrategy::GREEDY);
    EXPECT_EQ(strategies[1], BoardUpdateStrategy::GLOBAL_OPTIMAL);
    EXPECT_EQ(strategies[2], BoardUpdateStrategy::HYBRID);
}

// Test strategy characteristics
TEST_F(BoardUpdateStrategyTest, StrategyCharacteristics) {
    auto strategies = get_all_strategies();
    
    for (const auto& strategy : strategies) {
        // Should not crash for any valid strategy
        std::string str = to_string(strategy);
        std::string desc = get_strategy_description(strategy);
        
        // Should return non-empty strings
        EXPECT_FALSE(str.empty());
        EXPECT_FALSE(desc.empty());
        
        // Should have consistent behavior
        bool immediate = uses_immediate_updates(strategy);
        bool global = uses_global_optimization(strategy);
        
        // HYBRID should use global optimization but not immediate updates
        if (strategy == BoardUpdateStrategy::HYBRID) {
            EXPECT_FALSE(immediate);
            EXPECT_TRUE(global);
        }
    }
}

// Test strategy exclusivity and combinations
TEST_F(BoardUpdateStrategyTest, StrategyCombinations) {
    // GREEDY should be immediate only
    EXPECT_TRUE(uses_immediate_updates(BoardUpdateStrategy::GREEDY));
    EXPECT_FALSE(uses_global_optimization(BoardUpdateStrategy::GREEDY));
    
    // GLOBAL_OPTIMAL should be global only
    EXPECT_FALSE(uses_immediate_updates(BoardUpdateStrategy::GLOBAL_OPTIMAL));
    EXPECT_TRUE(uses_global_optimization(BoardUpdateStrategy::GLOBAL_OPTIMAL));
    
    // HYBRID should be global only
    EXPECT_FALSE(uses_immediate_updates(BoardUpdateStrategy::HYBRID));
    EXPECT_TRUE(uses_global_optimization(BoardUpdateStrategy::HYBRID));
}

// Test consistency
TEST_F(BoardUpdateStrategyTest, ConsistencyChecks) {
    // Ensure string representations are unique
    std::string greedy_str = to_string(BoardUpdateStrategy::GREEDY);
    std::string global_str = to_string(BoardUpdateStrategy::GLOBAL_OPTIMAL);
    std::string hybrid_str = to_string(BoardUpdateStrategy::HYBRID);
    
    EXPECT_NE(greedy_str, global_str);
    EXPECT_NE(global_str, hybrid_str);
    EXPECT_NE(greedy_str, hybrid_str);
    
    // Ensure descriptions are unique and meaningful
    std::string greedy_desc = get_strategy_description(BoardUpdateStrategy::GREEDY);
    std::string global_desc = get_strategy_description(BoardUpdateStrategy::GLOBAL_OPTIMAL);
    std::string hybrid_desc = get_strategy_description(BoardUpdateStrategy::HYBRID);
    
    EXPECT_NE(greedy_desc, global_desc);
    EXPECT_NE(global_desc, hybrid_desc);
    EXPECT_NE(greedy_desc, hybrid_desc);
    
    // Descriptions should contain relevant keywords
    EXPECT_NE(greedy_desc.find("immediately"), std::string::npos);
    EXPECT_NE(global_desc.find("global"), std::string::npos);
    EXPECT_NE(hybrid_desc.find("Combination"), std::string::npos);
}

// Test practical usage scenarios
TEST_F(BoardUpdateStrategyTest, PracticalUsage) {
    // Test strategy selection logic
    BoardUpdateStrategy strategy = BoardUpdateStrategy::GREEDY;
    
    // Should be identified as immediate update strategy
    EXPECT_TRUE(uses_immediate_updates(strategy));
    EXPECT_FALSE(uses_global_optimization(strategy));
    EXPECT_EQ(to_string(strategy), "GREEDY");
    
    // Switch to global optimal
    strategy = BoardUpdateStrategy::GLOBAL_OPTIMAL;
    
    // Should be identified as global optimization strategy
    EXPECT_FALSE(uses_immediate_updates(strategy));
    EXPECT_TRUE(uses_global_optimization(strategy));
    EXPECT_EQ(to_string(strategy), "GLOBAL_OPTIMAL");
    
    // Switch to hybrid
    strategy = BoardUpdateStrategy::HYBRID;
    
    // Should be identified as hybrid strategy
    EXPECT_FALSE(uses_immediate_updates(strategy));
    EXPECT_TRUE(uses_global_optimization(strategy));
    EXPECT_EQ(to_string(strategy), "HYBRID");
    
    // Test description usage
    std::string desc = get_strategy_description(strategy);
    EXPECT_NE(desc.find("Combination"), std::string::npos);
}

// Test compilation-time evaluation
TEST_F(BoardUpdateStrategyTest, CompileTimeEvaluation) {
    // These should be evaluated at compile-time
    constexpr bool greedy_immediate = uses_immediate_updates(BoardUpdateStrategy::GREEDY);
    constexpr bool global_optimal = uses_global_optimization(BoardUpdateStrategy::GLOBAL_OPTIMAL);
    constexpr bool hybrid_global = uses_global_optimization(BoardUpdateStrategy::HYBRID);
    
    EXPECT_TRUE(greedy_immediate);
    EXPECT_TRUE(global_optimal);
    EXPECT_TRUE(hybrid_global);
    
    constexpr auto strategies = get_all_strategies();
    EXPECT_EQ(strategies.size(), 3);
}

} // namespace autoroute
} // namespace freerouting
