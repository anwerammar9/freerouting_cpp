#pragma once

#include "autoroute/AutorouteControl.h"
#include "autoroute/ExpandableObject.h"
#include "autoroute/MazeSearchAlgo.h"
#include "board/AngleRestriction.h"
#include "geometry/planar/Point.h"
#include "geometry/planar/TileShape.h"

#include <map>
#include <memory>
#include <optional>
#include <set>
#include <vector>

namespace freerouting {
namespace board {
class Item;
class ShapeSearchTree;
} // namespace board

namespace autoroute {

class CompleteExpansionRoom;
class TargetItemExpansionDoor;

/**
 * Locates and constructs trace connection geometries from maze search
 * backtrack paths. Port of Java
 * app.freerouting.autoroute.path.FoundConnectionLocator.
 *
 * Divergence: Java checks `autorouteResult.connectionItems == null` in
 * AutorouteEngine.autorouteConnection to detect an unusable locator. The C++
 * port signals the same condition with FoundConnectionLocator::is_valid(),
 * because the result list is a value-typed std::vector.
 */
class FoundConnectionLocator
{
public:
    /**
     * Type of a single item in the result list connection_items. Used to
     * create a new PolylineTrace. Port of Java FoundConnectionLocator.ResultItem.
     */
    struct ResultItem
    {
        std::vector<geometry::planar::IntPoint> corners;
        int layer = 0;
    };

    /**
     * Type of the elements of the list returned by backtrack(). next_room is
     * the common room of the current door and the next door in the backtrack
     * list. Port of Java FoundConnectionLocator.BacktrackElement.
     */
    struct BacktrackElement
    {
        ExpandableObject* door = nullptr;
        int section_no_of_door = 0;
        CompleteExpansionRoom* next_room = nullptr;
    };

    virtual ~FoundConnectionLocator() = default;

    /** The new items implementing the found connection. */
    std::vector<ResultItem> connection_items;

    /** The start item of the new routed connection. */
    board::Item* start_item = nullptr;

    /** The layer of the connection to the start item. */
    int start_layer = 0;

    /** The destination item of the new routed connection. */
    board::Item* target_item = nullptr;

    /** The layer of the connection to the target item. */
    int target_layer = 0;

    /** The backtrack doors from the destination to the start of the found connection. */
    const std::vector<BacktrackElement>& backtrack_array() const { return backtrack_array_; }

    /**
     * Returns false, if the maze search result could not be turned into a
     * connection geometry. This is the C++ counterpart of the Java
     * `autorouteResult == null` / `connectionItems == null` checks of
     * AutorouteEngine.autorouteConnection().
     */
    bool is_valid() const { return valid_; }

    /**
     * Returns a new instance of FoundConnectionLocator, or nullptr, if
     * p_maze_result is null. Port of Java FoundConnectionLocator.getInstance().
     */
    static std::unique_ptr<FoundConnectionLocator> get_instance(
        const MazeSearchAlgo::Result* p_maze_result,
        const AutorouteControl& p_ctrl,
        board::ShapeSearchTree* p_search_tree,
        board::AngleRestriction p_angle_restriction,
        std::set<board::Item*>* p_ripped_item_list = nullptr,
        std::map<board::Item*, int>* p_ripup_costs = nullptr);

    /**
     * Calculates an additional corner, so that for the lines from p_from_point
     * to the result corner and from the result corner to p_to_point
     * p_angle_restriction is fulfilled. Port of Java
     * FoundConnectionLocator.calculateAdditionalCorner().
     */
    static geometry::planar::FloatPoint calculate_additional_corner(
        const geometry::planar::FloatPoint& p_from_point,
        const geometry::planar::FloatPoint& p_to_point,
        bool p_horizontal_first,
        board::AngleRestriction p_angle_restriction);

protected:
    FoundConnectionLocator() = default;

