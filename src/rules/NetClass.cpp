#include "rules/NetClass.h"

#include <algorithm>
#include <sstream>
#include <stdexcept>

#include "rules/ViaRule.h"

namespace freerouting {
namespace rules {

NetClass::NetClass(const std::string& p_name,
                       const board::LayerStructure& p_layer_structure,
                       const ClearanceMatrix& p_clearance_matrix,
                       bool p_is_ignored_by_autorouter)
    : clearance_matrix_(p_clearance_matrix),
      board_layer_structure_(p_layer_structure),
      trace_half_width_arr_(p_layer_structure.layer_count(), 0),
      active_routing_layer_arr_(p_layer_structure.layer_count(), false),
      name_(p_name),
      via_rule_(nullptr),
      trace_clearance_class_(1),
      is_ignored_by_autorouter_(p_is_ignored_by_autorouter),
      shove_fixed_(false),
      pull_tight_(true),
      ignore_cycles_with_areas_(false),
      minimum_trace_length_(0),
      maximum_trace_length_(0)
{
    for (int i = 0; i < p_layer_structure.layer_count(); ++i)
    {
        active_routing_layer_arr_[i] = p_layer_structure.get_layer(i)->is_signal;
    }
}

std::string NetClass::get_name() const
{
    return name_;
}

void NetClass::set_name(const std::string& p_name)
{
    name_ = p_name;
}

int NetClass::get_trace_clearance_class() const
{
    return trace_clearance_class_;
}

void NetClass::set_trace_clearance_class(int p_class)
{
    trace_clearance_class_ = p_class;
}

const ViaRule* NetClass::get_via_rule() const
{
    return via_rule_;
}

void NetClass::set_via_rule(const ViaRule* p_via_rule)
{
    via_rule_ = p_via_rule;
}

bool NetClass::is_shove_fixed() const
{
    return shove_fixed_;
}

void NetClass::set_shove_fixed(bool p_value)
{
    shove_fixed_ = p_value;
}

bool NetClass::is_pull_tight() const
{
    return pull_tight_;
}

void NetClass::set_pull_tight(bool p_value)
{
    pull_tight_ = p_value;
}

bool NetClass::is_ignore_cycles_with_areas() const
{
    return ignore_cycles_with_areas_;
}

void NetClass::set_ignore_cycles_with_areas(bool p_value)
{
    ignore_cycles_with_areas_ = p_value;
}

double NetClass::get_minimum_trace_length() const
{
    return minimum_trace_length_;
}

void NetClass::set_minimum_trace_length(double p_value)
{
    minimum_trace_length_ = p_value;
}

double NetClass::get_maximum_trace_length() const
{
    return maximum_trace_length_;
}

void NetClass::set_maximum_trace_length(double p_value)
{
    maximum_trace_length_ = p_value;
}

int NetClass::get_trace_half_width(int p_layer) const
{
    if (p_layer < 0 || p_layer >= static_cast<int>(trace_half_width_arr_.size()))
    {
        return 0;
    }
    return trace_half_width_arr_[p_layer];
}

void NetClass::set_trace_half_width(int p_layer, int p_width)
{
    if (p_layer >= 0 && p_layer < static_cast<int>(trace_half_width_arr_.size()))
    {
        trace_half_width_arr_[p_layer] = p_width;
    }
}

void NetClass::set_trace_half_width(int p_width)
{
    std::fill(trace_half_width_arr_.begin(), trace_half_width_arr_.end(), p_width);
}

void NetClass::set_trace_half_width_on_inner(int p_value)
{
    for (size_t i = 1; i + 1 < trace_half_width_arr_.size(); ++i)
    {
        trace_half_width_arr_[i] = p_value;
    }
}

bool NetClass::is_active_routing_layer(int p_layer) const
{
    if (p_layer < 0 || p_layer >= static_cast<int>(active_routing_layer_arr_.size()))
    {
        return false;
    }
    return active_routing_layer_arr_[p_layer];
}

void NetClass::set_active_routing_layer(int p_layer, bool p_active)
{
    if (p_layer >= 0 && p_layer < static_cast<int>(active_routing_layer_arr_.size()))
    {
        active_routing_layer_arr_[p_layer] = p_active;
    }
}

void NetClass::set_all_layers_active(bool p_value)
{
    std::fill(active_routing_layer_arr_.begin(), active_routing_layer_arr_.end(), p_value);
}

void NetClass::set_all_inner_layers_active(bool p_value)
{
    for (size_t i = 1; i + 1 < active_routing_layer_arr_.size(); ++i)
    {
        active_routing_layer_arr_[i] = p_value;
    }
}

bool NetClass::trace_width_is_layer_dependent() const
{
    int compare_value = trace_half_width_arr_.empty() ? 0 : trace_half_width_arr_[0];
    for (size_t i = 1; i < trace_half_width_arr_.size(); ++i)
    {
        if (board_layer_structure_.get_layer(static_cast<int>(i))->is_signal)
        {
            if (trace_half_width_arr_[i] != compare_value)
            {
                return true;
            }
        }
    }
    return false;
}

std::string NetClass::to_string() const
{
    return name_;
}

} // namespace rules
} // namespace freerouting