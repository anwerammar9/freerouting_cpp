/**
 * @file StopRequestState_test.cpp
 * @brief Unit tests for StopRequestState enum class
 */

#include "StopRequestState.h"
#include <gtest/gtest.h>

namespace freerouting {
namespace core {

class StopRequestStateTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Test setup if needed
    }
};

// Test basic enum functionality
TEST_F(StopRequestStateTest, BasicEnumValues) {
    EXPECT_EQ(static_cast<int>(StopRequestState::NONE), 0);
    EXPECT_EQ(static_cast<int>(StopRequestState::AUTO_ROUTER_ONLY), 1);
    EXPECT_EQ(static_cast<int>(StopRequestState::ALL), 2);
}

// Test string conversion
TEST_F(StopRequestStateTest, ToStringConversion) {
    EXPECT_EQ(to_string(StopRequestState::NONE), "NONE");
    EXPECT_EQ(to_string(StopRequestState::AUTO_ROUTER_ONLY), "AUTO_ROUTER_ONLY");
    EXPECT_EQ(to_string(StopRequestState::ALL), "ALL");
}

// Test stop descriptions
TEST_F(StopRequestStateTest, StopDescriptions) {
    EXPECT_EQ(get_stop_description(StopRequestState::NONE), 
              "No stop is requested");
    EXPECT_EQ(get_stop_description(StopRequestState::AUTO_ROUTER_ONLY), 
              "Only the auto-router is requested to stop");
    EXPECT_EQ(get_stop_description(StopRequestState::ALL), 
              "The entire thread is requested to stop");
}

// Test autorouter stop detection
TEST_F(StopRequestStateTest, AutorouterStopDetection) {
    EXPECT_FALSE(should_stop_autorouter(StopRequestState::NONE));
    EXPECT_TRUE(should_stop_autorouter(StopRequestState::AUTO_ROUTER_ONLY));
    EXPECT_TRUE(should_stop_autorouter(StopRequestState::ALL));
}

// Test thread stop detection
TEST_F(StopRequestStateTest, ThreadStopDetection) {
    EXPECT_FALSE(should_stop_thread(StopRequestState::NONE));
    EXPECT_FALSE(should_stop_thread(StopRequestState::AUTO_ROUTER_ONLY));
    EXPECT_TRUE(should_stop_thread(StopRequestState::ALL));
}

// Test all stop states array
TEST_F(StopRequestStateTest, GetAllStopStates) {
    auto states = get_all_stop_states();
    EXPECT_EQ(states.size(), 3);
    
    // Verify correct order (increasing severity)
    EXPECT_EQ(states[0], StopRequestState::NONE);
    EXPECT_EQ(states[1], StopRequestState::AUTO_ROUTER_ONLY);
    EXPECT_EQ(states[2], StopRequestState::ALL);
}

// Test stop request hierarchy
TEST_F(StopRequestStateTest, StopRequestHierarchy) {
    auto states = get_all_stop_states();
    
    // Test that higher severity states include lower severity effects
    for (const auto& state : states) {
        if (should_stop_thread(state)) {
            // If thread should stop, autorouter should also stop
            EXPECT_TRUE(should_stop_autorouter(state)) 
                << "Thread stop state should also stop autorouter: " << to_string(state);
        }
    }
    
    // Verify specific cases
    EXPECT_FALSE(should_stop_autorouter(StopRequestState::NONE));
    EXPECT_FALSE(should_stop_thread(StopRequestState::NONE));
    
    EXPECT_TRUE(should_stop_autorouter(StopRequestState::AUTO_ROUTER_ONLY));
    EXPECT_FALSE(should_stop_thread(StopRequestState::AUTO_ROUTER_ONLY));
    
    EXPECT_TRUE(should_stop_autorouter(StopRequestState::ALL));
    EXPECT_TRUE(should_stop_thread(StopRequestState::ALL));
}

// Test state progression logic
TEST_F(StopRequestStateTest, StateProgression) {
    // Test logical progression from no stop to complete stop
    auto states = get_all_stop_states();
    
    int autorouter_stop_count = 0;
    int thread_stop_count = 0;
    
    for (const auto& state : states) {
        if (should_stop_autorouter(state)) {
            autorouter_stop_count++;
        }
        if (should_stop_thread(state)) {
            thread_stop_count++;
        }
    }
    
    // Should have 2 autorouter stops and 1 thread stop
    EXPECT_EQ(autorouter_stop_count, 2);
    EXPECT_EQ(thread_stop_count, 1);
    
    // Thread stop should be subset of autorouter stop
    EXPECT_LE(thread_stop_count, autorouter_stop_count);
}

// Test edge cases and robustness
TEST_F(StopRequestStateTest, EdgeCases) {
    auto states = get_all_stop_states();
    for (const auto& state : states) {
        // Should not crash for any valid state
        std::string str = to_string(state);
        std::string desc = get_stop_description(state);
        
        // Should return non-empty strings
        EXPECT_FALSE(str.empty());
        EXPECT_FALSE(desc.empty());
        
        // Should have consistent behavior
        bool autorouter_stop = should_stop_autorouter(state);
        bool thread_stop = should_stop_thread(state);
        
        // Thread stop implies autorouter stop
        if (thread_stop) {
            EXPECT_TRUE(autorouter_stop);
        }
    }
}

// Test consistency
TEST_F(StopRequestStateTest, ConsistencyChecks) {
    // Ensure string representations are unique
    std::string none_str = to_string(StopRequestState::NONE);
    std::string autorouter_str = to_string(StopRequestState::AUTO_ROUTER_ONLY);
    std::string all_str = to_string(StopRequestState::ALL);
    
    EXPECT_NE(none_str, autorouter_str);
    EXPECT_NE(autorouter_str, all_str);
    EXPECT_NE(none_str, all_str);
    
    // Ensure descriptions are unique and meaningful
    std::string none_desc = get_stop_description(StopRequestState::NONE);
    std::string autorouter_desc = get_stop_description(StopRequestState::AUTO_ROUTER_ONLY);
    std::string all_desc = get_stop_description(StopRequestState::ALL);
    
    EXPECT_NE(none_desc, autorouter_desc);
    EXPECT_NE(autorouter_desc, all_desc);
    EXPECT_NE(none_desc, all_desc);
    
    // Descriptions should contain relevant keywords
    EXPECT_NE(none_desc.find("No stop"), std::string::npos);
    EXPECT_NE(autorouter_desc.find("auto-router"), std::string::npos);
    EXPECT_NE(all_desc.find("entire thread"), std::string::npos);
}

} // namespace core
} // namespace freerouting
