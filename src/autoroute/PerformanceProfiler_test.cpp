/**
 * @file PerformanceProfiler_test.cpp
 * @brief Unit tests for PerformanceProfiler class
 */

#include "autoroute/PerformanceProfiler.h"
#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <vector>

namespace freerouting {
namespace autoroute {

class PerformanceProfilerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Reset profiler before each test
        PerformanceProfiler::reset();
    }
    
    void TearDown() override {
        // Clean up after each test
        PerformanceProfiler::reset();
    }
};

// Test basic start/end functionality
TEST_F(PerformanceProfilerTest, BasicStartEnd) {
    // Test basic timing
    PerformanceProfiler::start("test_section");
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    PerformanceProfiler::end("test_section");
    
    // Should not crash
    EXPECT_NO_THROW({
        PerformanceProfiler::print_results();
    });
}

// Test nested timing
TEST_F(PerformanceProfilerTest, NestedTiming) {
    // Test nested sections
    PerformanceProfiler::start("outer");
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    
    PerformanceProfiler::start("inner");
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    PerformanceProfiler::end("inner");
    
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    PerformanceProfiler::end("outer");
    
    // Should not crash
    EXPECT_NO_THROW({
        PerformanceProfiler::print_results();
    });
}

// Test multiple sections
TEST_F(PerformanceProfilerTest, MultipleSections) {
    // Test multiple different sections
    PerformanceProfiler::start("section1");
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    PerformanceProfiler::end("section1");
    
    PerformanceProfiler::start("section2");
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    PerformanceProfiler::end("section2");
    
    PerformanceProfiler::start("section3");
    std::this_thread::sleep_for(std::chrono::milliseconds(3));
    PerformanceProfiler::end("section3");
    
    // Should not crash
    EXPECT_NO_THROW({
        PerformanceProfiler::print_results();
    });
}

// Test repeated sections
TEST_F(PerformanceProfilerTest, RepeatedSections) {
    // Test same section multiple times
    for (int i = 0; i < 3; ++i) {
        PerformanceProfiler::start("repeated_section");
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        PerformanceProfiler::end("repeated_section");
    }
    
    // Should not crash
    EXPECT_NO_THROW({
        PerformanceProfiler::print_results();
    });
}

// Test pass recording
TEST_F(PerformanceProfilerTest, PassRecording) {
    // Record some passes
    PerformanceProfiler::record_pass(1, 100, 5000, 50);
    PerformanceProfiler::record_pass(2, 75, 4500, 45);
    PerformanceProfiler::record_pass(3, 25, 3000, 30);
    
    // Should not crash
    EXPECT_NO_THROW({
        PerformanceProfiler::print_results();
    });
}

// Test configuration recording
TEST_F(PerformanceProfilerTest, ConfigurationRecording) {
    // Record configuration
    std::vector<double> preferred_costs = {1.0, 2.0, 3.0};
    std::vector<double> against_costs = {1.5, 2.5, 3.5};
    
    PerformanceProfiler::record_configuration(10, 20, preferred_costs, against_costs);
    
    // Should not crash
    EXPECT_NO_THROW({
        PerformanceProfiler::print_results();
    });
}

// Test thread safety
TEST_F(PerformanceProfilerTest, ThreadSafety) {
    const int num_threads = 4;
    const int operations_per_thread = 10;
    
    std::vector<std::thread> threads;
    
    // Create multiple threads that use the profiler
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([t, operations_per_thread]() {
            for (int i = 0; i < operations_per_thread; ++i) {
                std::string section = "thread_" + std::to_string(t) + "_section_" + std::to_string(i);
                
                PerformanceProfiler::start(section);
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                PerformanceProfiler::end(section);
                
                // Record some passes
                PerformanceProfiler::record_pass(i, 10, 100, 5);
            }
        });
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Should not crash and should have data from all threads
    EXPECT_NO_THROW({
        PerformanceProfiler::print_results();
    });
}

