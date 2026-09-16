#pragma once

#include <memory>
#include <string>
#include <vector>
#include <functional>

#include "board/BasicBoard.h"
#include "board/Communication.h"
#include "board/ChangedArea.h"
#include "board/BoardOutline.h"
#include "board/FixedState.h"
#include "board/SearchTreeManager.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/Polyline.h"
#include "geometry/planar/TileShape.h"
#include "designforms/specctra/Scanner.h"

namespace freerouting::geometry::planar {
class LineSegment;
} // namespace freerouting::geometry::planar

namespace freerouting::board {

class Item;
class Component;
class Components;
class Padstack;
class PolylineTrace;
class Trace;
class Via;

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

    // Creates a new PolylineTrace through p_points on p_layer and adds it to
    // the board. Returns the created trace, or nullptr on failure.
    PolylineTrace* add_polyline_trace(const std::vector<geometry::planar::IntPoint>& p_points,
                                      int p_layer, int p_half_width,
                                      const std::vector<int>& p_net_nos);

    // Returns the name of the net with number p_net_no, or "NET<p_net_no>"
    // if the net is unknown.
    std::string get_net_name(int p_net_no) const;

    // Create a route connection between two items
    // Returns true if successful
    bool add_route_connection(Item* p_from_item, int p_from_layer, int p_from_x, int p_from_y,
                              Item* p_to_item, int p_to_layer, int p_to_x, int p_to_y,
                              int p_trace_width, int p_net_no);

    // Get items for routing
    std::vector<Item*> get_autoroute_items() const;
    std::vector<Item*> get_connectable_items() const;

    /**
     * Returns all items of net p_net_no which can be connected by routing.
     * Mirrors Java BasicBoard.get_connectable_items(int).
     */
    std::vector<Item*> get_connectable_items(int p_net_no) const;

    /** Returns all Pin items on the board. Mirrors Java BasicBoard.get_pins(). */
    std::vector<Pin*> get_pins() const;

    /** Returns all Trace items on the board. Mirrors Java BasicBoard.get_traces(). */
    std::vector<Trace*> get_traces() const;

    /** Returns all Via items on the board. Mirrors Java BasicBoard.get_vias(). */
    std::vector<Via*> get_vias() const;

    /** Returns the board communication information. */
    Communication* communication() const { return board_communication_; }
    void set_communication(Communication* p_communication) { board_communication_ = p_communication; }

    /** Returns the components placed on the board. Mirrors Java BasicBoard.components. */
    Components* components() { return components_.get(); }
    const Components* components() const { return components_.get(); }

    // Create Pin items from parsed data
    void create_pins_from_parsed();

    const designforms::specctra::Result& get_parsed_result() const { return parsed_result_; }
    void set_parsed_result(const designforms::specctra::Result& p_result) { parsed_result_ = p_result; }

    int get_pin_count() const { return static_cast<int>(parsed_result_.pins.size()); }
    const designforms::specctra::PinDef* get_pin(int p_index) const;

    int get_item_count() const { return static_cast<int>(items_.size()); }
    Item* get_item(int p_index) const;

    // Search tree manager access
    SearchTreeManager& search_tree_manager() { return *search_tree_manager_; }
    const SearchTreeManager& search_tree_manager() const { return *search_tree_manager_; }

    // Shove failure tracking
    void set_shove_failing_obstacle(Item* p_item) { shove_failing_obstacle_ = p_item; }
    void set_shove_failing_layer(int p_layer) { shove_failing_layer_ = p_layer; }

    // Board outline
    const BoardOutline* get_outline() const;

    // Trace tail handling
    bool contains_trace_tails(const std::vector<Item*>& p_obstacles, const std::vector<int>& p_net_no_arr) const;
    Trace* get_trace_tail(const geometry::planar::Point& p_point, int p_layer, const std::vector<int>& p_net_no_arr);

    // Changed area
    void join_changed_area(const geometry::planar::Point& p_point, int p_layer);
    ChangedArea* changed_area() { return changed_area_.get(); }
    const ChangedArea* changed_area() const { return changed_area_.get(); }

    /**
     * Notifies the graphics observers of a changed box. Headless no-op until
     * the boardgraphics observers are ported. Mirrors Java
     * BasicBoard.join_graphics_update_box().
     */
    void join_graphics_update_box(const geometry::planar::IntBox& p_box);

    // Item removal
    void remove_items(const std::vector<Item*>& p_items);

    // Clearance value access
    int clearance_value(int p_class_1, int p_class_2, int p_layer) const;

    /**
     * Normalizes all PolylineTraces of net p_net_no. Returns true if something
     * was changed. Mirrors Java BasicBoard.normalize_traces().
     */
    bool normalize_traces(int p_net_no);

    /**
     * Checks if a trace segment from p_from_point to p_to_point can be
     * inserted without conflict. Returns the maximal insertable length
     * (INT_MAX if no conflict exists). Mirrors Java
     * RoutingBoard.check_trace_segment().
     */
    double check_trace_segment(const geometry::planar::Point& p_from_point,
                               const geometry::planar::Point& p_to_point,
                               int p_layer,
                               const std::vector<int>& p_net_no_arr,
                               int p_trace_half_width,
                               int p_cl_class_no,
                               bool p_only_not_shovable_obstacles);
    double check_trace_segment(const geometry::planar::LineSegment& p_line_segment,
                               int p_layer,
                               const std::vector<int>& p_net_no_arr,
                               int p_trace_half_width,
                               int p_cl_class_no,
                               bool p_only_not_shovable_obstacles);

    // Inserts a via into the board. p_attach_allowed indicates, if the via may
    // overlap with smd pins of the same net. Mirrors Java BasicBoard.insert_via
    // (trace splitting is a TODO until split_traces is ported).
    Via* insert_via(Padstack* p_padstack,
                    const geometry::planar::Point& p_center,
                    const std::vector<int>& p_net_no_arr,
                    int p_clearance_class,
                    FixedState p_fixed_state,
                    bool p_attach_allowed);

    std::string to_string() const { return "RoutingBoard"; }

    const geometry::planar::IntBox& get_bounding_box() const { return bounding_box_; }

private:
    static constexpr int64_t PULL_TIGHT_TIME_LIMIT = 2000;

    Item* shove_failing_obstacle_ = nullptr;
    int shove_failing_layer_ = -1;
    std::unique_ptr<ChangedArea> changed_area_;
    std::unique_ptr<SearchTreeManager> search_tree_manager_;

    geometry::planar::IntBox bounding_box_;
    Communication* board_communication_ = nullptr;
    std::vector<Item*> items_;
    std::unique_ptr<Components> components_;
    designforms::specctra::Result parsed_result_;
};

} // namespace freerouting::board