/**
 * @file TaskState_test.cpp
 * @brief Unit tests for TaskState enum class
 */

#include "autoroute/TaskState.h"
#include <gtest/gtest.h>

namespace freerouting {
namespace autoroute {

class TaskStateTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Test setup if needed
    }
};

// Test basic enum functionality
TEST_F(TaskStateTest, BasicEnumValues) {
    EXPECT_EQ(static_cast<int>(TaskState::IDLE), 0);
    EXPECT_EQ(static_cast<int>(TaskState::STARTED), 1);
    EXPECT_EQ(static_cast<int>(TaskState::RUNNING), 2);
    EXPECT_EQ(static_cast<int>(TaskState::FINISHED), 3);
    EXPECT_EQ(static_cast<int>(TaskState::CANCELLED), 4);
    EXPECT_EQ(static_cast<int>(TaskState::TIMED_OUT), 5);
}

// Test string conversion
TEST_F(TaskStateTest, ToStringConversion) {
    EXPECT_EQ(to_string(TaskState::IDLE), "IDLE");
    EXPECT_EQ(to_string(TaskState::STARTED), "STARTED");
    EXPECT_EQ(to_string(TaskState::RUNNING), "RUNNING");
    EXPECT_EQ(to_string(TaskState::FINISHED), "FINISHED");
    EXPECT_EQ(to_string(TaskState::CANCELLED), "CANCELLED");
    EXPECT_EQ(to_string(TaskState::TIMED_OUT), "TIMED_OUT");
}

// Test task descriptions
TEST_F(TaskStateTest, TaskDescriptions) {
    EXPECT_EQ(get_task_description(TaskState::IDLE), "Task has not started yet");
    EXPECT_EQ(get_task_description(TaskState::STARTED), "Task has been started");
    EXPECT_EQ(get_task_description(TaskState::RUNNING), "Task is currently running");
    EXPECT_EQ(get_task_description(TaskState::FINISHED), "Task completed successfully");
    EXPECT_EQ(get_task_description(TaskState::CANCELLED), "Task was cancelled");
    EXPECT_EQ(get_task_description(TaskState::TIMED_OUT), "Task timed out");
}

// Test active state detection
TEST_F(TaskStateTest, ActiveStateDetection) {
    EXPECT_FALSE(is_active_state(TaskState::IDLE));
    EXPECT_TRUE(is_active_state(TaskState::STARTED));
    EXPECT_TRUE(is_active_state(TaskState::RUNNING));
    EXPECT_FALSE(is_active_state(TaskState::FINISHED));
    EXPECT_FALSE(is_active_state(TaskState::CANCELLED));
    EXPECT_FALSE(is_active_state(TaskState::TIMED_OUT));
}

// Test terminal state detection
TEST_F(TaskStateTest, TerminalStateDetection) {
    EXPECT_FALSE(is_terminal_state(TaskState::IDLE));
    EXPECT_FALSE(is_terminal_state(TaskState::STARTED));
    EXPECT_FALSE(is_terminal_state(TaskState::RUNNING));
    EXPECT_TRUE(is_terminal_state(TaskState::FINISHED));
    EXPECT_TRUE(is_terminal_state(TaskState::CANCELLED));
    EXPECT_TRUE(is_terminal_state(TaskState::TIMED_OUT));
}

// Test all task states array
TEST_F(TaskStateTest, GetAllTaskStates) {
    auto states = get_all_task_states();
    EXPECT_EQ(states.size(), 6);
    
    // Verify correct order
    EXPECT_EQ(states[0], TaskState::IDLE);
    EXPECT_EQ(states[1], TaskState::STARTED);
    EXPECT_EQ(states[2], TaskState::RUNNING);
    EXPECT_EQ(states[3], TaskState::FINISHED);
    EXPECT_EQ(states[4], TaskState::CANCELLED);
    EXPECT_EQ(states[5], TaskState::TIMED_OUT);
}

// Test state progression logic
TEST_F(TaskStateTest, StateProgression) {
    auto states = get_all_task_states();
    
    // Test logical progression
    int active_count = 0;
    int terminal_count = 0;
    
    for (const auto& state : states) {
        if (is_active_state(state)) {
            active_count++;
        }
        if (is_terminal_state(state)) {
            terminal_count++;
        }
    }
    
    // Should have 2 active states and 3 terminal states
    EXPECT_EQ(active_count, 2);
    EXPECT_EQ(terminal_count, 3);
    
    // IDLE should be neither active nor terminal
    EXPECT_FALSE(is_active_state(TaskState::IDLE));
    EXPECT_FALSE(is_terminal_state(TaskState::IDLE));
}