// Test reset functionality
TEST_F(PerformanceProfilerTest, ResetFunctionality) {
    // Add some data
    PerformanceProfiler::start("test_section");
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    PerformanceProfiler::end("test_section");
    
    PerformanceProfiler::record_pass(1, 10, 1000, 10);
    
    std::vector<double> costs = {1.0, 2.0};
    PerformanceProfiler::record_configuration(5, 10, costs, costs);
    
    // Verify data exists (print_results should work)
    EXPECT_NO_THROW({
        PerformanceProfiler::print_results();
    });
    
    // Reset
    PerformanceProfiler::reset();
    
    // After reset, print_results should still work but show no data
    EXPECT_NO_THROW({
        PerformanceProfiler::print_results();
    });
}

// Test edge cases
TEST_F(PerformanceProfilerTest, EdgeCases) {
    // Test end without start
    EXPECT_NO_THROW({
        PerformanceProfiler::end("nonexistent_section");
    });
    
    // Test empty section names
    EXPECT_NO_THROW({
        PerformanceProfiler::start("");
        PerformanceProfiler::end("");
    });
    
    // Test very long section names
    std::string long_name(1000, 'x');
    EXPECT_NO_THROW({
        PerformanceProfiler::start(long_name);
        PerformanceProfiler::end(long_name);
    });
    
    // Should not crash
    EXPECT_NO_THROW({
        PerformanceProfiler::print_results();
    });
}

// Test mismatched section names
TEST_F(PerformanceProfilerTest, MismatchedSectionNames) {
    // Start with one name, end with another
    PerformanceProfiler::start("section_a");
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    PerformanceProfiler::end("section_b");  // Different name
    
    // Should not crash, but may record under section_b
    EXPECT_NO_THROW({
        PerformanceProfiler::print_results();
    });
}

// Test PassInfo struct
TEST_F(PerformanceProfilerTest, PassInfoStruct) {
    // Create PassInfo directly
    PerformanceProfiler::PassInfo info(5, 25, 7500, 75);
    
    EXPECT_EQ(info.pass_no, 5);
    EXPECT_EQ(info.unrouted_items, 25);
    EXPECT_EQ(info.duration_ms, 7500);
    EXPECT_EQ(info.ripup_cost, 75);
}

// Test ConfigInfo struct
TEST_F(PerformanceProfilerTest, ConfigInfoStruct) {
    // Create ConfigInfo directly
    std::vector<double> preferred = {1.0, 2.0, 3.0};
    std::vector<double> against = {1.5, 2.5, 3.5};
    
    PerformanceProfiler::ConfigInfo config(10, 20, preferred, against);
    
    EXPECT_EQ(config.via_costs, 10);
    EXPECT_EQ(config.plane_via_costs, 20);
    EXPECT_EQ(config.preferred_costs, preferred);
    EXPECT_EQ(config.against_costs, against);
}

// Test performance with many operations
TEST_F(PerformanceProfilerTest, PerformanceWithManyOperations) {
    const int num_operations = 1000;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Perform many timing operations
    for (int i = 0; i < num_operations; ++i) {
        std::string section = "perf_test_" + std::to_string(i % 10);
        PerformanceProfiler::start(section);
        // Minimal sleep to simulate work
        std::this_thread::sleep_for(std::chrono::microseconds(1));
        PerformanceProfiler::end(section);
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time).count();
    
    // Should complete in reasonable time (less than 5 seconds)
    EXPECT_LT(duration, 5000);
    
    // Should not crash
    EXPECT_NO_THROW({
        PerformanceProfiler::print_results();
    });
}

// Test concurrent nested timing
TEST_F(PerformanceProfilerTest, ConcurrentNestedTiming) {
    const int num_threads = 3;
    
    std::vector<std::thread> threads;
    
    // Each thread does nested timing
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([t]() {
            for (int i = 0; i < 5; ++i) {
                std::string outer = "thread_" + std::to_string(t) + "_outer_" + std::to_string(i);
                std::string inner = "thread_" + std::to_string(t) + "_inner_" + std::to_string(i);
                
                PerformanceProfiler::start(outer);
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                
                PerformanceProfiler::start(inner);
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                PerformanceProfiler::end(inner);
                
                PerformanceProfiler::end(outer);
            }
        });
    }
    
    // Wait for all threads
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Should not crash
    EXPECT_NO_THROW({
        PerformanceProfiler::print_results();
    });
}

} // namespace autoroute
} // namespace freerouting
