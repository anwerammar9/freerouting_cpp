#pragma once

#include "geometry/planar/Point.h"

#include <memory>
#include <vector>

namespace freerouting::datastructures {

class PlanarDelaunayTriangulation
{
public:
    class Storable
    {
    public:
        virtual ~Storable() = default;
        virtual std::vector<std::shared_ptr<geometry::planar::Point>>
        get_triangulation_corners() const = 0;
    };

    struct ResultEdge
    {
        std::shared_ptr<geometry::planar::Point> start_point;
        std::shared_ptr<Storable> start_object;
        std::shared_ptr<geometry::planar::Point> end_point;
        std::shared_ptr<Storable> end_object;
    };

    explicit PlanarDelaunayTriangulation(
        const std::vector<std::shared_ptr<Storable>>& p_object_list);

    std::vector<ResultEdge> get_edge_lines() const;
    bool validate() const;

public:
    struct Corner
    {
        std::shared_ptr<Storable> object;
        std::shared_ptr<geometry::planar::Point> point;
        double x = 0;
        double y = 0;
        bool super = false;
    };

    struct Triangle
    {
        int a = 0;
        int b = 0;
        int c = 0;
    };

private:
    std::vector<Corner> corners_;
    std::vector<Triangle> triangles_;
};

} // namespace freerouting::datastructures
