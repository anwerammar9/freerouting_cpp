// Rules functions that read board::Padstack shapes or change board::Item clearance classes. They are declared in
// rules/BoardRules.h and rules/ViaRule.h but compiled into freerouting_board,
// because the padstack lives in the board library (which already depends on
// the rules library; the reverse link dependency would be cyclic).

#include <algorithm>

#include "board/Item.h"
#include "board/Padstack.h"
#include "geometry/planar/ConvexShape.h"
#include "geometry/planar/Shape.h"
#include "rules/BoardRules.h"
#include "rules/ViaRule.h"

namespace freerouting {
namespace rules {

namespace {

/** Java ConvexShape.maxWidth for the C++ shape types of a padstack. */
double shape_max_width(const geometry::planar::Shape& p_shape)
{
    if (const auto* convex = dynamic_cast<const geometry::planar::ConvexShape*>(&p_shape))
    {
        return convex->max_width();
    }
    if (const auto* circle = dynamic_cast<const geometry::planar::Circle*>(&p_shape))
    {
        return circle->max_width();
    }
    return p_shape.bounding_box().max_width();
}

} // namespace

std::shared_ptr<ViaInfo> ViaRule::get_layer_range(int p_from_layer, int p_to_layer) const
{
    for (const auto& curr_via : via_list_)
    {
        const board::Padstack* padstack = curr_via->get_padstack();
        if (padstack->get_from_layer() == p_from_layer && padstack->get_to_layer() == p_to_layer)
        {
            return curr_via;
        }
    }
    return nullptr;
}

void BoardRules::create_default_via_rule(NetClass* p_net_class, const std::string& p_name)
{
    if (via_infos_.count() == 0)
    {
        return;
    }
    auto default_rule = std::make_unique<ViaRule>(p_name);
    const int default_via_cl_class = p_net_class->get_default_item_clearance_classes().get(ItemClass::VIA);
    for (int i = 0; i < via_infos_.count(); ++i)
    {
        std::shared_ptr<ViaInfo> curr_via_info = via_infos_.get(i);
        if (curr_via_info->get_clearance_class() != default_via_cl_class)
        {
            continue;
        }
        const board::Padstack* curr_padstack = curr_via_info->get_padstack();
        const int curr_from_layer = curr_padstack->get_from_layer();
        const int curr_to_layer = curr_padstack->get_to_layer();
        std::shared_ptr<ViaInfo> existing_via = default_rule->get_layer_range(curr_from_layer, curr_to_layer);
        if (existing_via != nullptr)
        {
            const geometry::planar::Shape* new_shape = curr_padstack->get_shape(curr_from_layer);
            const geometry::planar::Shape* existing_shape = existing_via->get_padstack()->get_shape(curr_from_layer);
            if (shape_max_width(*new_shape) < shape_max_width(*existing_shape))
            {
                default_rule->remove_via(existing_via);
                default_rule->append_via(curr_via_info);
            }
        }
        else
        {
            default_rule->append_via(curr_via_info);
        }
    }
    p_net_class->set_via_rule(add_via_rule(std::move(default_rule)));
}

double BoardRules::get_default_via_diameter() const
{
    const ViaRule* default_via_rule = get_default_via_rule();
    if (default_via_rule == nullptr || default_via_rule->via_count() <= 0)
    {
        return 0;
    }
    const board::Padstack* via_padstack = default_via_rule->get_via(0)->get_padstack();
    const geometry::planar::Shape* curr_shape = via_padstack->get_shape(via_padstack->get_from_layer());
    double result = curr_shape != nullptr ? shape_max_width(*curr_shape) : 0;
    curr_shape = via_padstack->get_shape(via_padstack->get_to_layer());
    return curr_shape != nullptr ? std::max(result, shape_max_width(*curr_shape)) : result;
}

void BoardRules::change_clearance_class_index(int p_from_index, int p_to_index,
    const std::vector<board::Item*>& p_board_items)
{
    for (board::Item* curr_item : p_board_items)
    {
        if (curr_item->get_clearance_class() == p_from_index)
        {
            curr_item->set_clearance_class(p_to_index);
        }
    }
    for (int i = 0; i < net_classes_.count(); ++i)
    {
        NetClass* curr_net_class = net_classes_.get(i);
        if (curr_net_class->get_trace_clearance_class() == p_from_index)
        {
            curr_net_class->set_trace_clearance_class(p_to_index);
        }
        DefaultItemClearanceClasses& item_classes = curr_net_class->get_default_item_clearance_classes();
        for (int k = 0; k < DefaultItemClearanceClasses::item_class_count; ++k)
        {
            const auto item_class = static_cast<ItemClass>(k);
            if (item_classes.get(item_class) == p_from_index)
            {
                item_classes.set(item_class, p_to_index);
            }
        }
    }
    for (int i = 0; i < via_infos_.count(); ++i)
    {
        std::shared_ptr<ViaInfo> curr_via = via_infos_.get(i);
        if (curr_via->get_clearance_class() == p_from_index)
        {
            curr_via->set_clearance_class(p_to_index);
        }
    }
}

bool BoardRules::remove_clearance_class(int p_index, const std::vector<board::Item*>& p_board_items)
{
    for (board::Item* curr_item : p_board_items)
    {
        if (curr_item->get_clearance_class() == p_index)
        {
            return false;
        }
    }
    for (int i = 0; i < net_classes_.count(); ++i)
    {
        NetClass* curr_net_class = net_classes_.get(i);
        if (curr_net_class->get_trace_clearance_class() == p_index)
        {
            return false;
        }
        for (int k = 0; k < DefaultItemClearanceClasses::item_class_count; ++k)
        {
            if (curr_net_class->get_default_item_clearance_classes().get(static_cast<ItemClass>(k)) == p_index)
            {
                return false;
            }
        }
    }
    for (int i = 0; i < via_infos_.count(); ++i)
    {
        if (via_infos_.get(i)->get_clearance_class() == p_index)
        {
            return false;
        }
    }
    for (board::Item* curr_item : p_board_items)
    {
        if (curr_item->get_clearance_class() > p_index)
        {
            curr_item->set_clearance_class(curr_item->get_clearance_class() - 1);
        }
    }
    for (int i = 0; i < net_classes_.count(); ++i)
    {
        NetClass* curr_net_class = net_classes_.get(i);
        if (curr_net_class->get_trace_clearance_class() > p_index)
        {
            curr_net_class->set_trace_clearance_class(curr_net_class->get_trace_clearance_class() - 1);
        }
        DefaultItemClearanceClasses& item_classes = curr_net_class->get_default_item_clearance_classes();
        for (int k = 0; k < DefaultItemClearanceClasses::item_class_count; ++k)
        {
            const auto item_class = static_cast<ItemClass>(k);
            const int curr_class_no = item_classes.get(item_class);
            if (curr_class_no > p_index)
            {
                item_classes.set(item_class, curr_class_no - 1);
            }
        }
    }
    for (int i = 0; i < via_infos_.count(); ++i)
    {
        std::shared_ptr<ViaInfo> curr_via = via_infos_.get(i);
        if (curr_via->get_clearance_class() > p_index)
        {
            curr_via->set_clearance_class(curr_via->get_clearance_class() - 1);
        }
    }
    clearance_matrix_->remove_class(p_index);
    return true;
}

} // namespace rules
} // namespace freerouting
