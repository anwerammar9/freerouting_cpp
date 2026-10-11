#include "board/LogicalPart.h"

#include <algorithm>
#include <cctype>

#include "logger/FRLogger.h"

namespace freerouting::board {

namespace {

bool equals_ignore_case(const std::string& p_left, const std::string& p_right)
{
    if (p_left.size() != p_right.size())
    {
        return false;
    }
    for (std::size_t i = 0; i < p_left.size(); ++i)
    {
        if (std::tolower(static_cast<unsigned char>(p_left[i])) != std::tolower(static_cast<unsigned char>(p_right[i])))
        {
            return false;
        }
    }
    return true;
}

} // namespace

LogicalPart::LogicalPart(std::string p_name, int p_id, std::vector<PartPin> p_part_pins)
    : name_(std::move(p_name))
    , id_(p_id)
    , part_pins_(std::move(p_part_pins))
{
}

const LogicalPart::PartPin* LogicalPart::get_pin(int p_pin_index) const
{
    if (p_pin_index < 0 || p_pin_index >= pin_count())
    {
        logger::FRLogger::instance().warn("LogicalPart.getPin: pinIndex out of range");
        return nullptr;
    }
    return &part_pins_[p_pin_index];
}

LogicalPart* LogicalParts::add(const std::string& p_name, std::vector<LogicalPart::PartPin> p_part_pins)
{
    // Java Arrays.sort on objects is a stable merge sort.
    std::stable_sort(p_part_pins.begin(), p_part_pins.end(),
                     [](const LogicalPart::PartPin& p_a, const LogicalPart::PartPin& p_b) {
                         return p_a.pin_index < p_b.pin_index;
                     });
    parts_.push_back(std::make_unique<LogicalPart>(p_name, count() + 1, std::move(p_part_pins)));
    return parts_.back().get();
}

LogicalPart* LogicalParts::get(const std::string& p_name) const
{
    for (const auto& part : parts_)
    {
        if (part && equals_ignore_case(part->get_name(), p_name))
        {
            return part.get();
        }
    }
    return nullptr;
}

LogicalPart* LogicalParts::get(int p_part_id) const
{
    // Java parity: Vector.elementAt throws for an invalid index; nullptr here.
    if (p_part_id < 1 || p_part_id > count())
    {
        return nullptr;
    }
    LogicalPart* result = parts_[p_part_id - 1].get();
    if (result != nullptr && result->get_id() != p_part_id)
    {
        logger::FRLogger::instance().warn("LogicalParts.get: inconsistent part ID");
    }
    return result;
}

} // namespace freerouting::board
