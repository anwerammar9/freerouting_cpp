#include "rules/BoardRules.h"

#include <sstream>

#include "rules/ClearanceMatrix.h"

namespace freerouting {
namespace rules {

NetClasses::NetClasses()
{
}

void NetClasses::add(NetClass* p_net_class)
{
    if (!p_net_class)
        return;

    classes_.push_back(p_net_class);
    names_.push_back(p_net_class->get_name());
}

NetClass* NetClasses::get(int p_index) const
{
    if (p_index >= 0 && p_index < class_count())
    {
        return classes_[p_index];
    }
    return nullptr;
}

NetClass* NetClasses::get(const std::string& p_name) const
{
    return get_by_name(p_name);
}

NetClass* NetClasses::get_by_name(const std::string& p_name) const
{
    for (size_t i = 0; i < names_.size(); ++i)
    {
        if (names_[i] == p_name)
            return classes_[i];
    }
    return nullptr;
}

NetClass* NetClasses::append(const std::string& p_name,
                             const board::LayerStructure& p_layer_structure,
                             const ClearanceMatrix& p_clearance_matrix,
                             bool p_is_ignored_by_autorouter)
{
    auto* new_class = new NetClass(p_name,
                                   p_layer_structure,
                                   p_clearance_matrix,
                                   p_is_ignored_by_autorouter);
    add(new_class);
    return new_class;
}

NetClass* NetClasses::append(const board::LayerStructure& p_layer_structure,
                             const ClearanceMatrix& p_clearance_matrix)
{
    int index = 0;
    std::string new_name;
    do
    {
        ++index;
        new_name = "class" + std::to_string(index);
    } while (get(new_name) != nullptr);
    return append(new_name, p_layer_structure, p_clearance_matrix, false);
}

NetClass* NetClasses::find(int p_trace_half_width,
                           int p_trace_clearance_class,
                           const ViaRule* p_via_rule) const
{
    for (NetClass* curr_class : classes_)
    {
        if (!curr_class)
            continue;
        if (curr_class->get_trace_clearance_class() != p_trace_clearance_class
            || curr_class->get_via_rule() != p_via_rule)
        {
            continue;
        }
        bool trace_widths_equal = true;
        for (int i = 0; i < curr_class->layer_count(); ++i)
        {
            if (curr_class->get_trace_half_width(i) != p_trace_half_width)
            {
                trace_widths_equal = false;
                break;
            }
        }
        if (trace_widths_equal)
        {
            return curr_class;
        }
    }
    return nullptr;
}

NetClass* NetClasses::find(const std::vector<int>& p_trace_half_width_arr,
                           int p_trace_clearance_class,
                           const ViaRule* p_via_rule) const
{
    for (NetClass* curr_class : classes_)
    {
        if (!curr_class)
            continue;
        if (curr_class->get_trace_clearance_class() != p_trace_clearance_class
            || curr_class->get_via_rule() != p_via_rule
            || static_cast<int>(p_trace_half_width_arr.size()) != curr_class->layer_count())
        {
            continue;
        }
        bool trace_widths_equal = true;
        for (int i = 0; i < curr_class->layer_count(); ++i)
        {
            if (curr_class->get_trace_half_width(i) != p_trace_half_width_arr[i])
            {
                trace_widths_equal = false;
                break;
            }
        }
        if (trace_widths_equal)
        {
            return curr_class;
        }
    }
    return nullptr;
}

bool NetClasses::remove(NetClass* p_net_class)
{
    for (int i = 0; i < class_count(); ++i)
    {
        if (classes_[i] == p_net_class)
        {
            classes_.erase(classes_.begin() + i);
            names_.erase(names_.begin() + i);
            return true;
        }
    }
    return false;
}

std::string NetClasses::to_string() const
{
    std::ostringstream ss;
    ss << "NetClasses[count=" << class_count() << "]";
    return ss.str();
}

BoardRules::BoardRules(const board::LayerStructure& p_layer_structure,
                            int p_default_clearance_value)
    : clearance_matrix_(std::make_unique<ClearanceMatrix>(
          ClearanceMatrix::get_default_instance(p_layer_structure, p_default_clearance_value))),
      net_classes_(),
      layer_structure_(p_layer_structure)
{
}

void BoardRules::set_trace_angle_restriction(AngleRestriction p_angle)
{
    trace_angle_restriction_ = p_angle;
}

void BoardRules::set_ignore_conduction(bool p_value)
{
    ignore_conduction_ = p_value;
}

void BoardRules::set_min_trace_half_width(int p_value)
{
    min_trace_half_width_ = p_value;
}

void BoardRules::set_max_trace_half_width(int p_value)
{
    max_trace_half_width_ = p_value;
}

void BoardRules::set_pin_edge_to_turn_dist(double p_value)
{
    pin_edge_to_turn_dist_ = p_value;
}

void BoardRules::set_use_slow_autoroute_algorithm(bool p_value)
{
    use_slow_autoroute_algorithm_ = p_value;
}

const ViaRule* BoardRules::get_default_via_rule() const
{
    // Java returns the first entry of the global via rule list. The C++ rules
    // carry no via rule list yet, so the first net class with a via rule is used.
    for (int i = 0; i < net_classes_.count(); ++i)
    {
        const ViaRule* current_rule = net_classes_.get(i)->get_via_rule();
        if (current_rule != nullptr)
        {
            return current_rule;
        }
    }
    return nullptr;
}

std::string BoardRules::to_string() const
{
    std::ostringstream ss;
    ss << "BoardRules[layers=" << layer_structure_.layer_count() << "]";
    return ss.str();
}

} // namespace rules
} // namespace freerouting