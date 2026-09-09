#pragma once

#include "board/Via.h"

namespace freerouting::board {

class Pin : public DrillItem
{
public:
    Pin(int p_component_no,
        int p_pin_no,
        const std::vector<int>& p_net_nos,
        int p_clearance_class,
        int p_id_no,
        FixedState p_fixed_state,
        BasicBoard* p_board);

    bool is_pin() const override { return true; }

    int get_pin_no() const { return pin_no_; }
    Pin* get_changed_to() const { return changed_to_; }
    void set_changed_to(Pin* p_pin) { changed_to_ = p_pin; }

    std::string to_string() const override;

private:
    int pin_no_;
    Pin* changed_to_;
};

} // namespace freerouting::board