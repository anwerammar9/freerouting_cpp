#include "autoroute/AutorouteEngine.h"
#include "autoroute/MazeSearchAlgo.h"
#include "autoroute/LocateFoundConnectionAlgo.h"
#include "autoroute/FoundConnectionInserter.h"
#include "autoroute/DrillPage.h"
#include "autoroute/DrillPageArray.h"
#include "autoroute/ExpansionRoom.h"
#include "autoroute/SortedRoomNeighbours.h"
#include "board/Padstack.h"
#include "board/RoutingBoard.h"
#include "board/PolylineTrace.h"
#include "board/ShapeSearchTree.h"
#include "geometry/planar/Shape.h"
#include "rules/BoardRules.h"
#include "rules/ClearanceMatrix.h"
#include "rules/ViaInfo.h"
#include "rules/ViaRule.h"
#include "geometry/planar/IntOctagon.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/TileShape.h"
#include "datastructures/Stoppable.h"
#include "datastructures/TimeLimit.h"
#include "logger/FRLogger.h"

#include <algorithm>
#include <cmath>
#include <queue>
#include <set>
#include <map>
#include <limits>

namespace freerouting::autoroute {

AutorouteEngine::AutorouteEngine(board::RoutingBoard* p_board, int p_trace_clearance_class_no, bool p_maintain_database)
    : board_(p_board)
    , maintain_database_(p_maintain_database)
    , trace_clearance_class_no_(p_trace_clearance_class_no)
{
    // Search tree is built lazily on first use
    if (board_ != nullptr)
    {
        // Java: maxDrillPageWidth = max(5 * board.rules.getDefaultViaDiameter(), 10000).
        // The diameter of the default via is derived from the default via rule
        // of the board rules, which is 0 as long as the DSN via rules are not
        // parsed, so that the Java lower bound of 10000 applies.
        double default_via_diameter = 0;
        const rules::ViaRule* default_via_rule = board_->get_rules().get_default_via_rule();
        if (default_via_rule != nullptr && default_via_rule->via_count() > 0)
        {
            auto first_via = default_via_rule->get_via(0);
            std::shared_ptr<board::Padstack> via_padstack =
                first_via != nullptr ? first_via->get_padstack() : nullptr;
            if (via_padstack != nullptr)
            {
                const geometry::planar::Shape* from_shape =
                    via_padstack->get_shape(via_padstack->get_from_layer());
                const geometry::planar::Shape* to_shape =
                    via_padstack->get_shape(via_padstack->get_to_layer());
                if (from_shape != nullptr && to_shape != nullptr)
                {
                    default_via_diameter = std::max(from_shape->bounding_box().max_width(),
                        to_shape->bounding_box().max_width());
                }
            }
        }
        int max_drill_page_width = static_cast<int>(5 * default_via_diameter);
        max_drill_page_width = std::max(max_drill_page_width, 10000);
        drill_page_array_ = std::make_unique<DrillPageArray>(board_, max_drill_page_width);
    }
}

AutorouteEngine::~AutorouteEngine() = default;

void AutorouteEngine::ensure_search_tree_initialized()
{
    if (autoroute_search_tree_)
    {
        return; // Already initialized
    }
    
    if (board_ && trace_clearance_class_no_ > 0)
    {
        // The tree is built without clearance compensation; clearance is
        // checked explicitly via overlapping_objects_with_clearance.
        autoroute_search_tree_ = std::make_unique<board::ShapeSearchTree>(board_, 0);
        
        logger::FRLogger::instance().debug("Created autoroute search tree (clearance class " + 
            std::to_string(trace_clearance_class_no_) + ")");
        
        auto items = board_->get_autoroute_items();
        for (int i = 0; i < static_cast<int>(items.size()); ++i)
        {
            autoroute_search_tree_->insert(items[i]);
        }
        logger::FRLogger::instance().debug("Inserted " + std::to_string(items.size()) + " items into search tree");
    }
}

void AutorouteEngine::set_time_limit(int64_t p_time_ms)
{
    if (p_time_ms > 0)
    {
        time_limit_ = std::make_unique<datastructures::TimeLimit>(static_cast<int>(p_time_ms));
    }
    else
    {
        time_limit_.reset();
    }
}

void AutorouteEngine::reset()
{
    expansion_room_instance_count_ = 0;
    incomplete_expansion_rooms_.clear();
    complete_expansion_rooms_.clear();
    if (drill_page_array_)
    {
        drill_page_array_->reset();
    }
}

void AutorouteEngine::invalidate_drill_pages(const geometry::planar::IntBox& p_shape)
{
    if (drill_page_array_ == nullptr)
    {
        return;
    }
    drill_page_array_->invalidate(p_shape);
}

bool AutorouteEngine::is_stop_requested() const
{
    if (time_limit_ && time_limit_->limit_exceeded())
    {
        return true;
    }
    if (stoppable_)
    {
        return stoppable_->is_stop_requested();
    }
    return false;
}

void AutorouteEngine::init_connection(int p_net_no, datastructures::Stoppable* p_stoppable, int64_t p_time_limit_ms)
{
    if (maintain_database_ && p_net_no != net_no_)
    {
        // When the autorouting database is retained across nets, rooms which
        // overlap net dependent objects cannot be reused for the new net.
        // Mirrors Java AutorouteEngine.initConnection().
        remove_net_dependent_rooms();
    }
    net_no_ = p_net_no;
    stoppable_ = p_stoppable;
    if (p_time_limit_ms > 0)
    {
        time_limit_ = std::make_unique<datastructures::TimeLimit>(static_cast<int>(p_time_limit_ms));
    }
    else
    {
        time_limit_.reset();
    }
}

bool AutorouteEngine::route_all(int p_iteration_limit, datastructures::Stoppable* p_stoppable)
{
    stoppable_ = p_stoppable;

    if (board_ == nullptr)
    {
        logger::FRLogger::instance().error("AutorouteEngine: No board set");
        return false;
    }

    logger::FRLogger::instance().info("AutorouteEngine: Starting routing for net " + std::to_string(net_no_));
    return true;
}

AutorouteAttemptResult AutorouteEngine::autoroute_connection(const std::set<board::Item*>& p_start_set,
                                           const std::set<board::Item*>& p_dest_set,
                                           const AutorouteControl& p_ctrl)
{
    AutorouteAttemptResult result;
    result.state = AutorouteAttemptState::NOT_ROUTED;

    if (board_ == nullptr)
    {
        result.details = "No board set";
        return result;
    }

    if (p_start_set.empty() || p_dest_set.empty())
    {
        result.details = "Empty start or destination set";
        return result;
    }

    auto* start_item = *p_start_set.begin();
    auto* dest_item = *p_dest_set.begin();

    if (!start_item || !dest_item)
    {
        result.details = "Invalid start or destination";
        return result;
    }

    if (start_item == dest_item)
    {
        result.state = AutorouteAttemptState::ROUTED;
        result.details = "Already connected";
        return result;
    }

    auto start_bbox = start_item->get_bounding_box();
    auto dest_bbox = dest_item->get_bounding_box();

    if (start_bbox.is_empty() || dest_bbox.is_empty())
    {
        result.details = "Invalid start or destination bounding box";
        return result;
    }

    const int net_no = p_ctrl.net_no > 0 ? p_ctrl.net_no : net_no_;
    const int layer = start_item->get_layer();
    if (dest_item->get_layer() != layer)
    {
        logger::FRLogger::instance().warn(
            "AutorouteEngine: layer change not supported yet, routing on layer " + std::to_string(layer));
    }
    const int half_width = p_ctrl.get_trace_half_width(layer);
    if (half_width <= 0)
    {
        result.details = "Invalid trace half width";
        return result;
    }

    auto center_of = [](const geometry::planar::IntBox& p_bbox)
    {
        return geometry::planar::IntPoint((p_bbox.ll.x + p_bbox.ur.x) / 2, (p_bbox.ll.y + p_bbox.ur.y) / 2);
    };
    const geometry::planar::IntPoint start_pt = center_of(start_bbox);
    const geometry::planar::IntPoint dest_pt = center_of(dest_bbox);

    if (start_pt == dest_pt)
    {
        result.state = AutorouteAttemptState::ROUTED;
        result.details = "Start and destination are at the same location";
        return result;
    }

    ensure_search_tree_initialized();
    if (!autoroute_search_tree_)
    {
        result.details = "Search tree not available";
        return result;
    }

    // ------------------------------------------------------------------
    // Grid setup over the board bounding box.
    // ------------------------------------------------------------------
    namespace fp = freerouting::geometry::planar;
    const fp::IntBox region = board_->get_bounding_box();
    const int64_t max_cells = 3000000;
    const int64_t min_cell = 64;
    int64_t cell = std::max<int64_t>(min_cell, half_width * 2);
    int64_t cols = std::max<int64_t>(1, (region.width() + cell - 1) / cell);
    int64_t rows = std::max<int64_t>(1, (region.height() + cell - 1) / cell);
    while (cols * rows > max_cells && cell < 200000)
    {
        cell *= 2;
        cols = std::max<int64_t>(1, (region.width() + cell - 1) / cell);
        rows = std::max<int64_t>(1, (region.height() + cell - 1) / cell);
    }
    if (cols * rows > 4 * max_cells)
    {
        result.details = "Routing region too large for maze router";
        return result;
    }

    auto clamp_to_grid = [&](const fp::IntPoint& p_pos)
    {
        int64_t gx = (p_pos.x - region.ll.x) / cell;
        int64_t gy = (p_pos.y - region.ll.y) / cell;
        return std::pair<int64_t, int64_t>{
            std::clamp<int64_t>(gx, 0, cols - 1),
            std::clamp<int64_t>(gy, 0, rows - 1)};
    };
    auto cell_center = [&](int64_t p_gx, int64_t p_gy)
    {
        return fp::IntPoint(region.ll.x + p_gx * cell + cell / 2, region.ll.y + p_gy * cell + cell / 2);
    };
    auto to_index = [&](int64_t p_gx, int64_t p_gy) { return static_cast<size_t>(p_gy * cols + p_gx); };

    // ------------------------------------------------------------------
    // Try expansion-room maze path (MazeSearchAlgo + FoundConnectionLocator + FoundConnectionInserter)
    // ------------------------------------------------------------------
    {
        auto maze_algo = MazeSearchAlgo::get_instance(p_start_set, p_dest_set, this, p_ctrl, stoppable_);
        if (maze_algo)
        {
            MazeSearchAlgo::Result maze_res = maze_algo->find_connection();
            if (maze_res.destination_door != nullptr)
            {
                // Determine angle restriction
                rules::AngleRestriction rangle = rules::AngleRestriction::FORTY_FIVE;
                if (board_ != nullptr)
                {
                    rangle = board_->get_rules().get_trace_angle_restriction();
                }
                board::AngleRestriction bangle = board::AngleRestriction::FORTYFIVE_DEGREE;
                if (rangle == rules::AngleRestriction::RIGHT)
                {
                    bangle = board::AngleRestriction::NINETY_DEGREE;
                }
                else if (rangle == rules::AngleRestriction::FORTY_FIVE)
                {
                    bangle = board::AngleRestriction::FORTYFIVE_DEGREE;
                }
                else
                {
                    bangle = board::AngleRestriction::NONE;
                }

                auto locator = FoundConnectionLocator::get_instance(&maze_res, p_ctrl, autoroute_search_tree_.get(),
                                                                    bangle, nullptr, nullptr);
                if (locator && locator->is_valid())
                {
                    auto inserter = FoundConnectionInserter::get_instance(locator.get(), board_, p_ctrl);
                    if (inserter)
                    {
                        result.state = AutorouteAttemptState::ROUTED;
                        result.details = "Maze expansion route inserted";
                        return result;
                    }
                }
            }
        }
    }

    const auto [start_col, start_row] = clamp_to_grid(start_pt);
    const auto [dest_col, dest_row] = clamp_to_grid(dest_pt);
    const size_t start_idx = to_index(start_col, start_row);
    const size_t dest_idx = to_index(dest_col, dest_row);
    const size_t cell_count = static_cast<size_t>(cols * rows);

    // ------------------------------------------------------------------
    // Blocked cells: any cell whose area inflated by the trace half width
    // and half clearance collides with an obstacle of another net.
    // ------------------------------------------------------------------
    std::vector<bool> blocked(cell_count, false);
    const std::vector<int> ignore_nets{net_no};
    const int clearance_class = p_ctrl.trace_clearance_class_no > 0 ? p_ctrl.trace_clearance_class_no : 1;
    const int clearance = board_->get_rules().get_clearance_matrix()
                              .get_value(clearance_class, clearance_class, layer, false);
    const int64_t expand = half_width + std::max<int>(clearance, 0) / 2;

    for (int64_t gy = 0; gy < rows; ++gy)
    {
        for (int64_t gx = 0; gx < cols; ++gx)
        {
            if (gx == start_col && gy == start_row)
            {
                continue;
            }
            if (gx == dest_col && gy == dest_row)
            {
                continue;
            }
            const fp::IntBox cell_box(
                region.ll.x + gx * cell, region.ll.y + gy * cell,
                region.ll.x + gx * cell + cell, region.ll.y + gy * cell + cell);
            const fp::IntOctagon query_shape = cell_box.enlarge(static_cast<double>(expand));
            std::set<board::SearchTreeObject*, board::TreeStorableLess> obstacles;
            autoroute_search_tree_->overlapping_objects_with_clearance(
                query_shape, layer, ignore_nets, clearance_class, obstacles);
            if (!obstacles.empty())
            {
                blocked[to_index(gx, gy)] = true;
            }
        }
    }
    blocked[start_idx] = false;
    blocked[dest_idx] = false;

    // ------------------------------------------------------------------
    // A* search on the 4-neighbour grid (Manhattan heuristic).
    // ------------------------------------------------------------------
    std::vector<int64_t> gcost(cell_count, -1);
    std::vector<int64_t> prev(cell_count, -1);
    using QueueElem = std::pair<int64_t, int64_t>; // (f value, cell index)
    std::priority_queue<QueueElem, std::vector<QueueElem>, std::greater<QueueElem>> open;
    gcost[start_idx] = 0;
    open.emplace(std::abs(dest_col - start_col) + std::abs(dest_row - start_row),
                 static_cast<int64_t>(start_idx));

    const int dir_x[4] = {1, -1, 0, 0};
    const int dir_y[4] = {0, 0, 1, -1};
    bool found = false;
    while (!open.empty())
    {
        const auto [f_value, cur] = open.top();
        (void)f_value;
        open.pop();
        if (cur == static_cast<int64_t>(dest_idx))
        {
            found = true;
            break;
        }
        if (is_stop_requested())
        {
            break;
        }
        const int64_t cur_g = gcost[static_cast<size_t>(cur)];
        const int64_t cur_col = cur % cols;
        const int64_t cur_row = cur / cols;
        for (int d = 0; d < 4; ++d)
        {
            const int64_t nx = cur_col + dir_x[d];
            const int64_t ny = cur_row + dir_y[d];
            if (nx < 0 || nx >= cols || ny < 0 || ny >= rows)
            {
                continue;
            }
            const size_t nidx = to_index(nx, ny);
            if (blocked[nidx])
            {
                continue;
            }
            const int64_t ng = cur_g + 1;
            if (gcost[nidx] != -1 && ng >= gcost[nidx])
            {
                continue;
            }
            gcost[nidx] = ng;
            prev[nidx] = cur;
            open.emplace(ng + std::abs(dest_col - nx) + std::abs(dest_row - ny),
                         static_cast<int64_t>(nidx));
        }
    }

    if (!found)
    {
        result.state = AutorouteAttemptState::FAILED;
        result.details = "No path found";
        return result;
    }

    // ------------------------------------------------------------------
    // Reconstruct the cell path and convert it to trace corner points.
    // ------------------------------------------------------------------
    std::vector<fp::IntPoint> points;
    {
        std::vector<std::pair<int64_t, int64_t>> cells;
        int64_t cur = static_cast<int64_t>(dest_idx);
        while (cur >= 0)
        {
            cells.emplace_back(cur % cols, cur / cols);
            if (cur == static_cast<int64_t>(start_idx))
            {
                break;
            }
            cur = prev[static_cast<size_t>(cur)];
        }
        std::reverse(cells.begin(), cells.end());
        for (const auto& [cx, cy] : cells)
        {
            const fp::IntPoint p = cell_center(cx, cy);
            if (points.empty() || points.back() != p)
            {
                points.push_back(p);
            }
        }

        if (points.size() < 2)
        {
            points = {start_pt, dest_pt};
        }
    }

    if (points.size() >= 2)
    {
        points.front() = start_pt;
        points.back() = dest_pt;
    }

    // Remove collinear intermediate corners.
    std::vector<fp::IntPoint> simplified;
    for (size_t i = 0; i < points.size(); ++i)
    {
        if (i > 0 && i + 1 < points.size())
        {
            const fp::IntPoint& a = points[i - 1];
            const fp::IntPoint& b = points[i];
            const fp::IntPoint& c = points[i + 1];
            if (b.side_of(a, c) == fp::Side::COLLINEAR)
            {
                continue;
            }
        }
        simplified.push_back(points[i]);
    }
    if (simplified.size() < 2)
    {
        simplified = points;
    }

    auto* trace = board_->add_polyline_trace(simplified, layer, half_width, std::vector<int>{net_no});
    if (trace == nullptr)
    {
        result.state = AutorouteAttemptState::FAILED;
        result.details = "Failed to create trace";
        return result;
    }
    if (autoroute_search_tree_)
    {
        autoroute_search_tree_->insert(trace);
    }

    result.state = AutorouteAttemptState::ROUTED;
    result.details = "Maze route found with " + std::to_string(simplified.size()) + " corners";
    return result;
}

int AutorouteEngine::get_net_no() const
{
    return net_no_;
}

void AutorouteEngine::clear()
{
    // Remove the incomplete rooms from the database and the complete rooms
    // from the database and the search tree. Mirrors Java
    // AutorouteEngine.clear().
    if (autoroute_search_tree_)
    {
        for (auto& room : complete_expansion_rooms_)
        {
            room->remove_from_tree(*autoroute_search_tree_);
        }
    }
    incomplete_expansion_rooms_.clear();
    complete_expansion_rooms_.clear();
    expansion_room_instance_count_ = 0;
}

void AutorouteEngine::remove_net_dependent_rooms()
{
    std::vector<CompleteFreeSpaceExpansionRoom*> rooms_to_remove;
    for (auto& room : complete_expansion_rooms_)
    {
        if (room->is_net_dependent())
        {
            rooms_to_remove.push_back(room.get());
        }
    }
    for (CompleteFreeSpaceExpansionRoom* room : rooms_to_remove)
    {
        remove_complete_expansion_room(room);
    }
}

void AutorouteEngine::remove_complete_expansion_room(CompleteFreeSpaceExpansionRoom* p_room)
{
    if (p_room == nullptr)
    {
        return;
    }

    // Mirrors Java AutorouteEngine.removeCompleteExpansionRoom(): create a new
    // incomplete expansion room for every neighbour of p_room, so that the
    // free space of the removed room stays reachable from the neighbour rooms.
    // The door of the removed room is removed from the neighbour side only;
    // p_room keeps its doors until remove_all_doors() below clears them.
    const geometry::planar::Simplex room_shape = p_room->get_shape_tile();
    const int room_layer = p_room->get_layer();
    // The engine owns p_room and destroys it when it is erased from
    // complete_expansion_rooms_ below, so the shape is copied for the drill
    // page invalidation at the end of this method.
    const geometry::planar::IntBox room_shape_copy = p_room->get_shape();
    for (auto* current_door : p_room->get_doors())
    {
        if (current_door == nullptr)
        {
            continue;
        }
        ExpansionRoom* current_neighbour = current_door->other_room(p_room);
        if (current_neighbour == nullptr)
        {
            continue;
        }
        current_neighbour->remove_door(current_door);
        const geometry::planar::Simplex neighbour_shape = current_neighbour->get_shape_tile();
        std::unique_ptr<geometry::planar::TileShape> intersection =
            geometry::planar::TileShape::intersection_of(room_shape, neighbour_shape);
        if (intersection == nullptr || intersection->dimension() != 1)
        {
            continue;
        }
        // A 1-dimensional intersection means that the neighbour touches the
        // removed room along a common border piece. The new incomplete room is
        // the half plane behind that border line of the neighbour, so it
        // contains exactly the part of the removed room outside the neighbour.
        const std::vector<int> touching_sides = room_shape.touching_sides(neighbour_shape);
        if (touching_sides.size() < 2)
        {
            logger::FRLogger::instance().warn(
                "AutorouteEngine::remove_complete_expansion_room: "
                "no touching sides between the removed room and its neighbour");
            continue;
        }
        const geometry::planar::Line new_room_line =
            neighbour_shape.border_line(touching_sides[1]).opposite();
        std::unique_ptr<geometry::planar::TileShape> new_room_shape =
            geometry::planar::TileShape::get_instance({new_room_line});
        if (new_room_shape == nullptr || new_room_shape->is_empty())
        {
            continue;
        }
        IncompleteFreeSpaceExpansionRoom* new_incomplete_room = add_incomplete_expansion_room(
            new_room_shape->to_Simplex(), room_layer, intersection->to_Simplex());
        if (new_incomplete_room == nullptr)
        {
            continue;
        }
        auto* new_door = new ExpansionDoor(current_neighbour, new_incomplete_room, 1);
        current_neighbour->add_door(new_door);
        new_incomplete_room->add_door(new_door);
    }

    remove_all_doors(p_room);
    if (autoroute_search_tree_)
    {
        p_room->remove_from_tree(*autoroute_search_tree_);
    }
    auto it = std::find_if(complete_expansion_rooms_.begin(), complete_expansion_rooms_.end(),
        [p_room](const std::unique_ptr<CompleteFreeSpaceExpansionRoom>& ptr) { return ptr.get() == p_room; });
    if (it == complete_expansion_rooms_.end())
    {
        logger::FRLogger::instance().warn(
            "AutorouteEngine::remove_complete_expansion_room: this.complete_expansion_rooms "
            "does not contain the room");
    }
    else
    {
        complete_expansion_rooms_.erase(it);
    }
    invalidate_drill_pages(room_shape_copy);
}

void AutorouteEngine::complete_neighbour_rooms(CompleteExpansionRoom* p_room)
{
    if (p_room == nullptr)
    {
        return;
    }
    // Port of Java AutorouteEngine.completeNeighbourRooms(): completing a
    // neighbour can change the door set of p_room (a completed room may be
    // split into several rooms), so the iteration restarts on the updated door
    // list. get_doors() returns a snapshot, therefore the loop re-reads it
    // after every completion, exactly like the Java iterator re-assignment.
    for (size_t door_index = 0;;)
    {
        const std::vector<ExpansionDoor*> doors = p_room->get_doors();
        if (door_index >= doors.size())
        {
            break;
        }
        ExpansionDoor* current_door = doors[door_index];
        if (current_door == nullptr)
        {
            ++door_index;
            continue;
        }
        // other_room() must be called with an ExpansionRoom pointer: the
        // CompleteExpansionRoom overload returns nullptr for a door to an
        // incomplete room (Java: the explicit (ExpansionRoom) cast).
        ExpansionRoom* neighbour_room = current_door->other_room(static_cast<ExpansionRoom*>(p_room));
        if (neighbour_room == nullptr)
        {
            ++door_index;
            continue;
        }
        if (auto* free_room = dynamic_cast<IncompleteFreeSpaceExpansionRoom*>(neighbour_room))
        {
            complete_expansion_room(free_room);
            // The door set may have changed, so start over.
            door_index = 0;
        }
        else if (auto* obstacle_room = dynamic_cast<ObstacleExpansionRoom*>(neighbour_room))
        {
            if (!obstacle_room->all_doors_calculated())
            {
                SortedRoomNeighbours::calculate(obstacle_room, this);
                obstacle_room->set_doors_calculated(true);
            }
            ++door_index;
        }
        else
        {
            ++door_index;
        }
    }
}

std::vector<CompleteFreeSpaceExpansionRoom*> AutorouteEngine::get_rooms_with_target_items(
    const std::set<board::Item*>& p_items) const
{
    // Port of Java AutorouteEngine.getRoomsWithTargetItems(Set<Item>): every
    // complete room with a target door to one of the given items is returned.
    // Java collects into a TreeSet, so the result is ordered by the room id.
    std::set<std::pair<int, CompleteFreeSpaceExpansionRoom*>> ordered_rooms;
    for (const auto& room : complete_expansion_rooms_)
    {
        for (auto* target_door : room->target_doors())
        {
            auto* item_door = dynamic_cast<TargetItemExpansionDoor*>(target_door);
            if (item_door == nullptr)
            {
                continue;
            }
            if (p_items.find(item_door->item) != p_items.end())
            {
                ordered_rooms.emplace(room->get_room_id(), room.get());
                break;
            }
        }
    }
    std::vector<CompleteFreeSpaceExpansionRoom*> result;
    result.reserve(ordered_rooms.size());
    for (const auto& entry : ordered_rooms)
    {
        result.push_back(entry.second);
    }
    return result;
}

int AutorouteEngine::generate_room_id_no()
{
    return ++expansion_room_instance_count_;
}

IncompleteFreeSpaceExpansionRoom* AutorouteEngine::add_incomplete_expansion_room(
    IncompleteFreeSpaceExpansionRoom* p_room)
{
    if (p_room == nullptr)
    {
        return nullptr;
    }
    incomplete_expansion_rooms_.push_back(std::unique_ptr<IncompleteFreeSpaceExpansionRoom>(p_room));
    return p_room;
}

IncompleteFreeSpaceExpansionRoom* AutorouteEngine::add_incomplete_expansion_room(
    const geometry::planar::Simplex& p_shape, int p_layer, const geometry::planar::Simplex& p_contained_shape)
{
    auto* new_room = new IncompleteFreeSpaceExpansionRoom(nullptr, p_layer, p_shape, p_contained_shape);
    incomplete_expansion_rooms_.push_back(
        std::unique_ptr<IncompleteFreeSpaceExpansionRoom>(new_room));
    return new_room;
}

IncompleteFreeSpaceExpansionRoom* AutorouteEngine::add_incomplete_expansion_room(
    WholePlaneShapeTag p_shape_kind, int p_layer, const geometry::planar::Simplex& p_contained_shape)
{
    auto* new_room = new IncompleteFreeSpaceExpansionRoom(nullptr, p_layer, p_shape_kind, p_contained_shape);
    incomplete_expansion_rooms_.push_back(
        std::unique_ptr<IncompleteFreeSpaceExpansionRoom>(new_room));
    return new_room;
}

IncompleteFreeSpaceExpansionRoom* AutorouteEngine::get_first_incomplete_expansion_room()
{
    if (incomplete_expansion_rooms_.empty())
    {
        return nullptr;
    }
    return incomplete_expansion_rooms_.front().get();
}

void AutorouteEngine::remove_all_doors(ExpansionRoom* p_room)
{
    if (p_room == nullptr)
    {
        return;
    }

    // Mirrors Java AutorouteEngine.removeAllDoors(): walk the doors of p_room
    // and of every incomplete free space room reached through them, removing
    // each door from both sides. Unlike Java (which relies on the garbage
    // collector), the C++ incomplete rooms are owned by the database, so the
    // walked rooms are only destroyed after the whole door walk; no door
    // iteration ever dereferences an already deleted room.
    std::vector<IncompleteFreeSpaceExpansionRoom*> incomplete_rooms_to_remove;
    std::vector<ExpansionRoom*> queue;
    std::set<ExpansionRoom*> visited;
    queue.push_back(p_room);
    for (size_t index = 0; index < queue.size(); ++index)
    {
        ExpansionRoom* current = queue[index];
        if (!visited.insert(current).second)
        {
            continue;
        }
        for (auto* door : current->get_doors())
        {
            if (door == nullptr)
            {
                continue;
            }
            ExpansionRoom* other_room = door->other_room(current);
            if (other_room == nullptr)
            {
                continue;
            }
            other_room->remove_door(door);
            auto* free_room = dynamic_cast<IncompleteFreeSpaceExpansionRoom*>(other_room);
            if (free_room != nullptr)
            {
                incomplete_rooms_to_remove.push_back(free_room);
                queue.push_back(free_room);
            }
        }
        current->clear_doors();
    }

    // Destroy the walked incomplete rooms now that no door still references
    // them. The erase is a pointer comparison only, so a room reached twice
    // is simply not found on the second attempt.
    for (auto* free_room : incomplete_rooms_to_remove)
    {
        auto it = std::find_if(incomplete_expansion_rooms_.begin(), incomplete_expansion_rooms_.end(),
            [free_room](const std::unique_ptr<IncompleteFreeSpaceExpansionRoom>& ptr) { return ptr.get() == free_room; });
        if (it != incomplete_expansion_rooms_.end())
        {
            incomplete_expansion_rooms_.erase(it);
        }
    }
}

void AutorouteEngine::remove_incomplete_expansion_room(IncompleteExpansionRoom* p_room)
{
    // Mirrors Java removeIncompleteExpansionRoom(): the doors of the room are
    // removed first, which also removes the incomplete free space rooms
    // reachable through them from the database.
    remove_all_doors(p_room);
    auto it = std::find_if(incomplete_expansion_rooms_.begin(), incomplete_expansion_rooms_.end(),
        [p_room](const std::unique_ptr<IncompleteFreeSpaceExpansionRoom>& ptr) { return ptr.get() == p_room; });
    if (it != incomplete_expansion_rooms_.end())
    {
        incomplete_expansion_rooms_.erase(it);
    }
}

std::vector<CompleteFreeSpaceExpansionRoom*> AutorouteEngine::complete_expansion_room(
    IncompleteFreeSpaceExpansionRoom* p_room)
{
    std::vector<CompleteFreeSpaceExpansionRoom*> result;
    if (p_room == nullptr)
    {
        return result;
    }

    // Mirrors the Java completeExpansionRoom() prologue: search the doors of
    // p_room for a door of dimension 2 to a neighbour CompleteFreeSpaceExpansionRoom
    // and use that room as the ignore object and its shape (the from-door
    // shape) as the ignore shape for the shape completion.
    board::SearchTreeObject* ignore_object = nullptr;
    std::unique_ptr<geometry::planar::IntBox> from_door_shape;
    for (auto* current_door : p_room->get_doors())
    {
        if (current_door == nullptr)
        {
            continue;
        }
        auto* other_room = current_door->other_room(p_room);
        auto* free_room = dynamic_cast<CompleteFreeSpaceExpansionRoom*>(other_room);
        if (free_room != nullptr && current_door->get_dimension() == 2)
        {
            from_door_shape = std::make_unique<geometry::planar::IntBox>(current_door->get_shape());
            ignore_object = free_room;
            break;
        }
    }

    const int layer = p_room->get_layer();

    board::ShapeSearchTree* search_tree = get_autoroute_search_tree();
    if (search_tree == nullptr)
    {
        // No tree: there is no free space to compute.
        return result;
    }

    // Mirrors Java AutorouteEngine.completeExpansionRoom(): the room's own
    // TileShape (null = whole plane, expressed as a nullptr input shape) and
    // its contained shape are handed to completeShape(), which starts from the
    // board bounding box intersected with the room shape.
    const geometry::planar::Simplex room_shape_tile = p_room->get_shape_tile();
    const geometry::planar::Simplex contained_shape_tile = p_room->get_contained_shape_tile();
    board::ShapeSearchTree::CompleteShapeInput input;
    input.shape = p_room->is_whole_plane() ? nullptr : &room_shape_tile;
    input.contained_shape = &contained_shape_tile;
    input.layer = layer;
    input.net_no = net_no_;
    input.ignore_object = ignore_object;
    input.ignore_shape = from_door_shape.get();
    std::vector<board::ShapeSearchTree::CompleteShapePiece> completed_shapes =
        search_tree->complete_shape(input);

    // Mirrors Java removeIncompleteExpansionRoom(): p_room no longer exists
    // after the completion. No-op if the room was never registered.
    remove_incomplete_expansion_room(p_room);

    bool is_first_completed_room = true;
    for (const board::ShapeSearchTree::CompleteShapePiece& piece : completed_shapes)
    {
        if (piece.shape == nullptr || piece.shape->dimension() != 2)
        {
            continue;
        }
        if (is_first_completed_room)
        {
            is_first_completed_room = false;
            add_complete_room(layer, piece, result);
        }
        else
        {
            // The shape of the first completed room may have changed and may
            // now intersect with the other shapes. Therefore the completed
            // shapes have to be recalculated. Mirrors the Java recalc loop.
            board::ShapeSearchTree::CompleteShapeInput recalc_input;
            recalc_input.shape = piece.shape.get();
            recalc_input.contained_shape = piece.contained_shape.get();
            recalc_input.layer = layer;
            recalc_input.net_no = net_no_;
            recalc_input.ignore_object = ignore_object;
            recalc_input.ignore_shape = from_door_shape.get();
            std::vector<board::ShapeSearchTree::CompleteShapePiece> recalc_shapes =
                search_tree->complete_shape(recalc_input);
            for (const board::ShapeSearchTree::CompleteShapePiece& recalc_piece : recalc_shapes)
            {
                add_complete_room(layer, recalc_piece, result);
            }
        }
    }
    return result;
}

CompleteFreeSpaceExpansionRoom* AutorouteEngine::add_complete_room(int p_layer,
    const board::ShapeSearchTree::CompleteShapePiece& p_piece, std::vector<CompleteFreeSpaceExpansionRoom*>& p_result)
{
    // Port of Java AutorouteEngine.addCompleteRoom(): the doors of the
    // completed room are calculated first (Java calculateDoors() =
    // SortedRoomNeighbours.complete()), and the room is only registered in the
    // room database and in the autoroute search tree if it is 2-dimensional.
    if (p_piece.shape == nullptr)
    {
        return nullptr;
    }
    const geometry::planar::Simplex shape = p_piece.shape->to_Simplex();
    const geometry::planar::Simplex contained_shape = p_piece.contained_shape != nullptr
        ? p_piece.contained_shape->to_Simplex()
        : shape;
    // Java addCompleteRoom() receives an incomplete free space room, which is
    // not part of any database, and the complete room is created from it by
    // SortedRoomNeighbours.calculateNeighbours().
    IncompleteFreeSpaceExpansionRoom incomplete_room(nullptr, p_layer, shape, contained_shape);
    CompleteExpansionRoom* calculated_room = SortedRoomNeighbours::calculate(&incomplete_room, this);
    auto* completed_room = dynamic_cast<CompleteFreeSpaceExpansionRoom*>(calculated_room);
    if (completed_room == nullptr || completed_room->get_shape().dimension() != 2)
    {
        // The room was not created by the door calculation, so no ownership is
        // transferred here.
        return nullptr;
    }
    std::unique_ptr<CompleteFreeSpaceExpansionRoom> owned_room(completed_room);
    complete_expansion_rooms_.push_back(std::move(owned_room));
    CompleteFreeSpaceExpansionRoom* room_ptr = complete_expansion_rooms_.back().get();
    // Store the completed room in the autoroute search tree so that later
    // lookups (e.g. subsequent drills on the same location) find it.
    // Mirrors Java AutorouteEngine.addCompleteRoom().
    if (board::ShapeSearchTree* search_tree = get_autoroute_search_tree())
    {
        search_tree->insert(*room_ptr);
    }
    p_result.push_back(room_ptr);
    logger::FRLogger::instance().trace("COMPLETE_ROOM added, net=" + std::to_string(net_no_)
        + ", layer=" + std::to_string(p_layer)
        + ", room_id=" + std::to_string(room_ptr->get_room_id())
        + ", bounds=" + room_ptr->get_shape().to_string());
    return room_ptr;
}

} // namespace freerouting::autoroute