#pragma once

#include <memory>
#include <optional>
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

namespace freerouting::datastructures {
class TimeLimit;
class Stoppable;
} // namespace freerouting::datastructures

namespace freerouting::settings {
class RouterSettings;
} // namespace freerouting::settings

namespace freerouting::geometry::planar {
class LineSegment;
} // namespace freerouting::geometry::planar

namespace freerouting::autoroute {
class AutorouteAttemptResult;
} // namespace freerouting::autoroute

namespace freerouting::board {

class Item;
class Component;
class Components;
class BoardLibrary;
class Pin;
class ObstacleArea;
class ViaObstacleArea;
class ComponentObstacleArea;
class ComponentOutline;
class ConductionArea;
class Padstack;
class Pin;
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

    /**
     * Autoroutes from the input pin until the first via, in case the pin and its connected set has
     * only 1 layer. Ripup is allowed if p_ripup_costs is >= 0.
     * Mirrors Java RoutingBoard.fanout.
     *
     * The implementation lives in the autoroute module
     * (src/autoroute/RoutingBoardFanout.cpp), because it needs AutorouteEngine and AutorouteControl.
     */
    autoroute::AutorouteAttemptResult fanout(Pin* p_pin,
                                            settings::RouterSettings* p_router_settings,
                                            int p_ripup_costs,
                                            datastructures::Stoppable* p_stoppable,
                                            datastructures::TimeLimit* p_time_limit);

    /**
     * Routes automatically p_item to another item of the same net, to which it
     * is not yet electrically connected. Returns the state of the attempt.
     * Mirrors Java RoutingBoard.autoroute(Item, RouterSettings, int, Stoppable,
     * TimeLimit).
     *
     * The implementation lives in the autoroute module
     * (src/autoroute/RoutingBoardAutoroute.cpp), because it needs AutorouteEngine
     * and AutorouteControl.
     */
    autoroute::AutorouteAttemptResult autoroute(board::Item* p_item,
                                                settings::RouterSettings* p_router_settings,
                                                int p_via_costs,
                                                datastructures::Stoppable* p_stoppable,
                                                datastructures::TimeLimit* p_time_limit);

    void add_item(Item* p_item);

    /**
     * Removes p_item from the board: notifies the change path, evicts the
     * search tree entries and unlinks the item list entry. Items with
     * is_deletion_forbidden() are kept. Mirrors Java
     * BoardItemRepository.removeItem(Item).
     */
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
    /** Takes ownership of the communication object (the DSN reader creates it with the board). */
    void adopt_communication(std::unique_ptr<Communication> p_communication)
    {
        owned_communication_ = std::move(p_communication);
        board_communication_ = owned_communication_.get();
    }

    /** Returns the components placed on the board. Mirrors Java BasicBoard.components. */
    Components* components() { return components_.get(); }
    const Components* components() const { return components_.get(); }

    /** The padstacks, packages and via padstacks of the board. Mirrors Java BasicBoard.library. */
    BoardLibrary& library() { return *library_; }
    const BoardLibrary& library() const { return *library_; }

    // Create Pin items from parsed data
    void create_pins_from_parsed();

    const designforms::specctra::Result& get_parsed_result() const { return parsed_result_; }
    void set_parsed_result(const designforms::specctra::Result& p_result) { parsed_result_ = p_result; }

    int get_pin_count() const { return static_cast<int>(parsed_result_.pins.size()); }
    const designforms::specctra::PinDef* get_pin(int p_index) const;

    int get_item_count() const { return static_cast<int>(items_.size()); }
    /**
     * The board items in Java item-list order: descending item id (newest first). Java
     * Item.compareTo returns other.id - this.id and the item list is a ConcurrentSkipListMap.
     */
    std::vector<Item*> get_items() const override { return std::vector<Item*>(items_.rbegin(), items_.rend()); }
    Item* get_item(int p_index) const;

    // Search tree manager access
    SearchTreeManager& search_tree_manager() { return *get_search_tree_manager(); }
    const SearchTreeManager& search_tree_manager() const { return *get_search_tree_manager(); }

    // Shove failure tracking
    void set_shove_failing_obstacle(Item* p_item) { shove_failing_obstacle_ = p_item; }

