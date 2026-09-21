#include "autoroute/AutorouteEngine.h"
#include "autoroute/MazeSearchAlgo.h"
#include "autoroute/LocateFoundConnectionAlgo.h"
#include "autoroute/InsertFoundConnectionAlgo.h"
#include "autoroute/ExpansionRoom.h"
#include "board/RoutingBoard.h"
#include "board/PolylineTrace.h"
#include "board/ShapeSearchTree.h"
#include "rules/ClearanceMatrix.h"
#include "geometry/planar/IntOctagon.h"
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
    if (autoroute_search_tree_)
    {
        p_room->remove_from_tree(*autoroute_search_tree_);
    }
    auto it = std::find_if(complete_expansion_rooms_.begin(), complete_expansion_rooms_.end(),
        [p_room](const std::unique_ptr<CompleteFreeSpaceExpansionRoom>& ptr) { return ptr.get() == p_room; });
    complete_expansion_rooms_.erase(it);
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

std::vector<CompleteFreeSpaceExpansionRoom*> AutorouteEngine::complete_expansion_room(
    IncompleteExpansionRoom* p_room)
{
    std::vector<CompleteFreeSpaceExpansionRoom*> result;
    if (p_room == nullptr)
    {
        return result;
    }
    geometry::planar::IntBox room_shape = p_room->get_shape();
    if (room_shape.dimension() < 2)
    {
        // A sub-2-dimensional room (e.g. the unit search shape of a drill
        // location) is inflated to a tiny 2-dimensional box. Java grows the
        // room to the neighbouring free space via the search tree; that port
        // (ShapeSearchTree.complete_shape, blocker 6) is not present yet.
        room_shape = room_shape.offset(1.0);
    }
    if (room_shape.dimension() != 2)
    {
        return result;
    }
    auto completed_room =
        std::make_unique<CompleteFreeSpaceExpansionRoom>(p_room->get_layer(),
                                                         generate_room_id_no(),
                                                         room_shape);
    CompleteFreeSpaceExpansionRoom* completed_room_ptr = completed_room.get();
    complete_expansion_rooms_.push_back(std::move(completed_room));
    // Store the completed room in the autoroute search tree so that later
    // lookups (e.g. subsequent drills on the same location) find it.
    // Mirrors Java AutorouteEngine.addCompleteRoom().
    board::ShapeSearchTree* search_tree = get_autoroute_search_tree();
    if (search_tree != nullptr)
    {
        search_tree->insert(*completed_room_ptr);
    }
    result.push_back(completed_room_ptr);
    logger::FRLogger::instance().trace(
        "AutorouteEngine.complete_expansion_room: net=" + std::to_string(net_no_)
        + ", layer=" + std::to_string(completed_room_ptr->get_layer())
        + ", room_id=" + std::to_string(completed_room_ptr->get_room_id()));
    return result;
}

} // namespace freerouting::autoroute