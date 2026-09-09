#include "board/Via.h"

#include <algorithm>
#include <limits>

#include "board/Padstack.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/Shape.h"

namespace freerouting::board {

double DrillItem::min_width() const
{
    double result = std::numeric_limits<double>::max();
    if (padstack_ != nullptr)
    {
        for (int layer = padstack_->get_from_layer(); layer <= padstack_->get_to_layer(); ++layer)
        {
            const geometry::planar::Shape* shape = padstack_->get_shape(layer);
            if (shape != nullptr)
            {
                geometry::planar::IntBox box = shape->bounding_box();
                result = std::min(result, static_cast<double>(box.width()));
                result = std::min(result, static_cast<double>(box.height()));
            }
        }
    }
    return result;
}

DrillItem::DrillItem(const geometry::planar::IntPoint& p_center,
                       const std::vector<int>& p_net_nos,
                       int p_clearance_class,
                       int p_id_no,
                       int p_component_no,
                       FixedState p_fixed_state,
                       BasicBoard* p_board)
    : Item(p_net_nos, p_clearance_class, p_id_no, p_component_no, p_fixed_state, p_board),
      center_(p_center),
      from_layer_(0),
      to_layer_(0),
      padstack_(nullptr)
{
}

Via::Via(Padstack* p_padstack,
             const geometry::planar::IntPoint& p_center,
             const std::vector<int>& p_net_nos,
             int p_clearance_class,
             int p_id_no,
             int p_component_no,
             FixedState p_fixed_state,
             bool p_attach_allowed,
             BasicBoard* p_board)
    : DrillItem(p_center, p_net_nos, p_clearance_class, p_id_no, p_component_no, p_fixed_state, p_board),
      attach_allowed_(p_attach_allowed)
{
    padstack_ = p_padstack;
}

Item* Via::copy(int p_id_no) const
{
    return new Via(padstack_, get_center(), net_nos_, clearance_class_,
                   p_id_no, component_no_, fixed_state_, attach_allowed_, board_);
}

std::string Via::to_string() const
{
    return "Via[id=" + std::to_string(id_no_) + "]";
}

const geometry::planar::TileShape* Via::get_tree_shape_on_layer(ShapeSearchTree* p_search_tree, int p_layer)
{
    if (!padstack_)
    {
        return nullptr;
    }
    int layer_index = p_layer - from_layer_;
    if (layer_index < 0 || layer_index >= tile_shape_count())
    {
        return nullptr;
    }
    return get_tree_shape(*p_search_tree, layer_index);
}

const geometry::planar::TileShape* Via::get_tree_shape_on_layer(const ShapeSearchTree* p_search_tree, int p_layer) const
{
    if (!padstack_)
    {
        return nullptr;
    }
    int layer_index = p_layer - from_layer_;
    if (layer_index < 0 || layer_index >= tile_shape_count())
    {
        return nullptr;
    }
    return get_tree_shape(*p_search_tree, layer_index);
}

} // namespace freerouting::board