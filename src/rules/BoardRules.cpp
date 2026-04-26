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

NetClass* NetClasses::get_by_name(const std::string& p_name) const
{
    for (size_t i = 0; i < names_.size(); ++i)
    {
        if (names_[i] == p_name)
            return classes_[i];
    }
    return nullptr;
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

std::string BoardRules::to_string() const
{
    std::ostringstream ss;
    ss << "BoardRules[layers=" << layer_structure_.layer_count() << "]";
    return ss.str();
}

} // namespace rules
} // namespace freerouting