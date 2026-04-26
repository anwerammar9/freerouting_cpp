#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include "autoroute/AutorouteAttemptState.h"

namespace freerouting {
namespace autoroute {

class Item;

/**
 * @brief Thread-safe logbook for tracking routing failures per item
 * 
 * Helps detect when the router is stuck attempting the same impossible
 * connection repeatedly. This class provides comprehensive failure tracking
 * with configurable thresholds and detailed failure information.
 */
class RoutingFailureLog {
public:
    /**
     * @brief Information about routing failures for a specific item
     */
    class ItemFailureInfo {
    public:
        const std::shared_ptr<Item> item;
        const int net_no;
        int failure_count;
        AutorouteAttemptState last_failure_state;
        std::string last_failure_reason;
        long last_attempt_pass;

        /**
         * @brief Constructor for ItemFailureInfo
         * 
         * @param p_item The item to track failures for
         */
        explicit ItemFailureInfo(std::shared_ptr<Item> p_item);

        /**
         * @brief Records a failure for this item (thread-safe)
         * 
         * @param pass_no The pass number
         * @param state The failure state
         * @param reason The failure reason
         */
        void record_failure(long pass_no, AutorouteAttemptState state, const std::string& reason);

        /**
         * @brief Checks if this item should be given up on (thread-safe)
         * 
         * @return true if failure count exceeds threshold
         */
        bool should_give_up() const;

        /**
         * @brief Get string representation of the failure info
         * 
         * @return Formatted string with item info and failure count
         */
        std::string to_string() const;

        // Accessors
        int get_failure_count() const { return failure_count; }
        AutorouteAttemptState get_last_failure_state() const { return last_failure_state; }
        const std::string& get_last_failure_reason() const { return last_failure_reason; }
        long get_last_attempt_pass() const { return last_attempt_pass; }

    private:
        mutable std::mutex mutex_;
    };

    /** Failure threshold - give up after this many failures for the same item */
    static constexpr int FAILURE_THRESHOLD = 50;

    /**
     * @brief Constructor for RoutingFailureLog
     */
    RoutingFailureLog();

    /**
     * @brief Records a routing failure for an item
     * 
     * @param item The item that failed to route
     * @param pass_no The pass number when the failure occurred
     * @param state The failure state
     * @param reason The failure reason/details
     */
    void record_failure(std::shared_ptr<Item> item, int pass_no, 
                        AutorouteAttemptState state, const std::string& reason);

    /**
     * @brief Checks if an item should be skipped due to exceeding the failure threshold
     * 
     * @param item The item to check
     * @return true if the item should be skipped
     */
    bool should_skip(std::shared_ptr<Item> item) const;

    /**
     * @brief Gets all items that have exceeded the failure threshold
     * 
     * @return List of unroutable items
     */
    std::vector<std::shared_ptr<ItemFailureInfo>> get_unroutable_items() const;

    /**
     * @brief Checks if there are any unroutable items
     * 
     * @return true if there are items that exceeded the failure threshold
     */
    bool has_unroutable_items() const;

    /**
     * @brief Gets the failure count for a specific item
     * 
     * @param item The item to check
     * @return The number of failures, or 0 if no failures recorded
     */
    int get_failure_count(std::shared_ptr<Item> item) const;

    /**
     * @brief Clears all failure records
     */
    void clear();

    /**
     * @brief Gets the total number of items with failures
     * 
     * @return Number of items in the failure log
     */
    size_t get_item_count() const;

    /**
     * @brief Gets failure statistics
     * 
     * @return Pair of (total_items, unroutable_items)
     */
    std::pair<size_t, size_t> get_statistics() const;

private:
    mutable std::mutex mutex_;
    std::unordered_map<int, std::shared_ptr<ItemFailureInfo>> failures_;

    /**
     * @brief Get or create failure info for an item (thread-safe)
     * 
     * @param item The item to get failure info for
     * @return Shared pointer to the failure info
     */
    std::shared_ptr<ItemFailureInfo> get_or_create_failure_info(std::shared_ptr<Item> item);
};

} // namespace autoroute
} // namespace freerouting
