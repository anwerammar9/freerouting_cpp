#include "rules/DefaultItemClearanceClasses.h"

namespace freerouting {
namespace rules {

DefaultItemClearanceClasses::DefaultItemClearanceClasses()
{
    arr.fill(0);
    set_all(1);
}

DefaultItemClearanceClasses::DefaultItemClearanceClasses(const DefaultItemClearanceClasses& p_classes)
    : arr(p_classes.arr)
{
}

int DefaultItemClearanceClasses::get(ItemClass p_item_class) const
{
    return arr[static_cast<int>(p_item_class)];
}

void DefaultItemClearanceClasses::set(ItemClass p_item_class, int p_index)
{
    arr[static_cast<int>(p_item_class)] = p_index;
}

void DefaultItemClearanceClasses::set_all(int p_index)
{
    for (int i = 1; i < item_class_count; ++i)
    {
        arr[i] = p_index;
    }
}

} // namespace rules
} // namespace freerouting