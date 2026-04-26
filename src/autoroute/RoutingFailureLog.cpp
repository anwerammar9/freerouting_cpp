#include "autoroute/RoutingFailureLog.h"
#include "board/Item.h"
#include <algorithm>
#include <sstream>

namespace freerouting {
namespace autoroute {

// ItemFailureInfo implementation
RoutingFailureLog::ItemFailureInfo::ItemFailureInfo(std::shared_ptr<Item> p_item)
    : item(std::move(p_item))
    , net_no(item && item->net_count() > 0 ? item->get_net_no(0) : -1)
    , failure_count(0)
    , last_failure_state(AutorouteAttemptState::UNKNOWN)
    , last_failure_reason("")
    , last_attempt_pass(0)
{
}

void RoutingFailureLog::ItemFailureInfo::record_failure(long pass_no, 
                                                      AutorouteAttemptState state, 
                                                      const std::string& reason) {
    std::lock_guard<std::mutex> lock(mutex_);
    failure_count++;
    last_attempt_pass = pass_no;
    last_failure_state = state;
    last_failure_reason = reason;
}

bool RoutingFailureLog::ItemFailureInfo::should_give_up() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return failure_count >= FAILURE_THRESHOLD;
}

std::string RoutingFailureLog::ItemFailureInfo::to_string() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ostringstream oss;
    
    if (item) {
        oss << "Item #" << item->get_id_no() 
            << " (net #" << net_no 
            << ", " << failure_count << " failures)";
    } else {
        oss << "Invalid item (" << failure_count << " failures)";
    }
    
    return oss.str();
}

// RoutingFailureLog implementation
RoutingFailureLog::RoutingFailureLog() {
}

void RoutingFailureLog::record_failure(std::shared_ptr<Item> item, int pass_no, 
                                       AutorouteAttemptState state, 
                                       const std::string& reason) {
    if (!item) {
        return;
    }

    auto failure_info = get_or_create_failure_info(item);
    failure_info->record_failure(pass_no, state, reason);
}

bool RoutingFailureLog::should_skip(std::shared_ptr<Item> item) const {
    if (!item) {
        return false;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    auto it = failures_.find(item->get_id_no());
    if (it != failures_.end()) {
        return it->second->should_give_up();
    }
    return false;
}

std::vector<std::shared_ptr<RoutingFailureLog::ItemFailureInfo>> 
RoutingFailureLog::get_unroutable_items() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::shared_ptr<ItemFailureInfo>> unroutable;
    for (const auto& pair : failures_) {
        if (pair.second->should_give_up()) {
            unroutable.push_back(pair.second);
        }
    }
    
    return unroutable;
}

bool RoutingFailureLog::has_unroutable_items() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (const auto& pair : failures_) {
        if (pair.second->should_give_up()) {
            return true;
        }
    }
    
    return false;
}

int RoutingFailureLog::get_failure_count(std::shared_ptr<Item> item) const {
    if (!item) {
        return 0;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    auto it = failures_.find(item->get_id_no());
    if (it != failures_.end()) {
        return it->second->get_failure_count();
    }
    return 0;
}

void RoutingFailureLog::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    failures_.clear();
}

size_t RoutingFailureLog::get_item_count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return failures_.size();
}

std::pair<size_t, size_t> RoutingFailureLog::get_statistics() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    size_t total_items = failures_.size();
    size_t unroutable_items = 0;
    
    for (const auto& pair : failures_) {
        if (pair.second->should_give_up()) {
            ++unroutable_items;
        }
    }
    
    return std::make_pair(total_items, unroutable_items);
}

std::shared_ptr<RoutingFailureLog::ItemFailureInfo> 
RoutingFailureLog::get_or_create_failure_info(std::shared_ptr<Item> item) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = failures_.find(item->get_id_no());
    if (it == failures_.end()) {
        auto failure_info = std::make_shared<ItemFailureInfo>(item);
        failures_[item->get_id_no()] = failure_info;
        return failure_info;
    }
    
    return it->second;
}

} // namespace autoroute
} // namespace freerouting
