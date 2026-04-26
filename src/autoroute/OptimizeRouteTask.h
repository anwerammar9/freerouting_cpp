#pragma once

#include <memory>
#include <chrono>
#include "autoroute/ItemRouteResult.h"

namespace freerouting {
namespace autoroute {

class BatchOptimizerMultiThreaded;
class RoutingBoard;
class RoutingJob;
class Item;
class BatchOptimizer;

/**
 * @brief Task for optimizing a single route item in parallel
 * 
 * This class implements a runnable task that optimizes a single routing item
 * using the BatchOptimizer. It's designed for multi-threaded execution
 * and tracks optimization results and performance metrics.
 */
class OptimizeRouteTask {
public:
    /**
     * @brief Creates a new optimization task
     * 
     * @param p_optimizer The multi-threaded batch optimizer
     * @param job The routing job containing the board and configuration
     * @param item_id The ID of the item to optimize
     * @param p_pass_no The current pass number
     * @param p_with_preferred_directions Whether to use preferred directions
     */
    OptimizeRouteTask(std::shared_ptr<BatchOptimizerMultiThreaded> p_optimizer, 
                     std::shared_ptr<RoutingJob> job, 
                     int item_id, 
                     int p_pass_no, 
                     bool p_with_preferred_directions);

    /**
     * @brief Run the optimization task
     * 
     * This method performs the actual optimization work and logs results.
     * It's designed to be called from a thread pool or executor.
     */
    void run();

    /**
     * @brief Get the optimization result
     * 
     * @return The route result from the optimization
     */
    std::shared_ptr<ItemRouteResult> get_route_result() const;

    /**
     * @brief Get the item that was optimized
     * 
     * @return The optimized item
     */
    std::shared_ptr<Item> get_item() const;

    /**
     * @brief Clean up resources to speed up memory release
     * 
     * This method clears references to help with garbage collection
     * and memory management in multi-threaded environments.
     */
    void clean();

    // Accessors for testing and monitoring
    std::shared_ptr<RoutingBoard> get_board() const { return board; }
    int get_pass_no() const { return pass_no_; }
    bool is_with_preferred_directions() const { return with_preferred_directions_; }

private:
    std::shared_ptr<RoutingBoard> board_;
    std::shared_ptr<BatchOptimizerMultiThreaded> optimizer_;
    const int pass_no_;
    const bool with_preferred_directions_;
    std::shared_ptr<RoutingJob> job_;
    std::shared_ptr<Item> item_to_optimize_;
    std::shared_ptr<ItemRouteResult> optimization_result_;

    /**
     * @brief Format duration for logging
     * 
     * @param duration_ms Duration in milliseconds
     * @return Formatted duration string
     */
    std::string format_duration(long duration_ms) const;

    /**
     * @brief Log task completion with performance metrics
     * 
     * @param start_time Task start time
     * @param winning_candidate Whether this was a winning candidate
     */
    void log_completion(std::chrono::steady_clock::time_point start_time, bool winning_candidate) const;
};

} // namespace autoroute
} // namespace freerouting
