#include <iostream>
#include <cassert>
#include <cmath>
#include <filesystem>
#include "designforms/specctra/Scanner.h"
#include "geometry/planar/Point.h"
#include "geometry/planar/Vector.h"
#include "geometry/planar/LineSegment.h"
#include "geometry/planar/Polyline.h"

namespace fs = std::filesystem;

using namespace freerouting::designforms::specctra;
using namespace freerouting::geometry::planar;

std::string get_test_file_path(const std::string& filename)
{
    // Try current directory
    if (fs::exists(filename)) return filename;
    // Try tests/data/
    if (fs::exists(fs::path("tests") / "data" / filename)) 
        return (fs::path("tests") / "data" / filename).string();
    // Try ../tests/data/ (from build/ or similar)
    if (fs::exists(fs::path("..") / "tests" / "data" / filename))
        return (fs::path("..") / "tests" / "data" / filename).string();
    // Try ../../tests/data/ (from build/tests/data/)
    if (fs::exists(fs::path("..") / ".." / "tests" / "data" / filename))
        return (fs::path("..") / ".." / "tests" / "data" / filename).string();
    // Try ../../../tests/data/ (from build/tests/data when ctest chdir differs)
    if (fs::exists(fs::path("..") / ".." / ".." / "tests" / "data" / filename))
        return (fs::path("..") / ".." / ".." / "tests" / "data" / filename).string();
    
    return filename; // Fallback
}

void test_scanner_empty_board()
{
    std::string path = get_test_file_path("empty_board.dsn");
    auto result = read_dsn(path);
    if (!result.success) {
        std::cerr << "Failed to read " << path << ": " << result.error_message << std::endl;
        std::cerr << "Current path: " << fs::current_path() << std::endl;
    }
    std::cout << "  empty_board.dsn: layers=" << result.layers.size()
              << " width=" << result.board_width << " height=" << result.board_height << std::endl;
    assert(result.success == true);
    std::cout << "test_scanner_empty_board PASSED" << std::endl;
}

void test_scanner_bbd_mars64()
{
    auto result = read_dsn(get_test_file_path("BBD_Mars-64.dsn"));
    std::cout << "  BBD_Mars-64.dsn: layers=" << result.layers.size()
              << " nets=" << result.nets.size() << " vias=" << result.vias.size() << std::endl;
    assert(result.success == true);
    assert(result.layers.size() == 2);
    assert(result.nets.size() == 94);
    std::cout << "test_scanner_bbd_mars64 PASSED" << std::endl;
}

void test_scanner_issue026()
{
    auto result = read_dsn(get_test_file_path("Issue026-J2_reference.dsn"));
    std::cout << "  Issue026-J2_reference.dsn: layers=" << result.layers.size()
              << " nets=" << result.nets.size() << std::endl;
    assert(result.success == true);
    assert(result.layers.size() > 0);
    assert(result.nets.size() > 0);
    std::cout << "test_scanner_issue026 PASSED" << std::endl;
}

void test_scanner_issue103()
{
    auto result = read_dsn(get_test_file_path("Issue103-Board-Unrouted.dsn"));
    std::cout << "  Issue103-Board-Unrouted.dsn: layers=" << result.layers.size()
              << " nets=" << result.nets.size() << std::endl;
    assert(result.success == true);
    assert(result.layers.size() > 0);
    assert(result.nets.size() > 0);
    std::cout << "test_scanner_issue103 PASSED" << std::endl;
}

void test_intpoint_projections_and_corners()
{
    const IntPoint base(5, 9);
    const IntPoint other(3, 2);

    const IntPoint orth = base.orthogonal_projection(other);
    assert(orth == IntPoint(3, 9));

    const IntPoint p45 = IntPoint(4, 2).fortyfive_degree_projection(IntPoint(0, 0));
    assert(p45 == IntPoint(3, 3));

    const IntPoint left_corner = IntPoint(0, 0).fortyfive_degree_corner(IntPoint(10, 4), true);
    assert(left_corner == IntPoint(6, 0));

    const IntPoint right_corner = IntPoint(0, 0).fortyfive_degree_corner(IntPoint(10, 4), false);
    assert(right_corner == IntPoint(4, 4));

    const IntPoint left_90 = IntPoint(0, 0).ninety_degree_corner(IntPoint(7, 3), true);
    assert(left_90 == IntPoint(7, 0));

    const IntPoint right_90 = IntPoint(0, 0).ninety_degree_corner(IntPoint(7, 3), false);
    assert(right_90 == IntPoint(0, 3));

    std::cout << "test_intpoint_projections_and_corners PASSED" << std::endl;
}