    /** Returns the obstacle which made the last shove fail, or nullptr. */
    Item* get_shove_failing_obstacle() const { return shove_failing_obstacle_; }
    void set_shove_failing_layer(int p_layer) { shove_failing_layer_ = p_layer; }

    /** Returns the layer on which the last shove failed, or -1. */
    int get_shove_failing_layer() const { return shove_failing_layer_; }

    /** Resets the shove failing obstacle tracking. Mirrors Java
     *  RoutingBoard.clear_shove_failing_obstacle(). */
    void clear_shove_failing_obstacle();

    /**
     * Checks if a trace polyline with the input parameters can be inserted
     * while shoving aside obstacle traces and vias. Mirrors Java
     * RoutingBoard.check_forced_trace_polyline().
     */
    bool check_forced_trace_polyline(const geometry::planar::Polyline& p_polyline,
                                     int p_half_width,
                                     int p_layer,
                                     const std::vector<int>& p_net_no_arr,
                                     int p_clearance_class_index,
                                     int p_max_recursion_depth,
                                     int p_max_via_recursion_depth,
                                     int p_max_spring_over_recursion_depth);

    /**
     * Tries to insert a trace polyline with the input parameters while shoving
     * aside obstacle traces and vias. Returns the last corner of the polyline,
     * to which the shove succeeded, or std::nullopt if the check was
     * inaccurate and an error occurred while inserting. Mirrors Java
     * RoutingBoard.insert_forced_trace_polyline().
     *
     * Divergence: Java compares the returned corner object against the
     * polyline corners by reference identity; the C++ port compares the
     * IntPoint values.
     */
    std::optional<geometry::planar::IntPoint> insert_forced_trace_polyline(
        const geometry::planar::Polyline& p_polyline,
        int p_half_width,
        int p_layer,
        const std::vector<int>& p_net_no_arr,
        int p_clearance_class_index,
        int p_max_recursion_depth,
        int p_max_via_recursion_depth,
        int p_max_spring_over_recursion_depth,
        int p_tidy_width,
        int p_pull_tight_accuracy,
        bool p_with_check,
        const datastructures::TimeLimit* p_time_limit);

    /**
     * Tries to insert a trace line from p_from_corner to p_to_corner while
     * shoving aside obstacle traces and vias. Returns the last point between
     * p_from_corner and p_to_corner, to which the shove succeeded, or
     * std::nullopt if the check was inaccurate. Mirrors Java
     * RoutingBoard.insert_forced_trace_segment().
     */
    std::optional<geometry::planar::IntPoint> insert_forced_trace_segment(
        const geometry::planar::IntPoint& p_from_corner,
        const geometry::planar::IntPoint& p_to_corner,
        int p_half_width,
        int p_layer,
        const std::vector<int>& p_net_no_arr,
        int p_clearance_class_index,
        int p_max_recursion_depth,
        int p_max_via_recursion_depth,
        int p_max_spring_over_recursion_depth,
        int p_tidy_width,
        int p_pull_tight_accuracy,
        bool p_with_check,
        const datastructures::TimeLimit* p_time_limit);

    /**
     * Inserts a trace from p_from_point to the nearest point on p_to_trace.
     * Returns false if that is not possible without clearance violation.
     * Mirrors Java RoutingBoard.connect_to_trace().
     */
    bool connect_to_trace(const geometry::planar::IntPoint& p_from_point,
                          Trace* p_to_trace,
                          int p_pen_half_width,
                          int p_clearance_class_index);

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

    /**
     * Removes all removable items of p_items from the board, evicting their
     * search tree entries. Items which are delete-fixed or user-fixed are
     * kept. Returns false if some items could not be removed because they are
     * fixed. Mirrors Java BasicBoard.removeItems(Collection).
     */
    bool remove_items(const std::vector<Item*>& p_items);

    /**
     * Removes all trace tails of the input net. If p_net_no <= 0, the tails
     * of all nets are removed. Returns true if something was removed.
     * Mirrors Java RoutingBoard.removeTraceTails(int, Item.StopConnectionOption).
     */
    bool remove_trace_tails(int p_net_no, Item::StopConnectionOption p_stop_option);

    /**
     * Clears the autoroute info of all items of the board. Mirrors Java
     * RoutingBoard.clearAllItemTemporaryAutorouteData().
     */
    void clear_all_item_temporary_autoroute_data();

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

