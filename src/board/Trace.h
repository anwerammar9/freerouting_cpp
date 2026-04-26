#pragma once

#include <memory>
#include <vector>

#include "board/Item.h"
#include "board/Connectable.h"

namespace freerouting {
namespace board {

class Trace : public Item, public Connectable
{
public:
    Trace(int p_layer, int p_half_width,
          const std::vector<int>& p_net_no_arr,
          int p_clearance_type, int p_id_no, int p_group_no,
          FixedState p_fixed_state);

    int get_layer() const override { return layer_; }
    void set_layer(int p_layer) override { layer_ = p_layer; }

    int get_half_width() const { return half_width_; }
    void set_half_width(int p_width) { half_width_ = p_width; }

    virtual const geometry::planar::Point& first_corner() const = 0;
    virtual const geometry::planar::Point& last_corner() const = 0;
    virtual const std::vector<std::shared_ptr<geometry::planar::Point>>& corners() const = 0;
    virtual int corner_count() const = 0;
    virtual const geometry::planar::Point& corner(int p_index) const = 0;

    virtual std::shared_ptr<TileShape> get_tile_shape() const override = 0;

    std::string to_string() const override;

    bool contains_net(int p_net_no) const override;
    bool shares_net_no(const std::vector<int>& p_net_no_arr) const override;

private:
    int half_width_;
    int layer_;
};

} // namespace board
} // namespace freerouting