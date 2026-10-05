#pragma once

#include <memory>
#include <vector>
#include <string>

#include "board/Item.h"

namespace freerouting {
namespace drc {

class UnconnectedItems
{
public:
    UnconnectedItems(board::Item* p_first,
                    board::Item* p_second);

    UnconnectedItems(board::Item* p_first,
                    board::Item* p_second,
                    const std::vector<board::Item*>& p_all_items);

    /** Creates a new instance with all_items = {p_first, p_second} and a specific type. */
    UnconnectedItems(board::Item* p_first,
                    board::Item* p_second,
                    const std::string& p_type);

    UnconnectedItems(board::Item* p_first,
                    board::Item* p_second,
                    const std::vector<board::Item*>& p_all_items,
                    const std::string& p_type);

    board::Item* get_first() const { return first_; }
    board::Item* get_second() const { return second_; }
    const std::vector<board::Item*>& get_all_items() const { return all_items_; }
    const std::string& get_type() const { return type_; }

private:
    board::Item* first_;
    board::Item* second_;
    std::vector<board::Item*> all_items_;
    std::string type_;
};

} // namespace drc
} // namespace freerouting