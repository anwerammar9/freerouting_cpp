#ifndef AUTOROUTE_MAZE_SEARCH_ALGO_H
#define AUTOROUTE_MAZE_SEARCH_ALGO_H

#include "autoroute/AutorouteControl.h"
#include "autoroute/DestinationDistance.h"
#include "autoroute/ExpandableObject.h"
#include "autoroute/ExpansionCostFactor.h"
#include "autoroute/ExpansionRoom.h"
#include "autoroute/MazeRandom.h"
#include "autoroute/MazeSearchElement.h"
#include "geometry/planar/FloatLine.h"
#include "geometry/planar/Point.h"

#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace freerouting::board {
class Item;
class RoutingBoard;
class ShapeSearchTree;
} // namespace freerouting::board

namespace freerouting::datastructures {
class Stoppable;
} // namespace freerouting::datastructures

namespace freerouting::autoroute {

class AutorouteEngine;
class DrillPage;
class ExpansionDrill;
class MazeExpansionEngine;
class MazeFanoutDiagnostics;
class MazeRipupResolver;
class ObstacleExpansionRoom;
class TargetItemExpansionDoor;

/**
 * Class for auto-routing an incomplete connection via a maze search
 * algorithm. Port of Java app.freerouting.autoroute.maze.MazeSearchEngine.
 */
class MazeSearchAlgo
{
public:
    /** The ripup cost of a room which is already ripped. Java ALREADY_RIPPED_COSTS. */
    static constexpr int ALREADY_RIPPED_COSTS = 1;

    /** The result type of MazeSearchAlgo::find_connection. Port of Java MazeSearchEngine.Result. */
    struct Result
    {
        ExpandableObject* destination_door = nullptr;
        int section_no_of_door = 0;
    };

    MazeSearchAlgo(board::RoutingBoard* p_board, datastructures::Stoppable* p_stoppable);
    ~MazeSearchAlgo();

    static std::unique_ptr<MazeSearchAlgo> get_instance(board::RoutingBoard* p_board,
                                                         datastructures::Stoppable* p_stoppable);

    /**
     * Creates a new instance of MazeSearchAlgo for searching a connection
     * between p_start_items and p_dest_items. Returns nullptr, if the
     * initialisation failed. Port of Java MazeSearchEngine.getInstance().
     */
    static std::unique_ptr<MazeSearchAlgo> get_instance(
        const std::set<board::Item*>& p_start_items,
        const std::set<board::Item*>& p_dest_items,
        AutorouteEngine* p_autoroute_engine,
        const AutorouteControl& p_ctrl,
        datastructures::Stoppable* p_stoppable = nullptr);

    /**
     * Initialises the maze search for a connection between p_start_items and
     * p_dest_items. Returns false if the initialisation failed. Port of Java
     * MazeSearchEngine.init().
     */
    bool init(const std::set<board::Item*>& p_start_items,
              const std::set<board::Item*>& p_dest_items);

    /**
     * Does a maze search to find a connection route between the start and the
     * destination items. If the algorithm succeeds, the expandable object and
     * its section number of the found destination is returned, from where the
     * whole found connection can be backtracked. Otherwise the return value is
     * a Result with a null destination door. Port of Java
     * MazeSearchEngine.findConnection().
     */
    Result find_connection();

    /**
     * Expands the next element in the maze expansion list. Returns false, if
     * the expansion list is exhausted or the destination is reached. Port of
     * Java MazeSearchEngine.occupyNextElement().
     */
    bool occupy_next_element();

    /**
     * Backtracks the found connection from p_result to the start door and
     * returns the doors/drills of the path in the order in which they are
     * traversed.
     */
    std::vector<ExpandableObject*> reconstruct_path(const Result& p_result) const;

    void set_net_no(int p_net_no) { control_.net_no = p_net_no; }

    /**
     * Sets the control of this search. Mirrors the Java constructor, which
     * seeds the ripup random generator from the ripup costs of the control, so
     * that the randomization stays deterministic across passes.
     */
    void set_control(const AutorouteControl& p_ctrl);

    const AutorouteControl& get_control() const { return control_; }

    /** The search tree compensated for the current net, used for obstacle lookups. */
    board::ShapeSearchTree* search_tree() const { return search_tree_; }

    /** The ripup randomizer, seeded with the ripup costs of the control. */
    MazeRandom& random_generator() { return random_generator_; }

    /** The ripup resolver of this search. */
    MazeRipupResolver* ripup_resolver() { return ripup_resolver_.get(); }
    const MazeRipupResolver* ripup_resolver() const { return ripup_resolver_.get(); }

    bool is_initialized() const { return initialized_; }

    /** The queue of expanded elements used in this search algorithm. */
    const std::set<MazeListElement>& maze_expansion_list() const { return maze_expansion_list_; }