    /**
     * Creates the backtrack array and the connection items of this locator.
     * Port of the Java FoundConnectionLocator constructor.
     *
     * Divergence: Java calls the overridable calculateNextTrace() from its
     * constructor, which is undefined behaviour in C++. The construction is
     * therefore split: get_instance() first creates the concrete subclass and
     * then calls this initialisation.
     *
     * @return false, if the backtrack path or the destination door has an
     *         unexpected type. The Java code raises an exception in this case,
     *         which the caller of getInstance() catches and maps to a null
     *         result.
     */
    bool initialize(const MazeSearchAlgo::Result& p_maze_result,
                    const AutorouteControl& p_ctrl,
                    board::ShapeSearchTree* p_search_tree,
                    board::AngleRestriction p_angle_restriction,
                    std::set<board::Item*>* p_ripped_item_list,
                    std::map<board::Item*, int>* p_ripup_costs);

    /**
     * Returns the next list of corners for the construction of the trace in
     * calculate_next_trace(). If the result is empty, the trace is already
     * completed. Port of Java
     * FoundConnectionLocator.calculateNextTraceCorners().
     */
    virtual std::vector<geometry::planar::FloatPoint> calculate_next_trace_corners() = 0;

    /**
     * Calculates the next trace of the connection under construction.
     * Port of Java FoundConnectionLocator.calculateNextTrace().
     */
    ResultItem calculate_next_trace(bool p_layer_changed, bool p_at_fanout_end);

    /**
     * Adjusts the start corner, so that a trace starting at this corner is
     * completely contained in the start room. Port of Java
     * FoundConnectionLocator.adjustStartCorner().
     */
    geometry::planar::FloatPoint adjust_start_corner() const;

    /**
     * Creates a list of doors by backtracking from the destination door to the
     * start door. Port of Java FoundConnectionLocator.backtrack().
     */
    static std::vector<BacktrackElement> backtrack(const MazeSearchAlgo::Result& p_maze_result,
                                                    int p_net_no,
                                                    std::set<board::Item*>* p_ripped_item_list,
                                                    std::map<board::Item*, int>* p_ripup_costs);

    /**
     * Calculates the starting point of the next trace on from_door.item.
     * Port of Java FoundConnectionLocator.calculateStartingPoint().
     */
    static std::optional<geometry::planar::FloatPoint> calculate_starting_point(
        TargetItemExpansionDoor* p_from_door, board::ShapeSearchTree* p_search_tree);

    /**
     * Returns the trace connection shape of p_item for the p_index-th tree shape
     * in the autoroute search tree.
     *
     * Divergence: Java casts the item to the Connectable interface and calls
     * Connectable.getTraceConnectionShape(). The C++ Connectable interface is a
     * dead draft implemented by no real item, so the item tree shape is used as
     * the connection shape, exactly like in the already ported
     * CompleteFreeSpaceExpansionRoom::calculate_target_doors() and
     * MazeSearchAlgo::get_trace_connection_shape().
     */
    static const geometry::planar::TileShape* get_trace_connection_shape(board::Item* p_item,
                                                                          int p_index,
                                                                          board::ShapeSearchTree* p_search_tree);

    /** C++ analogue of the Java `instanceof Connectable` guard. */
    static bool is_connectable(const board::Item* p_item);

    /**
     * Bounds-checked access to the per-layer compensated trace half width of the
     * control. Mirrors MazeSearchAlgo::compensated_trace_half_width_for_layer().
     */
    double compensated_trace_half_width(int p_layer) const;

    /** The array of backtrack doors of this locator. */
    std::vector<BacktrackElement> backtrack_array_;

    const AutorouteControl* ctrl_ = nullptr;
    board::AngleRestriction angle_restriction_ = board::AngleRestriction::NONE;
    TargetItemExpansionDoor* start_door_ = nullptr;
    geometry::planar::FloatPoint current_from_point_;
    geometry::planar::FloatPoint previous_from_point_;
    int current_trace_layer_ = 0;
    int current_from_door_index_ = 0;
    int current_to_door_index_ = 0;
    int current_target_door_index_ = 0;
    std::unique_ptr<geometry::planar::TileShape> current_target_shape_;

    bool valid_ = false;
};

} // namespace autoroute
} // namespace freerouting