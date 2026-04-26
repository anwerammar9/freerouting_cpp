/**
 * @file RoutingFailureLog_test.cpp
 * @brief Unit tests for RoutingFailureLog class
 */

#include "autoroute/RoutingFailureLog.h"
#include <gtest/gtest.h>
#include <memory>
#include <thread>
#include <vector>

namespace freerouting {
namespace autoroute {

// Mock implementations for testing
class MockItem : public Item {
public:
    int item_id;
    int net_count_val;
    int net_no_val;
    
    MockItem(int id, int net_cnt = 1, int net = 1) 
        : item_id(id), net_count_val(net_cnt), net_no_val(net) {}
    
    int get_id_no() const override { return item_id; }
    int net_count() const override { return net_count_val; }
    int get_net_no(int index) const override { 
        return (index == 0) ? net_no_val : -1; 
    }
    
    void set_id(int id) { item_id = id; }
    void set_net_count(int cnt) { net_count_val = cnt; }
    void set_net_no(int net) { net_no_val = net; }
};

class RoutingFailureLogTest : public ::testing::Test {
protected:
    void SetUp() override {
        log = std::make_unique<RoutingFailureLog>();
    }
    
    std::unique_ptr<RoutingFailureLog> log;
};

// Test constructor and basic functionality
TEST_F(RoutingFailureLogTest, ConstructorAndBasicFunctionality) {
    EXPECT_EQ(log->get_item_count(), 0);
    EXPECT_FALSE(log->has_unroutable_items());
    
    auto stats = log->get_statistics();
    EXPECT_EQ(stats.first, 0);  // total items
    EXPECT_EQ(stats.second, 0); // unroutable items
}

// Test failure recording
TEST_F(RoutingFailureLogTest, RecordFailure) {
    auto item = std::make_shared<MockItem>(1);
    
    // Record a failure
    log->record_failure(item, 1, AutorouteAttemptState::FAILED, "Test failure");
    
    EXPECT_EQ(log->get_item_count(), 1);
    EXPECT_EQ(log->get_failure_count(item), 1);
    EXPECT_FALSE(log->has_unroutable_items());
    EXPECT_FALSE(log->should_skip(item));
}

// Test multiple failures
TEST_F(RoutingFailureLogTest, MultipleFailures) {
    auto item = std::make_shared<MockItem>(1);
    
    // Record multiple failures
    for (int i = 0; i < 10; ++i) {
        log->record_failure(item, i + 1, AutorouteAttemptState::FAILED, 
                           "Failure " + std::to_string(i));
    }
    
    EXPECT_EQ(log->get_item_count(), 1);
    EXPECT_EQ(log->get_failure_count(item), 10);
    EXPECT_FALSE(log->has_unroutable_items());
    EXPECT_FALSE(log->should_skip(item));
}

// Test failure threshold
TEST_F(RoutingFailureLogTest, FailureThreshold) {
    auto item = std::make_shared<MockItem>(1);
    
    // Record failures up to threshold
    for (int i = 0; i < RoutingFailureLog::FAILURE_THRESHOLD; ++i) {
        log->record_failure(item, i + 1, AutorouteAttemptState::FAILED, 
                           "Failure " + std::to_string(i));
    }
    
    EXPECT_EQ(log->get_failure_count(item), RoutingFailureLog::FAILURE_THRESHOLD);
    EXPECT_TRUE(log->has_unroutable_items());
    EXPECT_TRUE(log->should_skip(item));
    
    auto unroutable = log->get_unroutable_items();
    EXPECT_EQ(unroutable.size(), 1);
    EXPECT_EQ(unroutable[0]->get_failure_count(), RoutingFailureLog::FAILURE_THRESHOLD);
}

// Test multiple items
TEST_F(RoutingFailureLogTest, MultipleItems) {
    auto item1 = std::make_shared<MockItem>(1);
    auto item2 = std::make_shared<MockItem>(2);
    auto item3 = std::make_shared<MockItem>(3);
    
    // Record failures for different items
    log->record_failure(item1, 1, AutorouteAttemptState::FAILED, "Item 1 failure");
    log->record_failure(item2, 1, AutorouteAttemptState::FAILED, "Item 2 failure");
    log->record_failure(item3, 1, AutorouteAttemptState::FAILED, "Item 3 failure");
    
    EXPECT_EQ(log->get_item_count(), 3);
    EXPECT_EQ(log->get_failure_count(item1), 1);
    EXPECT_EQ(log->get_failure_count(item2), 1);
    EXPECT_EQ(log->get_failure_count(item3), 1);
    
    auto stats = log->get_statistics();
    EXPECT_EQ(stats.first, 3);  // total items
    EXPECT_EQ(stats.second, 0); // unroutable items
}

// Test different failure states
TEST_F(RoutingFailureLogTest, DifferentFailureStates) {
    auto item = std::make_shared<MockItem>(1);
    
    // Record failures with different states
    log->record_failure(item, 1, AutorouteAttemptState::FAILED, "Failed");
    log->record_failure(item, 2, AutorouteAttemptState::SKIPPED, "Skipped");
    log->record_failure(item, 3, AutorouteAttemptState::NO_CONNECTIONS, "No connections");
    
    EXPECT_EQ(log->get_failure_count(item), 3);
    
    auto failure_info = log->get_unroutable_items();
    if (!failure_info.empty()) {
        EXPECT_EQ(failure_info[0]->get_last_failure_state(), AutorouteAttemptState::NO_CONNECTIONS);
        EXPECT_EQ(failure_info[0]->get_last_failure_reason(), "No connections");
        EXPECT_EQ(failure_info[0]->get_last_attempt_pass(), 3);
    }
}

// Test null item handling
TEST_F(RoutingFailureLogTest, NullItemHandling) {
    std::shared_ptr<Item> null_item = nullptr;
    
    // Should not crash with null item
    EXPECT_NO_THROW({
        log->record_failure(null_item, 1, AutorouteAttemptState::FAILED, "Null item");
    });
    
    EXPECT_EQ(log->get_item_count(), 0);
    EXPECT_EQ(log->get_failure_count(null_item), 0);
    EXPECT_FALSE(log->should_skip(null_item));
}

// Test clear functionality
TEST_F(RoutingFailureLogTest, ClearFunctionality) {
    auto item1 = std::make_shared<MockItem>(1);
    auto item2 = std::make_shared<MockItem>(2);
    
    // Record some failures
    log->record_failure(item1, 1, AutorouteAttemptState::FAILED, "Item 1 failure");
    log->record_failure(item2, 1, AutorouteAttemptState::FAILED, "Item 2 failure");
    
    EXPECT_EQ(log->get_item_count(), 2);
    
    // Clear the log
    log->clear();
    
    EXPECT_EQ(log->get_item_count(), 0);
    EXPECT_EQ(log->get_failure_count(item1), 0);
    EXPECT_EQ(log->get_failure_count(item2), 0);
    EXPECT_FALSE(log->has_unroutable_items());
}

// Test thread safety
TEST_F(RoutingFailureLogTest, ThreadSafety) {
    auto item = std::make_shared<MockItem>(1);
    const int num_threads = 10;
    const int failures_per_thread = 10;
    
    std::vector<std::thread> threads;
    
    // Create multiple threads that record failures
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([this, item, i, failures_per_thread]() {
            for (int j = 0; j < failures_per_thread; ++j) {
                log->record_failure(item, i * failures_per_thread + j, 
                                   AutorouteAttemptState::FAILED, 
                                   "Thread " + std::to_string(i) + " failure " + std::to_string(j));
            }
        });
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Should have recorded all failures
    EXPECT_EQ(log->get_failure_count(item), num_threads * failures_per_thread);
    
    auto stats = log->get_statistics();
    EXPECT_EQ(stats.first, 1);  // total items
    EXPECT_EQ(stats.second, 0); // unroutable items (assuming threshold not reached)
}

// Test ItemFailureInfo functionality
TEST_F(RoutingFailureLogTest, ItemFailureInfoFunctionality) {
    auto item = std::make_shared<MockItem>(1, 2, 5); // item with net_count=2, net_no=5
    
    // Record failures
    log->record_failure(item, 1, AutorouteAttemptState::FAILED, "First failure");
    log->record_failure(item, 2, AutorouteAttemptState::SKIPPED, "Second failure");
    log->record_failure(item, 3, AutorouteAttemptState::NO_CONNECTIONS, "Third failure");
    
    auto unroutable = log->get_unroutable_items();
    
    if (!unroutable.empty()) {
        auto failure_info = unroutable[0];
        
        // Test failure info properties
        EXPECT_EQ(failure_info->get_failure_count(), 3);
        EXPECT_EQ(failure_info->get_last_failure_state(), AutorouteAttemptState::NO_CONNECTIONS);
        EXPECT_EQ(failure_info->get_last_failure_reason(), "Third failure");
        EXPECT_EQ(failure_info->get_last_attempt_pass(), 3);
        
        // Test string representation
        std::string str = failure_info->to_string();
        EXPECT_FALSE(str.empty());
        EXPECT_NE(str.find("3 failures"), std::string::npos);
    }
}

// Test item with no networks
TEST_F(RoutingFailureLogTest, ItemWithNoNetworks) {
    auto item = std::make_shared<MockItem>(1, 0, -1); // item with no networks
    
    log->record_failure(item, 1, AutorouteAttemptState::FAILED, "No networks");
    
    EXPECT_EQ(log->get_item_count(), 1);
    EXPECT_EQ(log->get_failure_count(item), 1);
    
    auto unroutable = log->get_unroutable_items();
    if (!unroutable.empty()) {
        // Should handle item with no networks gracefully
        EXPECT_EQ(unroutable[0]->net_no, -1);
    }
}

// Test unroutable items filtering
TEST_F(RoutingFailureLogTest, UnroutableItemsFiltering) {
    auto item1 = std::make_shared<MockItem>(1);
    auto item2 = std::make_shared<MockItem>(2);
    auto item3 = std::make_shared<MockItem>(3);
    
    // Make item1 unroutable
    for (int i = 0; i < RoutingFailureLog::FAILURE_THRESHOLD; ++i) {
        log->record_failure(item1, i + 1, AutorouteAttemptState::FAILED, "Item 1 failure");
    }
    
    // Make item2 have some failures but not unroutable
    for (int i = 0; i < 10; ++i) {
        log->record_failure(item2, i + 1, AutorouteAttemptState::FAILED, "Item 2 failure");
    }
    
    // Make item3 unroutable
    for (int i = 0; i < RoutingFailureLog::FAILURE_THRESHOLD; ++i) {
        log->record_failure(item3, i + 1, AutorouteAttemptState::FAILED, "Item 3 failure");
    }
    
    EXPECT_EQ(log->get_item_count(), 3);
    EXPECT_TRUE(log->has_unroutable_items());
    
    auto unroutable = log->get_unroutable_items();
    EXPECT_EQ(unroutable.size(), 2); // item1 and item3
    
    auto stats = log->get_statistics();
    EXPECT_EQ(stats.first, 3);  // total items
    EXPECT_EQ(stats.second, 2); // unroutable items
}

// Test edge cases
TEST_F(RoutingFailureLogTest, EdgeCases) {
    // Test with empty reason
    auto item = std::make_shared<MockItem>(1);
    log->record_failure(item, 1, AutorouteAttemptState::FAILED, "");
    
    EXPECT_EQ(log->get_failure_count(item), 1);
    
    // Test with very long reason
    std::string long_reason(1000, 'x');
    log->record_failure(item, 2, AutorouteAttemptState::FAILED, long_reason);
    
    EXPECT_EQ(log->get_failure_count(item), 2);
    
    // Test with negative pass numbers
    log->record_failure(item, -1, AutorouteAttemptState::FAILED, "Negative pass");
    
    EXPECT_EQ(log->get_failure_count(item), 3);
    
    // Test with very large pass numbers
    log->record_failure(item, 1000000, AutorouteAttemptState::FAILED, "Large pass");
    
    EXPECT_EQ(log->get_failure_count(item), 4);
}

// Test const correctness
TEST_F(RoutingFailureLogTest, ConstCorrectness) {
    auto item = std::make_shared<MockItem>(1);
    log->record_failure(item, 1, AutorouteAttemptState::FAILED, "Test");
    
    const RoutingFailureLog& const_log = *log;
    
    // Should be able to call const methods
    EXPECT_EQ(const_log.get_item_count(), 1);
    EXPECT_EQ(const_log.get_failure_count(item), 1);
    EXPECT_FALSE(const_log.should_skip(item));
    EXPECT_FALSE(const_log.has_unroutable_items());
    
    auto stats = const_log.get_statistics();
    EXPECT_EQ(stats.first, 1);
    EXPECT_EQ(stats.second, 0);
    
    auto unroutable = const_log.get_unroutable_items();
    EXPECT_EQ(unroutable.size(), 0);
}

} // namespace autoroute
} // namespace freerouting
