/**
 * @file Rational_test.cpp
 * @brief Unit tests for the rational geometry classes RationalPoint,
 *        RationalVector and BigIntDirection.
 */

#include "geometry/planar/Rational.h"
#include "geometry/planar/Vector.h"
#include "geometry/planar/Direction.h"
#include "geometry/planar/Point.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/Limits.h"

#include <gtest/gtest.h>

namespace freerouting::geometry::planar {

using datastructures::BigInt;

TEST(RationalPointTest, ConstructorNormalizesZNegativeToPositive)
{
    const IntPoint p(3, -4);
    const RationalPoint rp(p);
    EXPECT_EQ(rp.x, 3);
    EXPECT_EQ(rp.y, -4);
    EXPECT_EQ(rp.z, 1);
}

TEST(RationalPointTest, ConstructorThrowsForNegativeZ)
{
    EXPECT_THROW(RationalPoint(BigInt(1), BigInt(2), BigInt(-1)), std::invalid_argument);
}

TEST(RationalPointTest, EqualityAcrossDenominators)
{
    const RationalPoint p1(BigInt(1), BigInt(2), BigInt(2));
    const RationalPoint p2(BigInt(2), BigInt(4), BigInt(4));
    EXPECT_TRUE(p1.equals(&p2));
    const RationalPoint p3(BigInt(3), BigInt(4), BigInt(4));
    EXPECT_FALSE(p1.equals(&p3));
}

TEST(RationalPointTest, ToFloat)
{
    const RationalPoint rp(BigInt(1), BigInt(3), BigInt(2));
    const FloatPoint fp = rp.to_float();
    EXPECT_NEAR(fp.x, 0.5, 1e-9);
    EXPECT_NEAR(fp.y, 1.5, 1e-9);
}

TEST(RationalPointTest, IsInfiniteForZeroZ)
{
    const RationalPoint rp(BigInt(1), BigInt(2), BigInt(0));
    EXPECT_TRUE(rp.is_infinite());
    const RationalPoint rp2(BigInt(1), BigInt(2), BigInt(3));
    EXPECT_FALSE(rp2.is_infinite());
}

TEST(RationalPointTest, IsContainedInBox)
{
    const RationalPoint rp(BigInt(5), BigInt(5), BigInt(2)); // (2.5, 2.5)
    EXPECT_TRUE(rp.is_contained_in(IntBox(0, 0, 3, 3)));
    EXPECT_FALSE(rp.is_contained_in(IntBox(0, 0, 2, 2)));
}

TEST(RationalPointTest, TranslateByIntVector)
{
    const RationalPoint rp(BigInt(1), BigInt(2), BigInt(2));
    const IntVector v(3, 4);
    std::unique_ptr<Point> result = rp.translate_by(v);
    const RationalPoint* r = dynamic_cast<const RationalPoint*>(result.get());
    ASSERT_NE(r, nullptr);
    FloatPoint fp = r->to_float();
    EXPECT_NEAR(fp.x, 3.5, 1e-9);
    EXPECT_NEAR(fp.y, 5.0, 1e-9);
}

TEST(RationalPointTest, TranslateByRationalVector)
{
    const RationalPoint rp(BigInt(1), BigInt(2), BigInt(2)); // (0.5, 1.0)
    const RationalVector v(BigInt(1), BigInt(1), BigInt(2)); // (0.5, 0.5)
    std::unique_ptr<Point> result = rp.translate_by(v);
    const RationalPoint* r = dynamic_cast<const RationalPoint*>(result.get());
    ASSERT_NE(r, nullptr);
    FloatPoint fp = r->to_float();
    EXPECT_NEAR(fp.x, 1.0, 1e-9);
    EXPECT_NEAR(fp.y, 1.5, 1e-9);
}

TEST(RationalPointTest, DifferenceByIntPoint)
{
    const RationalPoint rp(BigInt(1), BigInt(1), BigInt(2)); // (0.5, 0.5)
    const IntPoint other(2, 3);
    std::unique_ptr<Vector> diff = rp.difference_by(other);
    const RationalVector* rv = dynamic_cast<const RationalVector*>(diff.get());
    ASSERT_NE(rv, nullptr);
    FloatPoint fp = rv->to_float();
    EXPECT_NEAR(fp.x, -1.5, 1e-9);
    EXPECT_NEAR(fp.y, -2.5, 1e-9);
}

TEST(RationalPointTest, DifferenceByRationalPoint)
{
    const RationalPoint rp(BigInt(3), BigInt(3), BigInt(2)); // (1.5, 1.5)
    const RationalPoint other(BigInt(1), BigInt(1), BigInt(2)); // (0.5, 0.5)
    std::unique_ptr<Vector> diff = rp.difference_by(other);
    const RationalVector* rv = dynamic_cast<const RationalVector*>(diff.get());
    ASSERT_NE(rv, nullptr);
    FloatPoint fp = rv->to_float();
    EXPECT_NEAR(fp.x, 1.0, 1e-9);
    EXPECT_NEAR(fp.y, 1.0, 1e-9);
}

TEST(RationalPointTest, SideOfLine)
{
    const Line line(IntPoint(0, 0), IntPoint(2, 0));
    const RationalPoint rp(BigInt(1), BigInt(1), BigInt(1));
    EXPECT_EQ(rp.side_of(line), Side::ON_THE_LEFT);
    const RationalPoint collinear(BigInt(3), BigInt(0), BigInt(2)); // (1.5, 0)
    EXPECT_EQ(collinear.side_of(line), Side::COLLINEAR);
}

TEST(RationalPointTest, PerpendicularProjectionSimple)
{
    const Line line(IntPoint(0, 0), IntPoint(2, 0));
    const RationalPoint rp(BigInt(1), BigInt(1), BigInt(1));
    std::unique_ptr<Point> proj = rp.perpendicular_projection(line);
    const IntPoint* ip = dynamic_cast<const IntPoint*>(proj.get());
    ASSERT_NE(ip, nullptr);
    EXPECT_EQ(ip->x, 1);
    EXPECT_EQ(ip->y, 0);
}

TEST(RationalPointTest, PerpendicularProjectionRationalResult)
{
    const Line line(IntPoint(0, 0), IntPoint(1, 1));
    const RationalPoint rp(BigInt(1), BigInt(0), BigInt(1));
    std::unique_ptr<Point> proj = rp.perpendicular_projection(line);
    const RationalPoint* r = dynamic_cast<const RationalPoint*>(proj.get());
    ASSERT_NE(r, nullptr);
    FloatPoint fp = r->to_float();
    EXPECT_NEAR(fp.x, 0.5, 1e-9);
    EXPECT_NEAR(fp.y, 0.5, 1e-9);
}

TEST(RationalPointTest, CompareXIntAndRational)
{
    const IntPoint i(1, 0);
    const RationalPoint rp(BigInt(2), BigInt(1), BigInt(3)); // x = 2/3
    EXPECT_EQ(rp.compare_x(i), -1); // 2/3 < 1
    EXPECT_EQ(i.compare_x(rp), 1);  // 1 > 2/3
}

TEST(RationalPointTest, CompareXRationalAndRational)
{
    const RationalPoint a(BigInt(1), BigInt(0), BigInt(3)); // x = 1/3
    const RationalPoint b(BigInt(1), BigInt(0), BigInt(2)); // x = 1/2
    EXPECT_LT(a.compare_x(b), 0);
    EXPECT_GT(b.compare_x(a), 0);
    EXPECT_EQ(b.compare_y(a), 0); // both y coordinates are equal
}

TEST(RationalVectorTest, ConstructorNormalizesNegativeZ)
{
    const RationalVector rv(BigInt(1), BigInt(2), BigInt(-1));
    EXPECT_EQ(rv.x, -1);
    EXPECT_EQ(rv.y, -2);
    EXPECT_EQ(rv.z, 1);
}

TEST(RationalVectorTest, IsZero)
{
    const RationalVector zero(BigInt(0), BigInt(0), BigInt(5));
    EXPECT_TRUE(zero.is_zero());
    const RationalVector not_zero(BigInt(1), BigInt(0), BigInt(5));
    EXPECT_FALSE(not_zero.is_zero());
}

TEST(RationalVectorTest, Add)
{
    const RationalVector a(BigInt(1), BigInt(1), BigInt(2)); // (0.5, 0.5)
    const RationalVector b(BigInt(1), BigInt(1), BigInt(4)); // (0.25, 0.25)
    std::unique_ptr<Vector> sum = a.add(b);
    const RationalVector* r = dynamic_cast<const RationalVector*>(sum.get());
    ASSERT_NE(r, nullptr);
    FloatPoint fp = r->to_float();
    EXPECT_NEAR(fp.x, 0.75, 1e-9);
    EXPECT_NEAR(fp.y, 0.75, 1e-9);
}

TEST(RationalVectorTest, Negate)
{
    const RationalVector a(BigInt(1), BigInt(2), BigInt(3));
    std::unique_ptr<Vector> neg = a.negate();
    const RationalVector* r = dynamic_cast<const RationalVector*>(neg.get());
    ASSERT_NE(r, nullptr);
    EXPECT_EQ(r->x, -1);
    EXPECT_EQ(r->y, -2);
    EXPECT_EQ(r->z, 3);
}

TEST(RationalVectorTest, SideOf)
{
    const RationalVector a(BigInt(0), BigInt(1), BigInt(1)); // up
    const RationalVector b(BigInt(1), BigInt(0), BigInt(1)); // right
    EXPECT_EQ(a.side_of(b), Side::ON_THE_LEFT);
    const IntVector right(1, 0);
    EXPECT_EQ(a.side_of(right), Side::ON_THE_LEFT);
}

TEST(RationalVectorTest, ScalarProductAndProjection)
{
    const RationalVector a(BigInt(1), BigInt(2), BigInt(1));
    const IntVector b(2, 1);
    EXPECT_NEAR(a.scalar_product(b), 4.0, 1e-9);
    EXPECT_EQ(a.projection(b), datastructures::Signum::POSITIVE);
    const RationalVector c(BigInt(1), BigInt(-2), BigInt(1));
    EXPECT_EQ(c.projection(b), datastructures::Signum::ZERO);
}

TEST(RationalVectorTest, AddToIntPoint)
{
    const RationalVector a(BigInt(1), BigInt(1), BigInt(2)); // (0.5, 0.5)
    std::unique_ptr<Point> p = a.add_to(IntPoint(2, 2));
    const RationalPoint* r = dynamic_cast<const RationalPoint*>(p.get());
    ASSERT_NE(r, nullptr);
    FloatPoint fp = r->to_float();
    EXPECT_NEAR(fp.x, 2.5, 1e-9);
    EXPECT_NEAR(fp.y, 2.5, 1e-9);
}

TEST(RationalVectorTest, Turn90Degree)
{
    const RationalVector a(BigInt(1), BigInt(0), BigInt(1));
    std::unique_ptr<Vector> turned = a.turn_90_degree(1);
    const RationalVector* r = dynamic_cast<const RationalVector*>(turned.get());
    ASSERT_NE(r, nullptr);
    EXPECT_EQ(r->x, 0);
    EXPECT_EQ(r->y, 1);
    EXPECT_EQ(r->z, 1);
}

TEST(RationalVectorTest, ToNormalizedDirectionBigReturnsBigIntDirection)
{
    const BigInt big(10000000000LL);
    const RationalVector a(big, BigInt(3), BigInt(1));
    std::unique_ptr<Direction> dir = a.to_normalized_direction();
    const BigIntDirection* b = dynamic_cast<const BigIntDirection*>(dir.get());
    ASSERT_NE(b, nullptr);
    EXPECT_EQ(b->x, big);
    EXPECT_EQ(b->y, 3);
}

TEST(RationalVectorTest, GetInstanceReturnsRationalBeyondCritInt)
{
    std::unique_ptr<Vector> v = Vector::get_instance(2LL * Limits::CRIT_INT, 0);
    const RationalVector* r = dynamic_cast<const RationalVector*>(v.get());
    ASSERT_NE(r, nullptr);
    EXPECT_EQ(r->x, 2LL * Limits::CRIT_INT);
    EXPECT_EQ(r->z, 1);
}

TEST(BigIntDirectionTest, GetVectorAndOpposite)
{
    const BigIntDirection dir(BigInt(1), BigInt(2));
    std::unique_ptr<Vector> v = dir.get_vector();
    const RationalVector* rv = dynamic_cast<const RationalVector*>(v.get());
    ASSERT_NE(rv, nullptr);
    EXPECT_EQ(rv->x, 1);
    EXPECT_EQ(rv->y, 2);
    std::unique_ptr<Direction> opp = dir.opposite();
    const BigIntDirection* o = dynamic_cast<const BigIntDirection*>(opp.get());
    ASSERT_NE(o, nullptr);
    EXPECT_EQ(o->x, -1);
    EXPECT_EQ(o->y, -2);
}

TEST(BigIntDirectionTest, IsOrthogonalAndDiagonal)
{
    const BigIntDirection ortho(BigInt(5), BigInt(0));
    EXPECT_TRUE(ortho.is_orthogonal());
    EXPECT_FALSE(ortho.is_diagonal());
    const BigIntDirection diag(BigInt(5), BigInt(-5));
    EXPECT_TRUE(diag.is_diagonal());
}

TEST(BigIntDirectionTest, CompareOrdering)
{
    const BigIntDirection right(BigInt(1), BigInt(0));
    const BigIntDirection up(BigInt(0), BigInt(1));
    EXPECT_LT(right.compareTo(up), 0);
    EXPECT_GT(up.compareTo(right), 0);
    const BigIntDirection left(BigInt(-1), BigInt(0));
    EXPECT_GT(left.compareTo(right), 0);
    EXPECT_EQ(right.compareTo(right), 0);
}

TEST(BigIntDirectionTest, CompareWithIntDirection)
{
    const BigIntDirection big_right(BigInt(1), BigInt(0));
    EXPECT_EQ(big_right.compareTo(IntDirection::RIGHT), 0);
    EXPECT_LT(big_right.compareTo(IntDirection::UP), 0);
}

TEST(RationalInteropTest, IntPointDifferenceByRationalPoint)
{
    const IntPoint i(3, 1);
    const RationalPoint rp(BigInt(1), BigInt(1), BigInt(2)); // (0.5, 0.5)
    std::unique_ptr<Vector> diff = i.difference_by(rp);
    const RationalVector* rv = dynamic_cast<const RationalVector*>(diff.get());
    ASSERT_NE(rv, nullptr);
    FloatPoint fp = rv->to_float();
    EXPECT_NEAR(fp.x, 2.5, 1e-9);
    EXPECT_NEAR(fp.y, 0.5, 1e-9);
}

TEST(RationalInteropTest, IntPointTranslateByRationalVector)
{
    const IntPoint i(1, 1);
    const RationalVector v(BigInt(1), BigInt(1), BigInt(2)); // (0.5, 0.5)
    std::unique_ptr<Point> p = i.translate_by(v);
    const RationalPoint* r = dynamic_cast<const RationalPoint*>(p.get());
    ASSERT_NE(r, nullptr);
    FloatPoint fp = r->to_float();
    EXPECT_NEAR(fp.x, 1.5, 1e-9);
    EXPECT_NEAR(fp.y, 1.5, 1e-9);
}

TEST(RationalInteropTest, IntVectorSideOfRationalVector)
{
    const IntVector a(0, 1); // up
    const RationalVector b(BigInt(1), BigInt(0), BigInt(1)); // right
    // det(a, b) = 0*0 - 1*1 = -1 < 0 => ON_THE_LEFT
    EXPECT_EQ(a.side_of(b), Side::ON_THE_LEFT);
}

TEST(RationalInteropTest, LineSideOfRationalPoint)
{
    const Line line(IntPoint(0, 0), IntPoint(2, 0));
    const RationalPoint rp(BigInt(1), BigInt(1), BigInt(1));
    EXPECT_EQ(line.side_of(rp), Side::ON_THE_LEFT);
}

TEST(RationalInteropTest, LineIntersectionProducesRationalPoint)
{
    // Lines that intersect in a point with half-integer coordinates go through
    // the rational perpendicular projection path inside Line::intersection.
    Line l1 = Line::get_instance(IntPoint(0, 0), IntDirection(1, 0));
    Line l2 = Line::get_instance(IntPoint(0, 1), IntDirection(1, 1));
    std::unique_ptr<Point> p = l1.intersection(l2);
    ASSERT_NE(p, nullptr);
    FloatPoint fp = p->to_float();
    EXPECT_NEAR(fp.x, -1.0, 1e-9);
    EXPECT_NEAR(fp.y, 0.0, 1e-9);
}

TEST(RationalInteropTest, PointGetInstanceWithNonUnitZ)
{
    std::unique_ptr<Point> p = Point::get_instance(1, 1, 2);
    const RationalPoint* r = dynamic_cast<const RationalPoint*>(p.get());
    ASSERT_NE(r, nullptr);
    EXPECT_EQ(r->x, 1);
    EXPECT_EQ(r->y, 1);
    EXPECT_EQ(r->z, 2);
}

TEST(RationalInteropTest, PointGetInstanceUsesRationalPointForLargeCoordinates)
{
    const std::unique_ptr<Point> point =
        Point::get_instance(Limits::CRIT_INT + 1, 0);

    EXPECT_NE(dynamic_cast<const RationalPoint*>(point.get()), nullptr);
}

TEST(RationalInteropTest, PointGetInstanceReducesFraction)
{
    std::unique_ptr<Point> p = Point::get_instance(4, 6, 2);
    const IntPoint* i = dynamic_cast<const IntPoint*>(p.get());
    ASSERT_NE(i, nullptr);
    EXPECT_EQ(i->x, 2);
    EXPECT_EQ(i->y, 3);
}

TEST(RationalInteropTest, DirectionGetInstanceOfRationalVector)
{
    const RationalVector v(BigInt(2), BigInt(2), BigInt(1));
    std::unique_ptr<Direction> dir = Direction::get_instance(v);
    const IntDirection* i = dynamic_cast<const IntDirection*>(dir.get());
    ASSERT_NE(i, nullptr);
    EXPECT_EQ(i->x, 1);
    EXPECT_EQ(i->y, 1);
}

} // namespace freerouting::geometry::planar