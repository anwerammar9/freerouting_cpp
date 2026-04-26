#include "rules/ViaRule.h"

#include <stdexcept>
#include <sstream>

namespace freerouting {
namespace rules {

ViaInfo::ViaInfo(const std::string& p_name, int p_drill,
                     const std::vector<int>& p_layer_indices)
    : name_(p_name), drill_(p_drill), layer_indices_(p_layer_indices)
{
}

std::string ViaInfo::to_string() const
{
    std::ostringstream ss;
    ss << "ViaInfo[" << name_ << ", drill=" << drill_ << "]";
    return ss.str();
}

ViaRule::ViaRule(const std::string& p_name)
    : name_(p_name)
{
}

void ViaRule::append_via(const ViaInfo& p_via)
{
    via_list_.push_back(p_via);
}

bool ViaRule::remove_via(const ViaInfo& p_via)
{
    for (auto it = via_list_.begin(); it != via_list_.end(); ++it)
    {
        if (it->get_name() == p_via.get_name())
        {
            via_list_.erase(it);
            return true;
        }
    }
    return false;
}

const ViaInfo& ViaRule::get_via(int p_index) const
{
    if (p_index < 0 || p_index >= via_count())
    {
        throw std::out_of_range("ViaRule.get_via: index out of range");
    }
    return via_list_[p_index];
}

std::string ViaRule::to_string() const
{
    return name_;
}

} // namespace rules
} // namespace freerouting