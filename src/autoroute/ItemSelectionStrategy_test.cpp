/**
 * @file ItemSelectionStrategy_test.cpp
 * @brief Unit tests for ItemSelectionStrategy enum class
 */

#include "autoroute/ItemSelectionStrategy.h"
#include <gtest/gtest.h>

namespace freerouting {
namespace autoroute {

class ItemSelectionStrategyTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Test setup if needed
    }
};

// Test basic enum functionality
TEST_F(ItemSelectionStrategyTest, BasicEnumValues) {
    EXPECT_EQ(static_cast<int>(ItemSelectionStrategy::SEQUENTIAL), 0);
    EXPECT_EQ(static_cast<int>(ItemSelectionStrategy::RANDOM), 1);
    EXPECT_EQ(static_cast<int>(ItemSelectionStrategy::PRIORITIZED), 2);
}

// Test string conversion
TEST_F(ItemSelectionStrategyTest, ToStringConversion) {
    EXPECT_EQ(to_string(ItemSelectionStrategy::SEQUENTIAL), "SEQUENTIAL");
    EXPECT_EQ(to_string(ItemSelectionStrategy::RANDOM), "RANDOM");
    EXPECT_EQ(to_string(ItemSelectionStrategy::PRIORITIZED), "PRIORITIZED");
}

// Test strategy descriptions
TEST_F(ItemSelectionStrategyTest, StrategyDescriptions) {
    EXPECT_EQ(get_strategy_description(ItemSelectionStrategy::SEQUENTIAL), 
              "Process items in sequential order");
    EXPECT_EQ(get_strategy_description(ItemSelectionStrategy::RANDOM), 
              "Process items in random order");
    EXPECT_EQ(get_strategy_description(ItemSelectionStrategy::PRIORITIZED), 
              "Process items based on priority");
}

// Test deterministic ordering detection
TEST_F(ItemSelectionStrategyTest, DeterministicOrderingDetection) {
    EXPECT_TRUE(uses_deterministic_ordering(ItemSelectionStrategy::SEQUENTIAL));
    EXPECT_FALSE(uses_deterministic_ordering(ItemSelectionStrategy::RANDOM));
    EXPECT_TRUE(uses_deterministic_ordering(ItemSelectionStrategy::PRIORITIZED));
}

// Test priority selection detection
TEST_F(ItemSelectionStrategyTest, PrioritySelectionDetection) {
    EXPECT_FALSE(uses_priority_selection(ItemSelectionStrategy::SEQUENTIAL));
    EXPECT_FALSE(uses_priority_selection(ItemSelectionStrategy::RANDOM));
    EXPECT_TRUE(uses_priority_selection(ItemSelectionStrategy::PRIORITIZED));
}

// Test all selection strategies array
TEST_F(ItemSelectionStrategyTest, GetAllSelectionStrategies) {
    auto strategies = get_all_selection_strategies();
    EXPECT_EQ(strategies.size(), 3);
    
    // Verify correct order
    EXPECT_EQ(strategies[0], ItemSelectionStrategy::SEQUENTIAL);
    EXPECT_EQ(strategies[1], ItemSelectionStrategy::RANDOM);
    EXPECT_EQ(strategies[2], ItemSelectionStrategy::PRIORITIZED);
}

// Test strategy characteristics
TEST_F(ItemSelectionStrategyTest, StrategyCharacteristics) {
    auto strategies = get_all_selection_strategies();
    
    for (const auto& strategy : strategies) {
        // Should not crash for any valid strategy
        std::string str = to_string(strategy);
        std::string desc = get_strategy_description(strategy);
        
        // Should return non-empty strings
        EXPECT_FALSE(str.empty());
        EXPECT_FALSE(desc.empty());
        
        // Should have consistent behavior
        bool deterministic = uses_deterministic_ordering(strategy);
        bool priority = uses_priority_selection(strategy);
        
        // RANDOM should be non-deterministic and non-priority
        if (strategy == ItemSelectionStrategy::RANDOM) {
            EXPECT_FALSE(deterministic);
            EXPECT_FALSE(priority);
        }
        
        // PRIORITIZED should be deterministic and priority-based
        if (strategy == ItemSelectionStrategy::PRIORITIZED) {
            EXPECT_TRUE(deterministic);
            EXPECT_TRUE(priority);
        }
    }
}

