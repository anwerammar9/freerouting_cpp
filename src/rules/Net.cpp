#include "rules/Net.h"

#include <stdexcept>
#include <sstream>

namespace freerouting {
namespace rules {

Net::Net(int p_net_no, const std::string& p_name, NetClass* p_net_class)
    : net_no_(p_net_no),
      name_(p_name),
      net_class_(p_net_class),
      clearance_class_(0),
      is_ghost_(false),
      has_items_(false),
      routing_ignored_(false)
{
}

void Net::set_name(const std::string& p_name)
{
    name_ = p_name;
}

void Net::set_net_class(NetClass* p_net_class)
{
    net_class_ = p_net_class;
}

void Net::set_clearance_class(int p_class)
{
    clearance_class_ = p_class;
}

void Net::set_ghost(bool p_value)
{
    is_ghost_ = p_value;
}

void Net::set_has_items(bool p_value)
{
    has_items_ = p_value;
}

void Net::set_routing_ignored(bool p_value)
{
    routing_ignored_ = p_value;
}

std::string Net::to_string() const
{
    return name_;
}

Nets::Nets()
    : board_(nullptr)
{
}

void Nets::add(Net* p_net)
{
    if (!p_net)
        return;

    int net_no = p_net->get_net_no();
    if (net_no >= static_cast<int>(net_arr_.size()))
    {
        net_arr_.resize(net_no + 1, nullptr);
    }
    net_arr_[net_no] = p_net;
}

Net* Nets::get(int p_net_no) const
{
    if (p_net_no >= 0 && p_net_no < static_cast<int>(net_arr_.size()))
    {
        return net_arr_[p_net_no];
    }
    return nullptr;
}

Net* Nets::get_by_name(const std::string& p_name) const
{
    for (Net* net : net_arr_)
    {
        if (net && net->get_name() == p_name)
            return net;
    }
    return nullptr;
}

int Nets::signal_net_count() const
{
    int count = 0;
    for (Net* net : net_arr_)
    {
        if (net && !net->is_ghost() && net->get_net_class())
            ++count;
    }
    return count;
}

void Nets::set_board(void* p_board)
{
    board_ = p_board;
}

std::string Nets::to_string() const
{
    std::ostringstream ss;
    ss << "Nets[count=" << net_arr_.size() << "]";
    return ss.str();
}

} // namespace rules
} // namespace freerouting