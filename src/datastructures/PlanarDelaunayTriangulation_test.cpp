#include "datastructures/PlanarDelaunayTriangulation.h"

#include <gtest/gtest.h>

namespace freerouting::datastructures {
namespace {

class TestPointSet : public PlanarDelaunayTriangulation::Storable
{
public:
    explicit TestPointSet(std::initializer_list<std::pair<int, int>> p_points)
    {
        for (const auto& [x, y] : p_points)
        {
            points.push_back(std::make_shared<geometry::planar::IntPoint>(x, y));
        }
    }

    std::vector<std::shared_ptr<geometry::planar::Point>>
    get_triangulation_corners() const override
    {
        return points;
    }

    std::vector<std::shared_ptr<geometry::planar::Point>> points;
};

} // namespace

TEST(PlanarDelaunayTriangulationTest, TriangulatesThreeNonCollinearPoints)
{
    auto first = std::make_shared<TestPointSet>(std::initializer_list<std::pair<int, int>>{{0, 0}});
    auto second = std::make_shared<TestPointSet>(std::initializer_list<std::pair<int, int>>{{10, 0}});
    auto third = std::make_shared<TestPointSet>(std::initializer_list<std::pair<int, int>>{{0, 10}});

    PlanarDelaunayTriangulation triangulation({first, second, third});

    ASSERT_TRUE(triangulation.validate());
    const auto edges = triangulation.get_edge_lines();
    ASSERT_EQ(edges.size(), 3);
    for (const auto& edge : edges)
    {
        EXPECT_NE(edge.start_object, nullptr);
        EXPECT_NE(edge.end_object, nullptr);
    }
}

TEST(PlanarDelaunayTriangulationTest, SquareProducesUniquePlanarEdges)
{
    auto points = std::make_shared<TestPointSet>(
        std::initializer_list<std::pair<int, int>>{{0, 0}, {10, 0}, {10, 10}, {0, 10}});

    PlanarDelaunayTriangulation triangulation({points});

    ASSERT_TRUE(triangulation.validate());
    EXPECT_EQ(triangulation.get_edge_lines().size(), 5);
}

TEST(PlanarDelaunayTriangulationTest, DuplicateCoordinatesDoNotCreateDegenerateEdges)
{
    auto first = std::make_shared<TestPointSet>(std::initializer_list<std::pair<int, int>>{{0, 0}});
    auto duplicate = std::make_shared<TestPointSet>(std::initializer_list<std::pair<int, int>>{{0, 0}});
    auto third = std::make_shared<TestPointSet>(std::initializer_list<std::pair<int, int>>{{5, 5}});

    PlanarDelaunayTriangulation triangulation({first, duplicate, third});

    EXPECT_TRUE(triangulation.validate());
    EXPECT_TRUE(triangulation.get_edge_lines().empty());
}

} // namespace freerouting::datastructures
