/**
 * @file AutorouteAttemptState_test.cpp
 * @brief Unit tests for AutorouteAttemptState enum class
 */

#include "autoroute/AutorouteAttemptState.h"
#include <gtest/gtest.h>

namespace freerouting {
namespace autoroute {

class AutorouteAttemptStateTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Test setup if needed
    }
};

// Test basic enum functionality
TEST_F(AutorouteAttemptStateTest, BasicEnumValues) {
    EXPECT_EQ(static_cast<int>(AutorouteAttemptState::UNKNOWN), 0);
    EXPECT_EQ(static_cast<int>(AutorouteAttemptState::SKIPPED), 1);
    EXPECT_EQ(static_cast<int>(AutorouteAttemptState::NO_UNCONNECTED_NETS), 2);
    EXPECT_EQ(static_cast<int>(AutorouteAttemptState::CONNECTED_TO_PLANE), 3);
    EXPECT_EQ(static_cast<int>(AutorouteAttemptState::ALREADY_CONNECTED), 4);
    EXPECT_EQ(static_cast<int>(AutorouteAttemptState::NO_CONNECTIONS), 5);
    EXPECT_EQ(static_cast<int>(AutorouteAttemptState::ROUTED), 6);
    EXPECT_EQ(static_cast<int>(AutorouteAttemptState::FAILED), 7);
    EXPECT_EQ(static_cast<int>(AutorouteAttemptState::INSERT_ERROR), 8);
}

// Test string conversion
TEST_F(AutorouteAttemptStateTest, ToStringConversion) {
    EXPECT_EQ(to_string(AutorouteAttemptState::UNKNOWN), "UNKNOWN");
    EXPECT_EQ(to_string(AutorouteAttemptState::SKIPPED), "SKIPPED");
    EXPECT_EQ(to_string(AutorouteAttemptState::NO_UNCONNECTED_NETS), "NO_UNCONNECTED_NETS");
    EXPECT_EQ(to_string(AutorouteAttemptState::CONNECTED_TO_PLANE), "CONNECTED_TO_PLANE");
    EXPECT_EQ(to_string(AutorouteAttemptState::ALREADY_CONNECTED), "ALREADY_CONNECTED");
    EXPECT_EQ(to_string(AutorouteAttemptState::NO_CONNECTIONS), "NO_CONNECTIONS");
    EXPECT_EQ(to_string(AutorouteAttemptState::ROUTED), "ROUTED");
    EXPECT_EQ(to_string(AutorouteAttemptState::FAILED), "FAILED");
    EXPECT_EQ(to_string(AutorouteAttemptState::INSERT_ERROR), "INSERT_ERROR");
}

// Test state descriptions
TEST_F(AutorouteAttemptStateTest, StateDescriptions) {
    EXPECT_EQ(get_state_description(AutorouteAttemptState::UNKNOWN), "Unknown result");
    EXPECT_EQ(get_state_description(AutorouteAttemptState::SKIPPED), "Item was skipped");
    EXPECT_EQ(get_state_description(AutorouteAttemptState::NO_UNCONNECTED_NETS), "Item has no unconnected nets");
    EXPECT_EQ(get_state_description(AutorouteAttemptState::CONNECTED_TO_PLANE), "Item is connected to a conduction plane");
    EXPECT_EQ(get_state_description(AutorouteAttemptState::ALREADY_CONNECTED), "Item is already connected");
    EXPECT_EQ(get_state_description(AutorouteAttemptState::NO_CONNECTIONS), "The item has no connections to nets");
    EXPECT_EQ(get_state_description(AutorouteAttemptState::ROUTED), "Item was successfully routed");
    EXPECT_EQ(get_state_description(AutorouteAttemptState::FAILED), "Routing failed");
    EXPECT_EQ(get_state_description(AutorouteAttemptState::INSERT_ERROR), "Error inserting item");
}

