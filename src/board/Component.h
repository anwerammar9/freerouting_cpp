#pragma once

#include <string>
#include <memory>
#include <vector>

#include "core/Package.h"
#include "geometry/planar/Point.h"

namespace freerouting::board {

class LogicalPart;

class Component
{
public:
    Component(const std::string& p_name,
               int p_no,
               core::Package* p_package,
               bool p_position_fixed);

    const std::string& get_name() const { return name_; }
    int get_no() const { return no_; }
    bool is_position_fixed() const { return position_fixed_; }
    bool is_placed_on_front() const { return placed_on_front_; }
    void set_placed_on_front(bool p_value) { placed_on_front_ = p_value; }

    int64_t get_location_x() const { return location_x_; }
    int64_t get_location_y() const { return location_y_; }
    void set_location(int64_t p_x, int64_t p_y);
    geometry::planar::IntPoint get_location() const
    {
        return geometry::planar::IntPoint(location_x_, location_y_);
    }

    double get_rotation() const { return rotation_in_degree_; }
    void set_rotation(double p_rotation) { rotation_in_degree_ = p_rotation; }

    core::Package* get_package() const { return package_; }
    void set_package(core::Package* p_package) { package_ = p_package; }

    bool is_placed() const { return placed_; }
    void set_placed(bool p_value) { placed_ = p_value; }

    int get_pin_count() const { return pin_count_; }
    void set_pin_count(int p_count) { pin_count_ = p_count; }

    std::string to_string() const;

private:
    std::string name_;
    int no_;
    bool position_fixed_;
    int64_t location_x_;
    int64_t location_y_;
    double rotation_in_degree_;
    core::Package* package_ = nullptr;
    bool placed_ = false;
    bool placed_on_front_ = true;
    int pin_count_ = 0;
};

class Components
{
public:
    Components();

    void add(Component* p_component);
    Component* get(int p_no) const;
    Component* get_by_name(const std::string& p_name) const;

    int component_count() const { return static_cast<int>(components_.size()); }

    std::string to_string() const;

private:
    std::vector<Component*> components_;
    std::vector<std::string> names_;
};

} // namespace freerouting::board