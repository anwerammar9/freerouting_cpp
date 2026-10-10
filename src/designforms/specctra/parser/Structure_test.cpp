// Tests for the DSN structure scope reader (Java parser.Structure, Layer,
// LayerStructure, Plane): layers, boundaries, planes and layer rules.

#include "designforms/specctra/parser/Structure.h"

#include <gtest/gtest.h>

#include "designforms/specctra/parser/Keyword.h"
#include "designforms/specctra/parser/Rectangle.h"
#include "designforms/specctra/parser/ReadScopeParameter.h"
#include "designforms/specctra/parser/Shape.h"
#include "designforms/specctra/parser/SpecctraDsnStreamReader.h"
#include "designforms/specctra/parser/Path.h"

#include <sstream>

#include "board/Layer.h"
#include "rules/BoardRules.h"
#include "rules/ClearanceMatrix.h"
#include "rules/DefaultItemClearanceClasses.h"
#include "rules/NetClass.h"

namespace freerouting::designforms::specctra::parser {
namespace {

TEST(StructureTest, ReadLayerScopeReadsASignalLayer)
{
    SpecctraDsnStreamReader scanner("F.Cu (type signal))", true);
    Structure::BoardConstructionInfo info;
    ASSERT_TRUE(Structure::read_layer_scope(scanner, info, "\""));
    ASSERT_EQ(1u, info.layer_info.size());
    EXPECT_EQ("F.Cu", info.layer_info[0]->name);
    EXPECT_EQ(0, info.layer_info[0]->no);
    EXPECT_TRUE(info.layer_info[0]->is_signal);
    EXPECT_TRUE(info.layer_info[0]->net_names.empty());
    EXPECT_EQ(1, info.found_layer_count);
}

TEST(StructureTest, ReadLayerScopeReadsAPowerLayerWithNetNames)
{
    SpecctraDsnStreamReader scanner("GND (type power) (use_net GND) (use_net +5V))", true);
    Structure::BoardConstructionInfo info;
    ASSERT_TRUE(Structure::read_layer_scope(scanner, info, "\""));
    ASSERT_EQ(1u, info.layer_info.size());
    EXPECT_EQ("GND", info.layer_info[0]->name);
    EXPECT_FALSE(info.layer_info[0]->is_signal);
    ASSERT_EQ(2u, info.layer_info[0]->net_names.size());
    EXPECT_EQ("GND", info.layer_info[0]->net_names[0]);
    EXPECT_EQ("+5V", info.layer_info[0]->net_names[1]);
    EXPECT_EQ(1, info.found_layer_count);
}

TEST(StructureTest, ReadLayerScopeKeepsJumperTypeAsSignalLayer)
{
    SpecctraDsnStreamReader scanner("JMP (type jumper))", true);
    Structure::BoardConstructionInfo info;
    ASSERT_TRUE(Structure::read_layer_scope(scanner, info, "\""));
    ASSERT_EQ(1u, info.layer_info.size());
    EXPECT_TRUE(info.layer_info[0]->is_signal); // Java: only power clears isSignal
}

TEST(StructureTest, ReadLayerScopeDropsLayersWithUnknownType)
{
    SpecctraDsnStreamReader scanner("Bad (type exotic_layer))", true);
    Structure::BoardConstructionInfo info;
    ASSERT_TRUE(Structure::read_layer_scope(scanner, info, "\""));
    EXPECT_TRUE(info.layer_info.empty());
    EXPECT_EQ(0, info.found_layer_count);
}

TEST(StructureTest, ReadLayerScopeCollectsLayerRules)
{
    SpecctraDsnStreamReader scanner("F.Cu (type signal) (rule (width 12)))", true);
    Structure::BoardConstructionInfo info;
    ASSERT_TRUE(Structure::read_layer_scope(scanner, info, "\""));
    ASSERT_EQ(1u, info.layer_info.size());
    ASSERT_EQ(1u, info.layer_dependent_rules.size());
    EXPECT_EQ("F.Cu", info.layer_dependent_rules[0].layer_name);
    EXPECT_EQ(1u, info.layer_dependent_rules[0].rules.size());
}

TEST(StructureTest, ReadBoundaryScopeReadsTheBoundingRectangle)
{
    SpecctraDsnStreamReader scanner("(rect pcb 0 0 100 100))", true);
    Structure::BoardConstructionInfo info;
    ASSERT_TRUE(Structure::read_boundary_scope(scanner, info));
    ASSERT_NE(nullptr, info.bounding_shape);
    EXPECT_EQ(Layer::pcb(), info.bounding_shape->layer);
    ASSERT_TRUE(info.outline_shapes.empty());
}

TEST(StructureTest, ReadBoundaryScopeReadsOutlineShapesAndClearanceClass)
{
    SpecctraDsnStreamReader scanner(
        "(rect signal 0 0 10 10) (polyline_path signal 0 20 0 30 0) (clearance_class outline))", true);
    Structure::BoardConstructionInfo info;
    ASSERT_TRUE(Structure::read_boundary_scope(scanner, info));
    EXPECT_EQ("outline", info.outline_clearance_class_name);
    ASSERT_EQ(2u, info.outline_shapes.size());
    // the rect on signal and the polyline_path go to the outline shapes
}

TEST(StructureTest, ReadBoundaryScopeAddsSecondPcbShapeToTheOutlines)
{
    SpecctraDsnStreamReader scanner("(rect pcb 0 0 100 100) (rect pcb 200 0 300 100))", true);
    Structure::BoardConstructionInfo info;
    ASSERT_TRUE(Structure::read_boundary_scope(scanner, info));
    EXPECT_EQ(Layer::pcb(), info.bounding_shape->layer);
    ASSERT_EQ(1u, info.outline_shapes.size());
}

TEST(StructureTest, ReadScopeCollectsLayersBoundaryAndPlanes)
{
    SpecctraDsnStreamReader scanner(
        "(layer F.Cu (type signal))"
        "(layer B.Cu (type power) (use_net GND))"
        "(boundary (rect pcb 0 0 100 100))"
        "(plane GND (rect B.Cu 0 0 100 100) (clearance_class wide)))", true);
    ReadScopeParameter par(scanner);
    ASSERT_TRUE(Keyword::STRUCTURE_SCOPE->read_scope(par));
    ASSERT_NE(nullptr, par.layer_structure);
    ASSERT_EQ(2u, par.layer_structure->layers.size());
    EXPECT_EQ("F.Cu", par.layer_structure->layers[0]->name);
    EXPECT_TRUE(par.layer_structure->layers[0]->is_signal);
    EXPECT_FALSE(par.layer_structure->layers[1]->is_signal);
    EXPECT_TRUE(par.layer_structure->contains_plane("GND"));
    ASSERT_EQ(1u, par.plane_list.size());
    EXPECT_EQ("GND", par.plane_list[0].net_name);
    EXPECT_EQ(1u, par.plane_list[0].area.shape_list.size());
    EXPECT_EQ("wide", par.plane_list[0].area.clearance_class_name);
}

TEST(StructureTest, ReadScopeSkipsUnknownScopes)
{
    SpecctraDsnStreamReader scanner(
        "(layer F.Cu (type signal))(control (via_at_smd on))(rule (width 10))"
        "(plane GND (rect F.Cu 0 0 10 10)))",
        true);
    ReadScopeParameter par(scanner);
    ASSERT_TRUE(Keyword::STRUCTURE_SCOPE->read_scope(par));
    ASSERT_NE(nullptr, par.layer_structure);
    EXPECT_EQ(1, par.layer_structure->signal_layer_count());
    ASSERT_EQ(1u, par.plane_list.size());
    EXPECT_EQ("GND", par.plane_list[0].net_name);
}

TEST(PlaneTest, ReadScopeReadsNetNameAndArea)
{
    SpecctraDsnStreamReader scanner("GND (rect B.Cu 0 0 100 100) (clearance_class plane_cl))", true);
    ReadScopeParameter par(scanner);
    par.layer_structure =
        std::make_shared<LayerStructure>(std::vector<std::shared_ptr<Layer>>{
            std::make_shared<Layer>("F.Cu", 0, true), std::make_shared<Layer>("B.Cu", 1, false) });
    ASSERT_TRUE(Keyword::PLANE_SCOPE->read_scope(par));
    ASSERT_EQ(1u, par.plane_list.size());
    EXPECT_EQ("GND", par.plane_list[0].net_name);
    ASSERT_NE(nullptr, dynamic_cast<const Rectangle*>(par.plane_list[0].area.shape_list[0].get()));
    EXPECT_EQ("B.Cu", par.plane_list[0].area.shape_list[0]->layer->name);
    EXPECT_EQ("plane_cl", par.plane_list[0].area.clearance_class_name);
}

TEST(PlaneTest, AllegroHostSkipsWindowScopes)
{
    SpecctraDsnStreamReader scanner("GND (rect B.Cu 0 0 100 100) (window (rect B.Cu 5 5 6 6)))", true);
    ReadScopeParameter par(scanner);
    par.host_cad = "Allegro";
    par.layer_structure =
        std::make_shared<LayerStructure>(std::vector<std::shared_ptr<Layer>>{
            std::make_shared<Layer>("F.Cu", 0, true), std::make_shared<Layer>("B.Cu", 1, false) });
    ASSERT_TRUE(Keyword::PLANE_SCOPE->read_scope(par));
    ASSERT_EQ(1u, par.plane_list.size());
    EXPECT_EQ(1u, par.plane_list[0].area.shape_list.size());
}

TEST(LayerStructureTest, GetNoMapsTopAndBottomOnUnknownNames)
{
    LayerStructure layers({ std::make_shared<Layer>("F.Cu", 0, true),
        std::make_shared<Layer>("Inner", 1, true), std::make_shared<Layer>("B.Cu", 2, true) });
    EXPECT_EQ(0, layers.get_no("Top Copper"));
    EXPECT_EQ(2, layers.get_no("Bottom Copper"));
    EXPECT_EQ(-1, layers.get_no("NoSuch"));
    EXPECT_EQ(3, layers.signal_layer_count());
}

TEST(StructureTest, ReadViaPadstacksReadsNormalAndSpareVias)
{
    SpecctraDsnStreamReader scanner("Top Via1 (spare Spare1 Spare2))", true);
    std::optional<std::vector<std::string>> via_names = Structure::read_via_padstacks(scanner);
    ASSERT_TRUE(via_names.has_value());
    ASSERT_EQ(4u, via_names->size());
    EXPECT_EQ("Top", (*via_names)[0]);
    EXPECT_EQ("Via1", (*via_names)[1]);
    EXPECT_EQ("Spare1", (*via_names)[2]);
    EXPECT_EQ("Spare2", (*via_names)[3]);
}

TEST(StructureTest, ReadViaPadstacksSkipsUnknownSubScopes)
{
    SpecctraDsnStreamReader scanner("Top (unknown_scope (nested 1)) Via1)", true);
    std::optional<std::vector<std::string>> via_names = Structure::read_via_padstacks(scanner);
    ASSERT_TRUE(via_names.has_value());
    ASSERT_EQ(2u, via_names->size());
    EXPECT_EQ("Top", (*via_names)[0]);
    EXPECT_EQ("Via1", (*via_names)[1]);
}

TEST(StructureTest, ReadSnapAngleReadsAllRestrictions)
{
    {
        SpecctraDsnStreamReader scanner("ninety_degree)", true);
        std::optional<board::AngleRestriction> angle = Structure::read_snap_angle(scanner);
        ASSERT_TRUE(angle.has_value());
        EXPECT_EQ(board::AngleRestriction::NINETY_DEGREE, *angle);
    }
    {
        SpecctraDsnStreamReader scanner("fortyfive_degree)", true);
        std::optional<board::AngleRestriction> angle = Structure::read_snap_angle(scanner);
        ASSERT_TRUE(angle.has_value());
        EXPECT_EQ(board::AngleRestriction::FORTYFIVE_DEGREE, *angle);
    }
    {
        SpecctraDsnStreamReader scanner("none)", true);
        std::optional<board::AngleRestriction> angle = Structure::read_snap_angle(scanner);
        ASSERT_TRUE(angle.has_value());
        EXPECT_EQ(board::AngleRestriction::NONE, *angle);
    }
}

TEST(StructureTest, ReadSnapAngleRejectsUnexpectedToken)
{
    SpecctraDsnStreamReader scanner("diagonal)", true);
    EXPECT_FALSE(Structure::read_snap_angle(scanner).has_value());
}

TEST(StructureTest, ReadScopeReadsSnapAngle)
{
    SpecctraDsnStreamReader scanner("(layer F.Cu (type signal))(snap_angle ninety_degree))", true);
    ReadScopeParameter par(scanner);
    ASSERT_TRUE(Keyword::STRUCTURE_SCOPE->read_scope(par));
    EXPECT_EQ(board::AngleRestriction::NINETY_DEGREE, par.snap_angle);
}

TEST(StructureTest, ReadScopeDefaultSnapAngleIsFortyFiveDegree)
{
    SpecctraDsnStreamReader scanner("(layer F.Cu (type signal)))", true);
    ReadScopeParameter par(scanner);
    ASSERT_TRUE(Keyword::STRUCTURE_SCOPE->read_scope(par));
    EXPECT_EQ(board::AngleRestriction::FORTYFIVE_DEGREE, par.snap_angle);
}

TEST(StructureTest, ReadControlScopeReadsViaAtSmd)
{
    SpecctraDsnStreamReader scanner("(via_at_smd on))", true);
    ReadScopeParameter par(scanner);
    ASSERT_TRUE(Structure::read_control_scope(par));
    EXPECT_TRUE(par.via_at_smd_allowed);

    SpecctraDsnStreamReader off_scanner("(via_at_smd off))", true);
    ReadScopeParameter off_par(off_scanner);
    ASSERT_TRUE(Structure::read_control_scope(off_par));
    EXPECT_FALSE(off_par.via_at_smd_allowed);
}

TEST(StructureTest, ReadControlScopeSkipsUnknownSubScopes)
{
    SpecctraDsnStreamReader scanner("(unknown_scope (nested 1))(via_at_smd on))", true);
    ReadScopeParameter par(scanner);
    ASSERT_TRUE(Structure::read_control_scope(par));
    EXPECT_TRUE(par.via_at_smd_allowed);
}

TEST(StructureTest, ReadScopeReadsViaListControlAndKeepouts)
{
    SpecctraDsnStreamReader scanner(
        "(layer F.Cu (type signal))"
        "(via Top Via1)"
        "(control (via_at_smd on))"
        "(boundary (rect pcb 0 0 100 100))"
        "(keepout k1 (rect F.Cu 0 0 10 10))"
        "(via_keepout vk1 (rect F.Cu 0 0 5 5))"
        "(place_keepout pk1 (rect F.Cu 20 20 30 30)))",
        true);
    ReadScopeParameter par(scanner);
    ASSERT_TRUE(Keyword::STRUCTURE_SCOPE->read_scope(par));
    ASSERT_TRUE(par.via_padstack_names.has_value());
    ASSERT_EQ(2u, par.via_padstack_names->size());
    EXPECT_EQ("Top", (*par.via_padstack_names)[0]);
    EXPECT_EQ("Via1", (*par.via_padstack_names)[1]);
    EXPECT_TRUE(par.via_at_smd_allowed);
    ASSERT_EQ(1u, par.keepout_list.size());
    EXPECT_EQ("k1", par.keepout_list[0].area_name);
    ASSERT_EQ(1u, par.keepout_list[0].shape_list.size());
    EXPECT_EQ("F.Cu", par.keepout_list[0].shape_list[0]->layer->name);
    ASSERT_EQ(1u, par.via_keepout_list.size());
    EXPECT_EQ("vk1", par.via_keepout_list[0].area_name);
    ASSERT_EQ(1u, par.place_keepout_list.size());
    EXPECT_EQ("pk1", par.place_keepout_list[0].area_name);
}

TEST(StructureTest, ReadScopeReadsKeepoutWithWindowAndClearanceClass)
{
    SpecctraDsnStreamReader scanner(
        "(layer F.Cu (type signal))"
        "(keepout k1 (rect F.Cu 0 0 10 10) (window (rect F.Cu 2 2 3 3)) (clearance_class wide)))",
        true);
    ReadScopeParameter par(scanner);
    ASSERT_TRUE(Keyword::STRUCTURE_SCOPE->read_scope(par));
    ASSERT_EQ(1u, par.keepout_list.size());
    // the border plus one window hole
    EXPECT_EQ(2u, par.keepout_list[0].shape_list.size());
    EXPECT_EQ("wide", par.keepout_list[0].clearance_class_name);
}

// ---- T109b: board rules from the structure scope. Expected strings were printed by Java
// Structure.setClearanceRule (2.1.0 jar) on a 2-layer board, CoordinateTransform(10, 0, 0), quote '"'.

struct RulesFixture
{
    board::LayerStructure layers{std::vector<std::shared_ptr<board::Layer>>{
        std::make_shared<board::Layer>("Top", true), std::make_shared<board::Layer>("Bottom", true)}};
    rules::BoardRules rules{layers, rules::ClearanceMatrix::get_default_instance(layers, 0)};
};

std::string dump(rules::BoardRules& p_rules, bool p_result)
{
    rules::ClearanceMatrix& m = p_rules.get_clearance_matrix();
    std::ostringstream out;
    out << "result=" << (p_result ? "true" : "false") << " classes=";
    for (int i = 0; i < m.get_class_count(); ++i)
    {
        out << m.get_name(i) << (i + 1 < m.get_class_count() ? "," : "");
    }
    out << " values=";
    for (int l = 0; l < 2; ++l)
    {
        for (int i = 0; i < m.get_class_count(); ++i)
        {
            for (int j = 0; j < m.get_class_count(); ++j)
            {
                out << m.get_value(i, j, l, false) << ' ';
            }
        }
        out << "| ";
    }
    auto& d = p_rules.get_default_net_class()->get_default_item_clearance_classes();
    out << "defaults via=" << d.get(rules::ItemClass::VIA) << " smd=" << d.get(rules::ItemClass::SMD)
        << " pin=" << d.get(rules::ItemClass::PIN) << " area=" << d.get(rules::ItemClass::AREA);
    std::ostringstream edge;
    edge << p_rules.get_pin_edge_to_turn_dist();
    std::string edge_text = edge.str();
    if (edge_text.find('.') == std::string::npos)
    {
        edge_text += ".0"; // Java prints doubles with a fraction part
    }
    out << " pinEdge=" << edge_text;
    return out.str();
}

std::string run_rule(double p_value, int p_layer, std::vector<std::string> p_pairs)
{
    RulesFixture f;
    const bool result = Structure::set_clearance_rule(
        ClearanceRule(p_value, std::move(p_pairs)), p_layer, CoordinateTransform(10, 0, 0), f.rules, "\"");
    return dump(f.rules, result);
}

TEST(StructureRulesTest, DefaultClearanceOnAllLayers)
{
    EXPECT_EQ("result=false classes=null,default values=0 0 0 204 | 0 0 0 204 | defaults via=1 smd=1 pin=1 area=1 "
              "pinEdge=0.0",
        run_rule(20.25, -1, {}));
}

TEST(StructureRulesTest, DefaultClearanceOnOneLayer)
{
    EXPECT_EQ("result=false classes=null,default values=0 0 0 0 | 0 0 0 300 | defaults via=1 smd=1 pin=1 area=1 "
              "pinEdge=0.0",
        run_rule(30, 1, {}));
}

TEST(StructureRulesTest, WirePairCreatesTheDefaultClearanceClasses)
{
    EXPECT_EQ("result=false classes=null,default,via,smd,pin,area values=0 0 0 0 0 0 0 0 300 0 0 0 0 300 0 0 0 0 0 0 0 0 "
              "0 0 0 0 0 0 0 0 0 0 0 0 0 0 | 0 0 0 0 0 0 0 0 300 0 0 0 0 300 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 | "
              "defaults via=2 smd=3 pin=4 area=5 pinEdge=0.0",
        run_rule(30, -1, {"wire_via"}));
}

TEST(StructureRulesTest, SmdToTurnGapSetsThePinEdgeDistance)
{
    EXPECT_EQ("result=true classes=null,default values=0 0 0 0 | 0 0 0 0 | defaults via=1 smd=1 pin=1 area=1 "
              "pinEdge=250.0",
        run_rule(25, -1, {"smd_to_turn_gap"}));
}

TEST(StructureRulesTest, ThreePartNameSplitsAtTheFirstUnderscoreLikeJava)
{
    EXPECT_EQ("result=false classes=null,default,smd,via_same_net values=0 0 0 0 0 0 0 0 0 0 0 100 0 0 100 0 | 0 0 0 0 0 "
              "0 0 0 0 0 0 100 0 0 100 0 | defaults via=1 smd=2 pin=1 area=1 pinEdge=0.0",
        run_rule(10, -1, {"smd_via_same_net"}));
}

TEST(StructureRulesTest, QuotedClassName)
{
    EXPECT_EQ("result=false classes=null,default,via,smd,pin,area,a b values=0 0 0 0 0 0 0 0 0 0 0 0 0 200 0 0 0 0 0 0 0 0 "
              "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 200 0 0 0 0 0 | 0 0 0 0 0 0 0 0 0 0 0 0 0 200 0 0 0 0 0 0 0 0 0 0 0 "
              "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 200 0 0 0 0 0 | defaults via=2 smd=3 pin=4 area=5 pinEdge=0.0",
        run_rule(20, -1, {"\"a b\"_wire"}));
}

TEST(StructureRulesTest, TwoElementPairStripsQuotesAndUnderscoresLikeJava)
{
    EXPECT_EQ("result=false classes=null,default,x,y values=0 0 0 0 0 0 0 0 0 0 0 150 0 0 150 0 | 0 0 0 0 0 0 0 0 0 0 0 "
              "150 0 0 150 0 | defaults via=1 smd=1 pin=1 area=1 pinEdge=0.0",
        run_rule(15, -1, {"\"x\"", "_\"_y\""}));
}

TEST(StructureRulesTest, PairOnOneLayer)
{
    EXPECT_EQ("result=false classes=null,default,pin,smd values=0 0 0 0 0 0 0 0 0 0 0 120 0 0 120 0 | 0 0 0 0 0 0 0 0 0 0 "
              "0 0 0 0 0 0 | defaults via=1 smd=3 pin=2 area=1 pinEdge=0.0",
        run_rule(12, 0, {"pin_smd"}));
}

TEST(StructureRulesTest, UpdateBoardRulesAppliesWidthsAndPinEdgeDefault)
{
    SpecctraDsnStreamReader scanner("(layer Top (type signal) (rule (width 4)))(layer Bottom (type signal))"
                                    "(rule (width 2.5) (clearance 1.5)))",
        true);
    Structure::BoardConstructionInfo info;
    ReadScopeParameter par(scanner);
    // Read the scopes the way Structure::read_scope does, then apply the rules.
    for (;;)
    {
        Token t = scanner.next_token();
        if (t.is(Keyword::CLOSED_BRACKET) || t.is_eof())
        {
            break;
        }
        if (t.is(Keyword::OPEN_BRACKET))
        {
            Token k = scanner.next_token();
            if (k.is(Keyword::LAYER))
            {
                ASSERT_TRUE(Structure::read_layer_scope(scanner, info, "\""));
            }
            else if (k.is(Keyword::RULE))
            {
                auto rules = Rule::read_scope(scanner);
                ASSERT_TRUE(rules.has_value());
                info.default_rules.insert(info.default_rules.end(), rules->begin(), rules->end());
            }
        }
    }
    par.layer_structure = std::make_shared<LayerStructure>(info.layer_info);
    par.coordinate_transform = CoordinateTransform(10, 0, 0);
    RulesFixture f;
    Structure::update_board_rules(par, info, f.rules);
    EXPECT_EQ(13, f.rules.get_default_trace_half_width(1)); // round(25 / 2) = round(12.5) = 13
    EXPECT_EQ(20, f.rules.get_default_trace_half_width(0)); // layer rule width 4 -> 40 / 2
    EXPECT_EQ(16, f.rules.get_clearance_matrix().get_value(1, 1, 0, false)); // 15 -> even 16
    EXPECT_DOUBLE_EQ(f.rules.get_min_trace_half_width(), f.rules.get_pin_edge_to_turn_dist());
}

} // namespace
} // namespace freerouting::designforms::specctra::parser