#include "board/BoardOutline.h"

#include "board/BasicBoard.h"
#include "board/ObstacleArea.h"
#include "board/SearchTreeManager.h"
#include "board/ShapeSearchTree.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/PolygonShape.h"
#include "geometry/planar/PolylineArea.h"
#include "geometry/planar/TileShape.h"
#include "logger/FRLogger.h"

namespace freerouting {
namespace board {

namespace fp = geometry::planar;

BoardOutline::BoardOutline(std::vector<std::shared_ptr<fp::PolylineShape>> p_shapes, int p_clearance_class_no,
    int p_id_no, BasicBoard* p_board)
    : Item(std::vector<int>{}, p_clearance_class_no, p_id_no, 0, FixedState::SYSTEM_FIXED, p_board)
    , shapes_(std::move(p_shapes))
{
    // Java boundingBox(): the union of the outline shape boxes.
    fp::IntBox box = fp::IntBox::EMPTY;
    for (const auto& curr_shape : shapes_)
    {
        box = box.union_with(curr_shape->bounding_box());
    }
    set_bounding_box(box);
}

int BoardOutline::last_layer() const
{
    return get_board() != nullptr ? get_board()->get_layer_count() - 1 : 0;
}

int BoardOutline::tile_shape_count() const
{
    const int layer_count = get_board() != nullptr ? get_board()->get_layer_count() : 0;
    if (keepout_outside_outline_)
    {
        const fp::Area* keepout = get_keepout_area();
        return keepout != nullptr ? static_cast<int>(keepout->split_to_convex().size()) * layer_count : 0;
    }
    return line_count() * layer_count;
}

int BoardOutline::shape_layer(int p_index) const
{
    const int layer_count = get_board() != nullptr ? get_board()->get_layer_count() : 0;
    const int shape_count = tile_shape_count();
    const int result = shape_count > 0 ? p_index * layer_count / shape_count : 0;
    if (result < 0 || result >= layer_count)
    {
        logger::FRLogger::instance().warn("BoardOutline.shapeLayer: index out of range");
    }
    return result;
}

bool BoardOutline::is_obstacle(const Item* p_other) const
{
    // Java: !(other instanceof BoardOutline || other instanceof ObstacleArea)
    return dynamic_cast<const BoardOutline*>(p_other) == nullptr && dynamic_cast<const ObstacleArea*>(p_other) == nullptr;
}

const fp::PolylineShape* BoardOutline::get_shape(int p_index) const
{
    if (p_index < 0 || p_index >= shape_count())
    {
        logger::FRLogger::instance().warn("BoardOutline.get_shape: index out of range");
        return nullptr;
    }
    return shapes_[p_index].get();
}

const fp::Area* BoardOutline::get_keepout_area() const
{
    if (keepout_area_ == nullptr && get_board() != nullptr)
    {
        // Java: new PolylineArea(board.boundingBox, shapes). The C++ IntBox is
        // not a PolylineShape, so the border is the polygon of its corners.
        const fp::IntBox box = get_board()->get_bounding_box();
        auto border = std::make_shared<fp::PolygonShape>(std::vector<fp::IntPoint>{
            box.ll, fp::IntPoint(box.ur.x, box.ll.y), box.ur, fp::IntPoint(box.ll.x, box.ur.y) });
        keepout_area_ = std::make_shared<fp::PolylineArea>(border, shapes_);
    }
    return keepout_area_.get();
}

void BoardOutline::generate_keepout_outside(bool p_value)
{
    if (p_value == keepout_outside_outline_)
    {
        return;
    }
    keepout_outside_outline_ = p_value;
    if (get_board() == nullptr || get_board()->get_search_tree_manager() == nullptr)
    {
        return;
    }
    get_board()->get_search_tree_manager()->remove(this);
    clear_derived_data();
    get_board()->get_search_tree_manager()->insert(this);
}

int BoardOutline::line_count() const
{
    int result = 0;
    for (const auto& curr_shape : shapes_)
    {
        result += curr_shape->border_line_count();
    }
    return result;
}

bool BoardOutline::contains(const fp::IntPoint& p_point) const
{
    for (const auto& curr_shape : shapes_)
    {
        if (curr_shape->contains(p_point))
        {
            return true;
        }
    }
    return false;
}

bool BoardOutline::contains(const fp::FloatPoint& p_point) const
{
    for (const auto& curr_shape : shapes_)
    {
        if (curr_shape->contains(p_point))
        {
            return true;
        }
    }
    return false;
}

std::vector<std::shared_ptr<fp::TileShape>> BoardOutline::calculate_tree_shapes(const ShapeSearchTree& p_search_tree) const
{
    return p_search_tree.calculate_tree_shapes(*this);
}

} // namespace board
} // namespace freerouting
