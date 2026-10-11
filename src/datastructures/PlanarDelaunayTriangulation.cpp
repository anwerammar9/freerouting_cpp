#include "datastructures/PlanarDelaunayTriangulation.h"

#include <set>

#include "datastructures/JavaRandom.h"
#include "geometry/planar/Limits.h"
#include "geometry/planar/Vector.h"
#include "logger/FRLogger.h"

namespace freerouting::datastructures {

namespace gp = geometry::planar;

namespace {

/** Java PlanarDelaunayTriangulation.seed: the corners are shuffled with java.util.Random(99). */
constexpr int64_t kSeed = 99;

void warn(const char* p_message)
{
    logger::FRLogger::instance().warn(p_message);
}

/** Java Point.sideOf(Point p1, Point p2) = differenceBy(p1).sideOf(p2.differenceBy(p1)). */
gp::Side side_of(const gp::Point& p_point, const gp::Point& p_1, const gp::Point& p_2)
{
    const std::unique_ptr<gp::Vector> v1 = p_point.difference_by(p_1);
    const std::unique_ptr<gp::Vector> v2 = p_2.difference_by(p_1);
    return v1->side_of(*v2);
}

/** Java FloatPoint.circleCenter(p1, p2), verbatim (slope form, no special cases). */
gp::FloatPoint circle_center(const gp::FloatPoint& p_this, const gp::FloatPoint& p_1, const gp::FloatPoint& p_2)
{
    const double slope1 = (p_1.y - p_this.y) / (p_1.x - p_this.x);
    const double slope2 = (p_2.y - p_1.y) / (p_2.x - p_1.x);
    const double center_x =
        (slope1 * slope2 * (p_this.y - p_2.y) + slope2 * (p_this.x + p_1.x) - slope1 * (p_1.x + p_2.x))
        / (2 * (slope2 - slope1));
    const double center_y = (0.5 * (p_this.x + p_1.x) - center_x) / slope1 + 0.5 * (p_this.y + p_1.y);
    return gp::FloatPoint(center_x, center_y);
}

double distance_square(const gp::FloatPoint& p_a, const gp::FloatPoint& p_b)
{
    const double dx = p_a.x - p_b.x;
    const double dy = p_a.y - p_b.y;
    return dx * dx + dy * dy;
}

/** Java FloatPoint.insideCircle: strictly inside with a tolerance of 1 on the squared radius. */
bool inside_circle(const gp::FloatPoint& p_this, const gp::FloatPoint& p_1, const gp::FloatPoint& p_2,
    const gp::FloatPoint& p_3)
{
    const gp::FloatPoint center = circle_center(p_1, p_2, p_3);
    const double radius_square = distance_square(center, p_1);
    return distance_square(p_this, center) < radius_square - 1; // Java: "- 1 is a tolerance"
}

} // namespace

/** Java Corner: a corner point and the object it belongs to (null for the bounding corners). */
struct PlanarDelaunayTriangulation::Corner
{
    std::shared_ptr<Storable> object;
    std::shared_ptr<gp::Point> coor;

    gp::Side side_of(const Corner& p_1, const Corner& p_2) const { return datastructures::side_of(*coor, *p_1.coor, *p_2.coor); }
};

/** Java Edge. Ordered by its unique id (Java compareTo). */
struct PlanarDelaunayTriangulation::Edge
{
    Corner* start_corner = nullptr;
    Corner* end_corner = nullptr;
    int id = 0;
    Triangle* left_triangle = nullptr;
    Triangle* right_triangle = nullptr;

    /** Java commonCorner: Corner has no equals override, so this compares identities. */
    Corner* common_corner(const Edge& p_other) const
    {
        if (p_other.start_corner == start_corner || p_other.end_corner == start_corner)
        {
            return start_corner;
        }
        if (p_other.start_corner == end_corner || p_other.end_corner == end_corner)
        {
            return end_corner;
        }
        return nullptr;
    }

    Triangle* other_neighbour(const Triangle* p_triangle) const
    {
        if (p_triangle == left_triangle)
        {
            return right_triangle;
        }
        if (p_triangle == right_triangle)
        {
            return left_triangle;
        }
        warn("Edge.other_neighbour: inconsistent neighbour triangle");
        return nullptr;
    }

