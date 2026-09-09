#include "autoroute/Connection.h"
#include "board/Item.h"
#include "board/Trace.h"
#include "autoroute/ItemAutorouteInfo.h"
#include "geometry/planar/Point.h"
#include <cmath>
#include <limits>

namespace freerouting::autoroute {

Connection::Connection(std::shared_ptr<geometry::planar::Point> p_start_point, int p_start_layer,
                       std::shared_ptr<geometry::planar::Point> p_end_point, int p_end_layer,
                       const std::set<board::Item*>& p_item_list)
    : start_point(p_start_point), start_layer(p_start_layer),
      end_point(p_end_point), end_layer(p_end_layer),
      item_list(p_item_list) {
}

Connection* Connection::get(board::Item* p_item) {
    if (!p_item || !p_item->is_routable()) {
        return nullptr;
    }

    auto autoroute_info = p_item->get_autoroute_info();
    Connection* precalculated = autoroute_info->get_precalculated_connection();
    if (precalculated) {
        return precalculated;
    }

    // This is a simplified version of the Java logic to avoid infinite recursion
    // and handle the stubbed contact methods.
    std::set<board::Item*> connection_items;
    connection_items.insert(p_item);

    // In a real implementation, we would crawl the contacts here.
    // For now, we return a single-item connection.
    
    // Stubbed result
    return nullptr; 
}

double Connection::trace_length() const {
    double result = 0;
    for (auto* item : item_list) {
        if (item->is_trace()) {
            auto* trace = dynamic_cast<board::Trace*>(item);
            if (trace) {
                result += trace->get_length();
            }
        }
    }
    return result;
}

double Connection::get_detour() const {
    if (!start_point || !end_point) {
        return std::numeric_limits<double>::max();
    }
    
    // Note: This requires Point to have a distance method, which might be in FloatPoint
    double min_trace_length = 1.0; // Stubbed distance
    double detour = (trace_length() + DETOUR_ADD) / (min_trace_length + DETOUR_ADD) + 
                     DETOUR_ITEM_COST * (item_list.size() - 1);
    return detour;
}

} // namespace freerouting::autoroute
