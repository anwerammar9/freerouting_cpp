// Tests for the Java-faithful PlanarDelaunayTriangulation. Every expected edge list was printed by
// Java app.freerouting.datastructures.PlanarDelaunayTriangulation (freerouting 2.1.0 jar) for the
// same input objects, in getEdgeLines() order: "<object>(<x>,<y>)-<object>(<x>,<y>)".

#include "datastructures/PlanarDelaunayTriangulation.h"

#include <gtest/gtest.h>

#include <sstream>
#include <string>
#include <vector>

#include "geometry/planar/Point.h"

namespace freerouting::datastructures {
namespace {

class TestObject : public PlanarDelaunayTriangulation::Storable
{
public:
    TestObject(int p_index, std::vector<std::pair<int, int>> p_points) : index(p_index)
    {
        for (const auto& [x, y] : p_points)
        {
            points.push_back(std::make_shared<geometry::planar::IntPoint>(x, y));
        }
    }

    std::vector<std::shared_ptr<geometry::planar::Point>> get_triangulation_corners() const override { return points; }

    const int index;
    std::vector<std::shared_ptr<geometry::planar::Point>> points;
};

std::string run(const std::vector<std::shared_ptr<TestObject>>& p_objects)
{
    std::vector<std::shared_ptr<PlanarDelaunayTriangulation::Storable>> storables(p_objects.begin(), p_objects.end());
    PlanarDelaunayTriangulation triangulation(storables);
    std::ostringstream out;
    bool first = true;
    for (const auto& edge : triangulation.get_edge_lines())
    {
        const auto* start = dynamic_cast<const geometry::planar::IntPoint*>(edge.start_point.get());
        const auto* end = dynamic_cast<const geometry::planar::IntPoint*>(edge.end_point.get());
        out << (first ? "" : " ") << static_cast<const TestObject*>(edge.start_object.get())->index << "(" << start->x
            << "," << start->y << ")-" << static_cast<const TestObject*>(edge.end_object.get())->index << "(" << end->x
            << "," << end->y << ")";
        first = false;
    }
    return out.str();
}

std::shared_ptr<TestObject> obj(int p_index, std::vector<std::pair<int, int>> p_points)
{
    return std::make_shared<TestObject>(p_index, std::move(p_points));
}

TEST(PlanarDelaunayTriangulationTest, ThreePointsMatchJava)
{
    EXPECT_EQ("2(0,10)-0(0,0) 1(10,0)-0(0,0) 2(0,10)-1(10,0)",
        run({obj(0, {{0, 0}}), obj(1, {{10, 0}}), obj(2, {{0, 10}})}));
}

TEST(PlanarDelaunayTriangulationTest, SquareOfOneObjectMatchesJava)
{
    // Java returns 4 edges here (no (0,10)-(10,10)); reproduce, do not "fix".
    EXPECT_EQ("0(10,0)-0(0,0) 0(0,0)-0(0,10) 0(10,0)-0(0,10) 0(10,0)-0(10,10)",
        run({obj(0, {{0, 0}, {10, 0}, {10, 10}, {0, 10}})}));
}

TEST(PlanarDelaunayTriangulationTest, CollinearPointsMatchJava)
{
    EXPECT_EQ("4(40,0)-3(30,0) 1(10,0)-0(0,0) 3(30,0)-2(20,0) 2(20,0)-1(10,0)",
        run({obj(0, {{0, 0}}), obj(1, {{10, 0}}), obj(2, {{20, 0}}), obj(3, {{30, 0}}), obj(4, {{40, 0}})}));
}

TEST(PlanarDelaunayTriangulationTest, EqualCornersOfDifferentObjectsGiveADegenerateEdgeFirst)
{
    EXPECT_EQ("1(0,0)-0(0,0) 0(0,0)-2(5,5)", run({obj(0, {{0, 0}}), obj(1, {{0, 0}}), obj(2, {{5, 5}})}));
}

TEST(PlanarDelaunayTriangulationTest, CocircularPointsMatchJava)
{
    EXPECT_EQ("4(0,0)-3(0,-100) 4(0,0)-0(100,0) 3(0,-100)-0(100,0) 1(0,100)-4(0,0) 1(0,100)-0(100,0) "
              "1(0,100)-2(-100,0) 4(0,0)-2(-100,0) 3(0,-100)-2(-100,0)",
        run({obj(0, {{100, 0}}), obj(1, {{0, 100}}), obj(2, {{-100, 0}}), obj(3, {{0, -100}}), obj(4, {{0, 0}})}));
}

TEST(PlanarDelaunayTriangulationTest, TwentyRandomPointsMatchJava)
{
    // Points drawn by Java with new Random(7).nextInt(1000).
    const std::vector<std::pair<int, int>> points = {{236, 164}, {485, 44}, {380, 254}, {968, 649}, {850, 534},
        {200, 712}, {708, 911}, {495, 662}, {961, 738}, {307, 279}, {842, 924}, {883, 51}, {718, 4}, {239, 400},
        {576, 811}, {123, 73}, {679, 891}, {535, 492}, {818, 506}, {646, 234}};
    std::vector<std::shared_ptr<TestObject>> objects;
    for (int i = 0; i < static_cast<int>(points.size()); ++i)
    {
        objects.push_back(obj(i, {points[i]}));
    }
    EXPECT_EQ(
        "4(850,534)-18(818,506) 5(200,712)-16(679,891) 10(842,924)-16(679,891) 4(850,534)-16(679,891) "
        "18(818,506)-17(535,492) 16(679,891)-14(576,811) 5(200,712)-14(576,811) 14(576,811)-18(818,506) "
        "4(850,534)-14(576,811) 4(850,534)-3(968,649) 18(818,506)-11(883,51) 11(883,51)-4(850,534) "
        "3(968,649)-11(883,51) 0(236,164)-9(307,279) 10(842,924)-8(961,738) 3(968,649)-8(961,738) "
        "16(679,891)-8(961,738) 4(850,534)-8(961,738) 17(535,492)-19(646,234) 19(646,234)-11(883,51) "
        "19(646,234)-18(818,506) 11(883,51)-12(718,4) 19(646,234)-12(718,4) 19(646,234)-2(380,254) "
        "2(380,254)-0(236,164) 9(307,279)-2(380,254) 2(380,254)-17(535,492) 0(236,164)-13(239,400) "
        "13(239,400)-5(200,712) 9(307,279)-13(239,400) 17(535,492)-13(239,400) 13(239,400)-2(380,254) "
        "6(708,911)-10(842,924) 16(679,891)-6(708,911) 6(708,911)-5(200,712) 5(200,712)-15(123,73) "
        "15(123,73)-0(236,164) 13(239,400)-15(123,73) 12(718,4)-15(123,73) 12(718,4)-1(485,44) "
        "1(485,44)-15(123,73) 0(236,164)-1(485,44) 1(485,44)-2(380,254) 19(646,234)-1(485,44) "
        "17(535,492)-7(495,662) 7(495,662)-5(200,712) 14(576,811)-7(495,662) 7(495,662)-13(239,400) "
        "7(495,662)-18(818,506)",
        run(objects));
}

TEST(PlanarDelaunayTriangulationTest, ValidateReportsAConsistentTriangulation)
{
    std::vector<std::shared_ptr<PlanarDelaunayTriangulation::Storable>> storables = {
        obj(0, {{0, 0}, {10, 0}, {10, 10}, {0, 10}}), obj(1, {{5, 5}})};
    PlanarDelaunayTriangulation triangulation(storables);
    EXPECT_TRUE(triangulation.validate());
}

} // namespace
} // namespace freerouting::datastructures
