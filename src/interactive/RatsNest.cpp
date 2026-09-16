#include "interactive/RatsNest.h"

#include <algorithm>
#include <utility>

#include "board/Component.h"
#include "board/ConductionArea.h"
#include "board/Item.h"
#include "board/Pin.h"
#include "board/RoutingBoard.h"
#include "board/Trace.h"
#include "board/Via.h"
#include "interactive/NetIncompletes.h"
#include "rules/BoardRules.h"
#include "rules/Net.h"
#include "rules/NetClass.h"

namespace freerouting {
namespace interactive {

namespace {

/**
 * A connectable item is an item which can be routed (pins, vias, traces and
 * conduction areas). Mirrors the Java Connectable interface.
 */
bool is_connectable(const board::Item* p_item)
{
    return p_item->is_pin() || p_item->is_via() || p_item->is_trace()
        || dynamic_cast<const board::ConductionArea*>(p_item) != nullptr;
}

/**
 * Returns all connectable items of the net with number p_net_no on the board.
 * Mirrors Java BasicBoard.get_connectable_items(int).
 */
std::vector<board::Item*> connectable_items_of_net(board::RoutingBoard* p_board, int p_net_no)
{
    std::vector<board::Item*> result;
    for (int i = 0; i < p_board->get_item_count(); ++i)
    {
        board::Item* curr_item = p_board->get_item(i);
        if (curr_item != nullptr && curr_item->contains_net(p_net_no) && is_connectable(curr_item))
        {
            result.push_back(curr_item);
        }
    }
    return result;
}

/**
 * Returns the name of the component of the item with the input component
 * number, or an empty string, if the component does not exist.
 */
std::string component_name(board::RoutingBoard* p_board, int p_component_no)
{
    if (p_board == nullptr || p_component_no <= 0)
    {
        return "";
    }
    const board::Components* components = p_board->components();
    if (components == nullptr)
    {
        return "";
    }
    board::Component* component = components->get(p_component_no);
    return component != nullptr ? component->get_name() : "";
}

} // namespace

RatsNest::RatsNest(board::RoutingBoard* p_board) : board_(p_board)
{
    int max_net_no = p_board->get_rules().get_nets().max_net_no();
    // Create the net item lists at once for performance reasons.
    std::vector<std::vector<board::Item*>> net_item_lists(static_cast<size_t>(max_net_no));
    for (int i = 0; i < p_board->get_item_count(); ++i)
    {
        board::Item* curr_item = p_board->get_item(i);
        if (curr_item == nullptr || !is_connectable(curr_item))
        {
            continue;
        }
        const std::vector<int>& net_nos = curr_item->get_net_nos();
        for (size_t j = 0; j < net_nos.size(); ++j)
        {
            int curr_net_no = net_nos[j];
            if (curr_net_no >= 1 && curr_net_no <= max_net_no)
            {
                net_item_lists[static_cast<size_t>(curr_net_no - 1)].push_back(curr_item);
            }
        }
    }
    int total_item_count = 0;
    for (const auto& curr_list : net_item_lists)
    {
        total_item_count += static_cast<int>(curr_list.size());
    }
    this->max_connections = total_item_count - max_net_no;
    this->net_incompletes_.reserve(static_cast<size_t>(max_net_no));
    this->is_filtered_.assign(static_cast<size_t>(max_net_no), false);
    for (int i = 0; i < max_net_no; ++i)
    {
        this->net_incompletes_.push_back(std::make_shared<NetIncompletes>(
            i + 1, net_item_lists[static_cast<size_t>(i)], p_board));
    }
}

RatsNest::AirLine::AirLine(rules::Net* p_net,
                           board::Item* p_from_item,
                           const geometry::planar::FloatPoint& p_from_corner,
                           board::Item* p_to_item,
                           const geometry::planar::FloatPoint& p_to_corner)
    : net(p_net),
      from_item(p_from_item),
      from_corner(p_from_corner),
      to_item(p_to_item),
      to_corner(p_to_corner)
{
}

RatsNestItemInfo RatsNest::AirLine::get_item_info(board::RoutingBoard* p_board,
                                                  board::Item* p_item) const
{
    RatsNestItemInfo result;
    if (auto* pin = dynamic_cast<board::Pin*>(p_item))
    {
        result.type = RatsNestItemType::PIN;
        result.component_name = component_name(p_board, pin->get_component_no());
        result.name = std::to_string(pin->get_pin_no());
        result.text = result.component_name + ", " + result.name;
    }
    else if (auto* via = dynamic_cast<board::Via*>(p_item))
    {
        result.type = RatsNestItemType::VIA;
        result.component_name = component_name(p_board, via->get_component_no());
        result.text = "Via";
    }
    else if (auto* trace = dynamic_cast<board::Trace*>(p_item))
    {
        result.type = RatsNestItemType::TRACE;
        result.component_name = component_name(p_board, trace->get_component_no());
        result.text = "Trace";
    }
    else if (auto* conduction_area = dynamic_cast<board::ConductionArea*>(p_item))
    {
        result.type = RatsNestItemType::CONDUCTION_AREA;
        result.component_name = component_name(p_board, conduction_area->get_component_no());
        result.text = "Conduction Area";
    }
    else
    {
        result.type = RatsNestItemType::UNKNOWN;
        result.text = "Unknown";
    }
    return result;
}

std::string RatsNest::AirLine::to_string(board::RoutingBoard* p_board) const
{
    return std::string(net->get_name()) + ": " + get_item_info(p_board, from_item).text + " - "
        + get_item_info(p_board, to_item).text;
}

void RatsNest::recalculate(int p_net_no, board::RoutingBoard* p_board)
{
    if (p_net_no < 1 || p_net_no > static_cast<int>(this->net_incompletes_.size()))
    {
        return;
    }
    std::vector<board::Item*> item_list = connectable_items_of_net(p_board, p_net_no);
    this->net_incompletes_[static_cast<size_t>(p_net_no - 1)] =
        std::make_shared<NetIncompletes>(p_net_no, item_list, p_board);
}

void RatsNest::recalculate(int p_net_no, const std::vector<board::Item*>& p_item_list,
                           board::RoutingBoard* p_board)
{
    if (p_net_no < 1 || p_net_no > static_cast<int>(this->net_incompletes_.size()))
    {
        return;
    }
    // Copy p_item_list, because it will be changed inside the constructor of
    // NetIncompletes.
    std::vector<board::Item*> item_list = p_item_list;
    this->net_incompletes_[static_cast<size_t>(p_net_no - 1)] =
        std::make_shared<NetIncompletes>(p_net_no, item_list, p_board);
}

int RatsNest::incomplete_count() const
{
    int result = 0;
    for (const auto& curr_net_incompletes : this->net_incompletes_)
    {
        if (curr_net_incompletes != nullptr)
        {
            result += curr_net_incompletes->count();
        }
    }
    return result;
}

int RatsNest::incomplete_count(int p_net_no) const
{
    if (p_net_no <= 0 || p_net_no > static_cast<int>(this->net_incompletes_.size()))
    {
        return 0;
    }
    return this->net_incompletes_[static_cast<size_t>(p_net_no - 1)]->count();
}

int RatsNest::length_violation_count() const
{
    int result = 0;
    for (const auto& curr_net_incompletes : this->net_incompletes_)
    {
        if (curr_net_incompletes != nullptr && curr_net_incompletes->get_length_violation() != 0)
        {
            ++result;
        }
    }
    return result;
}

double RatsNest::get_length_violation(int p_net_no) const
{
    if (p_net_no <= 0 || p_net_no > static_cast<int>(this->net_incompletes_.size()))
    {
        return 0;
    }
    return this->net_incompletes_[static_cast<size_t>(p_net_no - 1)]->get_length_violation();
}

std::vector<std::shared_ptr<RatsNest::AirLine>> RatsNest::get_airlines() const
{
    std::vector<std::shared_ptr<AirLine>> result;
    for (const auto& curr_net_incompletes : this->net_incompletes_)
    {
        if (curr_net_incompletes == nullptr)
        {
            continue;
        }
        const std::list<std::shared_ptr<AirLine>>& curr_list = curr_net_incompletes->incompletes();
        result.insert(result.end(), curr_list.begin(), curr_list.end());
    }
    return result;
}

bool RatsNest::recalculate_length_violations()
{
    bool result = false;
    for (const auto& curr_net_incompletes : this->net_incompletes_)
    {
        if (curr_net_incompletes != nullptr && curr_net_incompletes->calc_length_violation())
        {
            result = true;
        }
    }
    return result;
}

void RatsNest::set_filter(int p_net_no, bool p_value)
{
    if (p_net_no < 1 || p_net_no > static_cast<int>(this->is_filtered_.size()))
    {
        return;
    }
    this->is_filtered_[static_cast<size_t>(p_net_no - 1)] = p_value;
}

bool RatsNest::is_filtered(int p_net_no) const
{
    if (p_net_no < 1 || p_net_no > static_cast<int>(this->is_filtered_.size()))
    {
        return false;
    }
    return this->is_filtered_[static_cast<size_t>(p_net_no - 1)];
}

} // namespace interactive
} // namespace freerouting