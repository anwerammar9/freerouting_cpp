#pragma once

#include <string>
#include <vector>

namespace freerouting {
namespace board {

class ItemSelectionFilter
{
public:
    enum class FilterType
    {
        ALL,
        TRACES,
        VIAS,
        PADS,
        COMPONENTS
    };
    
    ItemSelectionFilter();
    
    bool matches(const Item& item) const;
    
    void set_filter_type(FilterType type) { filter_type_ = type; }
    FilterType get_filter_type() const { return filter_type_; }
    
private:
    FilterType filter_type_ = FilterType::ALL;
    std::string name_pattern_;
};

} // namespace board
} // namespace freerouting