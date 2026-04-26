#pragma once

#include <memory>
#include <string>
#include <vector>
#include <functional>

#include "board/BasicBoard.h"
#include "board/Communication.h"
#include "board/ChangedArea.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/Polyline.h"
#include "designforms/specctra/Scanner.h"

namespace freerouting::board {

class Item;

class RoutingBoard : public BasicBoard
{
public:
    RoutingBoard(const geometry::planar::IntBox& p_bounding_box,
                 const LayerStructure& p_layer_structure,
                 std::unique_ptr<::freerouting::rules::BoardRules> p_rules,
                 Communication* p_board_communication = nullptr);

    virtual ~RoutingBoard();

    void additional_update_after_change(Item* p_item);

    bool remove_items_and_pull_tight(const std::vector<Item*>& p_item_list,
                                     int p_tidy_width,
                                     int p_pull_tight_accuracy);

    void start_marking_changed_area();
    void opt_changed_area(const std::vector<int>& p_nets_to_optimize,
                          const geometry::planar::IntOctagon* p_tidy_region,
                          int p_pull_tight_accuracy,
                          void* p_stoppable,
                          std::function<bool()> p_cancel_callback,
                          int64_t p_time_limit_ms);

    void combine_traces(int p_net_no);

    void add_item(Item* p_item);
    void remove_item(Item* p_item);

    // Create a route connection between two items
    // Returns true if successful
    bool add_route_connection(Item* p_from_item, int p_from_layer, int p_from_x, int p_from_y,
                              Item* p_to_item, int p_to_layer, int p_to_x, int p_to_y,
                              int p_trace_width, int p_net_no);

    // Get items for routing
    std::vector<Item*> get_autoroute_items() const;
    std::vector<Item*> get_connectable_items() const;

    // Create Pin items from parsed data
    void create_pins_from_parsed();

    const designforms::specctra::Result& get_parsed_result() const { return parsed_result_; }
    void set_parsed_result(const designforms::specctra::Result& p_result) { parsed_result_ = p_result; }

    int get_pin_count() const { return static_cast<int>(parsed_result_.pins.size()); }
    const designforms::specctra::PinDef* get_pin(int p_index) const;

    int get_item_count() const { return static_cast<int>(items_.size()); }
    Item* get_item(int p_index) const;

    std::string to_string() const { return "RoutingBoard"; }

    const geometry::planar::IntBox& get_bounding_box() const { return bounding_box_; }

private:
    static constexpr int64_t PULL_TIGHT_TIME_LIMIT = 2000;

    Item* shove_failing_obstacle_ = nullptr;
    int shove_failing_layer_ = -1;
    std::unique_ptr<ChangedArea> changed_area_;

    const geometry::planar::IntBox& bounding_box_;
    Communication* board_communication_ = nullptr;
    std::vector<Item*> items_;
    designforms::specctra::Result parsed_result_;
};

} // namespace freerouting::board