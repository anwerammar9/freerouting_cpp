#include "interactive/InteractiveSettings.h"

#include <sstream>

#include "board/RoutingBoard.h"

namespace freerouting {
namespace interactive {

InteractiveSettings::InteractiveSettings()
{
    trace_half_widths_.resize(10, 1000);
    item_selection_filter = std::make_unique<board::ItemSelectionFilter>();
}

void InteractiveSettings::update_from_routing_board(const std::shared_ptr<RoutingBoard>& p_board)
{
    if (!p_board || !p_board->rules)
    {
        return;
    }
    auto net_class = p_board->rules->get_default_net_class();
    if (net_class)
    {
        trace_half_widths_ = net_class->get_trace_half_widths();
    }
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

void InteractiveSettings::setViaRuleIndex(int p_index)
{
    manual_via_rule_index = p_index;
    manual_rule_selection = true;
}

std::string InteractiveSettings::to_string() const
{
    std::ostringstream ss;
    ss << "InteractiveSettings(layer=" << layer_
       << ", pull_tight_acc=" << trace_pull_tight_accuracy
       << ", auto_neckdown=" << (automatic_neckdown ? "on" : "off") << ")";
    return ss.str();
}

} // namespace interactive
} // namespace freerouting