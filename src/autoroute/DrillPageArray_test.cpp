#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include "autoroute/AutorouteEngine.h"
#include "autoroute/DrillPage.h"
#include "autoroute/DrillPageArray.h"
#include "autoroute/ExpansionDrill.h"
#include "board/Layer.h"
#include "board/PolylineTrace.h"
#include "board/RoutingBoard.h"
#include "board/ShapeSearchTree.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/Polyline.h"
#include "rules/BoardRules.h"

namespace fr = freerouting;

namespace {

std::shared_ptr<fr::board::RoutingBoard> make_routing_board()
{
    std::vector<std::shared_ptr<fr::board::Layer>> layers;
    layers.push_back(std::make_shared<fr::board::Layer>("Top", true));
    layers.push_back(std::make_shared<fr::board::Layer>("Bottom", true));
    fr::board::LayerStructure layer_structure(layers);
    auto board_rules = std::make_unique<fr::rules::BoardRules>(layer_structure, 10);
    return std::make_shared<fr::board::RoutingBoard>(
        fr::geometry::planar::IntBox(0, 0, 10000, 10000), layer_structure,
        std::move(board_rules), nullptr);
}

fr::board::PolylineTrace* insert_trace(
    fr::board::RoutingBoard* p_board,
    const std::vector<fr::geometry::planar::IntPoint>& p_corners,
    const std::vector<int>& p_nets)
{
    auto polyline = std::make_shared<fr::geometry::planar::Polyline>(
        fr::geometry::planar::Polyline::from_corners(p_corners));
    return p_board->insert_trace_without_cleaning(polyline, 0, 50, p_nets, 1,
                                                  fr::board::FixedState::NOT_FIXED);
}

} // namespace

TEST(DrillPageArrayTest, GridLayout)
{
    auto board = make_routing_board();
    fr::autoroute::DrillPageArray array(board.get(), 5000);

    EXPECT_EQ(array.get_bounds(), fr::geometry::planar::IntBox(0, 0, 10000, 10000));
    EXPECT_EQ(array.get_column_count(), 2);
    EXPECT_EQ(array.get_row_count(), 2);
    EXPECT_EQ(array.get_page_width(), 5000);
    EXPECT_EQ(array.get_page_height(), 5000);
}

TEST(DrillPageArrayTest, PageAccess)
{
    auto board = make_routing_board();
    fr::autoroute::DrillPageArray array(board.get(), 5000);

    EXPECT_NE(array.get_page(0, 0), nullptr);
    EXPECT_NE(array.get_page(1, 1), nullptr);
    EXPECT_EQ(array.get_page(2, 0), nullptr);
    EXPECT_EQ(array.get_page(0, 2), nullptr);
    EXPECT_EQ(array.get_page(-1, 0), nullptr);

    EXPECT_EQ(array.get_page(1, 1)->shape, fr::geometry::planar::IntBox(5000, 5000, 10000, 10000));
    EXPECT_EQ(array.get_page(0, 1)->shape, fr::geometry::planar::IntBox(5000, 0, 10000, 5000));
    EXPECT_EQ(array.get_page(1, 0)->shape, fr::geometry::planar::IntBox(0, 5000, 5000, 10000));

    array.reset();
}

TEST(DrillPageArrayTest, OverlappingPages)
{
    auto board = make_routing_board();
    fr::autoroute::DrillPageArray array(board.get(), 5000);

    // A shape inside one page.
    auto inside = array.overlapping_pages(fr::geometry::planar::IntBox(1000, 1000, 2000, 2000));
    ASSERT_EQ(inside.size(), 1u);
    EXPECT_EQ(inside[0]->shape, fr::geometry::planar::IntBox(0, 0, 5000, 5000));

    // A shape spanning the whole board covers every page.
    auto all = array.overlapping_pages(fr::geometry::planar::IntBox(0, 0, 10000, 10000));
    EXPECT_EQ(all.size(), 4u);

    // A shape spanning two columns in the first row.
    auto two_cols = array.overlapping_pages(fr::geometry::planar::IntBox(0, 0, 9000, 3000));
    ASSERT_EQ(two_cols.size(), 2u);
    EXPECT_EQ(two_cols[0]->shape, fr::geometry::planar::IntBox(0, 0, 5000, 5000));
    EXPECT_EQ(two_cols[1]->shape, fr::geometry::planar::IntBox(5000, 0, 10000, 5000));

    // A shape crossing all four quarters.
    auto quarters = array.overlapping_pages(fr::geometry::planar::IntBox(4000, 4000, 6000, 6000));
    EXPECT_EQ(quarters.size(), 4u);

    // Shapes outside the board or with no 2-dimensional overlap give nothing.
    EXPECT_TRUE(
        array.overlapping_pages(fr::geometry::planar::IntBox(20000, 20000, 30000, 30000)).empty());
    EXPECT_TRUE(array.overlapping_pages(fr::geometry::planar::IntBox(5000, 0, 5000, 3000)).empty());
}

TEST(DrillPageArrayTest, InvalidateRecomputesAffectedPages)
{
    auto board = make_routing_board();
    fr::autoroute::AutorouteEngine engine(board.get(), 1, false);
    engine.set_net_no(1);
    fr::autoroute::DrillPageArray array(board.get(), 5000);

    fr::autoroute::DrillPage* affected = array.get_page(1, 1);
    ASSERT_NE(affected, nullptr);
    auto drills = affected->get_drills(&engine, false);
    EXPECT_EQ(drills.size(), 1u);

    // The obstacle crosses the affected page (5000..10000 in both axes) before
    // the engine search tree is built, so it becomes a drill cutout.
    fr::board::PolylineTrace* trace =
        insert_trace(board.get(),
                     {fr::geometry::planar::IntPoint(6000, 7500), fr::geometry::planar::IntPoint(9000, 7500)},
                     {2});
    engine.get_autoroute_search_tree()->insert(trace);

    // The cached value is still served until the page is invalidated.
    EXPECT_EQ(affected->get_drills(&engine, false).size(), 1u);

    array.invalidate(fr::geometry::planar::IntBox(6000, 7000, 9000, 8000));

    auto recomputed = affected->get_drills(&engine, false);
    EXPECT_GE(recomputed.size(), 2u);
    for (fr::autoroute::ExpansionDrill* drill : recomputed)
    {
        // The obstacle trace centerline is y = 7500 (x 6000..9000, half width
        // 50). Its shape must not be contained in any drill piece; note that a
        // convex piece beside the obstacle can still have its centre of gravity
        // inside the obstacle band, so check containment instead of the y band.
        EXPECT_FALSE(drill->get_shape().contains(fr::geometry::planar::IntPoint(7500, 7500)));
    }
}