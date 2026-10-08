#pragma once

#include <memory>
#include <string>
#include <vector>

#include "board/Padstack.h"

namespace freerouting::board {

/** The list of padstacks of a board library. Port of Java core.library.Padstacks. */
class Padstacks
{
public:
    explicit Padstacks(int p_board_layer_count);

    /** Returns the padstack with the input name (ignoring case), or nullptr. Java Padstacks.get(String). */
    Padstack* get(const std::string& p_name) const;

    /** Returns the padstack with the input ID (1 <= id <= count()), or nullptr. Java Padstacks.get(int). */
    Padstack* get(int p_padstack_id) const;

    int count() const { return static_cast<int>(padstacks_.size()); }
    int board_layer_count() const { return board_layer_count_; }

    /** Appends a new padstack with ID count() + 1. Java Padstacks.add(String, ConvexShape[], boolean, boolean). */
    Padstack* add(const std::string& p_name, std::vector<std::unique_ptr<geometry::planar::Shape>> p_shapes,
        bool p_drill_allowed, bool p_placed_absolute);

    /** Appends a new padstack named "padstack#<id>". Java Padstacks.add(ConvexShape[]). */
    Padstack* add(std::vector<std::unique_ptr<geometry::planar::Shape>> p_shapes);

private:
    int board_layer_count_;
    std::vector<std::unique_ptr<Padstack>> padstacks_;
};

} // namespace freerouting::board
