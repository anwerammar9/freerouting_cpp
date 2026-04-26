#pragma once

#include <memory>

namespace freerouting {
namespace autoroute {

/**
 * @brief Result of routing a single item with comparison metrics
 * 
 * This class stores the before and after metrics for an item routing
 * operation, including via counts, trace lengths, and incomplete connections.
 * It implements comparable interface for sorting by improvement metrics.
 */
class ItemRouteResult {
public:
    /**
     * @brief Constructor with only item ID (defaults to no improvement)
     * 
     * @param p_item_id The ID of the item
     */
    explicit ItemRouteResult(int p_item_id);

    /**
     * @brief Constructor with full before/after metrics
     * 
     * @param p_item_id The ID of the item
     * @param p_via_count_before Number of vias before routing
     * @param p_via_count_after Number of vias after routing
     * @param p_trace_length_before Total trace length before routing
     * @param p_trace_length_after Total trace length after routing
     * @param p_incomplete_count_before Number of incomplete connections before routing
     * @param p_incomplete_count_after Number of incomplete connections after routing
     */
    ItemRouteResult(int p_item_id, 
                   int p_via_count_before, 
                   int p_via_count_after, 
                   double p_trace_length_before, 
                   double p_trace_length_after, 
                   int p_incomplete_count_before,
                   int p_incomplete_count_after);

    // Comparison operators
    auto operator<=>(const ItemRouteResult& other) const;
    bool operator<(const ItemRouteResult& other) const;

    /**
     * @brief Check if this result is better than another result
     * 
     * @param r The other result to compare with
     * @return true if this result is better
     */
    bool improved_over(const ItemRouteResult& r) const;

    // Accessors
    int item_id() const { return item_id_; }
    bool improved() const { return improved_; }
    float improvement_percentage() const { return improvement_percentage_; }
    int via_count() const { return via_count_after_; }
    double trace_length() const { return trace_length_after_; }
    int incomplete_count() const { return incomplete_count_after_; }
    int via_count_reduced() const { return via_count_before_ - via_count_after_; }
    double length_reduced() const { return trace_length_before_ - trace_length_after_; }
    int incomplete_count_before() const { return incomplete_count_before_; }

    /**
     * @brief Update the improved flag
     * 
     * @param p_improved The new improved status
     */
    void update_improved(bool p_improved) { improved_ = p_improved; }

private:
    const int item_id_;
    float improvement_percentage_;
    const int via_count_before_;
    const int via_count_after_;
    const double trace_length_before_;
    const double trace_length_after_;
    const int incomplete_count_before_;
    const int incomplete_count_after_;
    bool improved_;

    /**
     * @brief Calculate improvement percentage based on via and trace length reduction
     * 
     * @return Improvement percentage (0.0 to 1.0)
     */
    float calculate_improvement_percentage() const;
};

} // namespace autoroute
} // namespace freerouting
