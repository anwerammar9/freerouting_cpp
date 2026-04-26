#include "autoroute/MazeSearchElement.h"

namespace freerouting::autoroute {

MazeSearchElement::MazeSearchElement() = default;

void MazeSearchElement::reset()
{
    is_occupied = false;
    backtrack_door = nullptr;
    section_no_of_backtrack_door = 0;
    room_ripped = false;
    adjustment = Adjustment::NONE;
}

void MazeSearchElement::reset_section_array(std::vector<MazeSearchElement>& p_arr)
{
    for (auto& elem : p_arr)
    {
        elem.reset();
    }
}

MazeListElement MazeListElement::obtain(ExpansionDoor* p_door,
                                        int p_section_no_of_door,
                                        ExpansionDoor* p_backtrack_door,
                                        int p_section_no_of_backtrack_door,
                                        double p_expansion_value,
                                        double p_sorting_value,
                                        ExpansionRoom* p_next_room,
                                        double p_shape_entry_x,
                                        double p_shape_entry_y,
                                        bool p_room_ripped,
                                        MazeSearchElement::Adjustment p_adjustment,
                                        bool p_already_checked)
{
    MazeListElement result;
    result.door = p_door;
    result.section_no_of_door = p_section_no_of_door;
    result.backtrack_door = p_backtrack_door;
    result.section_no_of_backtrack_door = p_section_no_of_backtrack_door;
    result.expansion_value = p_expansion_value;
    result.sorting_value = p_sorting_value;
    result.next_room = p_next_room;
    result.shape_entry_x = p_shape_entry_x;
    result.shape_entry_y = p_shape_entry_y;
    result.already_checked = p_already_checked;
    result.room_ripped = p_room_ripped;
    result.adjustment = p_adjustment;
    return result;
}

bool MazeListElement::operator<(const MazeListElement& p_other) const
{
    return sorting_value < p_other.sorting_value;
}

} // namespace freerouting::autoroute