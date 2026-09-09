#include "board/Component.h"

#include <sstream>

namespace freerouting::board {

Component::Component(const std::string& p_name,
                        int p_no,
                        core::Package* p_package,
                        bool p_position_fixed)
    : name_(p_name)
    , no_(p_no)
    , position_fixed_(p_position_fixed)
    , location_x_(0)
    , location_y_(0)
    , rotation_in_degree_(0)
    , package_(p_package)
    , placed_(false)
    , pin_count_(0)
{
}

void Component::set_location(int64_t p_x, int64_t p_y)
{
    location_x_ = p_x;
    location_y_ = p_y;
}

std::string Component::to_string() const
{
    return "Component[" + name_ + "]";
}

Components::Components()
{
}

void Components::add(Component* p_component)
{
    if (!p_component)
        return;

    components_.push_back(p_component);
    names_.push_back(p_component->get_name());
}

Component* Components::get(int p_no) const
{
    if (p_no >= 0 && p_no < component_count())
    {
        return components_[p_no];
    }
    return nullptr;
}

Component* Components::get_by_name(const std::string& p_name) const
{
    for (size_t i = 0; i < names_.size(); ++i)
    {
        if (names_[i] == p_name)
            return components_[i];
    }
    return nullptr;
}

std::string Components::to_string() const
{
    std::ostringstream ss;
    ss << "Components[count=" << component_count() << "]";
    return ss.str();
}

} // namespace freerouting::board