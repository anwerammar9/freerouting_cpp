#include "rules/Net.h"

#include <cctype>
#include <sstream>
#include <stdexcept>

#include "logger/FRLogger.h"

namespace freerouting {
namespace rules {

namespace {

bool strings_equal_ignore_case(const std::string& p_first, const std::string& p_second)
{
    if (p_first.size() != p_second.size())
    {
        return false;
    }
    for (size_t i = 0; i < p_first.size(); ++i)
    {
        char first = static_cast<char>(std::tolower(static_cast<unsigned char>(p_first[i])));
        char second = static_cast<char>(std::tolower(static_cast<unsigned char>(p_second[i])));
        if (first != second)
        {
            return false;
        }
    }
    return true;
}

} // namespace

Net::Net(int p_net_no, const std::string& p_name, NetClass* p_net_class)
    : Net(p_net_no, p_name, 1, p_net_class, false)
{
}

Net::Net(int p_net_no,
         const std::string& p_name,
         int p_subnet_number,
         NetClass* p_net_class,
         bool p_contains_plane)
    : net_no_(p_net_no),
      name_(p_name),
      subnet_number_(p_subnet_number),
      net_class_(p_net_class),
      clearance_class_(0),
      is_ghost_(false),
      has_items_(false),
      routing_ignored_(false),
      contains_plane_(p_contains_plane)
{
}

void Net::set_name(const std::string& p_name)
{
    name_ = p_name;
}

void Net::set_contains_plane(bool p_value)
{
    contains_plane_ = p_value;
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
    if (!is_normal_net_no(net_no))
        return;

    if (net_no > static_cast<int>(net_arr_.size()))
    {
        net_arr_.resize(net_no, nullptr);
    }
    net_arr_[net_no - 1] = p_net;
}

Net* Nets::get(int p_net_no) const
{
    if (p_net_no < 1 || p_net_no > static_cast<int>(net_arr_.size()))
    {
        return nullptr;
    }
    Net* result = net_arr_[p_net_no - 1];
    if (result && result->get_net_no() != p_net_no)
    {
        logger::FRLogger::instance().warn("Nets.get: inconsistent net_no");
    }
    return result;
}

Net* Nets::get(const std::string& p_name, int p_subnet_number) const
{
    for (Net* net : net_arr_)
    {
        if (net && strings_equal_ignore_case(net->get_name(), p_name)
            && net->get_subnet_number() == p_subnet_number)
        {
            return net;
        }
    }
    return nullptr;
}

std::vector<Net*> Nets::get_all_by_name(const std::string& p_name) const
{
    std::vector<Net*> result;
    for (Net* net : net_arr_)
    {
        if (net && strings_equal_ignore_case(net->get_name(), p_name))
        {
            result.push_back(net);
        }
    }
    return result;
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

Net* Nets::new_net()
{
    std::string net_name = "net" + std::to_string(net_arr_.size() + 1);
    return add(net_name, 1, false);
}

Net* Nets::add(const std::string& p_name, int p_subnet_number, bool p_contains_plane)
{
    int new_net_no = static_cast<int>(net_arr_.size()) + 1;
    if (new_net_no >= max_legal_net_no)
    {
        logger::FRLogger::instance().warn("Nets.add_net: max_net_no out of range");
    }
    auto* new_net = new Net(new_net_no, p_name, p_subnet_number, nullptr, p_contains_plane);
    net_arr_.push_back(new_net);
    return new_net;
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