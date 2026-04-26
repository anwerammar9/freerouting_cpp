#include "autoroute/AutorouteEngine.h"
#include "autoroute/MazeSearchAlgo.h"
#include "autoroute/LocateFoundConnectionAlgo.h"
#include "autoroute/InsertFoundConnectionAlgo.h"
#include "autoroute/ExpansionRoom.h"
#include "board/RoutingBoard.h"
#include "board/ShapeSearchTree.h"
#include "datastructures/Stoppable.h"
#include "datastructures/TimeLimit.h"
#include "logger/FRLogger.h"

#include <cmath>
#include <set>
#include <map>

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
        autoroute_search_tree_ = std::make_unique<board::ShapeSearchTree>(board_, trace_clearance_class_no_);
        
        logger::FRLogger::instance().debug("Created autoroute search tree with clearance class " + 
            std::to_string(trace_clearance_class_no_));
        
        auto items = board_->get_autoroute_items();
        for (int i = 0; i < static_cast<int>(items.size()); ++i)
        {
            autoroute_search_tree_->insert(items[i], 0);
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

    auto start_bbox = start_item->get_bounding_box();
    auto dest_bbox = dest_item->get_bounding_box();

    if (start_bbox.ll.x == start_bbox.ur.x && start_bbox.ll.y == start_bbox.ur.y)
    {
        result.details = "Invalid start bounding box";
        return result;
    }
    if (dest_bbox.ll.x == dest_bbox.ur.x && dest_bbox.ll.y == dest_bbox.ur.y)
    {
        result.details = "Invalid dest bounding box";
        return result;
    }

    // Simple L-shaped routing between centers - use direct connection
    int start_x = (start_bbox.ll.x + start_bbox.ur.x) / 2;
    int start_y = (start_bbox.ll.y + start_bbox.ur.y) / 2;
    int dest_x = (dest_bbox.ll.x + dest_bbox.ur.x) / 2;
    int dest_y = (dest_bbox.ll.y + dest_bbox.ur.y) / 2;
    
    int start_layer = start_item->get_layer();
    int dest_layer = dest_item->get_layer();
    int trace_width = 100; // Default trace width
    
    // Create a simple connection using add_route_connection
    auto connection_result = board_->add_route_connection(
        start_item, start_layer, start_x, start_y,
        dest_item, dest_layer, dest_x, dest_y,
        trace_width, net_no_);
    
    if (connection_result)
    {
        result.state = AutorouteAttemptState::ROUTED;
        result.details = "L-shaped connection created";
    }
    else
    {
        result.details = "Failed to create connection";
    }
    
    return result;
}

int AutorouteEngine::get_net_no() const
{
    return net_no_;
}

void AutorouteEngine::clear()
{
    incomplete_expansion_rooms_.clear();
    complete_expansion_rooms_.clear();
    expansion_room_instance_count_ = 0;
}

int AutorouteEngine::generate_room_id_no()
{
    return ++expansion_room_instance_count_;
}

} // namespace freerouting::autoroute