    /**
     * Returns true, if p_element may be added to the maze expansion list of a
     * fanout pass. Port of the filter of the anonymous TreeSet of Java
     * MazeSearchEngine, which rejects expansion elements which are too far
     * away from the fanout pin on the start layer, and drills which are too
     * close to the fanout pin. Outside of a fanout pass every element is
     * allowed.
     *
     * Java implements this filter inside an anonymous TreeSet subclass, so it
     * is not accessible from outside there. The C++ port exposes it, because
     * it is the only place where the fanout limits are evaluated.
     */
    bool passes_fanout_filter(const MazeListElement& p_element) const;

    /**
     * Returns the targeted fanout diagnostics of this search. Mirrors Java
     * MazeSearchEngine.fanoutDiagnostics, which holds the control of the search.
     */
    MazeFanoutDiagnostics& fanout_diagnostics() { return *fanout_diagnostics_; }
    const MazeFanoutDiagnostics& fanout_diagnostics() const { return *fanout_diagnostics_; }

private:
    /** The resolver of the ripup costs, used by the maze expansion. */
    friend class MazeRipupResolver;

    /**
     * The expansion engine inserts its new elements into the maze expansion
     * list of this search and reads its control and destination distance.
     */
    friend class MazeExpansionEngine;

    /**
     * The expansion engine of this search, which adds the drill, drill page
     * and other layer expansions to the maze expansion list. Mirrors Java
     * MazeSearchEngine.expansionEngine.
     */
    std::unique_ptr<MazeExpansionEngine> expansion_engine_;

    board::RoutingBoard* board_ = nullptr;
    datastructures::Stoppable* stoppable_ = nullptr;
    AutorouteControl control_;

    /**
     * Targeted diagnostics of a fanout expansion. Mirrors Java
     * MazeSearchEngine.fanoutDiagnostics, which holds a reference to the
     * control of the search. Therefore it is recreated whenever the control of
     * this search is replaced. Declared after control_, so that the
     * diagnostics are destroyed before the control they refer to.
     */
    std::unique_ptr<MazeFanoutDiagnostics> fanout_diagnostics_;
    bool initialized_ = false;

    /**
     * The randomizer of the ripup costs. Mirrors the Java
     * MazeSearchEngine.randomGenerator, which is seeded with the ripup costs
     * of the control to keep the randomization deterministic across passes.
     */
    MazeRandom random_generator_;

    /** Resolves whether an obstacle room may be ripped and its ripup cost. */
    std::unique_ptr<MazeRipupResolver> ripup_resolver_;

    /**
     * The queue of expanded elements used in this search algorithm. Java uses
     * a TreeSet, so the C++ port uses a std::set with the Java
     * compareTo() ordering: the element with the lowest sorting value comes
     * first and comparing equal elements are the same list entry.
     */
    std::set<MazeListElement> maze_expansion_list_;

    /**
     * Used for calculating a good lower bound for the distance between a new
     * MazeListElement and the destination set of the expansion.
     */
    std::unique_ptr<DestinationDistance> destination_distance_;

    /** The search tree for expanding. It is the tree compensated for the current net. */
    board::ShapeSearchTree* search_tree_ = nullptr;

    /** The autoroute engine owning the expansion room database. */
    AutorouteEngine* autoroute_engine_ = nullptr;

    /**
     * Returns the expansion engine of this search, so that the maze search and
     * its ripup resolver can seed it with their accessors.
     */
    MazeExpansionEngine* expansion_engine() { return expansion_engine_.get(); }

    /**
     * Returns the control of this search by non const reference, which the
     * expansion engine needs to read the via info. Mirrors the Java field
     * access on the final search class.
     */
    AutorouteControl& control() { return control_; }

    /** Returns the autoroute engine owning the expansion room database. */
    AutorouteEngine* autoroute_engine() { return autoroute_engine_; }

    /** Returns the destination distance of this search. */
    DestinationDistance* destination_distance() { return destination_distance_.get(); }

    /** Returns the obstacle expansion room of the item, or nullptr. */
    ObstacleExpansionRoom* obstacle_room_of_item(const board::Item* p_item) const;

    /** Adds p_element to the maze expansion list of this search. */
    void add_to_maze_expansion_list(const MazeListElement& p_element)
    {
        if (!passes_fanout_filter(p_element))
        {
            return;
        }
        maze_expansion_list_.insert(p_element);
    }

    std::set<board::Item*> start_items_;
    std::set<board::Item*> dest_items_;

    /** The destination door found by the expanding algorithm. */
    ExpandableObject* destination_door_ = nullptr;
    int section_no_of_destination_door_ = 0;

    bool is_stop_requested() const;

