/**
 * @file Line_test.cpp
 * @brief Unit tests for Line class
 */

#include "geometry/planar/Line.h"
#include <gtest/gtest.h>
#include <memory>
#include <vector>

namespace freerouting {
namespace geometry {
namespace planar {

class LineTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test lines
        horizontal_line = std::make_shared<Line>(0, 0, 10, 0);
        vertical_line = std::make_shared<Line>(0, 0, 0, 10);
        diagonal_line = std::make_shared<Line>(0, 0, 10, 10);
        anti_diagonal_line = std::make_shared<Line>(0, 10, 10, 0);
        general_line = std::make_shared<Line>(1, 2, 5, 7);
    }
    
    std::shared_ptr<Line> horizontal_line;
    std::shared_ptr<Line> vertical_line;
    std::shared_ptr<Line> diagonal_line;
    std::shared_ptr<Line> anti_diagonal_line;
    std::shared_ptr<Line> general_line;
};

// Test constructors
TEST_F(LineTest, Constructors) {
    // Test point constructor
    Line line1(IntPoint(0, 0), IntPoint(10, 10));
    EXPECT_EQ(line1.a.x, 0);
    EXPECT_EQ(line1.a.y, 0);
    EXPECT_EQ(line1.b.x, 10);
    EXPECT_EQ(line1.b.y, 10);
    
    // Test coordinate constructor
    Line line2(1, 2, 3, 4);
    EXPECT_EQ(line2.a.x, 1);
    EXPECT_EQ(line2.a.y, 2);
    EXPECT_EQ(line2.b.x, 3);
    EXPECT_EQ(line2.b.y, 4);
    
    // Test point and direction constructor
    Direction dir = Direction::get_instance(IntVector(1, 1));
    Line line3(IntPoint(0, 0), dir);
    EXPECT_EQ(line3.a.x, 0);
    EXPECT_EQ(line3.a.y, 0);
    EXPECT_EQ(line3.b.x, 1);
    EXPECT_EQ(line3.b.y, 1);
}

// Test static factory method
TEST_F(LineTest, GetInstance) {
    Direction dir = Direction::get_instance(IntVector(2, 3));
    Line line = Line::get_instance(IntPoint(5, 5), dir);
    EXPECT_EQ(line.a.x, 5);
    EXPECT_EQ(line.a.y, 5);
    EXPECT_EQ(line.b.x, 7);
    EXPECT_EQ(line.b.y, 8);
}

// Test equality methods
TEST_F(LineTest, Equality) {
    // Test equals
    Line line1(0, 0, 10, 10);
    Line line2(0, 0, 10, 10);
    Line line3(10, 10, 0, 0); // Same line, opposite direction
    Line line4(0, 0, 5, 5);   // Same line, different segment
    
    EXPECT_TRUE(line1.equals(line2));
    EXPECT_TRUE(line1.equals(line3));
    EXPECT_TRUE(line1.equals(line4));
    
    // Test fast_equals
    EXPECT_TRUE(line1.fast_equals(line2));
    EXPECT_TRUE(line1.fast_equals(line3));
    EXPECT_TRUE(line1.fast_equals(line4));
    
    // Test different lines
    Line line5(0, 0, 10, 0);
    EXPECT_FALSE(line1.equals(line5));
    EXPECT_FALSE(line1.fast_equals(line5));
}

// Test direction
TEST_F(LineTest, Direction) {
    Direction horizontal_dir = horizontal_line->direction();
    EXPECT_TRUE(horizontal_dir.is_horizontal());
    
    Direction vertical_dir = vertical_line->direction();
    EXPECT_TRUE(vertical_dir.is_vertical());
    
    Direction diagonal_dir = diagonal_line->direction();
    EXPECT_TRUE(diagonal_dir.is_diagonal());
}

// Test side determination
TEST_F(LineTest, SideOf) {
    // Test horizontal line
    EXPECT_EQ(horizontal_line->side_of(IntPoint(5, 1)), Side::ON_THE_LEFT);
    EXPECT_EQ(horizontal_line->side_of(IntPoint(5, -1)), Side::ON_THE_RIGHT);
    EXPECT_EQ(horizontal_line->side_of(IntPoint(5, 0)), Side::COLLINEAR);
    
    // Test vertical line
    EXPECT_EQ(vertical_line->side_of(IntPoint(1, 5)), Side::ON_THE_LEFT);
    EXPECT_EQ(vertical_line->side_of(IntPoint(-1, 5)), Side::ON_THE_RIGHT);
    EXPECT_EQ(vertical_line->side_of(IntPoint(0, 5)), Side::COLLINEAR);
    
    // Test diagonal line
    EXPECT_EQ(diagonal_line->side_of(IntPoint(1, 3)), Side::ON_THE_LEFT);
    EXPECT_EQ(diagonal_line->side_of(IntPoint(3, 1)), Side::ON_THE_RIGHT);
    EXPECT_EQ(diagonal_line->side_of(IntPoint(2, 2)), Side::COLLINEAR);
}

// Test side determination with float points
TEST_F(LineTest, SideOfFloatPoint) {
    FloatPoint point_left(5.0, 1.0);
    FloatPoint point_right(5.0, -1.0);
    FloatPoint point_on(5.0, 0.0);
    
    EXPECT_EQ(horizontal_line->side_of(point_left), Side::ON_THE_LEFT);
    EXPECT_EQ(horizontal_line->side_of(point_right), Side::ON_THE_RIGHT);
    EXPECT_EQ(horizontal_line->side_of(point_on), Side::COLLINEAR);
    
    // Test with tolerance
    FloatPoint point_near(5.0, 0.01);
    EXPECT_EQ(horizontal_line->side_of(point_near, 0.1), Side::COLLINEAR);
    EXPECT_EQ(horizontal_line->side_of(point_near, 0.001), Side::ON_THE_LEFT);
}

// Test overlaps
TEST_F(LineTest, Overlaps) {
    Line line1(0, 0, 10, 0);
    Line line2(5, 0, 15, 0);  // Same line, different segment
    Line line3(10, 0, 0, 0); // Same line, opposite direction
    Line line4(0, 1, 10, 1);  // Parallel but different line
    
    EXPECT_TRUE(line1.overlaps(line2));
    EXPECT_TRUE(line1.overlaps(line3));
    EXPECT_FALSE(line1.overlaps(line4));
}

// Test opposite
TEST_F(LineTest, Opposite) {
    Line line(0, 0, 10, 10);
    Line opposite = line.opposite();
    
    EXPECT_EQ(opposite.a.x, 10);
    EXPECT_EQ(opposite.a.y, 10);
    EXPECT_EQ(opposite.b.x, 0);
    EXPECT_EQ(opposite.b.y, 0);
    
    EXPECT_TRUE(line.overlaps(opposite));
}

// Test intersection
TEST_F(LineTest, Intersection) {
    // Test perpendicular lines
    auto intersection = horizontal_line->intersection(*vertical_line);
    ASSERT_NE(intersection, nullptr);
    EXPECT_EQ(intersection->x, 0);
    EXPECT_EQ(intersection->y, 0);
    
    // Test diagonal intersection
    auto intersection2 = diagonal_line->intersection(*anti_diagonal_line);
    ASSERT_NE(intersection2, nullptr);
    EXPECT_EQ(intersection2->x, 5);
    EXPECT_EQ(intersection2->y, 5);
    
    // Test parallel lines (should return nullptr)
    auto parallel_intersection = horizontal_line->intersection(Line(0, 5, 10, 5));
    EXPECT_EQ(parallel_intersection, nullptr);
}

// Test approximate intersection
TEST_F(LineTest, IntersectionApprox) {
    FloatPoint intersection = horizontal_line->intersection_approx(*vertical_line);
    EXPECT_EQ(intersection.x, 0.0);
    EXPECT_EQ(intersection.y, 0.0);
    
    // Test parallel lines
    FloatPoint parallel_intersection = horizontal_line->intersection_approx(Line(0, 5, 10, 5));
    EXPECT_TRUE(std::isinf(parallel_intersection.x));
    EXPECT_TRUE(std::isinf(parallel_intersection.y));
}

// Test line properties
TEST_F(LineTest, LineProperties) {
    EXPECT_TRUE(horizontal_line->is_horizontal());
    EXPECT_FALSE(horizontal_line->is_vertical());
    
    EXPECT_TRUE(vertical_line->is_vertical());
    EXPECT_FALSE(vertical_line->is_horizontal());
    
    EXPECT_FALSE(diagonal_line->is_horizontal());
    EXPECT_FALSE(diagonal_line->is_vertical());
}

// Test parallel and perpendicular
TEST_F(LineTest, ParallelPerpendicular) {
    Line parallel1(0, 0, 10, 0);
    Line parallel2(5, 5, 15, 5);
    Line perpendicular1(0, 0, 10, 0);
    Line perpendicular2(5, 0, 5, 10);
    
    EXPECT_TRUE(parallel1.is_parallel_to(parallel2));
    EXPECT_FALSE(parallel1.is_parallel_to(perpendicular2));
    
    EXPECT_TRUE(perpendicular1.is_perpendicular_to(perpendicular2));
    EXPECT_FALSE(perpendicular1.is_perpendicular_to(parallel1));
}

// Test shift operations
TEST_F(LineTest, ShiftOperations) {
    // Shift by vector
    IntVector shift_vec(2, 3);
    Line shifted = horizontal_line->shift_by(shift_vec);
    EXPECT_EQ(shifted.a.x, 2);
    EXPECT_EQ(shifted.a.y, 3);
    EXPECT_EQ(shifted.b.x, 12);
    EXPECT_EQ(shifted.b.y, 3);
    
    // Shift by direction
    Direction dir = Direction::get_instance(IntVector(1, 1));
    Line shifted_dir = horizontal_line->shift_by(dir);
    EXPECT_EQ(shifted_dir.a.x, 1);
    EXPECT_EQ(shifted_dir.a.y, 1);
    EXPECT_EQ(shifted_dir.b.x, 11);
    EXPECT_EQ(shifted_dir.b.y, 1);
    
    // Shift by float point
    FloatPoint shift_fp(1.5, 2.5);
    Line shifted_fp = horizontal_line->shift_by(shift_fp);
    EXPECT_EQ(shifted_fp.a.x, 1);
    EXPECT_EQ(shifted_fp.a.y, 2);
    EXPECT_EQ(shifted_fp.b.x, 11);
    EXPECT_EQ(shifted_fp.b.y, 2);
    
    // Shift by integer amount
    Line shifted_int = horizontal_line->shift_by(5);
    EXPECT_EQ(shifted_int.a.x, 5);
    EXPECT_EQ(shifted_int.a.y, 0);
    EXPECT_EQ(shifted_int.b.x, 15);
    EXPECT_EQ(shifted_int.b.y, 0);
}

// Test determinant
TEST_F(LineTest, Determinant) {
    double det = horizontal_line->determinant(*vertical_line);
    EXPECT_GT(std::abs(det), 0.0);  // Should be non-zero for perpendicular lines
    
    double parallel_det = horizontal_line->determinant(Line(0, 5, 10, 5));
    EXPECT_NEAR(parallel_det, 0.0, 1e-10);  // Should be zero for parallel lines
}

// Test string representation
TEST_F(LineTest, ToString) {
    std::string str = horizontal_line->to_string();
    EXPECT_NE(str.find("Line"), std::string::npos);
    EXPECT_NE(str.find("(0,0)"), std::string::npos);
    EXPECT_NE(str.find("(10,0)"), std::string::npos);
}

// Test comparison operator
TEST_F(LineTest, Comparison) {
    Line line1(0, 0, 10, 0);
    Line line2(0, 0, 10, 0);
    Line line3(0, 1, 10, 1);
    
    EXPECT_FALSE(line1 < line2);  // Equal lines
    EXPECT_TRUE(line1 < line3);   // line1 comes before line3
    EXPECT_FALSE(line3 < line1);  // line3 comes after line1
}

// Test const correctness
TEST_F(LineTest, ConstCorrectness) {
    const Line& const_line = *horizontal_line;
    
    // Should be able to call const methods
    EXPECT_TRUE(const_line.is_horizontal());
    EXPECT_FALSE(const_line.is_vertical());
    EXPECT_EQ(const_line.side_of(IntPoint(5, 1)), Side::ON_THE_LEFT);
    EXPECT_TRUE(const_line.overlaps(*horizontal_line));
    
    Direction dir = const_line.direction();
    EXPECT_TRUE(dir.is_horizontal());
    
    std::string str = const_line.to_string();
    EXPECT_FALSE(str.empty());
    
    double det = const_line.determinant(*vertical_line);
    EXPECT_GT(std::abs(det), 0.0);
}

// Test edge cases
TEST_F(LineTest, EdgeCases) {
    // Test zero-length line
    Line zero_line(5, 5, 5, 5);
    EXPECT_TRUE(zero_line.overlaps(zero_line));
    
    // Test very long line
    Line long_line(-1000, -1000, 1000, 1000);
    EXPECT_TRUE(long_line.is_diagonal());
    
    // Test line with negative coordinates
    Line negative_line(-5, -5, -1, -1);
    EXPECT_TRUE(negative_line.is_diagonal());
    
    // Test line with mixed coordinates
    Line mixed_line(-5, 10, 15, -20);
    EXPECT_FALSE(mixed_line.is_horizontal());
    EXPECT_FALSE(mixed_line.is_vertical());
}

// Test multiple operations
TEST_F(LineTest, MultipleOperations) {
    std::vector<Line> lines;
    lines.push_back(Line(0, 0, 10, 0));
    lines.push_back(Line(0, 0, 0, 10));
    lines.push_back(Line(0, 0, 10, 10));
    lines.push_back(Line(0, 10, 10, 0));
    
    for (const auto& line : lines) {
        // Test basic properties
        EXPECT_FALSE(line.to_string().empty());
        EXPECT_TRUE(line.overlaps(line));
        
        // Test direction
        Direction dir = line.direction();
        EXPECT_TRUE(dir.is_horizontal() || dir.is_vertical() || dir.is_diagonal());
        
        // Test side determination
        EXPECT_EQ(line.side_of(line.a), Side::COLLINEAR);
        EXPECT_EQ(line.side_of(line.b), Side::COLLINEAR);
    }
}

} // namespace planar
} // namespace geometry
} // namespace freerouting
