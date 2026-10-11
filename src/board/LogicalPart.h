#pragma once

#include <memory>
#include <string>
#include <vector>

namespace freerouting::board {

/**
 * Describes the logical properties of a part, used for pin and gate swapping. Port of Java
 * core.library.LogicalPart (the GUI printInfo is not ported).
 */
class LogicalPart
{
public:
    /** Java LogicalPart.PartPin; ordered by pin_index (Java compareTo). */
    struct PartPin
    {
        /** The index of the pin in the package. */
        int pin_index = 0;
        std::string pin_name;
        std::string gate_name;
        /** Gates of the part with the same nonzero swap code can be swapped. */
        int gate_swap_code = 0;
        std::string gate_pin_name;
        /** Pins of a gate with the same nonzero swap code can be swapped. */
        int gate_pin_swap_code = 0;
    };

    LogicalPart(std::string p_name, int p_id, std::vector<PartPin> p_part_pins);

    const std::string& get_name() const { return name_; }
    /** The 1-based ID. Java LogicalPart.id. */
    int get_id() const { return id_; }

    int pin_count() const { return static_cast<int>(part_pins_.size()); }
    /** Java getPin: nullptr (with a warning) if p_pin_index is out of range. */
    const PartPin* get_pin(int p_pin_index) const;

private:
    std::string name_;
    int id_;
    std::vector<PartPin> part_pins_;
};

/** The logical parts of a board library. Port of Java core.library.LogicalParts. */
class LogicalParts
{
public:
    /** Java LogicalParts.add: sorts the pins by pin index; the new part's ID is count() + 1. */
    LogicalPart* add(const std::string& p_name, std::vector<LogicalPart::PartPin> p_part_pins);
    /** Java LogicalParts.get(String): case-insensitive name lookup, nullptr if not found. */
    LogicalPart* get(const std::string& p_name) const;
    /** Java LogicalParts.get(int): the part with the 1-based ID, nullptr if out of range. */
    LogicalPart* get(int p_part_id) const;
    int count() const { return static_cast<int>(parts_.size()); }

private:
    std::vector<std::unique_ptr<LogicalPart>> parts_;
};

} // namespace freerouting::board
