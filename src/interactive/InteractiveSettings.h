#pragma once

#include <memory>
#include <vector>
#include <string>

#include "board/Item.h"
#include "board/ItemSelectionFilter.h"

namespace freerouting {
namespace interactive {

class InteractiveSettings
{
public:
    InteractiveSettings();
    
    void update_from_routing_board(const std::shared_ptr<board::RoutingBoard>& p_board);
    
    int get_trace_half_width(int p_net_no) const;
    void set_trace_half_width(int p_index, int p_half_width);
    void setViaRuleIndex(int p_index);
    
    std::string to_string() const;
    
    // Fields
    std::string input_file;
    std::string output_file;
    int layer = 0;
    bool automatic_neckdown = true;
    int trace_pull_tight_accuracy = 10;
    int pull_tight_iterations = 10;
    bool multi_thread = false;
    const char* input_file_suffix = ".dsn";
    const char* session_file_suffix = ".ses";
    const char* output_file_suffix = ".dsn";
    int manual_via_rule_index = 0;
    bool manual_rule_selection = false;
    
private:
    std::vector<int> trace_half_widths_;
};

} // namespace interactive
} // namespace freerouting