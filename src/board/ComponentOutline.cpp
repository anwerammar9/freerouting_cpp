#include "board/ComponentOutline.h"
#include "geometry/planar/Shape.h"

#include "board/ItemSelectionFilter.h"
#include "board/BasicBoard.h"
#include "logger/FRLogger.h"

#include <cmath>

namespace freerouting::board
{

ComponentOutline::ComponentOutline(const std::shared_ptr<geometry::planar::Area>& p_area,
                                   bool p_is_front,
                                   const geometry::planar::Vector& p_translation,
                                   double p_rotation_in_degree,
                                   int p_id_no,
                                   int p_component_no,
                                   FixedState p_fixed_state,
                                   BasicBoard* p_board)
    : Item({}, 0, p_id_no, p_component_no, p_fixed_state, p_board)
    , relative_area_(p_area)
    , translation_(std::make_shared<geometry::planar::IntVector>(0, 0))
    , rotation_in_degree_(p_rotation_in_degree)
    , is_front_(p_is_front)
{
    const auto* int_vector = dynamic_cast<const geometry::planar::IntVector*>(&p_translation);
    if (int_vector != nullptr)
    {
        translation_ = std::make_shared<geometry::planar::IntVector>(*int_vector);
    }
    else
    {
        logger::FRLogger::instance().warn("ComponentOutline: only IntVector translations are supported");
    }
}

std::unique_ptr<Item> ComponentOutline::copy(int p_id_no) const
{
    return std::make_unique<ComponentOutline>(relative_area_, is_front_, *translation_,
                                              rotation_in_degree_, p_id_no, component_no_,
                                              fixed_state_, board_);
}

std::shared_ptr<geometry::planar::Area> ComponentOutline::get_area() const
{
    if (precalculated_absolute_area_)
    {
        return precalculated_absolute_area_;
    }
    if (!relative_area_)
    {
        logger::FRLogger::instance().warn("ComponentOutline.get_area: area is null");
        return nullptr;
    }
    std::shared_ptr<geometry::planar::Area> turned_area = relative_area_;
    const geometry::planar::IntPoint zero(0, 0);
    const geometry::planar::FloatPoint zero_float(0.0, 0.0);

    const bool flip_rotate_first = board_ != nullptr && board_->get_flip_style_rotate_first();
    if (!is_front_ && !flip_rotate_first)
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
    if (!is_front_ && flip_rotate_first)
    {
        turned_area = turned_area->mirror_vertical(zero);
    }
    precalculated_absolute_area_ = turned_area->translate_by(*translation_);
    return precalculated_absolute_area_;
}

std::shared_ptr<geometry::planar::Area> ComponentOutline::get_relative_area() const
{
    return relative_area_;
}

int ComponentOutline::get_layer() const
{
    if (is_front_ || board_ == nullptr || board_->get_layer_count() <= 1)
    {
        return 0;
    }
    return board_->get_layer_count() - 1;
}

int ComponentOutline::first_layer() const
{
    return get_layer();
}

int ComponentOutline::last_layer() const
{
    return get_layer();
}

bool ComponentOutline::is_on_layer(int p_layer) const
{
    return get_layer() == p_layer;
}

geometry::planar::IntBox ComponentOutline::bounding_box() const
{
    std::shared_ptr<geometry::planar::Area> area = get_area();
    if (!area)
    {
        return geometry::planar::IntBox();
    }
    return area->bounding_box();
}

bool ComponentOutline::is_obstacle(const Item* p_other) const
{
    (void)p_other;
    return false;
}

int ComponentOutline::shape_layer(int p_index) const
{
    (void)p_index;
    return get_layer();
}

int ComponentOutline::tile_shape_count() const
{
    return 0;
}

void ComponentOutline::translate_by(const geometry::planar::Vector& p_vector)
{
    translation_ = translation_->add(p_vector);
    clear_derived_data();
}

void ComponentOutline::turn_90_degree(int p_factor, const geometry::planar::IntPoint& p_pole)
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
        logger::FRLogger::instance().warn("ComponentOutline.turn_90_degree: only IntVector translations are supported");
        return;
    }
    const geometry::planar::IntPoint rel_location(translation->x, translation->y);
    const geometry::planar::IntPoint turned_location = rel_location.turn_90_degree(p_factor, p_pole);
    translation_ = std::make_shared<geometry::planar::IntVector>(turned_location.x, turned_location.y);
    clear_derived_data();
}

void ComponentOutline::rotate_approx(double p_angle_in_degree, const geometry::planar::FloatPoint& p_pole)
{
    double turn_angle = p_angle_in_degree;
    if (!is_front_ && board_ != nullptr && board_->get_flip_style_rotate_first())
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

void ComponentOutline::change_placement_side(const geometry::planar::IntPoint& p_pole)
{
    is_front_ = !is_front_;
    const auto* translation = dynamic_cast<const geometry::planar::IntVector*>(translation_.get());
    if (translation == nullptr)
    {
        logger::FRLogger::instance().warn("ComponentOutline.change_placement_side: only IntVector translations are supported");
        return;
    }
    const geometry::planar::IntPoint rel_location(translation->x, translation->y);
    const geometry::planar::IntPoint mirrored_location = rel_location.mirror_vertical(p_pole);
    translation_ = std::make_shared<geometry::planar::IntVector>(mirrored_location.x, mirrored_location.y);
    clear_derived_data();
}

bool ComponentOutline::is_selected_by_filter(const ItemSelectionFilter& p_filter) const
{
    (void)p_filter;
    return false;
}

void ComponentOutline::clear_derived_data()
{
    precalculated_absolute_area_.reset();
}

std::string ComponentOutline::to_string() const
{
    std::string result = "ComponentOutline";
    result += "[layer=" + std::to_string(get_layer()) + ", rotation=" + std::to_string(rotation_in_degree_) + "]";
    return result;
}

} // namespace freerouting::board