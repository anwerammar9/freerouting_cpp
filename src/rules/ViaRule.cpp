#include "rules/ViaRule.h"

#include <algorithm>
#include <stdexcept>

namespace freerouting {
namespace rules {

const ViaRule ViaRule::EMPTY("empty");

ViaRule::ViaRule(const std::string& p_name)
    : name_(p_name)
{
}

void ViaRule::append_via(const std::shared_ptr<ViaInfo>& p_via)
{
    via_list_.push_back(p_via);
}

bool ViaRule::remove_via(const std::shared_ptr<ViaInfo>& p_via)
{
    for (auto it = via_list_.begin(); it != via_list_.end(); ++it)
    {
        if (*it == p_via)
        {
            via_list_.erase(it);
            return true;
        }
    }
    return false;
}

std::shared_ptr<ViaInfo> ViaRule::get_via(int p_index) const
{
    if (p_index < 0 || p_index >= via_count())
    {
        throw std::out_of_range("ViaRule.get_via: index out of range");
    }
    return via_list_[p_index];
}

bool ViaRule::contains(const std::shared_ptr<ViaInfo>& p_via_info) const
{
    for (const auto& curr_info : via_list_)
    {
        if (curr_info == p_via_info)
        {
            return true;
        }
    }
    return false;
}

bool ViaRule::contains_padstack(const board::Padstack* p_padstack) const
{
    if (p_padstack == nullptr)
    {
        return false;
    }
    for (const auto& curr_info : via_list_)
    {
        if (curr_info->get_padstack() == p_padstack)
        {
            return true;
        }
    }
    return false;
}

bool ViaRule::swap(const std::shared_ptr<ViaInfo>& p_1, const std::shared_ptr<ViaInfo>& p_2)
{
    int index_1 = -1;
    int index_2 = -1;
    for (int i = 0; i < via_count(); ++i)
    {
        if (via_list_[i] == p_1)
            index_1 = i;
        if (via_list_[i] == p_2)
            index_2 = i;
    }
    if (index_1 < 0 || index_2 < 0)
    {
        return false;
    }
    if (index_1 == index_2)
    {
        return true;
    }
    std::swap(via_list_[index_1], via_list_[index_2]);
    return true;
}

std::string ViaRule::to_string() const
{
    return name_;
}

} // namespace rules
} // namespace freerouting