    bool is_legal() const;
    bool validate() const;
};

/** Java Triangle: edge lines counter-clockwise; children when used as a search-graph node. */
struct PlanarDelaunayTriangulation::Triangle
{
    std::array<Edge*, 3> edge_lines{};
    Triangle* first_parent = nullptr;
    std::vector<Triangle*> children;
    bool is_on_the_left_initialized = false;
    std::array<bool, 3> is_on_the_left_of_edge_line{};

    bool is_leaf() const { return children.empty(); }

    Corner* get_corner(int p_no) const
    {
        if (p_no < 0 || p_no >= 3)
        {
            warn("Triangle.get_corner: no out of range");
            return nullptr;
        }
        const Edge* current_edge = edge_lines[p_no];
        if (current_edge->left_triangle == this)
        {
            return current_edge->start_corner;
        }
        if (current_edge->right_triangle == this)
        {
            return current_edge->end_corner;
        }
        warn("Triangle.get_corner: inconsistent edge lines");
        return nullptr;
    }

    Corner* opposite_corner(const Edge* p_edge_line) const
    {
        int edge_line_no = -1;
        for (int i = 0; i < 3; ++i)
        {
            if (edge_lines[i] == p_edge_line)
            {
                edge_line_no = i;
                break;
            }
        }
        if (edge_line_no < 0)
        {
            warn("Triangle.opposite_corner: edgeLine not found");
            return nullptr;
        }
        const Edge* next_edge = edge_lines[(edge_line_no + 1) % 3];
        return next_edge->left_triangle == this ? next_edge->end_corner : next_edge->start_corner;
    }

    bool contains(const Corner& p_corner) const
    {
        if (!is_on_the_left_initialized)
        {
            warn("Triangle.contains: array isOnTheLeftOfEdgeLine not initialized");
            return false;
        }
        for (int i = 0; i < 3; ++i)
        {
            const Edge* current_edge = edge_lines[i];
            const gp::Side current_side = p_corner.side_of(*current_edge->start_corner, *current_edge->end_corner);
            if (is_on_the_left_of_edge_line[i])
            {
                if (current_side == gp::Side::ON_THE_RIGHT)
                {
                    return false;
                }
            }
            else if (current_side == gp::Side::ON_THE_LEFT)
            {
                return false;
            }
        }
        return true;
    }

    void get_leaf_edges(std::set<Edge*, bool (*)(const Edge*, const Edge*)>& p_result) const
    {
        if (is_leaf())
        {
            for (Edge* current_edge : edge_lines)
            {
                // Skip edges containing a bounding corner.
                if (current_edge->start_corner->object != nullptr && current_edge->end_corner->object != nullptr)
                {
                    p_result.insert(current_edge);
                }
            }
        }
        else
        {
            for (const Triangle* child : children)
            {
                if (child->first_parent == this) // to prevent traversing nodes more than once
                {
                    child->get_leaf_edges(p_result);
                }
            }
        }
    }

    bool validate() const;

