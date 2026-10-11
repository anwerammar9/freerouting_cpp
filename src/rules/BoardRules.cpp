#include "rules/BoardRules.h"

#include <algorithm>
#include <sstream>

#include "rules/ClearanceMatrix.h"
#include "logger/FRLogger.h"

namespace freerouting {
namespace rules {

void NetClasses::add(NetClass* p_net_class)
{
    if (p_net_class != nullptr)
    {
        classes_.emplace_back(p_net_class);
    }
}

NetClass* NetClasses::get(int p_index) const
{
    if (p_index >= 0 && p_index < class_count())
    {
        return classes_[p_index].get();
    }
    return nullptr;
}

NetClass* NetClasses::get(const std::string& p_name) const
{
    for (const auto& curr_class : classes_)
    {
        if (curr_class->get_name() == p_name)
        {
            return curr_class.get();
        }
    }
    return nullptr;
}

NetClass* NetClasses::append(const std::string& p_name, const board::LayerStructure& p_layer_structure,
    const ClearanceMatrix& p_clearance_matrix, bool p_is_ignored_by_autorouter)
{
    classes_.push_back(std::make_unique<NetClass>(p_name, p_layer_structure, p_clearance_matrix, p_is_ignored_by_autorouter));
    return classes_.back().get();
}

NetClass* NetClasses::append(const board::LayerStructure& p_layer_structure, const ClearanceMatrix& p_clearance_matrix)
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

NetClass* NetClasses::find(int p_trace_half_width, int p_trace_clearance_class, const ViaRule* p_via_rule) const
{
    for (const auto& curr_class : classes_)
    {
        if (curr_class->get_trace_clearance_class() != p_trace_clearance_class || curr_class->get_via_rule() != p_via_rule)
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
            return curr_class.get();
        }
    }
    return nullptr;
}

NetClass* NetClasses::find(const std::vector<int>& p_trace_half_width_arr, int p_trace_clearance_class,
    const ViaRule* p_via_rule) const
{
    for (const auto& curr_class : classes_)
    {
        if (curr_class->get_trace_clearance_class() != p_trace_clearance_class || curr_class->get_via_rule() != p_via_rule
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
            return curr_class.get();
        }
    }
    return nullptr;
}

bool NetClasses::remove(NetClass* p_net_class)
{
    for (auto it = classes_.begin(); it != classes_.end(); ++it)
    {
        if (it->get() == p_net_class)
        {
            removed_.push_back(std::move(*it));
            classes_.erase(it);
            return true;
        }
    }
    return false;
}

std::string NetClasses::to_string() const
{
    return "NetClasses[count=" + std::to_string(class_count()) + "]";
}

BoardRules::BoardRules(const board::LayerStructure& p_layer_structure, const ClearanceMatrix& p_clearance_matrix)
    : clearance_matrix_(std::make_unique<ClearanceMatrix>(p_clearance_matrix))
    , layer_structure_(p_layer_structure)
{
    nets_.set_rules(this);
}

BoardRules::BoardRules(const board::LayerStructure& p_layer_structure, int p_default_clearance_value)
    : BoardRules(p_layer_structure, ClearanceMatrix::get_default_instance(p_layer_structure, p_default_clearance_value))
{
}

ViaRule* BoardRules::add_via_rule(std::unique_ptr<ViaRule> p_via_rule)
{
    via_rules_.push_back(std::move(p_via_rule));
    return via_rules_.back().get();
}

bool BoardRules::remove_via_rule(const ViaRule* p_via_rule)
{
    for (auto it = via_rules_.begin(); it != via_rules_.end(); ++it)
    {
        if (it->get() == p_via_rule)
        {
            via_rules_.erase(it);
            return true;
        }
    }
    return false;
}

int BoardRules::get_trace_half_width(int p_net_no, int p_layer) const
{
    // Java dereferences the net without a null check.
    const Net* curr_net = nets_.get(p_net_no);
    if (curr_net == nullptr || curr_net->get_net_class() == nullptr)
    {
        return 0;
    }
    return curr_net->get_net_class()->get_trace_half_width(p_layer);
}

bool BoardRules::trace_widths_are_layer_dependent(int p_net_no) const
{
    const int compare_width = get_trace_half_width(p_net_no, 0);
    for (int i = 1; i < layer_structure_.layer_count(); ++i)
    {
        if (get_trace_half_width(p_net_no, i) != compare_width)
        {
            return true;
        }
    }
    return false;
}

void BoardRules::set_hole_clearance(int p_value)
{
    hole_clearance_ = std::max(0, p_value);
}

void BoardRules::set_default_trace_half_width(int p_layer, int p_value)
{
    get_default_net_class()->set_trace_half_width(p_layer, p_value);
    min_trace_half_width_ = std::min(min_trace_half_width_, p_value);
    max_trace_half_width_ = std::max(max_trace_half_width_, p_value);
}

int BoardRules::get_default_trace_half_width(int p_layer)
{
    return get_default_net_class()->get_trace_half_width(p_layer);
}

void BoardRules::set_default_trace_half_widths(int p_value)
{
    if (p_value <= 0)
    {
        logger::FRLogger::instance().warn("BoardRules.set_trace_half_widths: value out of range");
        return;
    }
    get_default_net_class()->set_trace_half_width(p_value);
    min_trace_half_width_ = std::min(min_trace_half_width_, p_value);
    max_trace_half_width_ = std::max(max_trace_half_width_, p_value);
}

NetClass* BoardRules::get_default_net_class()
{
    if (net_classes_.count() <= 0)
    {
        create_default_net_class();
    }
    return net_classes_.get(0);
}

const NetClass* BoardRules::get_default_net_class() const
{
    return net_classes_.get(0);
}

NetClass* BoardRules::get_new_net_class()
{
    NetClass* result = net_classes_.append(layer_structure_, *clearance_matrix_);
    result->set_trace_clearance_class(get_default_net_class()->get_trace_clearance_class());
    result->set_via_rule(get_default_via_rule());
    result->set_trace_half_width(get_default_net_class()->get_trace_half_width(0));
    return result;
}

NetClass* BoardRules::get_new_net_class(const std::string& p_name)
{
    NetClass* result = net_classes_.append(p_name, layer_structure_, *clearance_matrix_, false);
    result->set_trace_clearance_class(get_default_net_class()->get_trace_clearance_class());
    result->set_via_rule(get_default_via_rule());
    result->set_trace_half_width(get_default_net_class()->get_trace_half_width(0));
    return result;
}

void BoardRules::create_default_net_class()
{
    NetClass* default_net_class = net_classes_.append("default", layer_structure_, *clearance_matrix_, false);
    constexpr int kDefaultTraceHalfWidth = 1500;
    default_net_class->set_trace_half_width(kDefaultTraceHalfWidth);
    default_net_class->set_trace_clearance_class(1);
}

NetClass* BoardRules::append_net_class()
{
    NetClass* new_class = net_classes_.append(layer_structure_, *clearance_matrix_);
    NetClass* default_class = net_classes_.get(0);
    new_class->set_via_rule(default_class->get_via_rule());
    new_class->set_trace_half_width(default_class->get_trace_half_width(0));
    new_class->set_trace_clearance_class(default_class->get_trace_clearance_class());
    return new_class;
}

NetClass* BoardRules::append_net_class(const std::string& p_name)
{
    if (NetClass* found_class = net_classes_.get(p_name))
    {
        return found_class;
    }
    NetClass* new_class = net_classes_.append(p_name, layer_structure_, *clearance_matrix_, false);
    NetClass* default_class = net_classes_.get(0);
    new_class->get_default_item_clearance_classes() = DefaultItemClearanceClasses(default_class->get_default_item_clearance_classes());
    new_class->set_via_rule(default_class->get_via_rule());
    new_class->set_trace_half_width(default_class->get_trace_half_width(0));
    new_class->set_trace_clearance_class(default_class->get_trace_clearance_class());
    return new_class;
}

ViaRule* BoardRules::get_default_via_rule() const
{
    return via_rules_.empty() ? nullptr : via_rules_.front().get();
}

ViaRule* BoardRules::get_via_rule(const std::string& p_name) const
{
    for (const auto& curr_rule : via_rules_)
    {
        if (curr_rule->get_name() == p_name)
        {
            return curr_rule.get();
        }
    }
    return nullptr;
}

std::string BoardRules::to_string() const
{
    return "BoardRules[layers=" + std::to_string(layer_structure_.layer_count()) + "]";
}

} // namespace rules
} // namespace freerouting
