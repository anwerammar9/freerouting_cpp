#include "rules/ViaInfo.h"

#include <stdexcept>
#include <algorithm>

namespace freerouting {
namespace rules {

ViaInfo::ViaInfo(const std::string& p_name,
             std::shared_ptr<Padstack> p_padstack,
             int p_clearance_class,
             bool p_attach_smd_allowed,
             BoardRules* p_board_rules)
    : name_(p_name)
    , padstack_(p_padstack)
    , clearance_class_(p_clearance_class)
    , attach_smd_allowed_(p_attach_smd_allowed)
    , board_rules_(p_board_rules)
{
}

int ViaInfo::compareTo(const ViaInfo& p_other) const
{
    return name_.compare(p_other.name_);
}

bool ViaInfos::add(const std::shared_ptr<ViaInfo>& p_via_info)
{
    if (name_exists(p_via_info->get_name()))
    {
        return false;
    }
    list_.push_back(p_via_info);
    return true;
}

std::shared_ptr<ViaInfo> ViaInfos::get(int p_no) const
{
    if (p_no < 0 || p_no >= count())
    {
        throw std::out_of_range("ViaInfos index out of range");
    }
    return list_[p_no];
}

std::shared_ptr<ViaInfo> ViaInfos::get(const std::string& p_name) const
{
    for (const auto& via : list_)
    {
        if (via->get_name() == p_name)
        {
            return via;
        }
    }
    return nullptr;
}

bool ViaInfos::name_exists(const std::string& p_name) const
{
    return get(p_name) != nullptr;
}

bool ViaInfos::remove(const std::shared_ptr<ViaInfo>& p_via_info)
{
    auto it = std::find(list_.begin(), list_.end(), p_via_info);
    if (it == list_.end())
    {
        return false;
    }
    list_.erase(it);
    return true;
}

} // namespace rules
} // namespace freerouting