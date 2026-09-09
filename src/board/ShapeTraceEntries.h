#pragma once

#include <vector>
#include <memory>
#include <optional>
#include <string>

#include "geometry/planar/TileShape.h"
#include "geometry/planar/Polyline.h"
#include "geometry/planar/Point.h"

namespace freerouting::board
{

class RoutingBoard;
class PolylineTrace;
class Via;
class Item;
class CalcFromSide;

/**
 * Auxiliary class used by the shove functions (headless).
 * Mirrors Java ShapeTraceEntries.java (726 lines). GUI/server methods
 * (cutout_trace, cutout_traces with board.item_list reuse) are stubbed for
 * headless; the core shove bookkeeping (store_items, next_substitute_trace_piece,
 * stack levels, entrance points) is fully ported.
 */
class ShapeTraceEntries
{
public:
    // Headless constructor – used for shoving traces/vias out of p_shape.
    ShapeTraceEntries(const geometry::planar::TileShape& p_shape, int p_layer,
                      const std::vector<int>& p_own_net_nos, int p_cl_type,
                      const CalcFromSide* p_from_side, RoutingBoard* p_board);

    // Via list that must be shoved (cleared by store_items)
    std::vector<Via*> shove_via_list;

    // Returns false if p_item_list contains unshovable obstacles.
    bool store_items(const std::vector<Item*>& p_item_list, bool p_is_pad_check,
                     bool p_copper_sharing_allowed);

    // Returns next substitute trace piece or nullptr at end.
    PolylineTrace* next_substitute_trace_piece();

    int stack_depth() const { return max_stack_level_; }
    int substitute_trace_count() const { return trace_piece_count_; }
    bool trace_tails_in_shape() const { return shape_contains_trace_tails_; }
    Item* get_found_obstacle() const { return found_obstacle_; }

    void cutout_traces(const std::vector<Item*>& p_item_list);

    // Static helper used by board trace cutout (headless stub – warns if called)
    static void cutout_trace(PolylineTrace* p_trace,
                             const geometry::planar::TileShape& p_shape,
                             int p_cl_class);

private:
    struct EntryPoint
    {
        PolylineTrace* trace = nullptr;
        int trace_line_no = 0;
        int edge_no = 0;
        geometry::planar::FloatPoint entry_approx;
        int stack_level = -1;
        EntryPoint* next = nullptr;
    };

    bool store_trace(PolylineTrace* p_trace);
    void search_from_side();
    void resort();
    bool calculate_stack_levels();
    EntryPoint* pop_piece_internal();
    void insert_entry_point(PolylineTrace* p_trace, int p_trace_line_no, int p_edge_no,
                            const geometry::planar::FloatPoint& p_entry_approx);
    static bool net_nos_equal(const std::vector<int>& a, const std::vector<int>& b);

    const geometry::planar::TileShape* shape_ = nullptr;
    std::unique_ptr<geometry::planar::TileShape> shape_owned_;
    int layer_ = 0;
    std::vector<int> own_net_nos_;
    int cl_class_ = 0;
    RoutingBoard* board_ = nullptr;
    std::unique_ptr<CalcFromSide> from_side_;
    EntryPoint* list_anchor_ = nullptr;
    std::vector<std::unique_ptr<EntryPoint>> owned_entries_;
    int trace_piece_count_ = 0;
    int max_stack_level_ = 0;
    bool shape_contains_trace_tails_ = false;
    Item* found_obstacle_ = nullptr;

    static constexpr double c_offset_add = 1.0;
};

} // namespace freerouting::board