// Test successful state detection
TEST_F(AutorouteAttemptStateTest, SuccessfulStateDetection) {
    EXPECT_FALSE(is_successful(AutorouteAttemptState::UNKNOWN));
    EXPECT_FALSE(is_successful(AutorouteAttemptState::SKIPPED));
    EXPECT_FALSE(is_successful(AutorouteAttemptState::NO_UNCONNECTED_NETS));
    EXPECT_TRUE(is_successful(AutorouteAttemptState::CONNECTED_TO_PLANE));
    EXPECT_TRUE(is_successful(AutorouteAttemptState::ALREADY_CONNECTED));
    EXPECT_FALSE(is_successful(AutorouteAttemptState::NO_CONNECTIONS));
    EXPECT_TRUE(is_successful(AutorouteAttemptState::ROUTED));
    EXPECT_FALSE(is_successful(AutorouteAttemptState::FAILED));
    EXPECT_FALSE(is_successful(AutorouteAttemptState::INSERT_ERROR));
}

// Test failure state detection
TEST_F(AutorouteAttemptStateTest, FailureStateDetection) {
    EXPECT_FALSE(is_failure(AutorouteAttemptState::UNKNOWN));
    EXPECT_FALSE(is_failure(AutorouteAttemptState::SKIPPED));
    EXPECT_FALSE(is_failure(AutorouteAttemptState::NO_UNCONNECTED_NETS));
    EXPECT_FALSE(is_failure(AutorouteAttemptState::CONNECTED_TO_PLANE));
    EXPECT_FALSE(is_failure(AutorouteAttemptState::ALREADY_CONNECTED));
    EXPECT_FALSE(is_failure(AutorouteAttemptState::NO_CONNECTIONS));
    EXPECT_FALSE(is_failure(AutorouteAttemptState::ROUTED));
    EXPECT_TRUE(is_failure(AutorouteAttemptState::FAILED));
    EXPECT_TRUE(is_failure(AutorouteAttemptState::INSERT_ERROR));
}

// Test skipped state detection
TEST_F(AutorouteAttemptStateTest, SkippedStateDetection) {
    EXPECT_FALSE(is_skipped(AutorouteAttemptState::UNKNOWN));
    EXPECT_TRUE(is_skipped(AutorouteAttemptState::SKIPPED));
    EXPECT_TRUE(is_skipped(AutorouteAttemptState::NO_UNCONNECTED_NETS));
    EXPECT_FALSE(is_skipped(AutorouteAttemptState::CONNECTED_TO_PLANE));
    EXPECT_FALSE(is_skipped(AutorouteAttemptState::ALREADY_CONNECTED));
    EXPECT_TRUE(is_skipped(AutorouteAttemptState::NO_CONNECTIONS));
    EXPECT_FALSE(is_skipped(AutorouteAttemptState::ROUTED));
    EXPECT_FALSE(is_skipped(AutorouteAttemptState::FAILED));
    EXPECT_FALSE(is_skipped(AutorouteAttemptState::INSERT_ERROR));
}

// Test completed state detection
TEST_F(AutorouteAttemptStateTest, CompletedStateDetection) {
    EXPECT_FALSE(is_completed(AutorouteAttemptState::UNKNOWN));
    EXPECT_FALSE(is_completed(AutorouteAttemptState::SKIPPED));
    EXPECT_FALSE(is_completed(AutorouteAttemptState::NO_UNCONNECTED_NETS));
    EXPECT_TRUE(is_completed(AutorouteAttemptState::CONNECTED_TO_PLANE));
    EXPECT_TRUE(is_completed(AutorouteAttemptState::ALREADY_CONNECTED));
    EXPECT_FALSE(is_completed(AutorouteAttemptState::NO_CONNECTIONS));
    EXPECT_TRUE(is_completed(AutorouteAttemptState::ROUTED));
    EXPECT_TRUE(is_completed(AutorouteAttemptState::FAILED));
    EXPECT_TRUE(is_completed(AutorouteAttemptState::INSERT_ERROR));
}