    /** Java initializeIsOnTheLeftOfEdgeLineArray: called while this triangle is still a leaf. */
    void initialize_is_on_the_left()
    {
        if (is_on_the_left_initialized)
        {
            return;
        }
        is_on_the_left_initialized = true;
        for (int i = 0; i < 3; ++i)
        {
            is_on_the_left_of_edge_line[i] = edge_lines[i]->left_triangle == this;
        }
    }
};

bool PlanarDelaunayTriangulation::Edge::is_legal() const
{
    if (left_triangle == nullptr || right_triangle == nullptr)
    {
        return true;
    }
    const Corner* left_opposite_corner = left_triangle->opposite_corner(this);
    const Corner* right_opposite_corner = right_triangle->opposite_corner(this);
    const bool inside = inside_circle(right_opposite_corner->coor->to_float(), start_corner->coor->to_float(),
        left_opposite_corner->coor->to_float(), end_corner->coor->to_float());
    return !inside;
}

bool PlanarDelaunayTriangulation::Edge::validate() const
{
    bool result = true;
    for (const Triangle* triangle : {left_triangle, right_triangle})
    {
        const bool is_left = triangle == left_triangle;
        if (triangle == nullptr)
        {
            if (start_corner->object != nullptr || end_corner->object != nullptr)
            {
                warn(is_left ? "Edge.validate: left triangle may be null only for bounding edges"
                             : "Edge.validate: right triangle may be null only for bounding edges");
                result = false;
            }
            continue;
        }
        bool found = false;
        for (const Edge* line : triangle->edge_lines)
        {
            if (line == this)
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
            warn(is_left ? "Edge.validate: left triangle does not contain this edge"
                         : "Edge.validate: right triangle does not contain this edge");
            result = false;
        }
    }
    return result;
}

bool PlanarDelaunayTriangulation::Triangle::validate() const
{
    bool result = true;
    if (is_leaf())
    {
        const Edge* prev_edge = edge_lines[2];
        for (int i = 0; i < 3; ++i)
        {
            const Edge* current_edge = edge_lines[i];
            if (!current_edge->validate())
            {
                result = false;
            }
            const Corner* prev_end_corner =
                prev_edge->left_triangle == this ? prev_edge->end_corner : prev_edge->start_corner;
            const Corner* current_start_corner;
            if (current_edge->left_triangle == this)
            {
                current_start_corner = current_edge->start_corner;
            }
            else if (current_edge->right_triangle == this)
            {
                current_start_corner = current_edge->end_corner;
            }
            else
            {
                warn("Triangle.validate: edge inconsistent");
                return false;
            }
            if (current_start_corner != prev_end_corner)
            {
                warn("Triangle.validate: corner inconsistent");
                result = false;
            }
            prev_edge = current_edge;
        }
    }
    else
    {
        for (const Triangle* child : children)
        {
            if (child->first_parent == this)
            {
                // Java parity: the result of the children is not used.
                child->validate();
            }
        }
    }
    return result;
}

// ---------------------------------------------------------------------------------------------

PlanarDelaunayTriangulation::PlanarDelaunayTriangulation(const std::vector<std::shared_ptr<Storable>>& p_object_list)
{
    std::vector<Corner*> corner_list;
    for (const std::shared_ptr<Storable>& current_object : p_object_list)
    {
        for (const std::shared_ptr<gp::Point>& current_corner : current_object->get_triangulation_corners())
        {
            corner_list.push_back(new_corner(current_object, current_corner));
        }
    }

    // Random permutation of the corners with a fixed seed (Java parity).
    JavaRandom random_generator(kSeed);
    java_shuffle(corner_list, random_generator);

    // A big triangle containing all corners to start with.
    const int bounding_coor = gp::Limits::CRIT_INT;
    std::array<Corner*, 3> bounding_corners = {
        new_corner(nullptr, std::make_shared<gp::IntPoint>(bounding_coor, 0)),
        new_corner(nullptr, std::make_shared<gp::IntPoint>(0, bounding_coor)),
        new_corner(nullptr, std::make_shared<gp::IntPoint>(-bounding_coor, -bounding_coor)),
    };
    std::array<Edge*, 3> edge_lines{};
    for (int i = 0; i < 2; ++i)
    {
        edge_lines[i] = new_edge(bounding_corners[i], bounding_corners[i + 1]);
    }
    edge_lines[2] = new_edge(bounding_corners[2], bounding_corners[0]);

    Triangle* start_triangle = new_triangle(edge_lines, nullptr);
    // The left triangle of the edge lines is start_triangle, the right triangles stay null.
    for (Edge* current_edge : edge_lines)
    {
        current_edge->left_triangle = start_triangle;
    }
    graph_insert(start_triangle, nullptr);

    for (Corner* current_corner : corner_list)
    {
        Triangle* triangle_to_split = position_locate(current_corner);
        // Java parity: Java would throw a NullPointerException on a null triangle; skip it here.
        if (triangle_to_split != nullptr)
        {
            split(triangle_to_split, current_corner);
        }
    }
}

PlanarDelaunayTriangulation::~PlanarDelaunayTriangulation() = default;

PlanarDelaunayTriangulation::Corner* PlanarDelaunayTriangulation::new_corner(
    std::shared_ptr<Storable> p_object, std::shared_ptr<gp::Point> p_coor)
{
    auto corner = std::make_unique<Corner>();
    corner->object = std::move(p_object);
    corner->coor = std::move(p_coor);
    corners_.push_back(std::move(corner));
    return corners_.back().get();
}

PlanarDelaunayTriangulation::Edge* PlanarDelaunayTriangulation::new_edge(Corner* p_start, Corner* p_end)
{
    auto edge = std::make_unique<Edge>();
    edge->start_corner = p_start;
    edge->end_corner = p_end;
    edge->id = ++last_edge_id_; // Java newEdgeId()
    edges_.push_back(std::move(edge));
    return edges_.back().get();
}

PlanarDelaunayTriangulation::Triangle* PlanarDelaunayTriangulation::new_triangle(
    const std::array<Edge*, 3>& p_edge_lines, Triangle* p_first_parent)
{
    auto triangle = std::make_unique<Triangle>();
    triangle->edge_lines = p_edge_lines;
    triangle->first_parent = p_first_parent;
    triangles_.push_back(std::move(triangle));
    return triangles_.back().get();
}

std::vector<PlanarDelaunayTriangulation::ResultEdge> PlanarDelaunayTriangulation::get_edge_lines() const
{
    std::vector<ResultEdge> result;
    for (const Edge* current_edge : degenerate_edges_)
    {
        result.push_back({current_edge->start_corner->coor, current_edge->start_corner->object,
            current_edge->end_corner->coor, current_edge->end_corner->object});
    }
    if (anchor_ != nullptr)
    {
        // Java TreeSet<Edge>: ordered by edge id.
        std::set<Edge*, bool (*)(const Edge*, const Edge*)> result_edges(
            [](const Edge* p_a, const Edge* p_b) { return p_a->id < p_b->id; });
        anchor_->get_leaf_edges(result_edges);
        for (const Edge* current_edge : result_edges)
        {
            result.push_back({current_edge->start_corner->coor, current_edge->start_corner->object,
                current_edge->end_corner->coor, current_edge->end_corner->object});
        }
    }
    return result;
}

bool PlanarDelaunayTriangulation::validate() const
{
    const bool result = anchor_ != nullptr && anchor_->validate();
    // Java logs both outcomes at warn level.
    warn(result ? "Delaunay triangulation check passed ok" : "Delaunay triangulation check has detected problems");
    return result;
}

// ------------------------------------------------------------------- search graph (TriangleGraph)

void PlanarDelaunayTriangulation::graph_insert(Triangle* p_triangle, Triangle* p_parent)
{
    p_triangle->initialize_is_on_the_left();
    if (p_parent == nullptr)
    {
        anchor_ = p_triangle;
    }
    else
    {
        p_parent->children.push_back(p_triangle);
    }
}

PlanarDelaunayTriangulation::Triangle* PlanarDelaunayTriangulation::position_locate(const Corner* p_corner) const
{
    if (anchor_ == nullptr)
    {
        return nullptr;
    }
    if (anchor_->children.empty())
    {
        return anchor_;
    }
    for (Triangle* current_child : anchor_->children)
    {
        Triangle* result = position_locate_reku(p_corner, current_child);
        if (result != nullptr)
        {
            return result;
        }
    }
    warn("TriangleGraph.position_locate: containing triangle not found");
    return nullptr;
}

PlanarDelaunayTriangulation::Triangle* PlanarDelaunayTriangulation::position_locate_reku(
    const Corner* p_corner, Triangle* p_triangle) const
{
    if (!p_triangle->contains(*p_corner))
    {
        return nullptr;
    }
    if (p_triangle->is_leaf())
    {
        return p_triangle;
    }
    for (Triangle* current_child : p_triangle->children)
    {
        Triangle* result = position_locate_reku(p_corner, current_child);
        if (result != nullptr)
        {
            return result;
        }
    }
    warn("TriangleGraph.position_locate_reku: containing triangle not found");
    return nullptr;
}

// ------------------------------------------------------------------- insertion

bool PlanarDelaunayTriangulation::split(Triangle* p_triangle, Corner* p_corner)
{
    // Check, if the corner is in the interior of the triangle or contained in an edge line.
    Edge* containing_edge = nullptr;
    for (int i = 0; i < 3; ++i)
    {
        Edge* current_edge = p_triangle->edge_lines[i];
        const gp::Side current_side = current_edge->left_triangle == p_triangle
            ? p_corner->side_of(*current_edge->start_corner, *current_edge->end_corner)
            : p_corner->side_of(*current_edge->end_corner, *current_edge->start_corner);
        if (current_side == gp::Side::ON_THE_RIGHT)
        {
            warn("PlanarDelaunayTriangulation.split: corner is outside");
            return false;
        }
        if (current_side == gp::Side::COLLINEAR)
        {
            if (containing_edge != nullptr)
            {
                // The corner is equal to a corner of this triangle.
                Corner* common_corner = current_edge->common_corner(*containing_edge);
                if (common_corner == nullptr)
                {
                    warn("PlanarDelaunayTriangulation.split: common corner expected");
                    return false;
                }
                if (p_corner->object == common_corner->object)
                {
                    return false;
                }
                degenerate_edges_.push_back(new_edge(p_corner, common_corner));
                return true;
            }
            containing_edge = current_edge;
        }
    }

    if (containing_edge == nullptr)
    {
        // Split the triangle into 3 new triangles by adding edges from its corners to the corner.
        const std::array<Triangle*, 3> new_triangles = split_at_inner_point(p_triangle, p_corner);
        for (Triangle* current_triangle : new_triangles)
        {
            graph_insert(current_triangle, p_triangle);
        }
        for (int i = 0; i < 3; ++i)
        {
            legalize_edge(p_corner, p_triangle->edge_lines[i]);
        }
    }
    else
    {
        // Split this triangle and the neighbour into 4 new triangles.
        Triangle* neighbour_to_split = containing_edge->other_neighbour(p_triangle);
        if (neighbour_to_split == nullptr)
        {
            return false; // Java would throw a NullPointerException here
        }
        const std::array<Triangle*, 4> new_triangles =
            split_at_border_point(p_triangle, p_corner, neighbour_to_split);
        if (new_triangles[0] == nullptr)
        {
            return false;
        }
        // The first 2 divide the triangle, the last 2 divide the neighbour.
        graph_insert(new_triangles[0], p_triangle);
        graph_insert(new_triangles[1], p_triangle);
        graph_insert(new_triangles[2], neighbour_to_split);
        graph_insert(new_triangles[3], neighbour_to_split);

        for (int i = 0; i < 3; ++i)
        {
            Edge* current_edge = p_triangle->edge_lines[i];
            if (current_edge != containing_edge)
            {
                legalize_edge(p_corner, current_edge);
            }
        }
        for (int i = 0; i < 3; ++i)
        {
            Edge* current_edge = neighbour_to_split->edge_lines[i];
            if (current_edge != containing_edge)
            {
                legalize_edge(p_corner, current_edge);
            }
        }
    }
    return true;
}

bool PlanarDelaunayTriangulation::legalize_edge(Corner* p_corner, Edge* p_edge)
{
    if (p_edge->is_legal())
    {
        return false;
    }
    Triangle* triangle_to_change;
    if (p_edge->left_triangle->opposite_corner(p_edge) == p_corner)
    {
        triangle_to_change = p_edge->right_triangle;
    }
    else if (p_edge->right_triangle->opposite_corner(p_edge) == p_corner)
    {
        triangle_to_change = p_edge->left_triangle;
    }
    else
    {
        warn("PlanarDelaunayTriangulation.legalize_edge: edge lines inconsistent");
        return false;
    }
    Edge* flipped_edge = flip(p_edge);
    if (flipped_edge == nullptr)
    {
        return false; // Java would throw a NullPointerException here
    }

    // Update the search graph.
    graph_insert(flipped_edge->left_triangle, p_edge->left_triangle);
    graph_insert(flipped_edge->right_triangle, p_edge->left_triangle);
    graph_insert(flipped_edge->left_triangle, p_edge->right_triangle);
    graph_insert(flipped_edge->right_triangle, p_edge->right_triangle);

    // Recursion for the other edge lines of triangle_to_change.
    for (int i = 0; i < 3; ++i)
    {
        Edge* current_edge = triangle_to_change->edge_lines[i];
        if (current_edge != p_edge)
        {
            legalize_edge(p_corner, current_edge);
        }
    }
    return true;
}

PlanarDelaunayTriangulation::Edge* PlanarDelaunayTriangulation::flip(Edge* p_edge)
{
    // The start corner of this edge ends up on the left of the flipped edge, the end corner on the right.
    Edge* flipped_edge = new_edge(
        p_edge->right_triangle->opposite_corner(p_edge), p_edge->left_triangle->opposite_corner(p_edge));

    Triangle* first_parent = p_edge->left_triangle;
    Triangle* left = p_edge->left_triangle;
    Triangle* right = p_edge->right_triangle;

    int left_index = -1;
    int right_index = -1;
    for (int i = 0; i < 3; ++i)
    {
        if (left->edge_lines[i] == p_edge)
        {
            left_index = i;
        }
        if (right->edge_lines[i] == p_edge)
        {
            right_index = i;
        }
    }
    if (left_index < 0 || right_index < 0)
    {
        warn("Edge.flip: edge line inconsistent");
        return nullptr;
    }
    Edge* left_prev_edge = left->edge_lines[(left_index + 2) % 3];
    Edge* left_next_edge = left->edge_lines[(left_index + 1) % 3];
    Edge* right_prev_edge = right->edge_lines[(right_index + 2) % 3];
    Edge* right_next_edge = right->edge_lines[(right_index + 1) % 3];

    // The left triangle of the flipped edge.
    Triangle* new_left_triangle = new_triangle({flipped_edge, left_prev_edge, right_next_edge}, first_parent);
    flipped_edge->left_triangle = new_left_triangle;
    if (left_prev_edge->left_triangle == left)
    {
        left_prev_edge->left_triangle = new_left_triangle;
    }
    else
    {
        left_prev_edge->right_triangle = new_left_triangle;
    }
    if (right_next_edge->left_triangle == right)
    {
        right_next_edge->left_triangle = new_left_triangle;
    }
    else
    {
        right_next_edge->right_triangle = new_left_triangle;
    }

    // The right triangle of the flipped edge.
    Triangle* new_right_triangle = new_triangle({flipped_edge, right_prev_edge, left_next_edge}, first_parent);
    flipped_edge->right_triangle = new_right_triangle;
    if (right_prev_edge->left_triangle == right)
    {
        right_prev_edge->left_triangle = new_right_triangle;
    }
    else
    {
        right_prev_edge->right_triangle = new_right_triangle;
    }
    if (left_next_edge->left_triangle == left)
    {
        left_next_edge->left_triangle = new_right_triangle;
    }
    else
    {
        left_next_edge->right_triangle = new_right_triangle;
    }
    return flipped_edge;
}

std::array<PlanarDelaunayTriangulation::Triangle*, 3> PlanarDelaunayTriangulation::split_at_inner_point(
    Triangle* p_triangle, Corner* p_corner)
{
    // Java parity: Java builds an unused array of 3 edges here first. They consume edge ids,
    // and edge ids order the result edges, so they are created too.
    for (int i = 0; i < 3; ++i)
    {
        new_edge(p_triangle->get_corner(i), p_corner);
    }

    std::array<Triangle*, 3> new_triangles{};

    new_triangles[0] = new_triangle({p_triangle->edge_lines[0], new_edge(p_triangle->get_corner(1), p_corner),
                                        new_edge(p_corner, p_triangle->get_corner(0))},
        p_triangle);
    new_triangles[1] = new_triangle({p_triangle->edge_lines[1], new_edge(p_triangle->get_corner(2), p_corner),
                                        new_triangles[0]->edge_lines[1]},
        p_triangle);
    new_triangles[2] = new_triangle(
        {p_triangle->edge_lines[2], new_triangles[0]->edge_lines[2], new_triangles[1]->edge_lines[1]}, p_triangle);

    // The new neighbour triangles of the old edge lines; the other neighbour stays valid.
    for (int i = 0; i < 3; ++i)
    {
        Edge* current_edge = new_triangles[i]->edge_lines[0];
        if (current_edge->left_triangle == p_triangle)
        {
            current_edge->left_triangle = new_triangles[i];
        }
        else
        {
            current_edge->right_triangle = new_triangles[i];
        }
    }

    Edge* current_edge = new_triangles[0]->edge_lines[1];
    current_edge->left_triangle = new_triangles[0];
    current_edge->right_triangle = new_triangles[1];

    current_edge = new_triangles[1]->edge_lines[1];
    current_edge->left_triangle = new_triangles[1];
    current_edge->right_triangle = new_triangles[2];

    current_edge = new_triangles[2]->edge_lines[1];
    current_edge->left_triangle = new_triangles[0];
    current_edge->right_triangle = new_triangles[2];
    return new_triangles;
}

std::array<PlanarDelaunayTriangulation::Triangle*, 4> PlanarDelaunayTriangulation::split_at_border_point(
    Triangle* p_triangle, Corner* p_corner, Triangle* p_neighbour)
{
    int this_touching_edge_no = -1;
    int neighbour_touching_edge_no = -1;
    Edge* touching_edge = nullptr;
    Edge* other_touching_edge = nullptr;
    for (int i = 0; i < 3; ++i)
    {
        Edge* current_edge = p_triangle->edge_lines[i];
        if (p_corner->side_of(*current_edge->start_corner, *current_edge->end_corner) == gp::Side::COLLINEAR)
        {
            this_touching_edge_no = i;
            touching_edge = current_edge;
        }
        current_edge = p_neighbour->edge_lines[i];
        if (p_corner->side_of(*current_edge->start_corner, *current_edge->end_corner) == gp::Side::COLLINEAR)
        {
            neighbour_touching_edge_no = i;
            other_touching_edge = current_edge;
        }
    }
    if (this_touching_edge_no < 0 || neighbour_touching_edge_no < 0)
    {
        warn("Triangle.split_at_border_point: touching edge not found");
        return {};
    }
    if (touching_edge != other_touching_edge)
    {
        warn("Triangle.split_at_border_point: edges inconsistent");
        return {};
    }

    std::array<Triangle*, 4> new_triangles{};

    // The 2 split triangles of this triangle lie on the left of the new common touching edges.
    Edge* first_common_new_edge;
    Edge* second_common_new_edge;
    if (p_triangle == touching_edge->left_triangle)
    {
        first_common_new_edge = new_edge(touching_edge->start_corner, p_corner);
        second_common_new_edge = new_edge(p_corner, touching_edge->end_corner);
    }
    else
    {
        first_common_new_edge = new_edge(touching_edge->end_corner, p_corner);
        second_common_new_edge = new_edge(p_corner, touching_edge->start_corner);
    }

    // First split triangle of this triangle.
    Edge* prev_edge = p_triangle->edge_lines[(this_touching_edge_no + 2) % 3];
    Edge* this_splitting_edge = p_triangle == prev_edge->left_triangle
        ? new_edge(p_corner, prev_edge->start_corner)
        : new_edge(p_corner, prev_edge->end_corner);
    new_triangles[0] = new_triangle({prev_edge, first_common_new_edge, this_splitting_edge}, p_triangle);
    if (p_triangle == prev_edge->left_triangle)
    {
        prev_edge->left_triangle = new_triangles[0];
    }
    else
    {
        prev_edge->right_triangle = new_triangles[0];
    }
    first_common_new_edge->left_triangle = new_triangles[0];
    this_splitting_edge->left_triangle = new_triangles[0];

    // Second split triangle of this triangle.
    Edge* next_edge = p_triangle->edge_lines[(this_touching_edge_no + 1) % 3];
    new_triangles[1] = new_triangle({this_splitting_edge, second_common_new_edge, next_edge}, p_triangle);
    this_splitting_edge->right_triangle = new_triangles[1];
    second_common_new_edge->left_triangle = new_triangles[1];
    if (p_triangle == next_edge->left_triangle)
    {
        next_edge->left_triangle = new_triangles[1];
    }
    else
    {
        next_edge->right_triangle = new_triangles[1];
    }

    // First split triangle of the neighbour.
    Edge* neighbour_next_edge = p_neighbour->edge_lines[(neighbour_touching_edge_no + 1) % 3];
    Edge* neighbour_splitting_edge = p_neighbour == neighbour_next_edge->left_triangle
        ? new_edge(neighbour_next_edge->end_corner, p_corner)
        : new_edge(neighbour_next_edge->start_corner, p_corner);
    new_triangles[2] =
        new_triangle({neighbour_splitting_edge, first_common_new_edge, neighbour_next_edge}, p_neighbour);
    neighbour_splitting_edge->left_triangle = new_triangles[2];
    first_common_new_edge->right_triangle = new_triangles[2];
    if (p_neighbour == neighbour_next_edge->left_triangle)
    {
        neighbour_next_edge->left_triangle = new_triangles[2];
    }
    else
    {
        neighbour_next_edge->right_triangle = new_triangles[2];
    }

    // Second split triangle of the neighbour.
    prev_edge = p_neighbour->edge_lines[(neighbour_touching_edge_no + 2) % 3];
    new_triangles[3] = new_triangle({prev_edge, second_common_new_edge, neighbour_splitting_edge}, p_neighbour);
    if (p_neighbour == prev_edge->left_triangle)
    {
        prev_edge->left_triangle = new_triangles[3];
    }
    else
    {
        prev_edge->right_triangle = new_triangles[3];
    }
    second_common_new_edge->right_triangle = new_triangles[3];
    neighbour_splitting_edge->right_triangle = new_triangles[3];

    return new_triangles;
}

} // namespace freerouting::datastructures
