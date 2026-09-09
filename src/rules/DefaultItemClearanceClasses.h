#pragma once

#include <array>
#include <vector>

namespace freerouting {
namespace rules {

enum class ItemClass
{
    NONE,
    TRACE,
    VIA,
    PIN,
    SMD,
    AREA
};

class DefaultItemClearanceClasses
{
public:
    static constexpr int item_class_count = 6;

    DefaultItemClearanceClasses();
    DefaultItemClearanceClasses(const DefaultItemClearanceClasses& p_classes);

    int get(ItemClass p_item_class) const;
    void set(ItemClass p_item_class, int p_index);
    void set_all(int p_index);

private:
    std::array<int, item_class_count> arr;
};

} // namespace rules
} // namespace freerouting