// Test all states array
TEST_F(AutorouteAttemptStateTest, GetAllStates) {
    auto states = get_all_states();
    EXPECT_EQ(states.size(), 9);
    
    // Verify correct order
    EXPECT_EQ(states[0], AutorouteAttemptState::UNKNOWN);
    EXPECT_EQ(states[1], AutorouteAttemptState::SKIPPED);
    EXPECT_EQ(states[2], AutorouteAttemptState::NO_UNCONNECTED_NETS);
    EXPECT_EQ(states[3], AutorouteAttemptState::CONNECTED_TO_PLANE);
    EXPECT_EQ(states[4], AutorouteAttemptState::ALREADY_CONNECTED);
    EXPECT_EQ(states[5], AutorouteAttemptState::NO_CONNECTIONS);
    EXPECT_EQ(states[6], AutorouteAttemptState::ROUTED);
    EXPECT_EQ(states[7], AutorouteAttemptState::FAILED);
    EXPECT_EQ(states[8], AutorouteAttemptState::INSERT_ERROR);
}

// Test state characteristics
TEST_F(AutorouteAttemptStateTest, StateCharacteristics) {
    auto states = get_all_states();
    
    for (const auto& state : states) {
        // Should not crash for any valid state
        std::string str = to_string(state);
        std::string desc = get_state_description(state);
        
        // Should return non-empty strings
        EXPECT_FALSE(str.empty());
        EXPECT_FALSE(desc.empty());
        
        // Should have consistent behavior
        bool successful = is_successful(state);
        bool failure = is_failure(state);
        bool skipped = is_skipped(state);
        bool completed = is_completed(state);
        
        // Successful states should also be completed
        if (successful) {
            EXPECT_TRUE(completed);
        }
        
        // Failure states should also be completed
        if (failure) {
            EXPECT_TRUE(completed);
        }
        
        // Skipped states should not be completed
        if (skipped) {
            EXPECT_FALSE(completed);
        }
    }
}

// Test state exclusivity
TEST_F(AutorouteAttemptStateTest, StateExclusivity) {
    auto states = get_all_states();
    
    for (const auto& state : states) {
        bool successful = is_successful(state);
        bool failure = is_failure(state);
        bool skipped = is_skipped(state);
        
        // States should be mutually exclusive
        if (successful) {
            EXPECT_FALSE(failure);
            EXPECT_FALSE(skipped);
        }
        
        if (failure) {
            EXPECT_FALSE(successful);
            EXPECT_FALSE(skipped);
        }
        
        if (skipped) {
            EXPECT_FALSE(successful);
            EXPECT_FALSE(failure);
        }
    }
}

// Test specific state scenarios
TEST_F(AutorouteAttemptStateTest, SpecificStateScenarios) {
    // Test successful routing scenarios
    EXPECT_TRUE(is_successful(AutorouteAttemptState::ROUTED));
    EXPECT_TRUE(is_completed(AutorouteAttemptState::ROUTED));
    EXPECT_FALSE(is_failure(AutorouteAttemptState::ROUTED));
    EXPECT_FALSE(is_skipped(AutorouteAttemptState::ROUTED));
    
    // Test already connected scenario
    EXPECT_TRUE(is_successful(AutorouteAttemptState::ALREADY_CONNECTED));
    EXPECT_TRUE(is_completed(AutorouteAttemptState::ALREADY_CONNECTED));
    EXPECT_FALSE(is_failure(AutorouteAttemptState::ALREADY_CONNECTED));
    EXPECT_FALSE(is_skipped(AutorouteAttemptState::ALREADY_CONNECTED));
    
    // Test failure scenarios
    EXPECT_TRUE(is_failure(AutorouteAttemptState::FAILED));
    EXPECT_TRUE(is_completed(AutorouteAttemptState::FAILED));
    EXPECT_FALSE(is_successful(AutorouteAttemptState::FAILED));
    EXPECT_FALSE(is_skipped(AutorouteAttemptState::FAILED));
    
    // Test skip scenarios
    EXPECT_TRUE(is_skipped(AutorouteAttemptState::SKIPPED));
    EXPECT_FALSE(is_completed(AutorouteAttemptState::SKIPPED));
    EXPECT_FALSE(is_successful(AutorouteAttemptState::SKIPPED));
    EXPECT_FALSE(is_failure(AutorouteAttemptState::SKIPPED));
    
    EXPECT_TRUE(is_skipped(AutorouteAttemptState::NO_UNCONNECTED_NETS));
    EXPECT_FALSE(is_completed(AutorouteAttemptState::NO_UNCONNECTED_NETS));
    EXPECT_FALSE(is_successful(AutorouteAttemptState::NO_UNCONNECTED_NETS));
    EXPECT_FALSE(is_failure(AutorouteAttemptState::NO_UNCONNECTED_NETS));
}

