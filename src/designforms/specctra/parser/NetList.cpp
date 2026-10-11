#include "designforms/specctra/parser/NetList.h"

namespace freerouting::designforms::specctra::parser {

Net* NetList::add_net(const Net::Id& p_net_id)
{
    if (nets_.contains(p_net_id))
    {
        return nullptr;
    }
    auto [it, inserted] = nets_.emplace(p_net_id, std::make_unique<Net>(p_net_id));
    return it->second.get();
}

Net* NetList::get_net(const Net::Id& p_net_id) const
{
    auto it = nets_.find(p_net_id);
    return it == nets_.end() ? nullptr : it->second.get();
}

std::vector<Net*> NetList::get_nets(const std::string& p_component_name, const std::string& p_pin_name) const
{
    std::vector<Net*> result;
    const Net::Pin search_pin{p_component_name, p_pin_name};
    for (const auto& [id, net] : nets_)
    {
        const auto& pins = net->get_pins();
        if (pins.has_value() && pins->contains(search_pin))
        {
            result.push_back(net.get());
        }
    }
    return result;
}

} // namespace freerouting::designforms::specctra::parser
