#pragma once

#include <string>
#include <vector>
#include <memory>

#include "rules/Net.h"
#include "rules/NetClass.h"
#include "board/Layer.h"

namespace freerouting {
namespace rules {

class NetClasses
{
public:
    NetClasses();

    void add(NetClass* p_net_class);
    NetClass* get(int p_index) const;
    NetClass* get_by_name(const std::string& p_name) const;

    int class_count() const { return static_cast<int>(classes_.size()); }

    std::string to_string() const;

private:
    std::vector<NetClass*> classes_;
    std::vector<std::string> names_;
};

enum class AngleRestriction
{
    NONE,
    RIGHT,
    FORTY_FIVE
};

class BoardRules
{
public:
    BoardRules(const board::LayerStructure& p_layer_structure,
               int p_default_clearance_value);

    ClearanceMatrix& get_clearance_matrix() { return *clearance_matrix_; }
    const ClearanceMatrix& get_clearance_matrix() const { return *clearance_matrix_; }

    Nets& get_nets() { return nets_; }
    const Nets& get_nets() const { return nets_; }

    NetClasses& get_net_classes() { return net_classes_; }
    const NetClasses& get_net_classes() const { return net_classes_; }

    const board::LayerStructure& get_layer_structure() const { return layer_structure_; }

    AngleRestriction get_trace_angle_restriction() const { return trace_angle_restriction_; }
    void set_trace_angle_restriction(AngleRestriction p_angle);

    bool is_ignore_conduction() const { return ignore_conduction_; }
    void set_ignore_conduction(bool p_value);

    int get_min_trace_half_width() const { return min_trace_half_width_; }
    void set_min_trace_half_width(int p_value);

    int get_max_trace_half_width() const { return max_trace_half_width_; }
    void set_max_trace_half_width(int p_value);

    double get_pin_edge_to_turn_dist() const { return pin_edge_to_turn_dist_; }
    void set_pin_edge_to_turn_dist(double p_value);

    bool is_use_slow_autoroute_algorithm() const { return use_slow_autoroute_algorithm_; }
    void set_use_slow_autoroute_algorithm(bool p_value);

    std::string to_string() const;

private:
    std::unique_ptr<ClearanceMatrix> clearance_matrix_;
    Nets nets_;
    NetClasses net_classes_;
    const board::LayerStructure& layer_structure_;

    AngleRestriction trace_angle_restriction_ = AngleRestriction::FORTY_FIVE;
    bool ignore_conduction_ = true;
    int min_trace_half_width_ = 0;
    int max_trace_half_width_ = 0;
    double pin_edge_to_turn_dist_ = 0;
    bool use_slow_autoroute_algorithm_ = false;
};

} // namespace rules
} // namespace freerouting