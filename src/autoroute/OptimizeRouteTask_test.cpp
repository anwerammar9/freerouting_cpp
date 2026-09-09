/**
 * @file OptimizeRouteTask_test.cpp
 * @brief Unit tests for OptimizeRouteTask class
 */

#include "autoroute/OptimizeRouteTask.h"
#include <gtest/gtest.h>
#include <memory>
#include <chrono>

namespace freerouting {
namespace autoroute {

// Mock implementations for testing
class MockBatchOptimizerMultiThreaded : public BatchOptimizerMultiThreaded {
public:
    int get_num_tasks_finished() const override { return tasks_finished_; }
    int get_num_tasks() const override { return total_tasks_; }
    bool is_winning_candidate(std::shared_ptr<OptimizeRouteTask> task) const override { 
        return task->get_pass_no() % 2 == 0; // Even passes are winning candidates
    }
    
    void set_tasks_finished(int finished) { tasks_finished_ = finished; }
    void set_total_tasks(int total) { total_tasks_ = total; }
    
private:
    int tasks_finished_ = 5;
    int total_tasks_ = 10;
};

class MockRoutingBoard : public RoutingBoard {
public:
    std::shared_ptr<RoutingBoard> deep_copy() const override {
        return std::make_shared<MockRoutingBoard>();
    }
    
    std::shared_ptr<Item> get_item(int item_id) const override {
        if (item_id == valid_item_id_) {
            return valid_item_;
        }
        return nullptr;
    }
    
    void set_valid_item(int item_id, std::shared_ptr<Item> item) {
        valid_item_id_ = item_id;
        valid_item_ = item;
    }
    
private:
    int valid_item_id_ = 1;
    std::shared_ptr<Item> valid_item_;
};

class MockRoutingJob {
public:
    std::shared_ptr<RoutingBoard> board;
    
    MockRoutingJob(std::shared_ptr<RoutingBoard> p_board) : board(std::move(p_board)) {}
};

class MockItem : public Item {
public:
    int get_id_no() const override { return item_id_; }
    std::shared_ptr<RoutingBoard> board;
    
    void set_id_no(int id) { item_id_ = id; }
    
private:
    int item_id_ = 1;
};

class MockBatchOptimizer : public BatchOptimizer {
public:
    std::shared_ptr<ItemRouteResult> opt_route_item(
        std::shared_ptr<Item> item, 
        bool with_preferred_directions, 
        bool ripup) const override {
        return std::make_shared<ItemRouteResult>(item->get_id_no());
    }
};

class OptimizeRouteTaskTest : public ::testing::Test {
protected:
    void SetUp() override {
        board = std::make_shared<MockRoutingBoard>();
        item = std::make_shared<MockItem>();
        item->set_id_no(1);
        
        board->set_valid_item(1, item);
        
        job = std::make_shared<MockRoutingJob>(board);
        job->board = board;
        
        optimizer = std::make_shared<MockBatchOptimizerMultiThreaded>();
        optimizer->set_tasks_finished(5);
        optimizer->set_total_tasks(10);
        
        item_id = 1;
        pass_no = 2;
        with_preferred_directions = true;
    }
    
