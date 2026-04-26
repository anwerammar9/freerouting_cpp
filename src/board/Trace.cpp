#include "board/Trace.h"

#include <algorithm>

namespace freerouting {
namespace board {

Trace::Trace(int p_layer, int p_half_width,
             const std::vector<int>& p_net_no_arr,
             int p_clearance_type, int p_id_no, int p_group_no,
             FixedState p_fixed_state)
    : Item(p_net_no_arr, p_clearance_type, p_id_no, p_group_no,
           p_fixed_state, nullptr)
    , half_width_(p_half_width)
    , layer_(std::max(p_layer, 0))
{
}

std::string Trace::to_string() const
{
    return "Trace";
}

bool Trace::contains_net(int p_net_no) const
{
    for (int net_no : net_no_arr_)
    {
        if (net_no == p_net_no)
        {
            return true;
        }
    }
    return false;
}

bool Trace::shares_net_no(const std::vector<int>& p_net_no_arr) const
{
    for (int this_net : net_no_arr_)
    {
        for (int other_net : p_net_no_arr)
        {
            if (this_net == other_net)
            {
                return true;
            }
        }
    }
    return false;
}

} // namespace board
} // namespace freerouting