void test_line_geometry_ops()
{
    const Line vertical(IntPoint(2, 0), IntPoint(2, 8));
    const Line horizontal(IntPoint(-3, 3), IntPoint(7, 3));
    const Line diagonal(IntPoint(0, 0), IntPoint(4, 4));
    const Line diagonal_shifted(IntPoint(1, 1), IntPoint(5, 5));

    assert(vertical.is_perpendicular_to(horizontal));
    assert(diagonal.is_parallel_to(diagonal_shifted));
    assert(diagonal.overlaps(diagonal_shifted));

    const FloatPoint is = vertical.intersection_approx(horizontal);
    assert(std::abs(is.x - 2.0) < 1e-9);
    assert(std::abs(is.y - 3.0) < 1e-9);

    const double dist = horizontal.signed_distance(FloatPoint(2.0, 6.0));
    assert(dist < 0.0);
    assert(std::abs(std::abs(dist) - 3.0) < 1e-9);

    assert(std::abs(diagonal.length_approx() - std::sqrt(32.0)) < 1e-9);

    std::cout << "test_line_geometry_ops PASSED" << std::endl;
}

void test_line_segment_basics()
{
    const LineSegment s1(IntPoint(0, 0), IntPoint(10, 0));
    const LineSegment s2(IntPoint(5, -2), IntPoint(5, 3));
    const LineSegment s3(IntPoint(20, 20), IntPoint(30, 30));

    assert(s1.contains(IntPoint(0, 0)));
    assert(s1.contains(IntPoint(4, 0)));
    assert(!s1.contains(IntPoint(4, 1)));

    const IntBox box = s1.bounding_box();
    assert(box.ll == IntPoint(0, 0));
    assert(box.ur == IntPoint(10, 0));

    assert(s1.intersects(s2));
    assert(!s1.intersects(s3));

    std::cout << "test_line_segment_basics PASSED" << std::endl;
}

void test_polyline_core_ops()
{
    const std::vector<IntPoint> points = {
        IntPoint(0, 0),
        IntPoint(4, 0),
        IntPoint(4, 3)
    };
    const Polyline pl(points);
    assert(pl.corner_count() == 1);
    assert(pl.is_orthogonal());

    const IntPoint c0 = pl.corner(0);
    assert(c0 == IntPoint(4, 0));
    assert(pl.first_corner() == IntPoint(4, 0));
    assert(pl.last_corner() == IntPoint(4, 0));

    const IntBox bb = pl.bounding_box();
    assert(bb.ll == IntPoint(4, 0));
    assert(bb.ur == IntPoint(4, 0));

    const auto moved = pl.translate_by(IntVector(2, 5));
    assert(moved->corner(0) == IntPoint(6, 5));

    const std::vector<IntPoint> points2 = {
        IntPoint(0, 0),
        IntPoint(10, 0),
        IntPoint(10, 10),
        IntPoint(20, 10)
    };
    const Polyline pl2(points2);
    const double d = pl2.distance(FloatPoint(10.0, 4.0));
    assert(std::abs(d) < 1e-9);

    std::cout << "test_polyline_core_ops PASSED" << std::endl;
}

int main()
{
    std::cout << "Running DSN Scanner tests..." << std::endl;
    
    test_scanner_empty_board();
    test_scanner_bbd_mars64();
    test_scanner_issue026();
    test_scanner_issue103();
    test_intpoint_projections_and_corners();
    test_line_geometry_ops();
    test_line_segment_basics();
    test_polyline_core_ops();
    
    std::cout << "All tests PASSED!" << std::endl;
    return 0;
}