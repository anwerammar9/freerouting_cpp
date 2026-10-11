#include "drc/UnconnectedItems.h"

namespace freerouting {
namespace drc {

UnconnectedItems::UnconnectedItems(board::Item* p_first,
                                   board::Item* p_second)
    : first_(p_first)
    , second_(p_second)
    , type_("unconnectedItems")
{
    all_items_.push_back(p_first);
    all_items_.push_back(p_second);
}

UnconnectedItems::UnconnectedItems(board::Item* p_first,
                                   board::Item* p_second,
                                   const std::vector<board::Item*>& p_all_items)
    : first_(p_first)
    , second_(p_second)
    , all_items_(p_all_items)
    , type_("unconnectedItems")
{
    if (all_items_.empty()) {
        all_items_.push_back(p_first);
        all_items_.push_back(p_second);
    }
}

UnconnectedItems::UnconnectedItems(board::Item* p_first,
                                   board::Item* p_second,
                                   const std::string& p_type)
    : first_(p_first)
    , second_(p_second)
    , type_(p_type)
{
    all_items_.push_back(p_first);
    all_items_.push_back(p_second);
}

UnconnectedItems::UnconnectedItems(board::Item* p_first,
                                   board::Item* p_second,
                                   const std::vector<board::Item*>& p_all_items,
                                   const std::string& p_type)
    : first_(p_first)
    , second_(p_second)
    , all_items_(p_all_items)
    , type_(p_type)
{
    if (all_items_.empty()) {
        all_items_.push_back(p_first);
        all_items_.push_back(p_second);
    }
}

} // namespace drc
} // namespace freerouting
