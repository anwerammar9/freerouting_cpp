// Tests for the (autoroute_settings ...) scope reader/writer (Java parser.AutorouteSettings).

#include "designforms/specctra/parser/AutorouteSettings.h"

#include <gtest/gtest.h>

#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "datastructures/IdentifierType.h"
#include "datastructures/IndentFileWriter.h"
#include "designforms/specctra/parser/Keyword.h"
#include "designforms/specctra/parser/Layer.h"
#include "designforms/specctra/parser/ReadScopeParameter.h"
#include "designforms/specctra/parser/SpecctraDsnStreamReader.h"
#include "designforms/specctra/parser/Structure.h"

namespace freerouting::designforms::specctra::parser {
namespace {

std::shared_ptr<Layer> make_layer(const std::string& p_name, int p_no, bool p_is_signal = true)
{
    return std::make_shared<Layer>(p_name, p_no, p_is_signal);
}

LayerStructure two_layers()
{
    return LayerStructure({make_layer("F.Cu", 0), make_layer("B.Cu", 1)});
}

TEST(AutorouteSettingsTest, ReadScopeDefaultsToRouterAndOptimizerOn)
{
    SpecctraDsnStreamReader scanner(")", true);
    const LayerStructure layers = two_layers();
    const auto settings = AutorouteSettings::read_scope(scanner, layers);
    ASSERT_NE(nullptr, settings);
    EXPECT_TRUE(settings->get_run_router());
    EXPECT_TRUE(settings->get_run_optimizer());
    EXPECT_TRUE(settings->get_vias_allowed());
    EXPECT_TRUE(settings->is_layer_active(0));
    EXPECT_TRUE(settings->is_layer_active(1));
}

TEST(AutorouteSettingsTest, ReadScopeReadsOnOffAndCostScopes)
{
    SpecctraDsnStreamReader scanner(
        "(fanout off)(autoroute off)(postroute off)(vias off)"
        "(via_costs 77)(plane_via_costs 30)(start_ripup_costs 200))",
        true);
    const LayerStructure layers = two_layers();
    const auto settings = AutorouteSettings::read_scope(scanner, layers);
    ASSERT_NE(nullptr, settings);
    EXPECT_FALSE(settings->get_run_router());
    EXPECT_FALSE(settings->get_run_optimizer());
    EXPECT_FALSE(settings->get_vias_allowed());
    EXPECT_EQ(77, settings->get_via_costs());
    EXPECT_EQ(30, settings->get_plane_via_costs());
    EXPECT_EQ(200, settings->get_start_ripup_costs());
}

TEST(AutorouteSettingsTest, CostsAreClampedLikeJava)
{
    SpecctraDsnStreamReader scanner(
        "(via_costs 0)(plane_via_costs -5)(start_ripup_costs 0)"
        "(layer_rule F.Cu (preferred_direction_trace_costs 0.01)"
        "(against_preferred_direction_trace_costs 0.05)))",
        true);
    const LayerStructure layers = two_layers();
    const auto settings = AutorouteSettings::read_scope(scanner, layers);
    ASSERT_NE(nullptr, settings);
    EXPECT_EQ(1, settings->get_via_costs());
    EXPECT_EQ(1, settings->get_plane_via_costs());
    EXPECT_EQ(1, settings->get_start_ripup_costs());
    EXPECT_DOUBLE_EQ(0.1, settings->get_preferred_direction_trace_costs(0));
    EXPECT_DOUBLE_EQ(0.1, settings->get_against_preferred_direction_trace_costs(0));
}

TEST(AutorouteSettingsTest, ReadScopeSkipsUnknownScopes)
{
    SpecctraDsnStreamReader scanner("(exotic (nested 1 2))(via_costs 12))", true);
    const LayerStructure layers = two_layers();
    const auto settings = AutorouteSettings::read_scope(scanner, layers);
    ASSERT_NE(nullptr, settings);
    EXPECT_EQ(12, settings->get_via_costs());
}

TEST(AutorouteSettingsTest, ReadScopeReturnsNullOnEndOfFile)
{
    SpecctraDsnStreamReader scanner("(via_costs 12)", true);
    const LayerStructure layers = two_layers();
    EXPECT_EQ(nullptr, AutorouteSettings::read_scope(scanner, layers));
}

TEST(AutorouteSettingsTest, LayerRuleSetsPerLayerValues)
{
    SpecctraDsnStreamReader scanner(
        "(layer_rule B.Cu (active off)(preferred_direction vertical)"
        "(preferred_direction_trace_costs 2.5)(against_preferred_direction_trace_costs 3.5)))",
        true);
    const LayerStructure layers = two_layers();
    const auto settings = AutorouteSettings::read_scope(scanner, layers);
    ASSERT_NE(nullptr, settings);
    EXPECT_TRUE(settings->is_layer_active(0));
    EXPECT_FALSE(settings->is_layer_active(1));
    EXPECT_FALSE(settings->get_preferred_direction_is_horizontal(1));
    EXPECT_DOUBLE_EQ(2.5, settings->get_preferred_direction_trace_costs(1));
    EXPECT_DOUBLE_EQ(3.5, settings->get_against_preferred_direction_trace_costs(1));
    // the other layer keeps the Java defaults: layer % 2 == 1 is horizontal, trace costs 1.0
    EXPECT_FALSE(settings->get_preferred_direction_is_horizontal(0));
    EXPECT_DOUBLE_EQ(1.0, settings->get_preferred_direction_trace_costs(0));
}

TEST(AutorouteSettingsTest, LayerRuleWithUnknownLayerFails)
{
    SpecctraDsnStreamReader scanner("(layer_rule X.Cu (active off)))", true);
    const LayerStructure layers = two_layers();
    EXPECT_EQ(nullptr, AutorouteSettings::read_scope(scanner, layers));
}

TEST(AutorouteSettingsTest, LayerRuleWithBadPreferredDirectionFails)
{
    SpecctraDsnStreamReader scanner("(layer_rule F.Cu (preferred_direction diagonal)))", true);
    const LayerStructure layers = two_layers();
    EXPECT_EQ(nullptr, AutorouteSettings::read_scope(scanner, layers));
}

TEST(AutorouteSettingsTest, ReadLayerRuleOperatesOnTheGivenSettings)
{
    SpecctraDsnStreamReader scanner("F.Cu (active off))", true);
    const LayerStructure layers = two_layers();
    auto settings = std::make_shared<settings::RouterSettings>();
    settings->set_layer_count(2);
    const auto result = AutorouteSettings::read_layer_rule(scanner, layers, settings);
    ASSERT_EQ(settings, result);
    EXPECT_FALSE(settings->is_layer_active(0));
    EXPECT_TRUE(settings->is_layer_active(1));
}

TEST(AutorouteSettingsTest, WriteScopeProducesTheDsnScope)
{
    const LayerStructure layers = two_layers();
    settings::RouterSettings settings;
    settings.set_layer_count(2);
    settings.set_run_router(false);
    settings.set_via_costs(77);
    settings.set_layer_active(1, false);
    settings.set_preferred_direction_trace_costs(0, 2.5);

    board::LayerStructure board_layers(std::vector<std::shared_ptr<board::Layer>>{
        std::make_shared<board::Layer>("F.Cu", true), std::make_shared<board::Layer>("B.Cu", true)});
    const datastructures::IdentifierType identifier_type({" ", "(", ")", "\t"}, "\"");

    std::ostringstream out;
    datastructures::IndentFileWriter writer(out);
    AutorouteSettings::write_scope(writer, settings, board_layers, identifier_type);
    const std::string text = out.str();

    EXPECT_NE(std::string::npos, text.find("(autoroute_settings"));
    EXPECT_NE(std::string::npos, text.find("(autoroute off)"));
    EXPECT_NE(std::string::npos, text.find("(postroute off)"));
    EXPECT_NE(std::string::npos, text.find("(vias on)"));
    EXPECT_NE(std::string::npos, text.find("(via_costs 77)"));
    EXPECT_NE(std::string::npos, text.find("(layer_rule F.Cu"));
    EXPECT_NE(std::string::npos, text.find("(layer_rule B.Cu"));
    EXPECT_NE(std::string::npos, text.find("(active off)"));
    EXPECT_NE(std::string::npos, text.find("(preferred_direction_trace_costs 2.5)"));
    // Java Float.toString always carries a fraction part
    EXPECT_NE(std::string::npos, text.find("(against_preferred_direction_trace_costs 1.0)"));
}

TEST(AutorouteSettingsTest, WrittenScopeReadsBackToTheSameSettings)
{
    const LayerStructure layers = two_layers();
    settings::RouterSettings settings;
    settings.set_layer_count(2);
    settings.set_run_optimizer(false);
    settings.set_plane_via_costs(40);
    settings.set_start_ripup_costs(250);
    settings.set_layer_active(0, false);
    settings.set_preferred_direction_is_horizontal(0, true);
    settings.set_against_preferred_direction_trace_costs(1, 4.5);

    board::LayerStructure board_layers(std::vector<std::shared_ptr<board::Layer>>{
        std::make_shared<board::Layer>("F.Cu", true), std::make_shared<board::Layer>("B.Cu", true)});
    const datastructures::IdentifierType identifier_type({" ", "(", ")", "\t"}, "\"");
    std::ostringstream out;
    datastructures::IndentFileWriter writer(out);
    AutorouteSettings::write_scope(writer, settings, board_layers, identifier_type);

    // The reader starts behind the "autoroute_settings" keyword.
    const std::string text = out.str();
    const std::string keyword = "autoroute_settings";
    const std::string body = text.substr(text.find(keyword) + keyword.size());
    SpecctraDsnStreamReader scanner(body, true);
    const auto read_back = AutorouteSettings::read_scope(scanner, layers);
    ASSERT_NE(nullptr, read_back);
    EXPECT_TRUE(read_back->get_run_router());
    EXPECT_FALSE(read_back->get_run_optimizer());
    EXPECT_EQ(40, read_back->get_plane_via_costs());
    EXPECT_EQ(250, read_back->get_start_ripup_costs());
    EXPECT_FALSE(read_back->is_layer_active(0));
    EXPECT_TRUE(read_back->get_preferred_direction_is_horizontal(0));
    EXPECT_DOUBLE_EQ(4.5, read_back->get_against_preferred_direction_trace_costs(1));
}

TEST(AutorouteSettingsTest, StructureScopeStoresTheAutorouteSettings)
{
    SpecctraDsnStreamReader scanner(
        "(layer F.Cu (type signal))(layer B.Cu (type signal))"
        "(autoroute_settings (autoroute off)(via_costs 33)))",
        true);
    ReadScopeParameter par(scanner);
    ASSERT_TRUE(Keyword::STRUCTURE_SCOPE->read_scope(par));
    ASSERT_NE(nullptr, par.autoroute_settings);
    EXPECT_FALSE(par.autoroute_settings->get_run_router());
    EXPECT_EQ(33, par.autoroute_settings->get_via_costs());
}

TEST(AutorouteSettingsTest, StructureWithoutAutorouteSettingsKeepsThemNull)
{
    SpecctraDsnStreamReader scanner("(layer F.Cu (type signal)))", true);
    ReadScopeParameter par(scanner);
    ASSERT_TRUE(Keyword::STRUCTURE_SCOPE->read_scope(par));
    EXPECT_EQ(nullptr, par.autoroute_settings);
}

} // namespace
} // namespace freerouting::designforms::specctra::parser
