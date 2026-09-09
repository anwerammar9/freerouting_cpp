#ifndef AUTOROUTE_BATCH_AUTOROUTER_H
#define AUTOROUTE_BATCH_AUTOROUTER_H

#include <memory>
#include <string>
#include <vector>
#include <set>

#include "settings/RouterSettings.h"
#include "autoroute/AutorouteControl.h"
#include "board/Item.h"

namespace freerouting::board {
class RoutingBoard;
}

namespace freerouting::datastructures {
class Stoppable;
}

namespace freerouting::autoroute {

struct RouteResult {
    int complete_count = 0;
    int incomplete_count = 0;
    int vias_removed = 0;
    double score = 0.0;
    bool success = false;
};

class BatchAutorouter {
public:
    BatchAutorouter();
    ~BatchAutorouter();

    void set_board(board::RoutingBoard* p_board);
    void set_settings(const settings::RouterSettings& p_settings);

    RouteResult route_all(int p_max_passes, datastructures::Stoppable* p_stoppable);
    void stop() { stopped_ = true; }

    std::vector<board::Item*> get_autoroute_items() const;
    int count_incomplete_connections() const;
    int count_nets_to_route() const;
    
    // Track nets that have been successfully routed
    void mark_net_routed(int p_net_no) { routed_nets_.insert(p_net_no); }
    int count_nets_routed() const { return static_cast<int>(routed_nets_.size()); }

    std::string to_string() const { return "BatchAutorouter"; }

private:
    board::RoutingBoard* board_ = nullptr;
    std::unique_ptr<AutorouteControl> control_;
    std::unique_ptr<settings::RouterSettings> settings_;
    bool stopped_ = false;
    std::set<int> routed_nets_;  // Track which nets have been routed at least once
};

} // namespace freerouting::autoroute

#endif // AUTOROUTE_BATCH_AUTOROUTER_H