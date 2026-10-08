#include "board/Package.h"

#include <algorithm>
#include <cctype>
#include <regex>

#include "logger/FRLogger.h"

namespace freerouting::board {

namespace {

bool equals_ignore_case(const std::string& p_a, const std::string& p_b)
{
    return p_a.size() == p_b.size()
        && std::equal(p_a.begin(), p_a.end(), p_b.begin(), [](unsigned char a, unsigned char b) {
               return std::tolower(a) == std::tolower(b);
           });
}

} // namespace

Package::Package(std::string p_name, int p_id, std::vector<Pin> p_pins,
    std::vector<std::shared_ptr<geometry::planar::Shape>> p_outline, std::vector<double> p_outline_widths,
    std::vector<bool> p_outline_is_closed, std::vector<Keepout> p_keepouts, std::vector<Keepout> p_via_keepouts,
    std::vector<Keepout> p_place_keepouts, bool p_is_front)
    : name_(std::move(p_name))
    , id_(p_id)
    , pins_(std::move(p_pins))
    , outline_(std::move(p_outline))
    , outline_widths_(std::move(p_outline_widths))
    , outline_is_closed_(std::move(p_outline_is_closed))
    , keepouts_(std::move(p_keepouts))
    , via_keepouts_(std::move(p_via_keepouts))
    , place_keepouts_(std::move(p_place_keepouts))
    , is_front_(p_is_front)
{
}

const Package::Pin* Package::get_pin(int p_pin_index) const
{
    if (p_pin_index < 0 || p_pin_index >= pin_count())
    {
        logger::FRLogger::instance().warn("Package.getPin: pinIndex out of range");
        return nullptr;
    }
    return &pins_[p_pin_index];
}

int Package::get_pin_index(const std::string& p_name) const
{
    for (int i = 0; i < pin_count(); ++i)
    {
        if (pins_[i].name == p_name)
        {
            return i;
        }
    }
    return -1;
}

Package* Packages::get(const std::string& p_name, bool p_is_front) const
{
    Package* other_side_package = nullptr;
    for (const auto& current_package : packages_)
    {
        if (equals_ignore_case(current_package->get_name(), p_name))
        {
            if (current_package->is_front() == p_is_front)
            {
                return current_package.get();
            }
            other_side_package = current_package.get();
        }
    }
    // Java: name.replaceAll("::\\d+$", "")
    static const std::regex kVariantSuffix("::\\d+$");
    const std::string base_name = std::regex_replace(p_name, kVariantSuffix, "");
    if (!equals_ignore_case(base_name, p_name))
    {
        for (const auto& current_package : packages_)
        {
            if (equals_ignore_case(current_package->get_name(), base_name))
            {
                if (current_package->is_front() == p_is_front)
                {
                    return current_package.get();
                }
                other_side_package = current_package.get();
            }
        }
    }
    return other_side_package;
}

Package* Packages::get(int p_package_id) const
{
    if (p_package_id < 1 || p_package_id > count())
    {
        // Java throws ArrayIndexOutOfBoundsException here.
        logger::FRLogger::instance().warn("Packages.get: package ID out of range");
        return nullptr;
    }
    Package* result = packages_[p_package_id - 1].get();
    if (result->get_id() != p_package_id)
    {
        logger::FRLogger::instance().warn("Packages.get: inconsistent package ID");
    }
    return result;
}

Package* Packages::add(const std::string& p_name, std::vector<Package::Pin> p_pins,
    std::vector<std::shared_ptr<geometry::planar::Shape>> p_outline, std::vector<double> p_outline_widths,
    std::vector<bool> p_outline_is_closed, std::vector<Package::Keepout> p_keepouts,
    std::vector<Package::Keepout> p_via_keepouts, std::vector<Package::Keepout> p_place_keepouts, bool p_is_front)
{
    packages_.push_back(std::make_unique<Package>(p_name, count() + 1, std::move(p_pins), std::move(p_outline),
        std::move(p_outline_widths), std::move(p_outline_is_closed), std::move(p_keepouts), std::move(p_via_keepouts),
        std::move(p_place_keepouts), p_is_front));
    return packages_.back().get();
}

} // namespace freerouting::board
