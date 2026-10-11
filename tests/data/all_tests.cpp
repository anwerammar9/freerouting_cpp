#include <iostream>
#include <cassert>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <memory>
#include <variant>
#include "board/RoutingBoard.h"
#include "designforms/specctra/DsnReader.h"
#include "rules/BoardRules.h"
#include "rules/Net.h"
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

/** Loads a DSN with the DsnReader (the board of a Success result, else nullptr). */
std::unique_ptr<freerouting::board::RoutingBoard> load_dsn(const std::string& p_path)
{
    std::ifstream input(p_path, std::ios::binary);
    BoardReadResult result = DsnReader::read_board(input, p_path);
    if (auto* success = std::get_if<board_read_result::Success>(&result))
    {
        return std::move(success->board);
    }
    std::cerr << "Failed to read " << p_path << " (current path " << fs::current_path() << ")" << std::endl;
    return nullptr;
}

int net_count(const freerouting::board::RoutingBoard& p_board)
{
    return p_board.get_rules().get_nets().max_net_no();
}

void test_scanner_empty_board()
{
    auto board = load_dsn(get_test_file_path("empty_board.dsn"));
    assert(board != nullptr);
    std::cout << "  empty_board.dsn: layers=" << board->get_layer_count() << std::endl;
    assert(board->get_layer_count() == 2);
    std::cout << "test_scanner_empty_board PASSED" << std::endl;
}

void test_scanner_bbd_mars64()
{
    auto board = load_dsn(get_test_file_path("BBD_Mars-64.dsn"));
    assert(board != nullptr);
    std::cout << "  BBD_Mars-64.dsn: layers=" << board->get_layer_count() << " nets=" << net_count(*board) << std::endl;
    assert(board->get_layer_count() == 2);
    assert(net_count(*board) == 94);
    std::cout << "test_scanner_bbd_mars64 PASSED" << std::endl;
}

void test_scanner_issue026()
{
    auto board = load_dsn(get_test_file_path("Issue026-J2_reference.dsn"));
    assert(board != nullptr);
    std::cout << "  Issue026-J2_reference.dsn: layers=" << board->get_layer_count() << " nets=" << net_count(*board)
              << std::endl;
    assert(board->get_layer_count() > 0);
    assert(net_count(*board) > 0);
    std::cout << "test_scanner_issue026 PASSED" << std::endl;
}

void test_scanner_issue103()
{
    auto board = load_dsn(get_test_file_path("Issue103-Board-Unrouted.dsn"));
    assert(board != nullptr);
    std::cout << "  Issue103-Board-Unrouted.dsn: layers=" << board->get_layer_count() << " nets=" << net_count(*board)
              << std::endl;
    assert(board->get_layer_count() > 0);
    assert(net_count(*board) > 0);
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
    const LineSegment s1 = LineSegment::from_endpoints(IntPoint(0, 0), IntPoint(10, 0));
    const LineSegment s2 = LineSegment::from_endpoints(IntPoint(5, -2), IntPoint(5, 3));
    const LineSegment s3 = LineSegment::from_endpoints(IntPoint(20, 20), IntPoint(30, 30));

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