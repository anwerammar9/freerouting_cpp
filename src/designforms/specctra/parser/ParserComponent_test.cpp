// Tests for the placement scope readers (Java parser.Component, ComponentPlacement, PlaceControl).

#include "designforms/specctra/parser/Component.h"

#include <gtest/gtest.h>

#include "board/RoutingBoard.h"
#include "designforms/specctra/parser/Keyword.h"
#include "designforms/specctra/parser/ReadScopeParameter.h"
#include "designforms/specctra/parser/SpecctraDsnStreamReader.h"

namespace freerouting::designforms::specctra::parser {
namespace {

TEST(ParserComponentTest, PlacementScopeCollectsComponentLocations)
{
    SpecctraDsnStreamReader scanner(
        "(component \"R0805\" (place R1 1000 2000 front 90) (place R2 1500.5 2000 back 0 (lock_type position)))"
        "(component C0603 (place C1 0 0 front 45 (pin 1 (clearance_class wide)) (keepout k1 (clearance_class kc))"
        "  (via_keepout v1 (clearance_class vc)) (place_keepout p1 (clearance_class pc)) (PN \"470nF\") (unknown 1)))"
        "(component NotPlaced (place U9)))",
        true);
    ReadScopeParameter par(scanner);
    ASSERT_TRUE(Keyword::PLACEMENT_SCOPE->read_scope(par));
    ASSERT_EQ(3u, par.placement_list.size());

    const ComponentPlacement& r = par.placement_list[0];
    EXPECT_EQ("R0805", r.lib_name);
    ASSERT_EQ(2u, r.locations.size());
    EXPECT_EQ("R1", r.locations[0].name);
    ASSERT_TRUE(r.locations[0].coor.has_value());
    EXPECT_DOUBLE_EQ(1000, (*r.locations[0].coor)[0]);
    EXPECT_DOUBLE_EQ(2000, (*r.locations[0].coor)[1]);
    EXPECT_TRUE(r.locations[0].is_front);
    EXPECT_DOUBLE_EQ(90, r.locations[0].rotation);
    EXPECT_FALSE(r.locations[0].position_fixed);
    EXPECT_FALSE(r.locations[1].is_front);
    EXPECT_DOUBLE_EQ(1500.5, (*r.locations[1].coor)[0]);
    EXPECT_TRUE(r.locations[1].position_fixed);

    const ComponentPlacement::ComponentLocation& c1 = par.placement_list[1].locations.at(0);
    EXPECT_EQ("wide", c1.pin_infos.at("1").clearance_class);
    EXPECT_EQ("kc", c1.keepout_infos.at("k1").clearance_class);
    EXPECT_EQ("vc", c1.via_keepout_infos.at("v1").clearance_class);
    EXPECT_EQ("pc", c1.place_keepout_infos.at("p1").clearance_class);
    EXPECT_EQ("470nF", c1.part_number);

    const ComponentPlacement::ComponentLocation& u9 = par.placement_list[2].locations.at(0);
    EXPECT_EQ("U9", u9.name);
    EXPECT_FALSE(u9.coor.has_value()); // Java null: not placed yet
}

TEST(ParserComponentTest, ItemClearanceInfoWithoutClassFails)
{
    SpecctraDsnStreamReader scanner("(component X (place X1 0 0 front 0 (pin 1 (other 2)))))", true);
    ReadScopeParameter par(scanner);
    EXPECT_FALSE(Keyword::PLACEMENT_SCOPE->read_scope(par));
}

TEST(ParserComponentTest, FlipStyleInPlaceControlAndInStructure)
{
    SpecctraDsnStreamReader structure_scanner(
        "(structure (layer F.Cu (type signal)) (boundary (rect pcb 0 0 100 100)) (flip_style rotate_first))"
        "(place_control (flip_style mirror_first)))",
        true);
    ReadScopeParameter par(structure_scanner);
    ASSERT_TRUE(Keyword::PCB_SCOPE->read_scope(par));
    ASSERT_NE(nullptr, par.get_board());
    EXPECT_TRUE(par.get_board()->get_flip_style_rotate_first()); // mirror_first in place_control does not reset it

    SpecctraDsnStreamReader control_scanner(
        "(structure (layer F.Cu (type signal)) (boundary (rect pcb 0 0 100 100)))"
        "(place_control (flip_style rotate_first)))",
        true);
    ReadScopeParameter control_par(control_scanner);
    ASSERT_TRUE(Keyword::PCB_SCOPE->read_scope(control_par));
    EXPECT_TRUE(control_par.get_board()->get_flip_style_rotate_first());
}

} // namespace
} // namespace freerouting::designforms::specctra::parser
