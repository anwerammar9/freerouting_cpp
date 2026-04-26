#include "board/ObstacleArea.h"

#include <sstream>

namespace freerouting {
namespace board {

ObstacleArea::ObstacleArea(const std::string& p_name)
    : name_(p_name)
{
}

std::string ObstacleArea::to_string() const
{
    if (name_.empty())
    {
        return "ObstacleArea";
    }
    return name_;
}

std::unique_ptr<Item> ObstacleArea::clone() const
{
    auto clone = std::make_unique<ObstacleArea>(name_);
    clone->relative_area_ = relative_area_;
    clone->layer_ = layer_;
    return clone;
}

} // namespace board
} // namespace freerouting