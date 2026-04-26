#pragma once

#include <string>
#include <memory>
#include <vector>

#include "geometry/planar/Point.h"

namespace freerouting {
namespace geometry {
namespace planar {

class IntBox;
class IntOctagon;

class Shape
{
public:
    virtual ~Shape() = default;
    virtual IntBox bounding_box() const = 0;
    virtual bool intersects(const Shape& other) const = 0;
    virtual bool contains(const IntPoint& point) const = 0;
    virtual std::string to_string() const = 0;
};

class Area : public Shape
{
public:
    virtual ~Area() = default;
    virtual IntBox bounding_box() const override = 0;
    virtual IntOctagon bounding_octagon() const = 0;
    virtual bool contains(const IntPoint& point) const override = 0;
    virtual bool is_empty() const = 0;
    virtual std::unique_ptr<Area> clone() const = 0;
};

class PolygonArea : public Area
{
public:
    std::vector<IntPoint> corners;

    explicit PolygonArea(const std::vector<IntPoint>& p_corners);
    PolygonArea() = default;

    IntBox bounding_box() const override;
    IntOctagon bounding_octagon() const override;
    bool contains(const IntPoint& point) const override;
    bool is_empty() const override;
    std::unique_ptr<Area> clone() const override;
    bool intersects(const Shape& other) const override;
    std::string to_string() const override;
};

class Circle : public Shape
{
public:
    const IntPoint center;
    const int64_t radius;

    Circle(const IntPoint& p_center, int64_t p_radius);

    IntBox bounding_box() const override;
    bool intersects(const Shape& other) const override;
    bool contains(const IntPoint& point) const override;
    std::string to_string() const override;
};

} // namespace planar
} // namespace geometry
} // namespace freerouting