#include "autoroute/PerformanceProfiler.h"
#include <algorithm>
#include <iomanip>
#include <sstream>

namespace freerouting {
namespace autoroute {

// Thread-local storage for timing stack
thread_local std::deque<PerformanceProfiler::Invocation> PerformanceProfiler::timing_stack_;

// Global static members
std::mutex PerformanceProfiler::timings_mutex_;
std::mutex PerformanceProfiler::counts_mutex_;
std::mutex PerformanceProfiler::pass_history_mutex_;
std::mutex PerformanceProfiler::config_mutex_;

std::map<std::string, std::atomic<long>> PerformanceProfiler::timings_;
std::map<std::string, std::atomic<long>> PerformanceProfiler::counts_;
std::vector<PerformanceProfiler::PassInfo> PerformanceProfiler::pass_history_;
std::unique_ptr<PerformanceProfiler::ConfigInfo> PerformanceProfiler::config_info_;

// PassInfo implementation
PerformanceProfiler::PassInfo::PassInfo(int p_pass_no, int p_unrouted_items, 
                                        long p_duration_ms, int p_ripup_cost)
    : pass_no(p_pass_no)
    , unrouted_items(p_unrouted_items)
    , duration_ms(p_duration_ms)
    , ripup_cost(p_ripup_cost)
{
}

// ConfigInfo implementation
PerformanceProfiler::ConfigInfo::ConfigInfo(int p_via_costs, int p_plane_via_costs,
                                          const std::vector<double>& p_preferred_costs,
                                          const std::vector<double>& p_against_costs)
    : via_costs(p_via_costs)
    , plane_via_costs(p_plane_via_costs)
    , preferred_costs(p_preferred_costs)
    , against_costs(p_against_costs)
{
}

// Invocation implementation
PerformanceProfiler::Invocation::Invocation(
    const std::string& p_name,
    std::chrono::time_point<std::chrono::high_resolution_clock> p_start_time)
    : name(p_name)
    , start_time(p_start_time)
{
}

void PerformanceProfiler::start(const std::string& section) {
    auto start_time = std::chrono::high_resolution_clock::now();
    timing_stack_.emplace_back(section, start_time);
}

void PerformanceProfiler::end(const std::string& section) {
    if (timing_stack_.empty()) {
        return; // No matching start call
    }
    
    auto invocation = std::move(timing_stack_.back());
    timing_stack_.pop_back();
    
    // Verify section name matches (optional, for robustness)
    if (invocation.name != section) {
        // Name mismatch - could be nested timing issue
        // For now, we'll record with the actual section name that ended
        // This maintains data integrity even with nesting issues
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(
        end_time - invocation.start_time).count();
    
    // Record timing data (thread-safe)
    {
        std::lock_guard<std::mutex> lock(timings_mutex_);
        timings_[section].fetch_add(duration);
    }
    
    {
        std::lock_guard<std::mutex> lock(counts_mutex_);
        counts_[section].fetch_add(1);
    }
}

void PerformanceProfiler::record_pass(int pass_no, int unrouted_items, 
                                   long duration_ms, int ripup_cost) {
    std::lock_guard<std::mutex> lock(pass_history_mutex_);
    pass_history_.emplace_back(pass_no, unrouted_items, duration_ms, ripup_cost);
}

void PerformanceProfiler::record_configuration(int via_costs, int plane_via_costs,
                                             const std::vector<double>& preferred_costs,
                                             const std::vector<double>& against_costs) {
    std::lock_guard<std::mutex> lock(config_mutex_);
    config_info_ = std::make_unique<ConfigInfo>(via_costs, plane_via_costs, 
                                                preferred_costs, against_costs);
}

void PerformanceProfiler::print_results() {
    FR_LOG_DEBUG("=== Performance Profile ===");
    
    // Print configuration info if available
    {
        std::lock_guard<std::mutex> lock(config_mutex_);
        if (config_info_) {
            FR_LOG_DEBUG("=== Routing Parameters ===");
            FR_LOG_DEBUG("  Via Costs: " + std::to_string(config_info_->via_costs) + 
                           ", Plane Via Costs: " + std::to_string(config_info_->plane_via_costs));
            FR_LOG_DEBUG("  Layer Costs (Preferred / Against):");
            
            for (size_t i = 0; i < config_info_->preferred_costs.size() && 
                        i < config_info_->against_costs.size(); ++i) {
                std::ostringstream oss;
                oss << "    Layer " << (i + 1) << ": " 
                    << std::fixed << std::setprecision(1) << config_info_->preferred_costs[i]
                    << " / " << config_info_->against_costs[i];
                FR_LOG_DEBUG(oss.str());
            }
            FR_LOG_DEBUG("");
        }
    }
    
    // Print timing results
    {
        std::lock_guard<std::mutex> timings_lock(timings_mutex_);
        std::lock_guard<std::mutex> counts_lock(counts_mutex_);
        
        // Create vector of entries for sorting
        std::vector<std::pair<std::string, long>> timing_entries;
        for (const auto& entry : timings_) {
            timing_entries.emplace_back(entry.first, entry.second.load());
        }
        
        // Sort by total time (descending)
        std::sort(timing_entries.begin(), timing_entries.end(),
                 [](const auto& a, const auto& b) { return a.second > b.second; });
        
        // Print sorted results
        for (const auto& entry : timing_entries) {
            const std::string& section = entry.first;
            long total_nanos = entry.second;
            
            long total_ms = total_nanos / 1'000'000;
            long count = counts_[section].load();
            long avg_ms = count > 0 ? total_ms / count : 0;
            
            std::ostringstream oss;
            oss << std::left << std::setw(40) << section << ": "
                << std::right << std::setw(8) << total_ms << " ms total, "
                << std::setw(8) << count << " calls, "
                << std::setw(6) << avg_ms << " ms avg";
            FR_LOG_DEBUG(oss.str());
        }
    }
    
    // Print pass history if available
    {
        std::lock_guard<std::mutex> lock(pass_history_mutex_);
        if (!pass_history_.empty()) {
            FR_LOG_DEBUG("");
            FR_LOG_DEBUG("=== Pass History ===");
            
            // Sort by pass number to ensure correct order
            auto sorted_passes = pass_history_;
            std::sort(sorted_passes.begin(), sorted_passes.end(),
                     [](const PassInfo& a, const PassInfo& b) { return a.pass_no < b.pass_no; });
            
            for (const auto& pass : sorted_passes) {
                std::ostringstream oss;
                oss << "  Pass " << std::setw(3) << pass.pass_no << ": "
                    << std::setw(4) << pass.unrouted_items << " unrouted items, "
                    << std::fixed << std::setprecision(2) << (pass.duration_ms / 1000.0) << " s, "
                    << std::setw(5) << pass.ripup_cost << " ripup cost";
                FR_LOG_DEBUG(oss.str());
            }
        }
    }
    
    FR_LOG_DEBUG("===========================");
}

void PerformanceProfiler::reset() {
    {
        std::lock_guard<std::mutex> lock(timings_mutex_);
        timings_.clear();
    }
    
    {
        std::lock_guard<std::mutex> lock(counts_mutex_);
        counts_.clear();
    }
    
    {
        std::lock_guard<std::mutex> lock(pass_history_mutex_);
        pass_history_.clear();
    }
    
    {
        std::lock_guard<std::mutex> lock(config_mutex_);
        config_info_.reset();
    }
    
    // Clear thread-local stack
    timing_stack_.clear();
}

std::string PerformanceProfiler::format_duration(long nanoseconds) {
    std::ostringstream oss;
    long ms = nanoseconds / 1'000'000;
    long us = (nanoseconds % 1'000'000) / 1'000;
    
    if (ms > 0) {
        oss << ms << " ms";
        if (us > 0) {
            oss << " " << us << " μs";
        }
    } else {
        oss << nanoseconds << " ns";
    }
    
    return oss.str();
}

long PerformanceProfiler::get_current_time_nanos() {
    auto now = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        now.time_since_epoch()).count();
}

} // namespace autoroute
} // namespace freerouting
