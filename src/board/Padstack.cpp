#include "board/Padstack.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <limits>

#include "geometry/planar/Direction.h"
#include "geometry/planar/IntOctagon.h"

namespace freerouting::board {

namespace {

/**
 * Drops every character that is not a digit or a decimal point, like the Java
 * String.replaceAll("[^0-9.]", "") inside Padstack.getDrillRadius().
 */
std::string keep_digits_and_point(const std::string& p_text)
{
    std::string result;
    result.reserve(p_text.size());
    for (const char c : p_text)
    {
        if ((c >= '0' && c <= '9') || c == '.')
        {
            result.push_back(c);
        }
    }
    return result;
}

/**
 * Parses a whole string as a number, mirroring the failure of Java's
 * Double.parseDouble for empty or malformed input.
 */
bool parse_double_strict(const std::string& p_text, double& p_value)
{
    if (p_text.empty())
    {
        return false;
    }
    const char* begin = p_text.c_str();
    char* end = nullptr;
    const double value = std::strtod(begin, &end);
    if (end != begin + p_text.size())
    {
        return false;
    }
    p_value = value;
    return true;
}

} // namespace

Padstack::Padstack(const std::string& p_name)
    : name_(p_name)
    , from_layer_(0)
    , to_layer_(0)
    , drill_(0)
{
}

void Padstack::add_shape(int p_layer, geometry::planar::Shape* p_shape)
{
    if (p_layer >= static_cast<int>(shapes_.size()))
    {
        shapes_.resize(p_layer + 1, nullptr);
    }
    shapes_[p_layer] = p_shape;
    cached_drill_radius_.reset();
}

const geometry::planar::Shape* Padstack::get_shape(int p_layer) const
{
    if (p_layer >= 0 && p_layer < static_cast<int>(shapes_.size()))
    {
        return shapes_[p_layer];
    }
    return nullptr;
}

geometry::planar::IntBox Padstack::get_bounding_box() const
{
    int64_t min_x = 0, max_x = 0, min_y = 0, max_y = 0;
    bool first = true;

    for (const auto* shape : shapes_)
    {
        if (shape)
        {
            auto box = shape->bounding_box();
            if (first)
            {
                min_x = box.ll.x;
                max_x = box.ur.x;
                min_y = box.ll.y;
                max_y = box.ur.y;
                first = false;
            }
            else
            {
                min_x = std::min(min_x, box.ll.x);
                max_x = std::max(max_x, box.ur.x);
                min_y = std::min(min_y, box.ll.y);
                max_y = std::max(max_y, box.ur.y);
            }
        }
    }

    if (first)
    {
        return geometry::planar::IntBox::EMPTY;
    }
    return geometry::planar::IntBox(min_x, min_y, max_x, max_y);
}

std::vector<std::unique_ptr<geometry::planar::Direction>> Padstack::get_trace_exit_directions(
    int p_layer, double p_factor) const
{
    using namespace geometry::planar;
    std::vector<std::unique_ptr<Direction>> result;
    if (p_layer < 0 || p_layer >= static_cast<int>(shapes_.size()))
    {
        return result;
    }
    const geometry::planar::Shape* curr_shape = shapes_[p_layer];
    if (curr_shape == nullptr)
    {
        return result;
    }
    // Currently implemented only for box shapes, mirroring Java.
    if (dynamic_cast<const geometry::planar::IntBox*>(curr_shape) == nullptr
        && dynamic_cast<const geometry::planar::IntOctagon*>(curr_shape) == nullptr)
    {
        return result;
    }
    const geometry::planar::IntBox curr_box = curr_shape->bounding_box();

    const bool all_dirs = std::max(curr_box.width(), curr_box.height())
        < p_factor * std::min(curr_box.width(), curr_box.height());

    if (all_dirs || curr_box.width() >= curr_box.height())
    {
        result.push_back(std::make_unique<IntDirection>(IntDirection::RIGHT));
        result.push_back(std::make_unique<IntDirection>(IntDirection::LEFT));
    }
    if (all_dirs || curr_box.width() <= curr_box.height())
    {
        result.push_back(std::make_unique<IntDirection>(IntDirection::UP));
        result.push_back(std::make_unique<IntDirection>(IntDirection::DOWN));
    }
    return result;
}

std::string Padstack::to_string() const
{
    return "Padstack[" + name_ + "]";
}

double Padstack::get_drill_radius() const
{
    if (cached_drill_radius_.has_value())
    {
        return *cached_drill_radius_;
    }
    double result = 0.0;
    const size_t colon_index = name_.find(':');
    if (colon_index != std::string::npos)
    {
        const size_t underscore_index = name_.find('_', colon_index);
        const std::string drill_str = keep_digits_and_point(
            underscore_index != std::string::npos && underscore_index > colon_index
            ? name_.substr(colon_index + 1, underscore_index - colon_index - 1)
            : name_.substr(colon_index + 1));
        double drill_dia = 0.0;
        if (parse_double_strict(drill_str, drill_dia))
        {
            const size_t last_underscore = name_.rfind('_', colon_index);
            if (last_underscore != std::string::npos)
            {
                const std::string outer_str = keep_digits_and_point(
                    name_.substr(last_underscore + 1, colon_index - last_underscore - 1));
                double outer_dia = 0.0;
                if (parse_double_strict(outer_str, outer_dia) && outer_dia > 0.0)
                {
                    const double actual_outer_radius = get_smallest_radius();
                    if (actual_outer_radius > 0.0)
                    {
                        result = actual_outer_radius * (drill_dia / outer_dia);
                        cached_drill_radius_ = result;
                        return result;
                    }
                }
            }
        }
    }
    result = get_smallest_radius() * 0.45;
    cached_drill_radius_ = result;
    return result;
}

double Padstack::get_smallest_radius() const
{
    double min_radius = std::numeric_limits<double>::max();
    for (const auto* shape : shapes_)
    {
        if (shape == nullptr)
        {
            continue;
        }
        const geometry::planar::IntBox box = shape->bounding_box();
        const double radius = std::min(static_cast<double>(box.width()),
                                       static_cast<double>(box.height()))
            / 2.0;
        min_radius = std::min(min_radius, radius);
    }
    if (min_radius == std::numeric_limits<double>::max())
    {
        return 0.0;
    }
    return min_radius;
}

} // namespace freerouting::board
