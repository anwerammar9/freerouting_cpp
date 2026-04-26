#include "board/Pin.h"

namespace freerouting::board {

Pin::Pin(int p_component_no,
             int p_pin_no,
             const std::vector<int>& p_net_nos,
             int p_clearance_class,
             int p_id_no,
             FixedState p_fixed_state,
             BasicBoard* p_board)
    : DrillItem(geometry::planar::IntPoint(0, 0), p_net_nos, p_clearance_class,
                p_id_no, p_component_no, p_fixed_state, p_board),
      pin_no_(p_pin_no),
      changed_to_(nullptr)
{
    changed_to_ = this;
}

std::string Pin::to_string() const
{
    return "Pin[id=" + std::to_string(id_no_) + ", no=" + std::to_string(pin_no_) + "]";
}

} // namespace freerouting::board