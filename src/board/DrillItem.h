#pragma once

#include <memory>

#include "board/Item.h"
#include "board/Connectable.h"

namespace freerouting::geometry::planar {
class Shape;
}

namespace freerouting {
namespace board {

class DrillItem : public Item, public Connectable
{
public:
    DrillItem(const std::shared_ptr<geometry::planar::Point>& p_center,
            const std::vector<int>& p_net_no_arr,
            int p_clearance_type, int p_id_no, int p_group_no,
            FixedState p_fixed_state);

    const std::shared_ptr<geometry::planar::Point>& get_center() const { return center_; }
    void set_center(const std::shared_ptr<geometry::planar::Point>& p_center) { center_ = p_center; }

    int get_first_layer() const override { return first_layer_; }
    int get_last_layer() const override { return last_layer_; }

    /**
     * Returns true, if this item has a shape on p_layer.
     * Mirrors Java DrillItem.isOnLayer(int).
     */
    bool is_on_layer(int p_layer) const { return p_layer >= first_layer() && p_layer <= last_layer(); }

    double get_min_width() const;

    virtual std::shared_ptr<geometry::planar::TileShape> get_tile_shape() const = 0;
    virtual std::shared_ptr<geometry::planar::Shape> get_shape(int p_layer) const = 0;

    void translate_by(const std::shared_ptr<geometry::planar::Vector>& p_vector) override;

    std::string to_string() const override;

    bool contains_net(int p_net_no) const override;
    bool shares_net_no(const std::vector<int>& p_net_no_arr) const override;

protected:
    std::shared_ptr<geometry::planar::Point> center_;
    double precalculated_min_width_ = -1;
    int precalculated_first_layer_ = -1;
    int precalculated_last_layer_ = -1;
};

} // namespace board
} // namespace freerouting