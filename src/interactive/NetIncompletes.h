#pragma once

#include <memory>
#include <vector>
#include <list>
#include <cstdint>

namespace freerouting {
namespace interactive {

class Net;
class RoutingBoard;
class Item;

struct AirLine
{
    int from_x = 0, from_y = 0;
    int to_x = 0, to_y = 0;
    int from_item_id = 0;
    int to_item_id = 0;
    int from_net_no = 0;
    int to_net_no = 0;
};

class NetIncompletes
{
public:
    NetIncompletes() = default;

    void add_air_line(const AirLine& p_line) { incompletes_.push_back(p_line); }

    int count() const { return static_cast<int>(incompletes_.size()); }

    bool empty() const { return incompletes_.empty(); }

    void clear() { incompletes_.clear(); }

    const std::list<AirLine>& lines() const { return incompletes_; }

    double get_length_violation() const { return length_violation_; }
    void set_length_violation(double p_violation) { length_violation_ = p_violation; }

    int get_net_no() const { return net_no_; }
    std::shared_ptr<Net> get_net() const { return net_; }

private:
    int net_no_ = 0;
    std::shared_ptr<Net> net_;
    double draw_marker_radius_ = 0;
    double length_violation_ = 0;
    std::list<AirLine> incompletes_;
};

} // namespace interactive
} // namespace freerouting