#ifndef AUTOROUTE_CONNECTION_H
#define AUTOROUTE_CONNECTION_H

#include <vector>
#include <set>
#include <memory>

namespace freerouting::geometry::planar {
class Point;
}

namespace freerouting::board {
class Item;
}

namespace freerouting::autoroute {

/**
 * Describes a routing connection ending at the next fork or terminal item.
 */
class Connection {
public:
    static Connection* get(board::Item* p_item);

    const std::shared_ptr<geometry::planar::Point> start_point;
    const int start_layer;
    const std::shared_ptr<geometry::planar::Point> end_point;
    const int end_layer;
    const std::set<board::Item*> item_list;

    double trace_length() const;
    double get_detour() const;

private:
    Connection(std::shared_ptr<geometry::planar::Point> p_start_point, int p_start_layer,
               std::shared_ptr<geometry::planar::Point> p_end_point, int p_end_layer,
               const std::set<board::Item*>& p_item_list);

    static constexpr double DETOUR_ADD = 100.0;
    static constexpr double DETOUR_ITEM_COST = 0.1;
};

} // namespace freerouting::autoroute

#endif // AUTOROUTE_CONNECTION_H
