#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <tuple>

#include "geometry/planar/IntBox.h"
#include "geometry/planar/IntOctagon.h"
#include "geometry/planar/Point.h"
#include "geometry/planar/Polyline.h"
#include "geometry/planar/Simplex.h"
#include "geometry/planar/Vector.h"

namespace freerouting {
namespace geometry {
namespace planar {
namespace {

constexpr double kTolerance = 1e-6;

TEST(TileShapeCornerIndexTest, CornerIndicesForAPoleOnTheRight)
{
    // Looking from the right of the box towards it, the left corner is the one
    // with the smaller y coordinate and the right corner the one with the larger.
    const IntBox box(0, 0, 100, 50);
    const FloatPoint pole(1000, 25);

    EXPECT_NEAR(0.0, box.corner_approx(box.index_of_left_most_corner(pole)).y, kTolerance);
    EXPECT_NEAR(50.0, box.corner_approx(box.index_of_right_most_corner(pole)).y, kTolerance);
}

TEST(TileShapeCornerIndexTest, CornerIndicesAreMirroredForAPoleOnTheLeft)
{
    const IntBox box(0, 0, 100, 50);
    const FloatPoint pole(-1000, 25);

    EXPECT_NEAR(50.0, box.corner_approx(box.index_of_left_most_corner(pole)).y, kTolerance);
    EXPECT_NEAR(0.0, box.corner_approx(box.index_of_right_most_corner(pole)).y, kTolerance);
}

TEST(TileShapeCornerIndexTest, CornerIndicesForAPoleAboveTheBox)
{
    // Looking from above towards the box, the left corner is the one with the
    // larger x coordinate.
    const IntBox box(0, 0, 100, 50);
    const FloatPoint pole(50, 1000);

    EXPECT_NEAR(100.0, box.corner_approx(box.index_of_left_most_corner(pole)).x, kTolerance);
    EXPECT_NEAR(0.0, box.corner_approx(box.index_of_right_most_corner(pole)).x, kTolerance);
}

TEST(TileShapeCornerIndexTest, CornerIndicesForAPoleBelowTheBox)
{
    const IntBox box(0, 0, 100, 50);
    const FloatPoint pole(50, -1000);

    EXPECT_NEAR(0.0, box.corner_approx(box.index_of_left_most_corner(pole)).x, kTolerance);
    EXPECT_NEAR(100.0, box.corner_approx(box.index_of_right_most_corner(pole)).x, kTolerance);
}

TEST(TileShapeCornerIndexTest, CornerIndicesOfADiagonalPoleAreCornersOfTheBox)
{
    const IntBox box(0, 0, 100, 50);
    const FloatPoint pole(1000, -1000);

    for (int index : {box.index_of_left_most_corner(pole), box.index_of_right_most_corner(pole)})
    {
        const FloatPoint corner = box.corner_approx(index);
        EXPECT_TRUE(corner.x == 0.0 || corner.x == 100.0);
        EXPECT_TRUE(corner.y == 0.0 || corner.y == 50.0);
    }
}

TEST(TileShapeShrinkTest, ShrinkReducesTheBoxByTheOffset)
{
    const IntBox box(0, 0, 100, 50);
    // IntBox also has an unrelated shrink(int), which hides TileShape.shrink(double).
    const TileShape& shape = box;

    std::unique_ptr<TileShape> shrunk = shape.shrink(10);

    ASSERT_NE(shrunk, nullptr);
    const IntBox shrunk_box = shrunk->bounding_box();
    EXPECT_EQ(10, shrunk_box.ll.x);
    EXPECT_EQ(10, shrunk_box.ll.y);
    EXPECT_EQ(90, shrunk_box.ur.x);
    EXPECT_EQ(40, shrunk_box.ur.y);
}

TEST(TileShapeShrinkTest, ShrinkOfAThinShapeKeepsItNonEmpty)
{
    // A shape thinner than twice the offset would vanish completely, so the
    // shrunk shape is intersected with the surrounding box of its centre.
    const IntBox box(0, 0, 100, 10);
    const TileShape& shape = box;

    std::unique_ptr<TileShape> shrunk = shape.shrink(20);

    ASSERT_NE(shrunk, nullptr);
    EXPECT_FALSE(shrunk->is_empty());
    EXPECT_LE(shrunk->dimension(), 2);
}

TEST(TileShapeShrinkTest, ShrinkOfAnOctagonReducesTheBoundingBox)
{
    const IntOctagon octagon(0, 0, 100, 100, 10, 90, 10, 90);
    const TileShape& shape = octagon;

    std::unique_ptr<TileShape> shrunk = shape.shrink(10);

    ASSERT_NE(shrunk, nullptr);
    EXPECT_FALSE(shrunk->is_empty());
    const IntBox shrunk_box = shrunk->bounding_box();
    const IntBox original_box = octagon.bounding_box();
    EXPECT_GE(shrunk_box.ll.x, original_box.ll.x);
    EXPECT_LE(shrunk_box.ur.x, original_box.ur.x);
    EXPECT_LE(shrunk_box.ur.x - shrunk_box.ll.x, original_box.ur.x - original_box.ll.x);
}


TEST(TileShapeBoundingOctagonTest, BoundingOctagonOfABoxIsTheOctagonOfItsCorners)
{
    const IntBox box(0, 0, 100, 200);
    const TileShape& shape = box;

    const IntOctagon octagon = shape.bounding_octagon();

    EXPECT_EQ(octagon.left, 0);
    EXPECT_EQ(octagon.bottom, 0);
    EXPECT_EQ(octagon.right, 100);
    EXPECT_EQ(octagon.top, 200);
}

TEST(TileShapeBoundingOctagonTest, BoundingOctagonOfAnOctagonIsTheOctagonItself)
{
    const IntOctagon octagon(0, 0, 100, 100, 10, 90, 10, 90);
    const TileShape& shape = octagon;

    const IntOctagon bounding_octagon = shape.bounding_octagon();

    EXPECT_EQ(bounding_octagon.left, octagon.left);
    EXPECT_EQ(bounding_octagon.bottom, octagon.bottom);
    EXPECT_EQ(bounding_octagon.right, octagon.right);
    EXPECT_EQ(bounding_octagon.top, octagon.top);
}

TEST(TileShapeBoundingOctagonTest, BoundingOctagonOfASimplexContainsTheSimplexCorners)
{
    // A diagonal simplex (45 degree), which is not a regular octagon itself.
    const Simplex simplex(std::vector<Line>{Line(IntPoint(0, 0), IntPoint(100, 100)),
                                           Line(IntPoint(100, 100), IntPoint(0, 100)),
                                           Line(IntPoint(0, 100), IntPoint(0, 0))});
    const TileShape& shape = simplex;

    const IntOctagon bounding_octagon = shape.bounding_octagon();

    EXPECT_EQ(bounding_octagon.left, 0);
    EXPECT_EQ(bounding_octagon.bottom, 0);
    EXPECT_EQ(bounding_octagon.right, 100);
    EXPECT_EQ(bounding_octagon.top, 100);
}

TEST(TileShapeCutoutTest, CutoutOfACrossingPolylineReturnsTheOutsidePieceAtTheEnd)
{
    // Java TileShape.entrance_points() scans the lines 1 .. n-2 only, so here
    // only the middle line is examined. It crosses the upper border line of the
    // box, and Polyline.first_corner() is the junction of the lines 0 and 1
    // (Java Polyline.corner(0)), which lies inside the box. Therefore the start
    // piece is skipped and the outside piece at the end of the polyline is
    // returned: the crossed border line plus the remaining polyline lines.
    const IntBox box(0, 0, 100, 100);
    const TileShape& shape = box;
    const Polyline polyline(std::vector<Line>{Line(IntPoint(-100, 50), IntPoint(50, 50)),
                                              Line(IntPoint(50, 50), IntPoint(50, 200)),
                                              Line(IntPoint(50, 200), IntPoint(200, 200))});

    const std::vector<Polyline> pieces = shape.cutout(polyline);

    ASSERT_EQ(pieces.size(), 1u);
    ASSERT_EQ(pieces[0].arr.size(), 3u);
    // The piece starts at the crossed border line and ends at the last corner
    // of the input polyline.
    EXPECT_EQ(pieces[0].arr.front().a.x, static_cast<int64_t>(0));
    EXPECT_EQ(pieces[0].arr.front().a.y, static_cast<int64_t>(100));
    EXPECT_EQ(pieces[0].arr.back().b.x, static_cast<int64_t>(200));
    EXPECT_EQ(pieces[0].arr.back().b.y, static_cast<int64_t>(200));
}

TEST(TileShapeCutoutTest, CutoutOfAPolylineOutsideTheBoxReturnsTheWholePolyline)
{
    const IntBox box(0, 0, 100, 100);
    const TileShape& shape = box;
    const Polyline polyline(std::vector<Line>{Line(IntPoint(-200, -50), IntPoint(200, -50)),
                                              Line(IntPoint(200, -50), IntPoint(200, 300)),
                                              Line(IntPoint(200, 300), IntPoint(600, 300))});

    const std::vector<Polyline> pieces = shape.cutout(polyline);

    ASSERT_EQ(pieces.size(), 1u);
    ASSERT_EQ(pieces[0].arr.size(), polyline.arr.size());
    EXPECT_EQ(pieces[0].arr.front().a.x, static_cast<int64_t>(-200));
    EXPECT_EQ(pieces[0].arr.back().b.x, static_cast<int64_t>(600));
}

TEST(TileShapeCutoutTest, CutoutOfAPolylineContainedInTheBoxReturnsNothing)
{
    const IntBox box(0, 0, 100, 100);
    const TileShape& shape = box;
    const Polyline polyline(std::vector<Line>{Line(IntPoint(10, 10), IntPoint(90, 10)),
                                              Line(IntPoint(90, 10), IntPoint(90, 90)),
                                              Line(IntPoint(90, 90), IntPoint(10, 90))});

    const std::vector<Polyline> pieces = shape.cutout(polyline);

    EXPECT_TRUE(pieces.empty());
}

TEST(TileShapeCutoutTest, CutoutOfASingleLinePolylineNeverSplitsIt)
{
    // Documented Java quirk: entrance_points() scans the lines 1 .. n-2 only,
    // so a one-line polyline has no entrance points and is returned unchanged.
    const IntBox box(0, 0, 100, 100);
    const TileShape& shape = box;
    const Polyline polyline(std::vector<Line>{Line(IntPoint(-100, 50), IntPoint(200, 50))});

    const std::vector<Polyline> pieces = shape.cutout(polyline);

    ASSERT_EQ(pieces.size(), 1u);
    ASSERT_EQ(pieces[0].arr.size(), 1u);
    EXPECT_EQ(pieces[0].arr.front().a.x, static_cast<int64_t>(-100));
    EXPECT_EQ(pieces[0].arr.back().b.x, static_cast<int64_t>(200));
}

// ---- TileShape::cutout(TileShape) ------------------------------------------
// Java A.cutout(B) == B.cutoutFrom(A): the convex pieces of A minus B, for all
// nine Simplex/IntOctagon/IntBox combinations of divide shape A and hole B.

enum class TileKind { kSimplex, kOctagon, kBox };

const char* tile_kind_name(TileKind p_kind)
{
    switch (p_kind)
    {
    case TileKind::kSimplex: return "Simplex";
    case TileKind::kOctagon: return "Octagon";
    case TileKind::kBox: return "Box";
    }
    return "?";
}

// The box (l, b, r, t) as the given tile type. Octagon and Simplex get their
// corners chamfered by p_chamfer, so they are not box-shaped.
std::unique_ptr<TileShape> make_tile(TileKind p_kind, int p_l, int p_b, int p_r, int p_t, int p_chamfer)
{
    if (p_kind == TileKind::kBox)
    {
        return std::make_unique<IntBox>(p_l, p_b, p_r, p_t);
    }
    IntOctagon oct = TileShape::get_instance(p_l, p_b, p_r, p_t, p_l - p_t + p_chamfer, p_r - p_b - p_chamfer,
        p_l + p_b + p_chamfer, p_r + p_t - p_chamfer);
    if (p_kind == TileKind::kOctagon)
    {
        return std::make_unique<IntOctagon>(oct);
    }
    return std::make_unique<Simplex>(oct.to_Simplex());
}

double sum_of_areas(const std::vector<std::unique_ptr<TileShape>>& p_pieces)
{
    double result = 0;
    for (const auto& piece : p_pieces)
    {
        result += piece->area();
    }
    return result;
}

class TileShapeCutoutTest : public ::testing::TestWithParam<std::tuple<TileKind, TileKind>>
{
};

TEST_P(TileShapeCutoutTest, ResultIsDivideShapeMinusHole)
{
    const auto [divide_kind, hole_kind] = GetParam();
    const auto divide = make_tile(divide_kind, 0, 0, 100, 100, 10);
    const auto hole = make_tile(hole_kind, 40, 30, 160, 70, 6);

    const auto pieces = divide->cutout(*hole);

    ASSERT_FALSE(pieces.empty());
    const double overlap = TileShape::intersection_of(*divide, *hole)->area();
    ASSERT_GT(overlap, 0.0);
    // Pieces of the divide shape, not of the hole (the old inverted branches
    // returned hole-minus-divide, which has a different area).
    EXPECT_NEAR(divide->area() - overlap, sum_of_areas(pieces), 1.0);
    const FloatPoint inside_overlap(70, 50);
    for (const auto& piece : pieces)
    {
        for (const FloatPoint& corner : piece->corner_approx_arr())
        {
            EXPECT_LE(divide->distance(corner), 1.0)
                << "corner (" << corner.x << ", " << corner.y << ") outside the divide shape";
        }
        EXPECT_FALSE(piece->contains(inside_overlap)) << "a piece covers the hole";
    }
}

TEST_P(TileShapeCutoutTest, DisjointHoleReturnsTheWholeDivideShape)
{
    const auto [divide_kind, hole_kind] = GetParam();
    const auto divide = make_tile(divide_kind, 0, 0, 100, 100, 10);
    const auto hole = make_tile(hole_kind, 300, 300, 400, 400, 6);

    const auto pieces = divide->cutout(*hole);

    ASSERT_EQ(1u, pieces.size());
    EXPECT_NEAR(divide->area(), pieces[0]->area(), 1e-6);
}

TEST_P(TileShapeCutoutTest, OnlyIntBoxCutoutSimplifiesThePieces)
{
    const auto [divide_kind, hole_kind] = GetParam();
    const auto divide = make_tile(divide_kind, 0, 0, 100, 100, 10);
    const auto hole = make_tile(hole_kind, 40, 30, 160, 70, 6);

    const auto pieces = divide->cutout(*hole);

    // Java IntBox.cutout simplifies each piece, so a 45-degree Simplex piece
    // comes back as a regular shape. Simplex.cutout keeps Simplex pieces.
    for (const auto& piece : pieces)
    {
        const bool is_simplex = dynamic_cast<const Simplex*>(piece.get()) != nullptr;
        if (divide_kind == TileKind::kBox)
        {
            EXPECT_FALSE(is_simplex);
        }
        else if (divide_kind == TileKind::kSimplex)
        {
            EXPECT_TRUE(is_simplex);
        }
    }
}

INSTANTIATE_TEST_SUITE_P(AllTileKinds, TileShapeCutoutTest,
    ::testing::Combine(::testing::Values(TileKind::kSimplex, TileKind::kOctagon, TileKind::kBox),
        ::testing::Values(TileKind::kSimplex, TileKind::kOctagon, TileKind::kBox)),
    [](const ::testing::TestParamInfo<TileShapeCutoutTest::ParamType>& p_info) {
        return std::string(tile_kind_name(std::get<0>(p_info.param))) + "Minus"
            + tile_kind_name(std::get<1>(p_info.param));
    });


} // namespace
} // namespace planar
} // namespace geometry
} // namespace freerouting
