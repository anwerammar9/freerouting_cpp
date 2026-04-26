#pragma once

#include <memory>
#include <vector>
#include <string>

namespace freerouting {
namespace designforms {
namespace specctra {

class PolylinePath
{
public:
    PolylinePath(double p_width, const std::vector<double>& p_corner_arr);

    double get_width() const { return width_; }
    const std::vector<double>& get_coordinates() const { return coordinates_; }
    int line_count() const { return static_cast<int>(coordinates_.size() / 4); }

    std::string to_string() const;

private:
    double width_;
    std::vector<double> coordinates_;
};

class PolygonPath
{
public:
    PolygonPath(double p_width, const std::vector<double>& p_corner_arr);

    double get_width() const { return width_; }
    const std::vector<double>& get_coordinates() const { return coordinates_; }
    int segment_count() const { return static_cast<int>(coordinates_.size() / 4); }

    std::string to_string() const;

private:
    double width_;
    std::vector<double> coordinates_;
};

} // namespace specctra
} // namespace designforms
} // namespace freerouting