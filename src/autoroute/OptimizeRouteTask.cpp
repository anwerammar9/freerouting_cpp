#include "autoroute/OptimizeRouteTask.h"
#include "autoroute/BatchOptimizerMultiThreaded.h"
#include "autoroute/BatchOptimizer.h"
#include "board/RoutingBoard.h"
#include "core/RoutingJob.h"
#include "board/Item.h"
#include "logger/FRLogger.h"

#include <sstream>
#include <iomanip>

namespace freerouting {
namespace autoroute {

OptimizeRouteTask::OptimizeRouteTask(
    std::shared_ptr<BatchOptimizerMultiThreaded> p_optimizer, 
    std::shared_ptr<RoutingJob> job, 
    int item_id, 
    int p_pass_no, 
    bool p_with_preferred_directions)
    : optimizer_(std::move(p_optimizer))
    , pass_no_(p_pass_no)
    , with_preferred_directions_(p_with_preferred_directions)
    , job_(std::move(job))
{
    // Create a deep copy of the board for this task
    board_ = job_->board->deep_copy();
    
    // Get the item to optimize
    item_to_optimize_ = board_->get_item(item_id);
}

void OptimizeRouteTask::run() {
    auto start_time = std::chrono::steady_clock::now();
    
    if (!item_to_optimize_) {
        return;
    }
    
    // Perform the optimization
    auto batch_optimizer = std::make_shared<BatchOptimizer>(job_);
    optimization_result_ = batch_optimizer->opt_route_item(
        item_to_optimize_, with_preferred_directions_, true);
    
    // Check if this is a winning candidate
    bool winning_candidate = optimizer_->is_winning_candidate(shared_from_this());
    
    // Log completion with performance metrics
    log_completion(start_time, winning_candidate);
    
    // Clean up if not a winning candidate
    if (!winning_candidate) {
        clean();
    }
}

std::shared_ptr<ItemRouteResult> OptimizeRouteTask::get_route_result() const {
    return optimization_result_;
}

std::shared_ptr<Item> OptimizeRouteTask::get_item() const {
    return item_to_optimize_;
}

void OptimizeRouteTask::clean() {
    // Clear references to speed up memory release
    if (item_to_optimize_) {
        item_to_optimize_->board = nullptr;
        item_to_optimize_ = nullptr;
    }
}

std::string OptimizeRouteTask::format_duration(long duration_ms) const {
    std::ostringstream oss;
    
    long minutes = duration_ms / 60000;
    float seconds = (duration_ms % 60000) / 1000.0f;
    
    if (minutes > 0) {
        oss << minutes << " m " << std::fixed << std::setprecision(1) << seconds << "s";
    } else {
        oss << std::fixed << std::setprecision(1) << seconds << "s";
    }
    
    return oss.str();
}

void OptimizeRouteTask::log_completion(std::chrono::steady_clock::time_point start_time, 
                                      bool winning_candidate) const {
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    long duration_ms = duration.count();
    
    std::ostringstream oss;
    
    oss << "Finished   task #" << optimizer_->get_num_tasks_finished() 
        << " of " << optimizer_->get_num_tasks() 
        << " for item #" << item_to_optimize_->get_id_no() 
        << " on pass " << pass_no_ 
        << " in " << format_duration(duration_ms)
        << ". Best so far: " << (winning_candidate ? "true" : "false")
        << ", improved: " << (optimization_result_ ? (optimization_result_->improved() ? "true" : "false") : "false")
        << ", via reduction: " << (optimization_result_ ? optimization_result_->via_count_reduced() : 0);
    
    if (winning_candidate && optimization_result_) {
        oss << ", length reduction: " << static_cast<int>(optimization_result_->length_reduced());
    }
    
    if (optimization_result_) {
        oss << ", incomplete trace reduction: " 
            << (optimization_result_->incomplete_count_before() - optimization_result_->incomplete_count());
    }
    
    FRLogger::debug(oss.str());
}

} // namespace autoroute
} // namespace freerouting