    // ---- Java BasicBoard insert facade (items go into the item list and all search trees) ----

    /**
     * Inserts p_item into the board: item list, search trees, autoroute
     * database update. A clearance class out of range is reset to 0 with a
     * warning. Port of Java BoardItemRepository.insertItem.
     */
    void insert_item(Item* p_item);
    /** Java BasicBoard.insertPin: the padstack comes from the component package. */
    Pin* insert_pin(int p_component_id, int p_pin_index, const std::vector<int>& p_net_nos, int p_clearance_class,
        FixedState p_fixed_state);
    /** Java BasicBoard.insertObstacle(Area, layer, clearanceClass, fixedState). */
    ObstacleArea* insert_obstacle(std::shared_ptr<geometry::planar::Area> p_area, int p_layer, int p_clearance_class,
        FixedState p_fixed_state);
    /** Java BasicBoard.insertObstacle with a component placement. */
    ObstacleArea* insert_obstacle(std::shared_ptr<geometry::planar::Area> p_area, int p_layer,
        const geometry::planar::Vector& p_translation, double p_rotation_in_degree, bool p_side_changed,
        int p_clearance_class, int p_component_id, const std::string& p_name, FixedState p_fixed_state);
    ViaObstacleArea* insert_via_obstacle(std::shared_ptr<geometry::planar::Area> p_area, int p_layer,
        int p_clearance_class, FixedState p_fixed_state);
    ViaObstacleArea* insert_via_obstacle(std::shared_ptr<geometry::planar::Area> p_area, int p_layer,
        const geometry::planar::Vector& p_translation, double p_rotation_in_degree, bool p_side_changed,
        int p_clearance_class, int p_component_id, const std::string& p_name, FixedState p_fixed_state);
    ComponentObstacleArea* insert_component_obstacle(std::shared_ptr<geometry::planar::Area> p_area, int p_layer,
        int p_clearance_class, FixedState p_fixed_state);
    ComponentObstacleArea* insert_component_obstacle(std::shared_ptr<geometry::planar::Area> p_area, int p_layer,
        const geometry::planar::Vector& p_translation, double p_rotation_in_degree, bool p_side_changed,
        int p_clearance_class, int p_component_id, const std::string& p_name, FixedState p_fixed_state);
    /**
     * Java BasicBoard.insertComponentOutline. The C++ ComponentOutline does not
     * store the courtyard/fabrication/closed flags yet; they are accepted for
     * signature parity.
     */
    ComponentOutline* insert_component_outline(std::shared_ptr<geometry::planar::Area> p_area, bool p_is_front,
        const geometry::planar::Vector& p_translation, double p_rotation_in_degree, int p_component_id,
        bool p_is_courtyard, bool p_is_fabrication, bool p_is_closed, FixedState p_fixed_state);
    /** Java BasicBoard.insertConductionArea. */
    ConductionArea* insert_conduction_area(std::shared_ptr<geometry::planar::Area> p_area, int p_layer,
        const std::vector<int>& p_net_nos, int p_clearance_class, bool p_is_obstacle, FixedState p_fixed_state);
    /** Java BasicBoard.insertOutline. */
    BoardOutline* insert_outline(std::vector<std::shared_ptr<geometry::planar::PolylineShape>> p_outline_shapes,
        int p_clearance_class);


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

    geometry::planar::IntBox get_bounding_box() const override { return bounding_box_; }

private:
    static constexpr int64_t PULL_TIGHT_TIME_LIMIT = 2000;

    Item* shove_failing_obstacle_ = nullptr;
    int shove_failing_layer_ = -1;
    std::unique_ptr<ChangedArea> changed_area_;

    geometry::planar::IntBox bounding_box_;
    Communication* board_communication_ = nullptr;
    std::unique_ptr<Communication> owned_communication_;
    /** Kept sorted by ascending item id; iterate it in reverse for Java's (descending id) order. */
    std::vector<Item*> items_;
    void add_to_item_list(Item* p_item);
    void remove_from_item_list(Item* p_item);
    std::unique_ptr<Components> components_;
    std::unique_ptr<BoardLibrary> library_;
    designforms::specctra::Result parsed_result_;
};

} // namespace freerouting::board