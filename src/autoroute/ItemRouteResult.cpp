#include "autoroute/ItemRouteResult.h"
#include <algorithm>
#include <cmath>

namespace freerouting {
namespace autoroute {

ItemRouteResult::ItemRouteResult(int p_item_id)
    : item_id_(p_item_id)
    , improvement_percentage_(0.0f)
    , via_count_before_(0)
    , via_count_after_(0)
    , trace_length_before_(0.0)
    , trace_length_after_(0.0)
    , incomplete_count_before_(0)
    , incomplete_count_after_(1)
    , improved_(false)
{
    // Default constructor with no improvement
}

ItemRouteResult::ItemRouteResult(int p_item_id, 
                               int p_via_count_before, 
                               int p_via_count_after, 
                               double p_trace_length_before, 
                               double p_trace_length_after, 
                               int p_incomplete_count_before,
                               int p_incomplete_count_after)
    : item_id_(p_item_id)
    , improvement_percentage_(0.0f)
    , via_count_before_(p_via_count_before)
    , via_count_after_(p_via_count_after)
    , trace_length_before_(p_trace_length_before)
    , trace_length_after_(p_trace_length_after)
    , incomplete_count_before_(p_incomplete_count_before)
    , incomplete_count_after_(p_incomplete_count_after)
{
    // Determine if the routing was improved
    if (incomplete_count_after_ < incomplete_count_before_) {
        improved_ = true;
    } else if (incomplete_count_after_ > incomplete_count_before_) {
        improved_ = false;
    } else { // incomplete_count_after == incomplete_count_before
        if (via_count_after_ < via_count_before_) {
            improved_ = true;
        } else if (via_count_after_ > via_count_before_) {
            improved_ = false;
        } else { // via_count_after == via_count_before
            if (trace_length_after_ < trace_length_before_) {
                improved_ = true;
            } else {
                improved_ = false;
            }
        }
    }

    // Calculate improvement percentage
    improvement_percentage_ = calculate_improvement_percentage();
}

std::strong_ordering ItemRouteResult::operator<=>(const ItemRouteResult& other) const {
    // Primary comparison: incomplete count (lower is better)
    if (incomplete_count_after_ < other.incomplete_count_after_) {
        return std::strong_ordering::less;
    } else if (incomplete_count_after_ > other.incomplete_count_after_) {
        return std::strong_ordering::greater;
    }
    
    // Secondary comparison: via count (lower is better)
    if (via_count_after_ < other.via_count_after_) {
        return std::strong_ordering::less;
    } else if (via_count_after_ > other.via_count_after_) {
        return std::strong_ordering::greater;
    }
    
    // Tertiary comparison: trace length (lower is better)
    if (trace_length_after_ < other.trace_length_after_) {
        return std::strong_ordering::less;
    } else if (trace_length_after_ > other.trace_length_after_) {
        return std::strong_ordering::greater;
    }
    
    // Equal
    return std::strong_ordering::equal;
}

bool ItemRouteResult::operator==(const ItemRouteResult& other) const {
    return (*this <=> other) == std::strong_ordering::equal;
}

bool ItemRouteResult::operator<(const ItemRouteResult& other) const {
    return (*this <=> other) == std::strong_ordering::less;
}

bool ItemRouteResult::improved_over(const ItemRouteResult& r) const {
    return *this < r;
}

float ItemRouteResult::calculate_improvement_percentage() const {
    // Calculate improvement based on via and trace length reduction
    if (via_count_before_ != 0 && trace_length_before_ != 0) {
        double via_ratio = (via_count_before_ > 0) ? 
            static_cast<double>(via_count_after_) / via_count_before_ : 1.0;
        double trace_ratio = (trace_length_before_ > 0) ? 
            trace_length_after_ / trace_length_before_ : 1.0;
        
        double avg_ratio = (via_ratio + trace_ratio) / 2.0;
        return static_cast<float>(1.0 - avg_ratio);
    }
    
    return 0.0f;
}

} // namespace autoroute
} // namespace freerouting
