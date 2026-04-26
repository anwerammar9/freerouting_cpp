#include "interactive/InteractiveSettings.h"

namespace freerouting {
namespace interactive {

InteractiveSettings::InteractiveSettings()
{
    trace_half_widths_.resize(10, 1000);
}

void InteractiveSettings::update_from_routing_board(const std::shared_ptr<board::RoutingBoard>& /*p_board*/)
{
    // Stub
}

int InteractiveSettings::get_trace_half_width(int p_net_no) const
{
    if (p_net_no >= 0 && p_net_no < static_cast<int>(trace_half_widths_.size()))
    {
        return trace_half_widths_[p_net_no];
    }
    return trace_half_widths_.empty() ? 1000 : trace_half_widths_[0];
}

void InteractiveSettings::set_trace_half_width(int p_index, int p_half_width)
{
    if (p_index >= 0 && p_index < static_cast<int>(trace_half_widths_.size()))
    {
        trace_half_widths_[p_index] = p_half_width;
    }
}

std::string InteractiveSettings::to_string() const
{
    return "InteractiveSettings()";
}

} // namespace interactive
} // namespace freerouting