#pragma once

#include <vector>
#include <memory>
#include <optional>
#include <string>

#include "geometry/planar/TileShape.h"
#include "geometry/planar/Polyline.h"
#include "geometry/planar/Point.h"
#include "board/CalcFromSide.h"

namespace freerouting::board
{

class RoutingBoard;
class PolylineTrace;
class Via;
class Item;

/**
 * Auxiliary class used by the shove functions (headless).
 * Mirrors Java ShapeTraceEntries.java (806 lines). Ported: store_items,
 * next_substitute_trace_piece, stack levels, entrance points, cutout_trace
 * and cutout_traces. The GUI/server methods of the Java class are not
 * ported, and the Java search tree entry reuse after a cutout
 * (SearchTreeManager.reuseEntriesAfterCutout) is skipped, because it only
 * saves allocations - the inserted pieces get freshly calculated entries.
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

    // Cuts the traces of p_item_list out of the stored shape. Port of Java
    // ShapeTraceEntries.cutoutTrace().
    static void cutout_trace(PolylineTrace* p_trace,
                             const geometry::planar::TileShape& p_shape,
                             int p_cl_class);

private:
    // Optimized function handling the performance critical standard cutout
    // case. Port of Java ShapeTraceEntries.fastCutoutTrace().
    static void fast_cutout_trace(PolylineTrace* p_trace,
                                  const geometry::planar::Polyline& p_start_piece,
                                  const geometry::planar::Polyline& p_end_piece);

    // Unlinks p_trace from the item list and from the search tree.
    static void remove_trace_from_board(RoutingBoard* p_board, PolylineTrace* p_trace);

    // Compares two polylines line by line (Java compares the cutout pieces
    // with the input polyline by object identity).
    static bool polyline_equals(const geometry::planar::Polyline& p_first,
                                const geometry::planar::Polyline& p_second);

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
    // Rotates the entry list, so that p_new_anchor becomes the list anchor
    // and the edge numbers of the entries before it are shifted by
    // p_edge_count. Port of Java rotateEntryListAroundAnchor().
    void rotate_entry_list_around_anchor(EntryPoint* p_new_anchor, int p_edge_count);
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
    std::optional<CalcFromSide> from_side_;
    EntryPoint* list_anchor_ = nullptr;
    std::vector<std::unique_ptr<EntryPoint>> owned_entries_;
    int trace_piece_count_ = 0;
    int max_stack_level_ = 0;
    bool shape_contains_trace_tails_ = false;
    Item* found_obstacle_ = nullptr;

    static constexpr double c_offset_add = 1.0;
};

} // namespace freerouting::board