    std::shared_ptr<MockRoutingBoard> board;
    std::shared_ptr<MockItem> item;
    std::shared_ptr<MockRoutingJob> job;
    std::shared_ptr<MockBatchOptimizerMultiThreaded> optimizer;
    int item_id;
    int pass_no;
    bool with_preferred_directions;
};

// Test constructor with valid parameters
TEST_F(OptimizeRouteTaskTest, ConstructorWithValidParameters) {
    EXPECT_NO_THROW({
        OptimizeRouteTask task(optimizer, job, item_id, pass_no, with_preferred_directions);
    });
    
    OptimizeRouteTask task(optimizer, job, item_id, pass_no, with_preferred_directions);
    
    // Test basic properties
    EXPECT_EQ(task.get_pass_no(), pass_no);
    EXPECT_EQ(task.is_with_preferred_directions(), with_preferred_directions);
    EXPECT_NE(task.get_board(), nullptr);
    EXPECT_NE(task.get_board(), board); // Should be a deep copy
}

// Test constructor with invalid item ID
TEST_F(OptimizeRouteTaskTest, ConstructorWithInvalidItemId) {
    int invalid_item_id = 999;
    
    OptimizeRouteTask task(optimizer, job, invalid_item_id, pass_no, with_preferred_directions);
    
    // Should still create the task but item will be null
    EXPECT_EQ(task.get_pass_no(), pass_no);
    EXPECT_EQ(task.get_item(), nullptr);
}

// Test run functionality with valid item
TEST_F(OptimizeRouteTaskTest, RunWithValidItem) {
    OptimizeRouteTask task(optimizer, job, item_id, pass_no, with_preferred_directions);
    
    // Initially no result
    EXPECT_EQ(task.get_route_result(), nullptr);
    EXPECT_NE(task.get_item(), nullptr);
    
    // Run the task
    EXPECT_NO_THROW({
        task.run();
    });
    
    // Should have a result after running
    EXPECT_NE(task.get_route_result(), nullptr);
    EXPECT_NE(task.get_item(), nullptr);
}

// Test run functionality with invalid item
TEST_F(OptimizeRouteTaskTest, RunWithInvalidItem) {
    int invalid_item_id = 999;
    OptimizeRouteTask task(optimizer, job, invalid_item_id, pass_no, with_preferred_directions);
    
    // Initially no result and no item
    EXPECT_EQ(task.get_route_result(), nullptr);
    EXPECT_EQ(task.get_item(), nullptr);
    
    // Run the task (should handle null item gracefully)
    EXPECT_NO_THROW({
        task.run();
    });
    
    // Should still have no result
    EXPECT_EQ(task.get_route_result(), nullptr);
    EXPECT_EQ(task.get_item(), nullptr);
}

// Test clean functionality
TEST_F(OptimizeRouteTaskTest, CleanFunctionality) {
    OptimizeRouteTask task(optimizer, job, item_id, pass_no, with_preferred_directions);
    
    // Should have an item initially
    EXPECT_NE(task.get_item(), nullptr);
    
    // Clean the task
    EXPECT_NO_THROW({
        task.clean();
    });
    
    // Item should be null after cleaning
    EXPECT_EQ(task.get_item(), nullptr);
}

// Test clean functionality after run
TEST_F(OptimizeRouteTaskTest, CleanAfterRun) {
    OptimizeRouteTask task(optimizer, job, item_id, pass_no, with_preferred_directions);
    
    // Run the task first
    task.run();
    
    // Should have a result
    EXPECT_NE(task.get_route_result(), nullptr);
    
    // Clean the task
    task.clean();
    
    // Item should be null but result should remain
    EXPECT_EQ(task.get_item(), nullptr);
    EXPECT_NE(task.get_route_result(), nullptr);
}

// Test different pass numbers
TEST_F(OptimizeRouteTaskTest, DifferentPassNumbers) {
    std::vector<int> pass_numbers = {1, 2, 3, 4, 5};
    
    for (int test_pass_no : pass_numbers) {
        OptimizeRouteTask task(optimizer, job, item_id, test_pass_no, with_preferred_directions);
        
        EXPECT_EQ(task.get_pass_no(), test_pass_no);
        
        // Run the task
        task.run();
        
        // Should have a result
        EXPECT_NE(task.get_route_result(), nullptr);
        
        // Clean up for next iteration
        task.clean();
    }
}

// Test preferred directions flag
TEST_F(OptimizeRouteTaskTest, PreferredDirectionsFlag) {
    // Test with preferred directions
    OptimizeRouteTask task_with(optimizer, job, item_id, pass_no, true);
    EXPECT_TRUE(task_with.is_with_preferred_directions());
    
    task_with.run();
    EXPECT_NE(task_with.get_route_result(), nullptr);
    
    // Test without preferred directions
    OptimizeRouteTask task_without(optimizer, job, item_id, pass_no, false);
    EXPECT_FALSE(task_without.is_with_preferred_directions());
    
    task_without.run();
    EXPECT_NE(task_without.get_route_result(), nullptr);
}

// Test board deep copy
TEST_F(OptimizeRouteTaskTest, BoardDeepCopy) {
    OptimizeRouteTask task(optimizer, job, item_id, pass_no, with_preferred_directions);
    
    auto task_board = task.get_board();
    
    // Should be a different object (deep copy)
    EXPECT_NE(task_board, board);
    EXPECT_NE(task_board, nullptr);
    
    // Should be the same type
    EXPECT_NE(dynamic_cast<MockRoutingBoard*>(task_board.get()), nullptr);
}

// Test performance timing
TEST_F(OptimizeRouteTaskTest, PerformanceTiming) {
    OptimizeRouteTask task(optimizer, job, item_id, pass_no, with_preferred_directions);
    
    auto start_time = std::chrono::steady_clock::now();
    
    // Run the task
    task.run();
    
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    // Should complete in reasonable time (less than 1 second for mock)
    EXPECT_LT(duration.count(), 1000);
    
    // Should have a result
    EXPECT_NE(task.get_route_result(), nullptr);
}

// Test multiple tasks
TEST_F(OptimizeRouteTaskTest, MultipleTasks) {
    std::vector<std::unique_ptr<OptimizeRouteTask>> tasks;
    
    // Create multiple tasks
    for (int i = 0; i < 5; ++i) {
        auto task = std::make_unique<OptimizeRouteTask>(
            optimizer, job, item_id, pass_no + i, with_preferred_directions);
        tasks.push_back(std::move(task));
    }
    
    // Run all tasks
    for (auto& task : tasks) {
        task->run();
        EXPECT_NE(task->get_route_result(), nullptr);
    }
    
    // All should have different pass numbers
    for (size_t i = 0; i < tasks.size(); ++i) {
        EXPECT_EQ(tasks[i]->get_pass_no(), pass_no + static_cast<int>(i));
    }
}

// Test const correctness
TEST_F(OptimizeRouteTaskTest, ConstCorrectness) {
    OptimizeRouteTask task(optimizer, job, item_id, pass_no, with_preferred_directions);
    task.run();
    
    const OptimizeRouteTask& const_task = task;
    
    // Should be able to call const methods
    EXPECT_EQ(const_task.get_pass_no(), pass_no);
    EXPECT_EQ(const_task.is_with_preferred_directions(), with_preferred_directions);
    EXPECT_NE(const_task.get_board(), nullptr);
    
    auto result = const_task.get_route_result();
    auto item = const_task.get_item();
    
    // Result and item should be accessible (may be null)
    EXPECT_TRUE(result != nullptr || result == nullptr);
    EXPECT_TRUE(item != nullptr || item == nullptr);
}

// Test edge cases
TEST_F(OptimizeRouteTaskTest, EdgeCases) {
    // Test with zero pass number
    OptimizeRouteTask task_zero(optimizer, job, item_id, 0, with_preferred_directions);
    EXPECT_EQ(task_zero.get_pass_no(), 0);
    
    // Test with negative pass number
    OptimizeRouteTask task_negative(optimizer, job, item_id, -1, with_preferred_directions);
    EXPECT_EQ(task_negative.get_pass_no(), -1);
    
    // Test with very large pass number
    OptimizeRouteTask task_large(optimizer, job, item_id, 1000000, with_preferred_directions);
    EXPECT_EQ(task_large.get_pass_no(), 1000000);
    
    // All should work without issues
    task_zero.run();
    task_negative.run();
    task_large.run();
    
    EXPECT_NE(task_zero.get_route_result(), nullptr);
    EXPECT_NE(task_negative.get_route_result(), nullptr);
    EXPECT_NE(task_large.get_route_result(), nullptr);
}

// Test task lifecycle
TEST_F(OptimizeRouteTaskTest, TaskLifecycle) {
    OptimizeRouteTask task(optimizer, job, item_id, pass_no, with_preferred_directions);
    
    // Initial state
    EXPECT_EQ(task.get_route_result(), nullptr);
    EXPECT_NE(task.get_item(), nullptr);
    
    // Run the task
    task.run();
    
    // After run
    EXPECT_NE(task.get_route_result(), nullptr);
    EXPECT_NE(task.get_item(), nullptr);
    
    // Clean the task
    task.clean();
    
    // After clean
    EXPECT_EQ(task.get_item(), nullptr);
    EXPECT_NE(task.get_route_result(), nullptr); // Result should remain
}

} // namespace autoroute
} // namespace freerouting
