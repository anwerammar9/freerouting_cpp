#ifndef AUTOROUTE_MAZE_SEARCH_ELEMENT_H
#define AUTOROUTE_MAZE_SEARCH_ELEMENT_H

#include <vector>

namespace freerouting::autoroute {

class ExpansionDoor;
class ExpansionRoom;

class MazeSearchElement {
public:
    enum class Adjustment {
        NONE,
        LEFT,
        RIGHT
    };

    MazeSearchElement();

    bool is_occupied = false;
    ExpansionDoor* backtrack_door = nullptr;
    int section_no_of_backtrack_door = 0;
    bool room_ripped = false;
    Adjustment adjustment = Adjustment::NONE;

    void reset();

    static constexpr int SECTION_COUNT_MAX = 64;
    static void reset_section_array(std::vector<MazeSearchElement>& p_arr);
};

class MazeListElement {
public:
    ExpansionDoor* door = nullptr;
    int section_no_of_door = 0;
    ExpansionDoor* backtrack_door = nullptr;
    int section_no_of_backtrack_door = 0;
    double expansion_value = 0.0;
    double sorting_value = 0.0;
    ExpansionRoom* next_room = nullptr;
    double shape_entry_x = 0.0;
    double shape_entry_y = 0.0;
    bool already_checked = false;
    bool room_ripped = false;
    MazeSearchElement::Adjustment adjustment = MazeSearchElement::Adjustment::NONE;

    MazeListElement() = default;

    static MazeListElement obtain(ExpansionDoor* p_door,
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
                                  bool p_already_checked);

    bool operator<(const MazeListElement& p_other) const;
};

struct MazeListElementComparator {
    bool operator()(const MazeListElement& a, const MazeListElement& b) const {
        return a.sorting_value > b.sorting_value;
    }
};

} // namespace freerouting::autoroute

#endif // AUTOROUTE_MAZE_SEARCH_ELEMENT_H