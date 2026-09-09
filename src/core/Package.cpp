#include "core/Package.h"

#include <sstream>

namespace freerouting::core {

Package::Package(const std::string& p_name, int p_no)
    : name_(p_name)
    , no_(p_no)
{
}

void Package::add_pin(int p_number, int64_t p_x, int64_t p_y, int p_layer)
{
    Pin pin;
    pin.number = p_number;
    pin.x = p_x;
    pin.y = p_y;
    pin.layer = p_layer;
    pins_.push_back(pin);
}

const Package::Pin* Package::get_pin(int p_index) const
{
    if (p_index >= 0 && p_index < get_pin_count())
    {
        return &pins_[p_index];
    }
    return nullptr;
}

std::string Package::to_string() const
{
    return "Package[" + name_ + "]";
}

Packages::Packages()
{
}

void Packages::add(Package* p_package)
{
    if (!p_package)
        return;

    packages_.push_back(p_package);
    names_.push_back(p_package->get_name());
}

Package* Packages::get(int p_no) const
{
    if (p_no >= 0 && p_no < count())
    {
        return packages_[p_no];
    }
    return nullptr;
}

Package* Packages::get_by_name(const std::string& p_name) const
{
    for (size_t i = 0; i < names_.size(); ++i)
    {
        if (names_[i] == p_name)
            return packages_[i];
    }
    return nullptr;
}

} // namespace freerouting::core