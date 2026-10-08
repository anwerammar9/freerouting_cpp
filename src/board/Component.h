#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "board/Package.h"
#include "geometry/planar/Point.h"

namespace freerouting::board {

/**
 * A component placed on the board. Port of Java
 * board.model.structure.Component (undo snapshots and logical parts are not
 * ported yet).
 */
class Component
{
public:
    /**
     * Creates a component. p_location is std::nullopt for an unplaced
     * component. The rotation is normalized to [0, 360). Port of the Java
     * Component constructor; use Components::add to get a valid id.
     */
    Component(std::string p_name, std::optional<geometry::planar::IntPoint> p_location, double p_rotation_in_degree,
        bool p_on_front, Package* p_package_front, Package* p_package_back, int p_id, bool p_position_fixed,
        std::string p_part_number = {});

    const std::string& get_name() const { return name_; }
    /** The component ID (1-based). Java Component.id. */
    int get_id() const { return id_; }
    int get_no() const { return id_; }
    bool is_position_fixed() const { return position_fixed_; }
    const std::string& get_part_number() const { return part_number_; }

    /** True, if the component has a location. Java Component.isPlaced. */
    bool is_placed() const { return location_.has_value(); }
    /** The location of the component; (0, 0) for an unplaced component (Java returns null). */
    geometry::planar::IntPoint get_location() const { return location_.value_or(geometry::planar::IntPoint(0, 0)); }
    double get_rotation() const { return rotation_in_degree_; }
    bool is_placed_on_front() const { return on_front_; }

    /** The package of the side the component is placed on. Java Component.getPackage. */
    Package* get_package() const { return on_front_ ? package_front_ : package_back_; }

    /** Java Component.translateBy. */
    void translate_by(const geometry::planar::IntVector& p_vector);
    /** Java Component.turn90Degree. */
    void turn_90_degree(int p_factor, const geometry::planar::IntPoint& p_pole);
    /** Java Component.changeSide. */
    void change_side(const geometry::planar::IntPoint& p_pole);

    std::string to_string() const { return name_; }

private:
    std::string name_;
    int id_;
    bool position_fixed_;
    Package* package_front_;
    Package* package_back_;
    std::string part_number_;
    std::optional<geometry::planar::IntPoint> location_;
    double rotation_in_degree_;
    bool on_front_;
};

/** The components of a board. Port of Java board.model.structure.Components. */
class Components
{
public:
    Components() = default;

    /**
     * Inserts a component; its ID is component_count() + 1. The items of the
     * component have to be inserted separately into the board. Java Components.add.
     */
    Component* add(const std::string& p_name, std::optional<geometry::planar::IntPoint> p_location,
        double p_rotation_in_degree, bool p_on_front, Package* p_package_front, Package* p_package_back,
        bool p_position_fixed, const std::string& p_part_number = {});

    /** Returns the component with the input ID (1 <= id <= count), or nullptr. Java Components.get(int). */
    Component* get(int p_component_id) const;
    /** Returns the component with the input name, or nullptr. Java Components.get(String). */
    Component* get_by_name(const std::string& p_name) const;

    int component_count() const { return static_cast<int>(components_.size()); }

    std::string to_string() const;

private:
    std::vector<std::unique_ptr<Component>> components_;
};

} // namespace freerouting::board