// Test consistency
TEST_F(AutorouteAttemptStateTest, ConsistencyChecks) {
    // Ensure string representations are unique
    std::vector<std::string> strings;
    auto states = get_all_states();
    
    for (const auto& state : states) {
        std::string str = to_string(state);
        EXPECT_FALSE(str.empty());
        
        // Check for uniqueness
        EXPECT_EQ(std::count(strings.begin(), strings.end(), str), 0);
        strings.push_back(str);
    }
    
    // Ensure descriptions are unique and meaningful
    std::vector<std::string> descriptions;
    for (const auto& state : states) {
        std::string desc = get_state_description(state);
        EXPECT_FALSE(desc.empty());
        
        // Check for uniqueness
        EXPECT_EQ(std::count(descriptions.begin(), descriptions.end(), desc), 0);
        descriptions.push_back(desc);
    }
}

// Test practical usage scenarios
TEST_F(AutorouteAttemptStateTest, PracticalUsage) {
    // Test routing result analysis
    auto analyze_result = [](AutorouteAttemptState state) {
        if (is_successful(state)) {
            return "Success";
        } else if (is_failure(state)) {
            return "Failed";
        } else if (is_skipped(state)) {
            return "Skipped";
        } else {
            return "Unknown";
        }
    };
    
    EXPECT_EQ(analyze_result(AutorouteAttemptState::ROUTED), "Success");
    EXPECT_EQ(analyze_result(AutorouteAttemptState::FAILED), "Failed");
    EXPECT_EQ(analyze_result(AutorouteAttemptState::SKIPPED), "Skipped");
    EXPECT_EQ(analyze_result(AutorouteAttemptState::UNKNOWN), "Unknown");
    
    // Test completion check
    auto is_routing_complete = [](AutorouteAttemptState state) {
        return is_completed(state);
    };
    
    EXPECT_TRUE(is_routing_complete(AutorouteAttemptState::ROUTED));
    EXPECT_TRUE(is_routing_complete(AutorouteAttemptState::FAILED));
    EXPECT_FALSE(is_routing_complete(AutorouteAttemptState::SKIPPED));
}

// Test compilation-time evaluation
TEST_F(AutorouteAttemptStateTest, CompileTimeEvaluation) {
    // These should be evaluated at compile-time
    constexpr bool routed_successful = is_successful(AutorouteAttemptState::ROUTED);
    constexpr bool failed_failure = is_failure(AutorouteAttemptState::FAILED);
    constexpr bool skipped_skipped = is_skipped(AutorouteAttemptState::SKIPPED);
    constexpr bool completed_routed = is_completed(AutorouteAttemptState::ROUTED);
    
    EXPECT_TRUE(routed_successful);
    EXPECT_TRUE(failed_failure);
    EXPECT_TRUE(skipped_skipped);
    EXPECT_TRUE(completed_routed);
    
    constexpr auto states = get_all_states();
    EXPECT_EQ(states.size(), 9);
}

} // namespace autoroute
} // namespace freerouting
