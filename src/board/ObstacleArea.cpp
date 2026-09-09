#include "board/ObstacleArea.h"

#include "board/ItemSelectionFilter.h"
#include "geometry/planar/Point.h"
#include "geometry/planar/Shape.h"
#include "logger/FRLogger.h"

#include <cmath>

namespace freerouting::board {

ObstacleArea::ObstacleArea(const std::shared_ptr<geometry::planar::Area>& p_area,
                           int p_layer,
                           const geometry::planar::Vector& p_translation,
                           double p_rotation_in_degree,
                           bool p_side_changed,
                           const std::vector<int>& p_net_no_arr,
                           int p_clearance_class,
                           int p_id_no,
                           int p_component_no,
                           const std::string& p_name,
                           FixedState p_fixed_state,
                           BasicBoard* p_board)
    : Item(p_net_no_arr, p_clearance_class, p_id_no, p_component_no, p_fixed_state, p_board)
    , name_(p_name)
    , relative_area_(p_area)
    , layer_(p_layer)
    , translation_(std::make_shared<geometry::planar::IntVector>(0, 0))
    , rotation_in_degree_(p_rotation_in_degree)
    , side_changed_(p_side_changed)
{
    const auto* int_vector = dynamic_cast<const geometry::planar::IntVector*>(&p_translation);
    if (int_vector != nullptr)
    {
        translation_ = std::make_shared<geometry::planar::IntVector>(*int_vector);
    }
    else
    {
        logger::FRLogger::instance().warn("ObstacleArea: only IntVector translations are supported");
    }
}

ObstacleArea::ObstacleArea(const std::shared_ptr<geometry::planar::Area>& p_area,
                           int p_layer,
                           const geometry::planar::Vector& p_translation,
                           double p_rotation_in_degree,
                           bool p_side_changed,
                           int p_clearance_class,
                           int p_id_no,
                           int p_component_no,
                           const std::string& p_name,
                           FixedState p_fixed_state,
                           BasicBoard* p_board)
    : ObstacleArea(p_area, p_layer, p_translation, p_rotation_in_degree, p_side_changed, {}, p_clearance_class,
                   p_id_no, p_component_no, p_name, p_fixed_state, p_board)
{
}

std::unique_ptr<Item> ObstacleArea::copy(int p_id_no) const
{
    return std::make_unique<ObstacleArea>(relative_area_, layer_, *translation_, rotation_in_degree_, side_changed_,
        net_nos_, clearance_class_, p_id_no, component_no_, name_, fixed_state_, board_);
}

std::shared_ptr<geometry::planar::Area> ObstacleArea::get_area() const
{
    if (precalculated_absolute_area_)
    {
        return precalculated_absolute_area_;
    }
    if (!relative_area_)
    {
        logger::FRLogger::instance().warn("ObstacleArea.get_area: area is null");
        return nullptr;
    }
    std::shared_ptr<geometry::planar::Area> turned_area = relative_area_;
    const geometry::planar::IntPoint zero(0, 0);
    const geometry::planar::FloatPoint zero_float(0.0, 0.0);

    if (side_changed_ && board_ != nullptr && !board_->get_flip_style_rotate_first())
    {
        turned_area = turned_area->mirror_vertical(zero);
    }
    if (rotation_in_degree_ != 0.0)
    {
        if (std::fmod(rotation_in_degree_, 90.0) == 0.0)
        {
            turned_area = turned_area->turn_90_degree(static_cast<int>(rotation_in_degree_) / 90, zero);
        }
        else
        {
            turned_area = turned_area->rotate_approx(rotation_in_degree_ * M_PI / 180.0, zero_float);
        }
    }
    if (side_changed_ && board_ != nullptr && board_->get_flip_style_rotate_first())
    {
        turned_area = turned_area->mirror_vertical(zero);
    }
    precalculated_absolute_area_ = turned_area->translate_by(*translation_);
    return precalculated_absolute_area_;
}

std::shared_ptr<geometry::planar::Area> ObstacleArea::get_relative_area() const
{
    return relative_area_;
}

bool ObstacleArea::is_on_layer(int p_layer) const
{
    return layer_ == p_layer;
}

int ObstacleArea::first_layer() const
{
    return layer_;
}

int ObstacleArea::last_layer() const
{
    return layer_;
}

int ObstacleArea::get_layer() const
{
    return layer_;
}

geometry::planar::IntBox ObstacleArea::bounding_box() const
{
    std::shared_ptr<geometry::planar::Area> area = get_area();
    if (!area)
    {
        return geometry::planar::IntBox();
    }
    return area->bounding_box();
}

bool ObstacleArea::is_obstacle(const Item* p_other) const
{
    if (p_other == nullptr)
    {
        return true;
    }
    if (shares_net(p_other))
    {
        return false;
    }
    return p_other->is_trace() || p_other->is_via();
}

const std::vector<std::unique_ptr<geometry::planar::TileShape>>& ObstacleArea::get_split_to_convex() const
{
    if (!relative_area_)
    {
        logger::FRLogger::instance().warn("ObstacleArea.split_to_convex: area is null");
        static const std::vector<std::unique_ptr<geometry::planar::TileShape>> empty_vect;
        return empty_vect;
    }
    return get_area()->split_to_convex();
}

int ObstacleArea::tile_shape_count() const
{
    return static_cast<int>(get_split_to_convex().size());
}

geometry::planar::IntOctagon ObstacleArea::get_tile_shape(int p_index) const
{
    const std::vector<std::unique_ptr<geometry::planar::TileShape>>& tile_shapes = get_split_to_convex();
    if (p_index < 0 || static_cast<size_t>(p_index) >= tile_shapes.size())
    {
        logger::FRLogger::instance().warn("ObstacleArea.get_tile_shape: p_index out of range");
        return geometry::planar::IntOctagon();
    }
    return tile_shapes[static_cast<size_t>(p_index)]->bounding_box().bounding_octagon();
}

int ObstacleArea::shape_layer(int p_index) const
{
    (void)p_index;
    return layer_;
}

void ObstacleArea::translate_by(const geometry::planar::Vector& p_vector)
{
    translation_ = translation_->add(p_vector);
    clear_derived_data();
}

void ObstacleArea::turn_90_degree(int p_factor, const geometry::planar::IntPoint& p_pole)
{
    rotation_in_degree_ += p_factor * 90.0;
    while (rotation_in_degree_ >= 360.0)
    {
        rotation_in_degree_ -= 360.0;
    }
    while (rotation_in_degree_ < 0.0)
    {
        rotation_in_degree_ += 360.0;
    }
    const auto* translation = dynamic_cast<const geometry::planar::IntVector*>(translation_.get());
    if (translation == nullptr)
    {
        logger::FRLogger::instance().warn("ObstacleArea.turn_90_degree: only IntVector translations are supported");
        return;
    }
    const geometry::planar::IntPoint rel_location(translation->x, translation->y);
    const geometry::planar::IntPoint turned_location = rel_location.turn_90_degree(p_factor, p_pole);
    translation_ = std::make_shared<geometry::planar::IntVector>(turned_location.x, turned_location.y);
    clear_derived_data();
}

void ObstacleArea::rotate_approx(double p_angle_in_degree, const geometry::planar::FloatPoint& p_pole)
{
    double turn_angle = p_angle_in_degree;
    if (side_changed_ && board_ != nullptr && board_->get_flip_style_rotate_first())
    {
        turn_angle = 360.0 - p_angle_in_degree;
    }
    rotation_in_degree_ += turn_angle;
    while (rotation_in_degree_ >= 360.0)
    {
        rotation_in_degree_ -= 360.0;
    }
    while (rotation_in_degree_ < 0.0)
    {
        rotation_in_degree_ += 360.0;
    }
    const geometry::planar::FloatPoint new_translation =
        translation_->to_float().rotate(p_angle_in_degree * M_PI / 180.0, p_pole);
    const geometry::planar::IntPoint rounded = new_translation.round();
    translation_ = std::make_shared<geometry::planar::IntVector>(rounded.x, rounded.y);
    clear_derived_data();
}

void ObstacleArea::change_placement_side(const geometry::planar::IntPoint& p_pole)
{
    side_changed_ = !side_changed_;
    if (board_ != nullptr)
    {
        layer_ = board_->get_layer_count() - layer_ - 1;
    }
    const auto* translation = dynamic_cast<const geometry::planar::IntVector*>(translation_.get());
    if (translation == nullptr)
    {
        logger::FRLogger::instance().warn("ObstacleArea.change_placement_side: only IntVector translations are supported");
        return;
    }
    const geometry::planar::IntPoint rel_location(translation->x, translation->y);
    const geometry::planar::IntPoint mirrored_location = rel_location.mirror_vertical(p_pole);
    translation_ = std::make_shared<geometry::planar::IntVector>(mirrored_location.x, mirrored_location.y);
    clear_derived_data();
}

bool ObstacleArea::is_selected_by_filter(const ItemSelectionFilter& p_filter) const
{
    if (!this->is_selected_by_fixed_filter(p_filter))
    {
        return false;
    }
    return p_filter.is_selected(ItemSelectionFilter::SelectableChoices::KEEPOUT);
}

void ObstacleArea::clear_derived_data()
{
    precalculated_absolute_area_.reset();
}

std::string ObstacleArea::to_string() const
{
    std::string result = name_.empty() ? "ObstacleArea" : name_;
    result += "[layer=" + std::to_string(layer_) + ", rotation=" + std::to_string(rotation_in_degree_) + "]";
    return result;
}

} // namespace freerouting::board