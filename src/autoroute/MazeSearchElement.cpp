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
    ripup_cost = 0;
}

void MazeSearchElement::reset_section_array(std::vector<MazeSearchElement>& p_arr)
{
    for (auto& elem : p_arr)
    {
        elem.reset();
    }
}

MazeListElement::MazeListElement(ExpandableObject* p_door,
                                 int p_section_no_of_door,
                                 ExpandableObject* p_backtrack_door,
                                 int p_section_no_of_backtrack_door,
                                 double p_expansion_value,
                                 double p_sorting_value,
                                 CompleteExpansionRoom* p_next_room,
                                 const geometry::planar::FloatLine& p_shape_entry,
                                 bool p_room_ripped,
                                 MazeSearchElement::Adjustment p_adjustment,
                                 bool p_already_checked)
    : door(p_door)
    , section_no_of_door(p_section_no_of_door)
    , backtrack_door(p_backtrack_door)
    , section_no_of_backtrack_door(p_section_no_of_backtrack_door)
    , expansion_value(p_expansion_value)
    , sorting_value(p_sorting_value)
    , next_room(p_next_room)
    , shape_entry(p_shape_entry)
    , room_ripped(p_room_ripped)
    , adjustment(p_adjustment)
    , already_checked(p_already_checked)
{
}

int MazeListElement::compare_to(const MazeListElement& p_other) const
{
    if (sorting_value < p_other.sorting_value)
    {
        return -1;
    }
    if (sorting_value > p_other.sorting_value)
    {
        return 1;
    }
    // Tie-break 1: expansionValue
    if (expansion_value < p_other.expansion_value)
    {
        return -1;
    }
    if (expansion_value > p_other.expansion_value)
    {
        return 1;
    }
    // Tie-break 2: door id
    const int64_t id1 = door != nullptr ? door->get_id() : 0;
    const int64_t id2 = p_other.door != nullptr ? p_other.door->get_id() : 0;
    if (id1 < id2)
    {
        return -1;
    }
    if (id1 > id2)
    {
        return 1;
    }
    // Tie-break 3: sectionIndex
    if (section_no_of_door < p_other.section_no_of_door)
    {
        return -1;
    }
    if (section_no_of_door > p_other.section_no_of_door)
    {
        return 1;
    }
    // If truly equal (same door, same section, same values), the element is the
    // same list entry.
    return 0;
}

} // namespace freerouting::autoroute
