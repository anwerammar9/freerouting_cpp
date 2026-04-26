#pragma once

#include <memory>
#include <vector>
#include <string>

#include "board/ItemSelectionFilter.h"

namespace freerouting {
namespace interactive {

class RoutingBoard;

class InteractiveSettings : public std::enable_shared_from_this<InteractiveSettings>
{
public:
    InteractiveSettings();

    int trace_pull_tight_accuracy = 500;
    bool automatic_neckdown = true;
    int layer = 0;
    bool push_enabled = true;
    bool drag_components_enabled = false;
    bool select_on_all_visible_layers = false;
    bool is_stitch_route = false;
    int trace_pull_tight_region_width = 0;
    bool via_snap_to_smd_center = true;
    int horizontal_component_grid = 0;
    int vertical_component_grid = 0;
    bool manual_rule_selection = false;
    bool hilight_routing_obstacle = false;
    int manual_trace_clearance_class = 0;
    int manual_via_rule_index = 0;
    bool zoom_with_wheel = true;

    std::unique_ptr<board::ItemSelectionFilter> item_selection_filter;

    void update_from_routing_board(const std::shared_ptr<RoutingBoard>& p_board);
    int get_trace_half_width(int p_net_no) const;

    void set_trace_half_width(int p_index, int p_half_width);
    void setViaRuleIndex(int p_index);

    int get_layer() const { return layer_; }
    void set_layer(int p_layer) { layer_ = p_layer; }
    bool is_push_enabled() const { return push_enabled_; }
    void set_push_enabled(bool p_enabled) { push_enabled_ = p_enabled; }

    std::string to_string() const;

private:
    int layer_ = 0;
    bool push_enabled_ = true;
    std::vector<int> trace_half_widths_;
};

} // namespace interactive
} // namespace freerouting