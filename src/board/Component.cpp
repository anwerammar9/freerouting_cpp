#include "board/Component.h"

#include "logger/FRLogger.h"

namespace freerouting::board {

namespace {

double normalize_rotation(double p_rotation)
{
    while (p_rotation >= 360)
    {
        p_rotation -= 360;
    }
    while (p_rotation < 0)
    {
        p_rotation += 360;
    }
    return p_rotation;
}

} // namespace

Component::Component(std::string p_name, std::optional<geometry::planar::IntPoint> p_location,
    double p_rotation_in_degree, bool p_on_front, Package* p_package_front, Package* p_package_back, int p_id,
    bool p_position_fixed, std::string p_part_number)
    : name_(std::move(p_name))
    , id_(p_id)
    , position_fixed_(p_position_fixed)
    , package_front_(p_package_front)
    , package_back_(p_package_back)
    , part_number_(std::move(p_part_number))
    , location_(p_location)
    , rotation_in_degree_(normalize_rotation(p_rotation_in_degree))
    , on_front_(p_on_front)
{
}

void Component::translate_by(const geometry::planar::IntVector& p_vector)
{
    if (location_)
    {
        location_ = geometry::planar::IntPoint(location_->x + p_vector.x, location_->y + p_vector.y);
    }
}

void Component::turn_90_degree(int p_factor, const geometry::planar::IntPoint& p_pole)
{
    if (p_factor == 0)
    {
        return;
    }
    rotation_in_degree_ = normalize_rotation(rotation_in_degree_ + p_factor * 90);
    if (location_)
    {
        location_ = location_->turn_90_degree(p_factor, p_pole);
    }
}

void Component::change_side(const geometry::planar::IntPoint& p_pole)
{
    on_front_ = !on_front_;
    if (location_)
    {
        location_ = location_->mirror_vertical(p_pole);
    }
}

Component* Components::add(const std::string& p_name, std::optional<geometry::planar::IntPoint> p_location,
    double p_rotation_in_degree, bool p_on_front, Package* p_package_front, Package* p_package_back,
    bool p_position_fixed, const std::string& p_part_number)
{
    components_.push_back(std::make_unique<Component>(p_name, p_location, p_rotation_in_degree, p_on_front,
        p_package_front, p_package_back, component_count() + 1, p_position_fixed, p_part_number));
    return components_.back().get();
}

Component* Components::get(int p_component_id) const
{
    if (p_component_id < 1 || p_component_id > component_count())
    {
        // Java throws ArrayIndexOutOfBoundsException.
        return nullptr;
    }
    Component* result = components_[p_component_id - 1].get();
    if (result->get_id() != p_component_id)
    {
        logger::FRLogger::instance().warn("Components.get: inconsistent component ID");
    }
    return result;
}

Component* Components::get_by_name(const std::string& p_name) const
{
    for (const auto& current : components_)
    {
        if (current->get_name() == p_name)
        {
            return current.get();
        }
    }
    return nullptr;
}

std::string Components::to_string() const
{
    return "Components[count=" + std::to_string(component_count()) + "]";
}

} // namespace freerouting::board
