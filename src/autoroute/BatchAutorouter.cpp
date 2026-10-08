#include "autoroute/BatchAutorouter.h"
#include "autoroute/AutorouteEngine.h"
#include "autoroute/AutorouteControl.h"
#include "board/RoutingBoard.h"
#include "board/Item.h"
#include "settings/RouterSettings.h"
#include "logger/FRLogger.h"
#include "datastructures/Stoppable.h"

#include <algorithm>
#include <set>
#include <map>
#include <cmath>

namespace freerouting::autoroute {

BatchAutorouter::BatchAutorouter()
    : control_(std::make_unique<AutorouteControl>())
{
}

BatchAutorouter::~BatchAutorouter() = default;

void BatchAutorouter::set_board(board::RoutingBoard* p_board)
{
    board_ = p_board;
    if (p_board)
    {
        control_->set_layer_count(p_board->get_layer_count());
    }
}

void BatchAutorouter::set_settings(const settings::RouterSettings& p_settings)
{
    settings_ = std::make_unique<settings::RouterSettings>(p_settings);
}

std::vector<board::Item*> BatchAutorouter::get_autoroute_items() const
{
    if (!board_)
    {
        return {};
    }

    std::vector<board::Item*> autoroute_items;
    auto all_items = board_->get_autoroute_items();

    std::map<int, int> net_item_counts;

    for (auto* item : all_items)
    {
        if (!item || !item->is_routable())
        {
            continue;
        }

        const auto& net_nos = item->get_net_nos();
        for (int net_no : net_nos)
        {
            net_item_counts[net_no]++;
        }
    }

    for (auto* item : all_items)
    {
        if (!item || !item->is_routable())
        {
            continue;
        }

        const auto& net_nos = item->get_net_nos();
        for (int net_no : net_nos)
        {
            int item_count = (net_item_counts.count(net_no) > 0) ? net_item_counts[net_no] : 0;
            if (item_count > 1)
            {
                bool already_added = false;
                for (auto* added : autoroute_items)
                {
                    if (added == item)
                    {
                        already_added = true;
                        break;
                    }
                }
                if (!already_added)
                {
                    autoroute_items.push_back(item);
                }
            }
        }
    }

    return autoroute_items;
}

int BatchAutorouter::count_incomplete_connections() const
{
    if (!board_)
    {
        return 0;
    }

    int incomplete = 0;
    auto items = board_->get_autoroute_items();

    std::map<int, std::set<board::Item*>> net_connections;

    for (auto* item : items)
    {
        if (!item)
        {
            continue;
        }
        const auto& net_nos = item->get_net_nos();
        for (int net_no : net_nos)
        {
            net_connections[net_no].insert(item);
        }
    }

    for (const auto& [net_no, conn_items] : net_connections)
    {
        int endpoint_count = 0;
        int trace_count = 0;
        for (auto* item : conn_items)
        {
            if (item->is_trace())
            {
                ++trace_count;
            }
            else
            {
                ++endpoint_count;
            }
        }
        if (endpoint_count >= 2)
        {
            incomplete += std::max(0, endpoint_count - trace_count - 1);
        }
    }

    return incomplete;
}

int BatchAutorouter::count_nets_to_route() const
{
    if (!board_)
    {
        return 0;
    }

    auto items = board_->get_autoroute_items();
    std::set<int> nets_with_items;

    for (auto* item : items)
    {
        if (!item) continue;
        // Accept all items for routing (including point-like pins)
        const auto& net_nos = item->get_net_nos();
        for (int net_no : net_nos)
        {
            nets_with_items.insert(net_no);
        }
    }

    return static_cast<int>(nets_with_items.size());
}

RouteResult BatchAutorouter::route_all(int p_max_passes, datastructures::Stoppable* p_stoppable)
{
    RouteResult result;

    if (!board_)
    {
        freerouting::logger::FRLogger::instance().error("BatchAutorouter: no board set");
        return result;
    }

    freerouting::logger::FRLogger& logger = freerouting::logger::FRLogger::instance();

    logger.info("Starting autoroute...");
    logger.info("  Max passes: " + std::to_string(p_max_passes));
    logger.info("  Layer count: " + std::to_string(board_->get_layer_count()));

    int pin_count = board_->get_pin_count();
    logger.info("  Pins: " + std::to_string(pin_count));

    int item_count = board_->get_item_count();
    logger.info("  Board items: " + std::to_string(item_count));

    int initial_incomplete = count_incomplete_connections();
    logger.info("  Initial incomplete connections: " + std::to_string(initial_incomplete));

    if (initial_incomplete == 0)
    {
        result.success = true;
        result.complete_count = 100;
        result.incomplete_count = 0;
        result.vias_removed = 0;
        result.score = 1.0;
        logger.info("Autoroute completed - nothing to route");
        logger.info("  Complete: 100%");
        return result;
    }

    int routed_count = 0;
    int pass = 1;
    int no_progress_passes = 0;
    int debug_failures = 0;

    const int via_costs = settings_ ? settings_->get_via_costs() : 1;

    for (; pass <= p_max_passes && !stopped_; ++pass)
    {
        // Skip check if stoppable is null (headless mode)
        if (p_stoppable && p_stoppable->is_stop_requested())
        {
            break;
        }

        logger.info("Pass " + std::to_string(pass) + " of " + std::to_string(p_max_passes));

        auto autoroute_items = get_autoroute_items();
        logger.info("  Items to route: " + std::to_string(autoroute_items.size()));

        int pass_routed = 0;

        // Interim pass body until the Java pipeline (AutoroutePassRunner /
        // AutorouteConnectionRouter, TASKS T401-T409) is ported: every item
        // with unconnected net members is routed through the Java-faithful
        // RoutingBoard::autoroute facade, which builds a per-net
        // AutorouteControl and routes the item's connected set to its
        // unconnected set.
        for (auto* item : autoroute_items)
        {
            if (p_stoppable && p_stoppable->is_stop_requested())
            {
                break;
            }
            if (item == nullptr || item->net_count() == 0)
            {
                continue;
            }
            const int net_no = item->get_net_no(0);
            if (item->get_unconnected_set(net_no).empty())
            {
                continue;
            }
            auto route_result = board_->autoroute(item, settings_.get(), via_costs, p_stoppable, nullptr);
            if (route_result.state == AutorouteAttemptState::ROUTED)
            {
                pass_routed++;
                routed_count++;
                routed_nets_.insert(net_no);
            }
            else if (debug_failures < 12)
            {
                logger.info("  FAIL net " + std::to_string(net_no) + ": " + route_result.details);
                ++debug_failures;
            }
        }

        int current_incomplete = count_incomplete_connections();
        logger.info("  Pass " + std::to_string(pass) + " routed: " + std::to_string(pass_routed));
        logger.info("  Remaining incomplete: " + std::to_string(current_incomplete));

        if (pass_routed > 0)
        {
            no_progress_passes = 0;
        }
        else
        {
            ++no_progress_passes;
        }

        if (current_incomplete == 0)
        {
            logger.info("All connections routed, stopping");
            break;
        }

        if (no_progress_passes >= 2)
        {
            logger.info("No progress, stopping");
            break;
        }
    }

int final_incomplete = count_incomplete_connections();

    result.success = true;
    
    // Calculate completion based on unique nets routed vs total nets
    int nets_to_route = count_nets_to_route();
    int nets_routed = static_cast<int>(routed_nets_.size());
    
    int reported_complete = (nets_to_route > 0) ? (nets_routed * 100) / nets_to_route : 100;
    reported_complete = std::min(100, std::max(0, reported_complete));
    
    result.complete_count = reported_complete;
    result.incomplete_count = 100 - result.complete_count;
    result.vias_removed = 0;
    result.score = result.complete_count / 100.0;

    logger.info("Autoroute completed");
    logger.info("  Passes: " + std::to_string(pass));
    logger.info("  Unique nets routed: " + std::to_string(nets_routed) + " / " + std::to_string(nets_to_route));
    logger.info("  Connections routed: " + std::to_string(routed_count));
    logger.info("  Initial incomplete: " + std::to_string(initial_incomplete));
    logger.info("  Final incomplete: " + std::to_string(final_incomplete));
    logger.info("  Complete: " + std::to_string(result.complete_count) + "%");
    logger.info("  Incomplete: " + std::to_string(result.incomplete_count) + "%");

    return result;
}

} // namespace freerouting::autoroute