// Test state exclusivity
TEST_F(TaskStateTest, StateExclusivity) {
    // A state cannot be both active and terminal
    auto states = get_all_task_states();
    for (const auto& state : states) {
        bool active = is_active_state(state);
        bool terminal = is_terminal_state(state);
        
        // States should be mutually exclusive (except IDLE which is neither)
        if (state != TaskState::IDLE) {
            EXPECT_TRUE(active || terminal) << "State should be either active or terminal: " << to_string(state);
            EXPECT_FALSE(active && terminal) << "State cannot be both active and terminal: " << to_string(state);
        }
    }
}

// Test edge cases and robustness
TEST_F(TaskStateTest, EdgeCases) {
    auto states = get_all_task_states();
    for (const auto& state : states) {
        // Should not crash for any valid state
        std::string str = to_string(state);
        std::string desc = get_task_description(state);
        
        // Should return non-empty strings
        EXPECT_FALSE(str.empty());
        EXPECT_FALSE(desc.empty());
        
        // Should have consistent behavior
        bool active = is_active_state(state);
        bool terminal = is_terminal_state(state);
        
        // Terminal states cannot be active
        if (terminal) {
            EXPECT_FALSE(active) << "Terminal state cannot be active: " << str;
        }
    }
}

// Test consistency
TEST_F(TaskStateTest, ConsistencyChecks) {
    // Ensure string representations are unique
    std::string idle_str = to_string(TaskState::IDLE);
    std::string started_str = to_string(TaskState::STARTED);
    std::string running_str = to_string(TaskState::RUNNING);
    std::string finished_str = to_string(TaskState::FINISHED);
    std::string cancelled_str = to_string(TaskState::CANCELLED);
    std::string timeout_str = to_string(TaskState::TIMED_OUT);
    
    EXPECT_NE(idle_str, started_str);
    EXPECT_NE(started_str, running_str);
    EXPECT_NE(running_str, finished_str);
    EXPECT_NE(finished_str, cancelled_str);
    EXPECT_NE(cancelled_str, timeout_str);
    EXPECT_NE(idle_str, timeout_str);
    
    // Ensure descriptions are unique and meaningful
    std::string idle_desc = get_task_description(TaskState::IDLE);
    std::string started_desc = get_task_description(TaskState::STARTED);
    std::string running_desc = get_task_description(TaskState::RUNNING);
    std::string finished_desc = get_task_description(TaskState::FINISHED);
    std::string cancelled_desc = get_task_description(TaskState::CANCELLED);
    std::string timeout_desc = get_task_description(TaskState::TIMED_OUT);
    
    EXPECT_NE(idle_desc, started_desc);
    EXPECT_NE(started_desc, running_desc);
    EXPECT_NE(running_desc, finished_desc);
    EXPECT_NE(finished_desc, cancelled_desc);
    EXPECT_NE(cancelled_desc, timeout_desc);
    EXPECT_NE(idle_desc, timeout_desc);
    
    // Descriptions should contain relevant keywords
    EXPECT_NE(idle_desc.find("started"), std::string::npos);
    EXPECT_NE(started_desc.find("started"), std::string::npos);
    EXPECT_NE(running_desc.find("running"), std::string::npos);
    EXPECT_NE(finished_desc.find("completed"), std::string::npos);
    EXPECT_NE(cancelled_desc.find("cancelled"), std::string::npos);
    EXPECT_NE(timeout_desc.find("timed"), std::string::npos);
}

// Test practical usage scenarios
TEST_F(TaskStateTest, PracticalUsage) {
    // Test typical task lifecycle
    TaskState state = TaskState::IDLE;
    
    // Task starts
    state = TaskState::STARTED;
    EXPECT_TRUE(is_active_state(state));
    EXPECT_FALSE(is_terminal_state(state));
    
    // Task runs
    state = TaskState::RUNNING;
    EXPECT_TRUE(is_active_state(state));
    EXPECT_FALSE(is_terminal_state(state));
    
    // Task completes successfully
    state = TaskState::FINISHED;
    EXPECT_FALSE(is_active_state(state));
    EXPECT_TRUE(is_terminal_state(state));
    
    // Test failure scenarios
    state = TaskState::CANCELLED;
    EXPECT_FALSE(is_active_state(state));
    EXPECT_TRUE(is_terminal_state(state));
    
    state = TaskState::TIMED_OUT;
    EXPECT_FALSE(is_active_state(state));
    EXPECT_TRUE(is_terminal_state(state));
}

} // namespace autoroute
} // namespace freerouting
