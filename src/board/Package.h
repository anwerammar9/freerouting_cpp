#pragma once

#include <memory>
#include <string>
#include <vector>

#include "geometry/planar/Shape.h"
#include "geometry/planar/Vector.h"

namespace freerouting::board {

class Packages;

/**
 * Component package template describing the padstacks and relative locations
 * of the package pins, and optional other stuff like an outline and package
 * keepouts. Port of Java core.library.Package.
 */
class Package
{
public:
    /** Describes a pin padstack of a package. Port of Java Package.Pin. */
    struct Pin
    {
        /** The name of the pin. */
        std::string name;
        /** The ID of the padstack mask of the pin (1-based, see Padstacks). */
        int padstack_id = 0;
        /** The location of the pin relative to its package. */
        geometry::planar::IntVector relative_location{0, 0};
        /** The rotation of the pin padstack. */
        double rotation_in_degree = 0.0;
    };

    /** Describes a named keepout belonging to a package. Port of Java Package.Keepout. */
    struct Keepout
    {
        std::string name;
        std::shared_ptr<geometry::planar::Area> area;
        int layer = -1;
    };

    Package(std::string p_name, int p_id, std::vector<Pin> p_pins,
        std::vector<std::shared_ptr<geometry::planar::Shape>> p_outline, std::vector<double> p_outline_widths,
        std::vector<bool> p_outline_is_closed, std::vector<Keepout> p_keepouts, std::vector<Keepout> p_via_keepouts,
        std::vector<Keepout> p_place_keepouts, bool p_is_front);

    const std::string& get_name() const { return name_; }
    int get_id() const { return id_; }
    bool is_front() const { return is_front_; }

    /** Returns the pin with the input index from this package, or nullptr. Java Package.getPin. */
    const Pin* get_pin(int p_pin_index) const;

    /** Returns the index of the pin named p_name, or -1. Java Package.getPinIndex. */
    int get_pin_index(const std::string& p_name) const;

    int pin_count() const { return static_cast<int>(pins_.size()); }

    const std::vector<std::shared_ptr<geometry::planar::Shape>>& outline() const { return outline_; }
    const std::vector<double>& outline_widths() const { return outline_widths_; }
    const std::vector<bool>& outline_is_closed() const { return outline_is_closed_; }
    const std::vector<Keepout>& keepouts() const { return keepouts_; }
    const std::vector<Keepout>& via_keepouts() const { return via_keepouts_; }
    const std::vector<Keepout>& place_keepouts() const { return place_keepouts_; }

    std::string to_string() const { return name_; }

private:
    std::string name_;
    int id_;
    std::vector<Pin> pins_;
    std::vector<std::shared_ptr<geometry::planar::Shape>> outline_;
    std::vector<double> outline_widths_;
    std::vector<bool> outline_is_closed_;
    std::vector<Keepout> keepouts_;
    std::vector<Keepout> via_keepouts_;
    std::vector<Keepout> place_keepouts_;
    bool is_front_;
};

/** A library of component packages. Port of Java core.library.Packages. */
class Packages
{
public:
    Packages() = default;

    /**
     * Returns the package with the input name and side, or the package with
     * that name on the other side, or nullptr. The name comparison ignores
     * case, and a "::<n>" suffix is retried without it. Java Packages.get(String, boolean).
     */
    Package* get(const std::string& p_name, bool p_is_front) const;

    /** Returns the package with the input ID (IDs start at 1). Java Packages.get(int). */
    Package* get(int p_package_id) const;

    int count() const { return static_cast<int>(packages_.size()); }

    /** Appends a new package; its ID is count() + 1. Java Packages.add. */
    Package* add(const std::string& p_name, std::vector<Package::Pin> p_pins,
        std::vector<std::shared_ptr<geometry::planar::Shape>> p_outline = {}, std::vector<double> p_outline_widths = {},
        std::vector<bool> p_outline_is_closed = {}, std::vector<Package::Keepout> p_keepouts = {},
        std::vector<Package::Keepout> p_via_keepouts = {}, std::vector<Package::Keepout> p_place_keepouts = {},
        bool p_is_front = true);

private:
    std::vector<std::unique_ptr<Package>> packages_;
};

} // namespace freerouting::board
