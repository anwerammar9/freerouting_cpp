#pragma once

#include <string>
#include <vector>

#include "rules/ClearanceMatrix.h"
#include "rules/DefaultItemClearanceClasses.h"
#include "board/Layer.h"

namespace freerouting {
namespace rules {

class ViaRule;

class NetClass
{
public:
    NetClass(const std::string& p_name,
            const board::LayerStructure& p_layer_structure,
            const ClearanceMatrix& p_clearance_matrix,
            bool p_is_ignored_by_autorouter);

    std::string get_name() const;
    void set_name(const std::string& p_name);

    int get_trace_clearance_class() const;
    void set_trace_clearance_class(int p_class);

    const ViaRule* get_via_rule() const;
    void set_via_rule(const ViaRule* p_via_rule);

    bool is_shove_fixed() const;
    void set_shove_fixed(bool p_value);

    bool is_pull_tight() const;
    void set_pull_tight(bool p_value);

    bool is_ignore_cycles_with_areas() const;
    void set_ignore_cycles_with_areas(bool p_value);

    double get_minimum_trace_length() const;
    void set_minimum_trace_length(double p_value);

    double get_maximum_trace_length() const;
    void set_maximum_trace_length(double p_value);

    bool is_ignored_by_autorouter() const { return is_ignored_by_autorouter_; }

    int get_trace_half_width(int p_layer) const;
    void set_trace_half_width(int p_layer, int p_width);
    void set_trace_half_width(int p_width);
    void set_trace_half_width_on_inner(int p_value);

    int layer_count() const { return static_cast<int>(trace_half_width_arr_.size()); }

    bool is_active_routing_layer(int p_layer) const;
    void set_active_routing_layer(int p_layer, bool p_active);
    void set_all_layers_active(bool p_value);
    void set_all_inner_layers_active(bool p_value);
    bool trace_width_is_layer_dependent() const;

    const ClearanceMatrix& get_clearance_matrix() const { return clearance_matrix_; }
    const board::LayerStructure& get_layer_structure() const { return board_layer_structure_; }

    DefaultItemClearanceClasses& get_default_item_clearance_classes()
    {
        return default_item_clearance_classes_;
    }

    std::string to_string() const;

private:
    const ClearanceMatrix& clearance_matrix_;
    const board::LayerStructure& board_layer_structure_;
    std::vector<int> trace_half_width_arr_;
    std::vector<bool> active_routing_layer_arr_;

    std::string name_;
    const ViaRule* via_rule_;
    int trace_clearance_class_;
    bool is_ignored_by_autorouter_;
    bool shove_fixed_;
    bool pull_tight_;
    bool ignore_cycles_with_areas_;
    double minimum_trace_length_;
    double maximum_trace_length_;
    DefaultItemClearanceClasses default_item_clearance_classes_;
};

} // namespace rules
} // namespace freerouting