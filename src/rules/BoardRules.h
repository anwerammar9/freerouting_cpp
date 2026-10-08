#pragma once

#include <memory>
#include <string>
#include <vector>

#include "board/Layer.h"
#include "rules/ClearanceMatrix.h"
#include "rules/Net.h"
#include "rules/NetClass.h"
#include "rules/ViaInfo.h"
#include "rules/ViaRule.h"

namespace freerouting {
namespace board {
class Item;
} // namespace board
namespace rules {

/** The net classes of a board. Port of Java rules.NetClasses (owns its classes). */
class NetClasses
{
public:
    NetClasses() = default;
    NetClasses(const NetClasses&) = delete;
    NetClasses& operator=(const NetClasses&) = delete;

    /** Takes ownership of p_net_class. */
    void add(NetClass* p_net_class);
    int count() const { return class_count(); }
    /** The class with index p_index, or nullptr. */
    NetClass* get(int p_index) const;
    /** The class named p_name (case sensitive, like Java), or nullptr. */
    NetClass* get(const std::string& p_name) const;
    NetClass* get_by_name(const std::string& p_name) const { return get(p_name); }

    NetClass* append(const std::string& p_name, const board::LayerStructure& p_layer_structure,
        const ClearanceMatrix& p_clearance_matrix, bool p_is_ignored_by_autorouter);
    /** Appends a class named "class<n>" with the first unused n. */
    NetClass* append(const board::LayerStructure& p_layer_structure, const ClearanceMatrix& p_clearance_matrix);
    NetClass* find(int p_trace_half_width, int p_trace_clearance_class, const ViaRule* p_via_rule) const;
    NetClass* find(const std::vector<int>& p_trace_half_width_arr, int p_trace_clearance_class,
        const ViaRule* p_via_rule) const;
    /** Removes p_net_class from the list (it stays alive, references to it remain valid). */
    bool remove(NetClass* p_net_class);
    int class_count() const { return static_cast<int>(classes_.size()); }
    std::string to_string() const;

private:
    std::vector<std::unique_ptr<NetClass>> classes_;
    std::vector<std::unique_ptr<NetClass>> removed_;
};

enum class AngleRestriction
{
    NONE,
    RIGHT,
    FORTY_FIVE
};

/** The design rules of a board. Port of Java rules.BoardRules. */
class BoardRules
{
public:
    /** Java BoardRules(LayerStructure, ClearanceMatrix). */
    BoardRules(const board::LayerStructure& p_layer_structure, const ClearanceMatrix& p_clearance_matrix);
    /** C++ convenience: the default clearance matrix with p_default_clearance_value. */
    BoardRules(const board::LayerStructure& p_layer_structure, int p_default_clearance_value);
    BoardRules(const BoardRules&) = delete;
    BoardRules& operator=(const BoardRules&) = delete;

    static int default_clearance_class() { return 1; }
    static int clearance_class_none() { return 0; }

    ClearanceMatrix& get_clearance_matrix() { return *clearance_matrix_; }
    const ClearanceMatrix& get_clearance_matrix() const { return *clearance_matrix_; }
    Nets& get_nets() { return nets_; }
    const Nets& get_nets() const { return nets_; }
    NetClasses& get_net_classes() { return net_classes_; }
    const NetClasses& get_net_classes() const { return net_classes_; }
    ViaInfos& get_via_infos() { return via_infos_; }
    const ViaInfos& get_via_infos() const { return via_infos_; }
    /** Java viaRules. */
    const std::vector<std::unique_ptr<ViaRule>>& get_via_rules() const { return via_rules_; }
    /** Appends p_via_rule to the via rules (Java viaRules.add) and returns it. */
    ViaRule* add_via_rule(std::unique_ptr<ViaRule> p_via_rule);
    const board::LayerStructure& get_layer_structure() const { return layer_structure_; }

    /** Java getTraceHalfWidth(netNumber, layer). */
    int get_trace_half_width(int p_net_no, int p_layer) const;
    /** Java traceWidthsAreLayerDependent. */
    bool trace_widths_are_layer_dependent(int p_net_no) const;
    int get_min_trace_half_width() const { return min_trace_half_width_; }
    void set_min_trace_half_width(int p_value) { min_trace_half_width_ = p_value; }
    int get_max_trace_half_width() const { return max_trace_half_width_; }
    void set_max_trace_half_width(int p_value) { max_trace_half_width_ = p_value; }
    int get_hole_clearance() const { return hole_clearance_; }
    void set_hole_clearance(int p_value);
    void set_default_trace_half_width(int p_layer, int p_value);
    int get_default_trace_half_width(int p_layer);
    void set_default_trace_half_widths(int p_value);

    /** The default net class (index 0), created on demand. Java getDefaultNetClass. */
    NetClass* get_default_net_class();
    /** The default net class if it exists (const access, no creation). */
    const NetClass* get_default_net_class() const;
    /** Java getNewNetClass(): a class "class<n>" with the properties of the default class. */
    NetClass* get_new_net_class();
    NetClass* get_new_net_class(const std::string& p_name);
    /** Creates the via rule p_name from the vias of the net class's via clearance class. Java createDefaultViaRule. */
    void create_default_via_rule(NetClass* p_net_class, const std::string& p_name);
    /** Java createDefaultNetClass: "default", trace half width 1500, clearance class 1. */
    void create_default_net_class();
    NetClass* append_net_class();
    /** Returns the existing class p_name or appends it with the properties of the default class. */
    NetClass* append_net_class(const std::string& p_name);
    /** The first via rule or nullptr. Java getDefaultViaRule. */
    ViaRule* get_default_via_rule() const;
    ViaRule* get_via_rule(const std::string& p_name) const;
    /** Java changeClearanceClassIndex. */
    void change_clearance_class_index(int p_from_index, int p_to_index, const std::vector<board::Item*>& p_board_items);
    /** Java removeClearanceClass; false if the class is still in use. */
    bool remove_clearance_class(int p_index, const std::vector<board::Item*>& p_board_items);
    /** The maximum width of the first via of the default via rule. Java getDefaultViaDiameter. */
    double get_default_via_diameter() const;

    double get_pin_edge_to_turn_dist() const { return pin_edge_to_turn_dist_; }
    void set_pin_edge_to_turn_dist(double p_value) { pin_edge_to_turn_dist_ = p_value; }
    bool is_ignore_conduction() const { return ignore_conduction_; }
    void set_ignore_conduction(bool p_value) { ignore_conduction_ = p_value; }
    AngleRestriction get_trace_angle_restriction() const { return trace_angle_restriction_; }
    void set_trace_angle_restriction(AngleRestriction p_angle) { trace_angle_restriction_ = p_angle; }
    bool is_use_slow_autoroute_algorithm() const { return use_slow_autoroute_algorithm_; }
    void set_use_slow_autoroute_algorithm(bool p_value) { use_slow_autoroute_algorithm_ = p_value; }

    std::string to_string() const;

private:
    std::unique_ptr<ClearanceMatrix> clearance_matrix_;
    Nets nets_;
    ViaInfos via_infos_;
    std::vector<std::unique_ptr<ViaRule>> via_rules_;
    NetClasses net_classes_;
    board::LayerStructure layer_structure_;
    AngleRestriction trace_angle_restriction_ = AngleRestriction::FORTY_FIVE;
    bool ignore_conduction_ = true;
    int min_trace_half_width_ = 100000;
    int max_trace_half_width_ = 100;
    double pin_edge_to_turn_dist_ = 0;
    bool use_slow_autoroute_algorithm_ = false;
    int hole_clearance_ = 0;
};

} // namespace rules
} // namespace freerouting
