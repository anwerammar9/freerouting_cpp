#pragma once

#include <string>
#include <vector>
#include <deque>
#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>
#include <map>
#include <memory>
#include "logger/FRLogger.h"

namespace freerouting {
namespace autoroute {

/**
 * @brief Simple performance profiler to measure time spent in different routing operations
 * 
 * Thread-safe for use in multi-threaded routing. Provides nested timing capabilities
 * and comprehensive reporting of performance metrics.
 */
class PerformanceProfiler {
public:
    /**
     * @brief Information about a completed routing pass
     */
    struct PassInfo {
        int pass_no;
        int unrouted_items;
        long duration_ms;
        int ripup_cost;
        
        PassInfo(int p_pass_no, int p_unrouted_items, long p_duration_ms, int p_ripup_cost);
    };
    
    /**
     * @brief Configuration metrics for reporting
     */
    struct ConfigInfo {
        int via_costs;
        int plane_via_costs;
        std::vector<double> preferred_costs;
        std::vector<double> against_costs;
        
        ConfigInfo(int p_via_costs, int p_plane_via_costs, 
                  const std::vector<double>& p_preferred_costs,
                  const std::vector<double>& p_against_costs);
    };

    /**
     * @brief Start timing a section
     * 
     * @param section The name of the section to start timing
     */
    static void start(const std::string& section);

    /**
     * @brief End timing a section and record the duration
     * 
     * @param section The name of the section to end timing
     */
    static void end(const std::string& section);

    /**
     * @brief Record statistics for a completed pass
     * 
     * @param pass_no The pass number
     * @param unrouted_items Number of unrouted items
     * @param duration_ms Duration in milliseconds
     * @param ripup_cost Ripup cost
     */
    static void record_pass(int pass_no, int unrouted_items, long duration_ms, int ripup_cost);

    /**
     * @brief Record configuration parameters
     * 
     * @param via_costs Via costs
     * @param plane_via_costs Plane via costs
     * @param preferred_costs Preferred costs per layer
     * @param against_costs Against costs per layer
     */
    static void record_configuration(int via_costs, int plane_via_costs,
                                   const std::vector<double>& preferred_costs,
                                   const std::vector<double>& against_costs);

    /**
     * @brief Print profiling results sorted by total time
     */
    static void print_results();

    /**
     * @brief Clear all profiling data
     */
    static void reset();

private:
    /**
     * @brief Internal invocation tracking structure
     */
    struct Invocation {
        std::string name;
        std::chrono::time_point<std::chrono::high_resolution_clock> start_time;
        
        Invocation(const std::string& p_name, 
                  std::chrono::time_point<std::chrono::high_resolution_clock> p_start_time);
    };

    // Thread-local storage for timing stack
    static thread_local std::deque<Invocation> timing_stack_;
    
    // Global timing data (thread-safe)
    static std::mutex timings_mutex_;
    static std::mutex counts_mutex_;
    static std::mutex pass_history_mutex_;
    static std::mutex config_mutex_;
    
    static std::map<std::string, std::atomic<long>> timings_;
    static std::map<std::string, std::atomic<long>> counts_;
    static std::vector<PassInfo> pass_history_;
    static std::unique_ptr<ConfigInfo> config_info_;

    /**
     * @brief Format time duration for display
     * 
     * @param nanoseconds Duration in nanoseconds
     * @return Formatted string
     */
    static std::string format_duration(long nanoseconds);
    
    /**
     * @brief Get current time in nanoseconds
     * 
     * @return Current time in nanoseconds
     */
    static long get_current_time_nanos();
};

} // namespace autoroute
} // namespace freerouting
