#pragma once

#include "geometry/planar/Point.h"

#include <array>
#include <memory>
#include <vector>

namespace freerouting::datastructures {

/**
 * Creates a Delaunay triangulation in the plane for the input objects. Port of Java
 * datastructures.PlanarDelaunayTriangulation (de Berg et al., Computational Geometry, ch. 9.3):
 * randomised incremental insertion with a triangle search DAG and edge flips.
 *
 * Parity notes: the input corners are shuffled with java.util.Random(99) exactly like Java;
 * edges carry increasing ids (Java Edge.compareTo) and the result lists the degenerate edges
 * (equal corners of different objects) first, then the leaf edges in id order — the same order
 * as Java's TreeSet, which the airline construction in NetIncompletes depends on.
 */
class PlanarDelaunayTriangulation
{
public:
    /** Objects that can be triangulated. Java PlanarDelaunayTriangulation.Storable. */
    class Storable
    {
    public:
        virtual ~Storable() = default;
        virtual std::vector<std::shared_ptr<geometry::planar::Point>> get_triangulation_corners() const = 0;
    };

    /** A line segment of the result. Java ResultEdge. */
    struct ResultEdge
    {
        std::shared_ptr<geometry::planar::Point> start_point;
        std::shared_ptr<Storable> start_object;
        std::shared_ptr<geometry::planar::Point> end_point;
        std::shared_ptr<Storable> end_object;
    };

    explicit PlanarDelaunayTriangulation(const std::vector<std::shared_ptr<Storable>>& p_object_list);
    ~PlanarDelaunayTriangulation();

    PlanarDelaunayTriangulation(const PlanarDelaunayTriangulation&) = delete;
    PlanarDelaunayTriangulation& operator=(const PlanarDelaunayTriangulation&) = delete;

    /** Returns all edge lines of the result. Java getEdgeLines. */
    std::vector<ResultEdge> get_edge_lines() const;

    /** Checks the consistency of the triangles. Java validate (debugging aid). */
    bool validate() const;

private:
    struct Corner;
    struct Edge;
    struct Triangle;

    Corner* new_corner(std::shared_ptr<Storable> p_object, std::shared_ptr<geometry::planar::Point> p_coor);
    Edge* new_edge(Corner* p_start, Corner* p_end);
    Triangle* new_triangle(const std::array<Edge*, 3>& p_edge_lines, Triangle* p_first_parent);

    bool split(Triangle* p_triangle, Corner* p_corner);
    bool legalize_edge(Corner* p_corner, Edge* p_edge);
    Edge* flip(Edge* p_edge);
    std::array<Triangle*, 3> split_at_inner_point(Triangle* p_triangle, Corner* p_corner);
    std::array<Triangle*, 4> split_at_border_point(Triangle* p_triangle, Corner* p_corner, Triangle* p_neighbour);

    void graph_insert(Triangle* p_triangle, Triangle* p_parent);
    Triangle* position_locate(const Corner* p_corner) const;
    Triangle* position_locate_reku(const Corner* p_corner, Triangle* p_triangle) const;

    std::vector<std::unique_ptr<Corner>> corners_;
    std::vector<std::unique_ptr<Edge>> edges_;
    std::vector<std::unique_ptr<Triangle>> triangles_;

    /** Root of the triangle search graph. Java TriangleGraph.anchor. */
    Triangle* anchor_ = nullptr;
    /** Edges between equal corners of different objects. Java degenerateEdges. */
    std::vector<Edge*> degenerate_edges_;
    int last_edge_id_ = 0;
};

} // namespace freerouting::datastructures