    /**
     * Bounds-checked accessors for the per-layer cost arrays of the control.
     * The C++ AutorouteControl can be left uninitialised (empty arrays), while
     * the Java code would raise an IndexOutOfBoundsException. A missing entry
     * is reported once and the neutral value is returned, so that an
     * incompletely configured control cannot corrupt the search.
     */
    bool is_layer_active(int p_layer) const;
    double compensated_trace_half_width_for_layer(int p_layer) const;
    double bend_cost_for_layer(int p_layer) const;
    const ExpansionCostFactor& trace_costs_for_layer(int p_layer) const;

    /**
     * Expands the doors of the next room of p_list_element. Returns true, if
     * the from door section has to be occupied, and false, if the occupation
     * is delayed. Port of Java MazeSearchEngine.expandToRoomDoors().
     */
    bool expand_to_room_doors(const MazeListElement& p_list_element);

    /**
     * Expands the target doors of the room. Returns true, if at least 1 target
     * door was expanded. Port of Java MazeSearchEngine.expandToTargetDoors().
     */
    bool expand_to_target_doors(const MazeListElement& p_list_element,
                                bool p_next_room_is_thick,
                                bool p_current_door_is_small,
                                const geometry::planar::FloatPoint& p_shape_entry_middle);

    /**
     * Expands the sections of p_to_door. Returns true, if at least 1 door
     * section was expanded. Port of Java MazeSearchEngine.expandToDoor().
     */
    bool expand_to_door(ExpansionDoor* p_to_door,
                        const MazeListElement& p_list_element,
                        int p_add_costs,
                        bool p_next_room_is_thick,
                        MazeSearchElement::Adjustment p_adjustment);

    /**
     * Expands a single door section. Returns true, if the door section was
     * successfully expanded. Port of Java
     * MazeSearchEngine.expandToDoorSection().
     */
    bool expand_to_door_section(ExpandableObject* p_door,
                                int p_section_index,
                                const geometry::planar::FloatLine& p_shape_entry,
                                const MazeListElement& p_from_element,
                                int p_add_costs,
                                MazeSearchElement::Adjustment p_adjustment);

    /**
     * Checks, if the width of p_door is big enough for a trace with width
     * p_trace_width. Port of Java MazeSearchEngine.doorIsSmall().
     */
    bool door_is_small(ExpansionDoor* p_door, double p_trace_width) const;

    /**
     * Returns true, if the obstacle room is wide enough for a trace. Port of
     * Java MazeSearchEngine.roomShapeIsThick().
     */
    bool room_shape_is_thick(ObstacleExpansionRoom* p_obstacle_room) const;

    /**
     * Shoves a trace room and expands the corresponding doors. Returns false,
     * if no door was expanded. In this case the occupation of the door section
     * by ripup can be delayed to allow shoving the room from a different door
     * section. Port of Java MazeSearchEngine.shoveTraceRoom().
     */
    bool shove_trace_room(const MazeListElement& p_list_element,
                          ObstacleExpansionRoom* p_obstacle_room);

    /**
     * Checks, if the next room contains a destination pin, where evtl.
     * neckdown is necessary. Returns the neck down width in this case, or 0,
     * if no such pin was found. Port of Java
     * MazeSearchEngine.checkNeckDownAtDestPin().
     */
    int check_neck_down_at_dest_pin(CompleteExpansionRoom* p_room) const;

    /**
     * Returns the perpendicular projection of p_from_segment onto
     * p_to_segment, or nothing if the projection is empty. Port of Java
     * MazeSearchEngine.segmentProjection().
     */
    static std::optional<geometry::planar::FloatLine> segment_projection(
        const geometry::planar::FloatLine& p_from_segment,
        const geometry::planar::FloatLine& p_to_segment);

    /**
     * Returns the trace connection shape of p_item for the p_index-th tree
     * shape in the autoroute search tree.
     *
     * Divergence: Java casts the item to the Connectable interface and calls
     * Connectable.getTraceConnectionShape(). The C++ Connectable interface is
     * a dead draft implemented by no real item, so the item tree shape is
     * used as the connection shape, exactly like in the already ported
     * CompleteFreeSpaceExpansionRoom::calculate_target_doors().
     */
    const geometry::planar::TileShape* get_trace_connection_shape(board::Item* p_item, int p_index) const;

    /** C++ analogue of the Java `instanceof Connectable` guard. */
    static bool is_connectable(const board::Item* p_item);

public:
    /** Returns a diagnostic description of an expandable object of the search. */
    static std::string describe_expandable(const ExpandableObject* p_door);

    /** Returns a diagnostic description of an expansion room of the search. */
    static std::string describe_room(const CompleteExpansionRoom* p_room);
};

} // namespace freerouting::autoroute

#endif // AUTOROUTE_MAZE_SEARCH_ALGO_H