// Test strategy relationships
TEST_F(ItemSelectionStrategyTest, StrategyRelationships) {
    // SEQUENTIAL should be deterministic but not priority-based
    EXPECT_TRUE(uses_deterministic_ordering(ItemSelectionStrategy::SEQUENTIAL));
    EXPECT_FALSE(uses_priority_selection(ItemSelectionStrategy::SEQUENTIAL));
    
    // RANDOM should be neither deterministic nor priority-based
    EXPECT_FALSE(uses_deterministic_ordering(ItemSelectionStrategy::RANDOM));
    EXPECT_FALSE(uses_priority_selection(ItemSelectionStrategy::RANDOM));
    
    // PRIORITIZED should be both deterministic and priority-based
    EXPECT_TRUE(uses_deterministic_ordering(ItemSelectionStrategy::PRIORITIZED));
    EXPECT_TRUE(uses_priority_selection(ItemSelectionStrategy::PRIORITIZED));
}

// Test consistency
TEST_F(ItemSelectionStrategyTest, ConsistencyChecks) {
    // Ensure string representations are unique
    std::string sequential_str = to_string(ItemSelectionStrategy::SEQUENTIAL);
    std::string random_str = to_string(ItemSelectionStrategy::RANDOM);
    std::string prioritized_str = to_string(ItemSelectionStrategy::PRIORITIZED);
    
    EXPECT_NE(sequential_str, random_str);
    EXPECT_NE(random_str, prioritized_str);
    EXPECT_NE(sequential_str, prioritized_str);
    
    // Ensure descriptions are unique and meaningful
    std::string sequential_desc = get_strategy_description(ItemSelectionStrategy::SEQUENTIAL);
    std::string random_desc = get_strategy_description(ItemSelectionStrategy::RANDOM);
    std::string prioritized_desc = get_strategy_description(ItemSelectionStrategy::PRIORITIZED);
    
    EXPECT_NE(sequential_desc, random_desc);
    EXPECT_NE(random_desc, prioritized_desc);
    EXPECT_NE(sequential_desc, prioritized_desc);
    
    // Descriptions should contain relevant keywords
    EXPECT_NE(sequential_desc.find("sequential"), std::string::npos);
    EXPECT_NE(random_desc.find("random"), std::string::npos);
    EXPECT_NE(prioritized_desc.find("priority"), std::string::npos);
}

// Test practical usage scenarios
TEST_F(ItemSelectionStrategyTest, PracticalUsage) {
    // Test strategy selection logic
    ItemSelectionStrategy strategy = ItemSelectionStrategy::SEQUENTIAL;
    
    // Should be identified as deterministic strategy
    EXPECT_TRUE(uses_deterministic_ordering(strategy));
    EXPECT_FALSE(uses_priority_selection(strategy));
    EXPECT_EQ(to_string(strategy), "SEQUENTIAL");
    
    // Switch to random
    strategy = ItemSelectionStrategy::RANDOM;
    
    // Should be identified as non-deterministic strategy
    EXPECT_FALSE(uses_deterministic_ordering(strategy));
    EXPECT_FALSE(uses_priority_selection(strategy));
    EXPECT_EQ(to_string(strategy), "RANDOM");
    
    // Switch to prioritized
    strategy = ItemSelectionStrategy::PRIORITIZED;
    
    // Should be identified as priority-based strategy
    EXPECT_TRUE(uses_deterministic_ordering(strategy));
    EXPECT_TRUE(uses_priority_selection(strategy));
    EXPECT_EQ(to_string(strategy), "PRIORITIZED");
    
    // Test description usage
    std::string desc = get_strategy_description(strategy);
    EXPECT_NE(desc.find("priority"), std::string::npos);
}

// Test compilation-time evaluation
TEST_F(ItemSelectionStrategyTest, CompileTimeEvaluation) {
    // These should be evaluated at compile-time
    constexpr bool sequential_deterministic = uses_deterministic_ordering(ItemSelectionStrategy::SEQUENTIAL);
    constexpr bool random_deterministic = uses_deterministic_ordering(ItemSelectionStrategy::RANDOM);
    constexpr bool prioritized_priority = uses_priority_selection(ItemSelectionStrategy::PRIORITIZED);
    
    EXPECT_TRUE(sequential_deterministic);
    EXPECT_FALSE(random_deterministic);
    EXPECT_TRUE(prioritized_priority);
    
    constexpr auto strategies = get_all_selection_strategies();
    EXPECT_EQ(strategies.size(), 3);
}

} // namespace autoroute
} // namespace freerouting
