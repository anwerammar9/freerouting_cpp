#include "datastructures/PlanarDelaunayTriangulation.h"

#include "geometry/planar/Limits.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <map>
#include <set>
#include <tuple>

namespace freerouting::datastructures {
namespace {

struct EdgeKey
{
    int first;
    int second;

    bool operator<(const EdgeKey& p_other) const
    {
        return std::tie(first, second) < std::tie(p_other.first, p_other.second);
    }
};

EdgeKey make_edge(int p_first, int p_second)
{
    if (p_first > p_second)
    {
        std::swap(p_first, p_second);
    }
    return {p_first, p_second};
}

double orientation(double p_ax, double p_ay, double p_bx, double p_by,
                   double p_cx, double p_cy)
{
    return (p_bx - p_ax) * (p_cy - p_ay) - (p_by - p_ay) * (p_cx - p_ax);
}

bool contains_in_circumcircle(double p_point_x, double p_point_y,
                              const PlanarDelaunayTriangulation::Corner& p_a,
                              const PlanarDelaunayTriangulation::Corner& p_b,
                              const PlanarDelaunayTriangulation::Corner& p_c)
{
    const double ax = p_a.x - p_point_x;
    const double ay = p_a.y - p_point_y;
    const double bx = p_b.x - p_point_x;
    const double by = p_b.y - p_point_y;
    const double cx = p_c.x - p_point_x;
    const double cy = p_c.y - p_point_y;
    const double determinant = (ax * ax + ay * ay) * (bx * cy - by * cx)
        - (bx * bx + by * by) * (ax * cy - ay * cx)
        + (cx * cx + cy * cy) * (ax * by - ay * bx);
    const double winding = orientation(p_a.x, p_a.y, p_b.x, p_b.y, p_c.x, p_c.y);
    return winding > 0 ? determinant > 1e-10 : determinant < -1e-10;
}

} // namespace

PlanarDelaunayTriangulation::PlanarDelaunayTriangulation(
    const std::vector<std::shared_ptr<Storable>>& p_object_list)
{
    for (const auto& object : p_object_list)
    {
        if (!object)
        {
            continue;
        }
        for (const auto& point : object->get_triangulation_corners())
        {
            if (!point || point->is_infinite())
            {
                continue;
            }
            const auto position = point->to_float();
            const bool duplicate = std::any_of(
                corners_.begin(), corners_.end(), [&position](const Corner& corner) {
                    return !corner.super && corner.x == position.x && corner.y == position.y;
                });
            if (!duplicate)
            {
                corners_.push_back({object, point, position.x, position.y, false});
            }
        }
    }

    if (corners_.empty())
    {
        return;
    }

    const double bound = static_cast<double>(geometry::planar::Limits::CRIT_INT);
    const int first_super = static_cast<int>(corners_.size());
    corners_.push_back({nullptr, std::make_shared<geometry::planar::IntPoint>(bound, 0), bound, 0, true});
    corners_.push_back({nullptr, std::make_shared<geometry::planar::IntPoint>(0, bound), 0, bound, true});
    corners_.push_back({nullptr, std::make_shared<geometry::planar::IntPoint>(-bound, -bound), -bound, -bound, true});
    triangles_.push_back({first_super, first_super + 1, first_super + 2});

    for (int point_index = 0; point_index < first_super; ++point_index)
    {
        std::vector<Triangle> remaining;
        std::map<EdgeKey, int> boundary_counts;
        for (const Triangle& triangle : triangles_)
        {
            if (contains_in_circumcircle(corners_[point_index].x, corners_[point_index].y,
                                         corners_[triangle.a],
                                         corners_[triangle.b], corners_[triangle.c]))
            {
                ++boundary_counts[make_edge(triangle.a, triangle.b)];
                ++boundary_counts[make_edge(triangle.b, triangle.c)];
                ++boundary_counts[make_edge(triangle.c, triangle.a)];
            }
            else
            {
                remaining.push_back(triangle);
            }
        }
        triangles_ = std::move(remaining);
        for (const auto& [edge, count] : boundary_counts)
        {
            if (count != 1)
            {
                continue;
            }
            const double winding = orientation(corners_[edge.first].x, corners_[edge.first].y,
                                               corners_[edge.second].x, corners_[edge.second].y,
                                               corners_[point_index].x, corners_[point_index].y);
            if (std::abs(winding) > 1e-10)
            {
                if (winding > 0)
                {
                    triangles_.push_back({edge.first, edge.second, point_index});
                }
                else
                {
                    triangles_.push_back({edge.second, edge.first, point_index});
                }
            }
        }
    }
}

std::vector<PlanarDelaunayTriangulation::ResultEdge>
PlanarDelaunayTriangulation::get_edge_lines() const
{
    std::set<EdgeKey> edges;
    for (const Triangle& triangle : triangles_)
    {
        // Skip edges containing a bounding corner instead of the whole
        // triangle, mirroring Java TriangleGraph.get_leaf_edges(). This keeps
        // hull edges of degenerate point sets (e.g. two isolated pins) which
        // only occur in triangles together with a bounding corner.
        const int corner_indices[3] = {triangle.a, triangle.b, triangle.c};
        for (int i = 0; i < 3; ++i)
        {
            const int first = corner_indices[i];
            const int second = corner_indices[(i + 1) % 3];
            if (!corners_[first].super && !corners_[second].super)
            {
                edges.insert(make_edge(first, second));
            }
        }
    }

    std::vector<ResultEdge> result;
    result.reserve(edges.size());
    for (const EdgeKey& edge : edges)
    {
        result.push_back({corners_[edge.first].point, corners_[edge.first].object,
                          corners_[edge.second].point, corners_[edge.second].object});
    }
    return result;
}

bool PlanarDelaunayTriangulation::validate() const
{
    for (const Triangle& triangle : triangles_)
    {
        if (triangle.a == triangle.b || triangle.b == triangle.c || triangle.c == triangle.a
            || std::abs(orientation(corners_[triangle.a].x, corners_[triangle.a].y,
                                    corners_[triangle.b].x, corners_[triangle.b].y,
                                    corners_[triangle.c].x, corners_[triangle.c].y)) < 1e-10)
        {
            return false;
        }
    }
    return true;
}

} // namespace freerouting::datastructures
