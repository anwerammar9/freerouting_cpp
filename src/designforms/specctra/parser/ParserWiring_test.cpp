// Tests for the DSN wiring scope reader (Java parser.Wiring, read part) and
// RoutingBoard::normalize_all_traces (T116). Expected values come from the Java
// 2.1.0 jar (throwaway WireDump.java, see docs/migration/MIGRATION_designforms.md).

#include "designforms/specctra/parser/Wiring.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "board/ConductionArea.h"
#include "board/Padstack.h"
#include "board/PolylineTrace.h"
#include "board/RoutingBoard.h"
#include "board/Via.h"
#include "designforms/specctra/parser/Keyword.h"
#include "designforms/specctra/parser/ReadScopeParameter.h"
#include "designforms/specctra/parser/SpecctraDsnStreamReader.h"
#include "geometry/planar/Polyline.h"

namespace freerouting::designforms::specctra::parser {
namespace {

std::string java_fixed_state_name(board::FixedState p_state)
{
    switch (p_state)
    {
    case board::FixedState::NOT_FIXED:
        return "UNFIXED";
    case board::FixedState::SHOVE_FIXED:
        return "SHOVE_FIXED";
    case board::FixedState::USER_FIXED:
        return "USER_FIXED";
    case board::FixedState::SYSTEM_FIXED:
        return "SYSTEM_FIXED";
    default:
        return "?";
    }
}

std::string nets_text(const board::Item& p_item)
{
    std::string result = "[";
    for (int i = 0; i < p_item.net_count(); ++i)
    {
        result += (i > 0 ? "," : "") + std::to_string(p_item.get_net_no(i));
    }
    return result + "]";
}

/** The same lines the Java WireDump prints: every PolylineTrace and Via, sorted. */
std::vector<std::string> wire_lines(const board::RoutingBoard& p_board)
{
    std::vector<std::string> lines;
    for (board::Item* item : p_board.get_items())
    {
        if (const auto* trace = dynamic_cast<const board::PolylineTrace*>(item))
        {
            std::string line = "TRACE " + std::to_string(trace->get_layer()) + " "
                + std::to_string(trace->get_half_width()) + " " + nets_text(*trace) + " "
                + java_fixed_state_name(trace->get_fixed_state()) + " " + std::to_string(trace->get_clearance_class());
            // Like the Java dumper: the float approximation of each (possibly rational) corner,
            // rounded with Math.round.
            for (const auto& corner : trace->polyline().corner_approx_arr())
            {
                line += " " + std::to_string(static_cast<long long>(std::floor(corner.x + 0.5))) + ","
                    + std::to_string(static_cast<long long>(std::floor(corner.y + 0.5)));
            }
            lines.push_back(line);
        }
        else if (const auto* via = dynamic_cast<const board::Via*>(item))
        {
            lines.push_back("VIA " + via->get_padstack()->get_name() + " " + std::to_string(via->get_center().x) + ","
                + std::to_string(via->get_center().y) + " " + nets_text(*via) + " "
                + java_fixed_state_name(via->get_fixed_state()) + " " + std::to_string(via->get_clearance_class()) + " "
                + std::to_string(via->first_layer()) + "-" + std::to_string(via->last_layer()));
        }
    }
    std::sort(lines.begin(), lines.end());
    return lines;
}

constexpr const char* kWiringBoard =
    "(pcb wiring_test"
    " (parser (host_cad \"test\") (host_version \"1\"))"
    " (resolution um 10) (unit um)"
    " (structure (layer F.Cu (type signal)) (layer B.Cu (type signal))"
    "  (boundary (rect pcb 0 0 100000 100000)) (via V1) (rule (width 250) (clearance 200)))"
    " (placement)"
    " (library (padstack V1 (shape (circle F.Cu 400)) (shape (circle B.Cu 400))))"
    " (network (net GND (pins)) (net VCC (pins)))"
    " (wiring"
    "  (wire (path F.Cu 250 10000 10000 20000 10000) (net GND) (type protect))"
    "  (wire (path F.Cu 250 20000 10000 30000 10000) (net GND))"
    "  (wire (path B.Cu 300 10000 20000 10000 30000 20000 40000) (net VCC) (type fix))"
    "  (wire (polyline_path F.Cu 200 50000 50000 60000 50000 60000 50000 60000 60000) (net VCC))"
    "  (wire (poly B.Cu 0 70000 70000 80000 70000 80000 80000 70000 80000) (net GND) (type shove_fixed))"
    "  (wire (path F.Cu 250 5000 5000 5000 5000) (net GND))"
    "  (via V1 30000 10000 (net GND))"
    "  (via V1 30000 10000 (net GND))"
    "  (via V1 20000 40000 (net VCC) (type route))"
    "  (via V1 40000 40000)))";

TEST(ParserWiringTest, WiresAndViasMatchTheJavaReader)
{
    SpecctraDsnStreamReader scanner(kWiringBoard, true);
    ReadScopeParameter par(scanner);
    ASSERT_TRUE(Keyword::PCB_SCOPE->read_scope(par));
    ASSERT_NE(nullptr, par.get_board());
    const std::vector<std::string> expected = {
        "TRACE 0 1250 [1] UNFIXED 1 200000,100000 300000,100000",
        "TRACE 0 1250 [1] USER_FIXED 1 100000,100000 200000,100000",
        "TRACE 1 1500 [2] SYSTEM_FIXED 1 100000,200000 100000,300000 200000,400000",
        "VIA V1 200000,400000 [2] USER_FIXED 1 0-1",
        "VIA V1 300000,100000 [1] UNFIXED 1 0-1",
        "VIA V1 400000,400000 [] UNFIXED 1 0-1",
    };
    EXPECT_EQ(expected, wire_lines(*par.get_board()));
    // The duplicate via and the zero-length wire are recorded as warnings.
    const auto has_warning = [&](const std::string& p_prefix) {
        return std::any_of(par.warnings.begin(), par.warnings.end(),
            [&](const std::string& p_warning) { return p_warning.rfind(p_prefix, 0) == 0; });
    };
    EXPECT_TRUE(has_warning("Wiring: duplicate via skipped at (300000, 100000)"));
    EXPECT_TRUE(has_warning("Wiring: degenerate wire trace skipped"));
}

TEST(ParserWiringTest, PolyWireBecomesAConductionArea)
{
    SpecctraDsnStreamReader scanner(kWiringBoard, true);
    ReadScopeParameter par(scanner);
    ASSERT_TRUE(Keyword::PCB_SCOPE->read_scope(par));
    std::vector<const board::ConductionArea*> areas;
    for (board::Item* item : par.get_board()->get_items())
    {
        if (const auto* area = dynamic_cast<const board::ConductionArea*>(item))
        {
            areas.push_back(area);
        }
    }
    ASSERT_EQ(1u, areas.size());
    EXPECT_EQ(1, areas[0]->get_layer());
    EXPECT_EQ(board::FixedState::SHOVE_FIXED, areas[0]->get_fixed_state());
    ASSERT_EQ(1, areas[0]->net_count());
    EXPECT_EQ(1, areas[0]->get_net_no(0));
    const auto box = areas[0]->bounding_box();
    EXPECT_EQ(700000, box.ll.x);
    EXPECT_EQ(800000, box.ur.y);
}

TEST(ParserWiringTest, UnknownViaPadstackAbortsTheWiringScope)
{
    std::string board_text = kWiringBoard;
    const std::string via = "(via V1 40000 40000)";
    board_text.replace(board_text.find(via), via.size(), "(via V9 40000 40000)");
    SpecctraDsnStreamReader scanner(board_text, true);
    ReadScopeParameter par(scanner);
    EXPECT_FALSE(Keyword::PCB_SCOPE->read_scope(par));
    EXPECT_TRUE(std::any_of(par.warnings.begin(), par.warnings.end(),
        [](const std::string& p_warning) { return p_warning.rfind("Wiring: via padstack 'V9' not found", 0) == 0; }));
}

TEST(ParserWiringTest, NormalizeAllTracesCombinesCollinearTracesOfANet)
{
    // Two touching collinear traces of one net (no via between them) are merged.
    std::string board_text = kWiringBoard;
    const std::string vias = "  (via V1 30000 10000 (net GND))  (via V1 30000 10000 (net GND))";
    board_text.replace(board_text.find(vias), vias.size(), "");
    const std::string protect = " (type protect)";
    board_text.replace(board_text.find(protect), protect.size(), "");
    SpecctraDsnStreamReader scanner(board_text, true);
    ReadScopeParameter par(scanner);
    ASSERT_TRUE(Keyword::PCB_SCOPE->read_scope(par));
    std::vector<std::string> gnd_traces;
    for (const std::string& line : wire_lines(*par.get_board()))
    {
        if (line.rfind("TRACE 0 1250 [1]", 0) == 0)
        {
            gnd_traces.push_back(line);
        }
    }
    ASSERT_EQ(1u, gnd_traces.size());
    EXPECT_EQ("TRACE 0 1250 [1] UNFIXED 1 100000,100000 300000,100000", gnd_traces[0]);
}

TEST(ParserWiringTest, CalcFixedAndReadNetId)
{
    {
        SpecctraDsnStreamReader scanner("shove_fixed)", true);
        EXPECT_EQ(board::FixedState::SHOVE_FIXED, Wiring::calc_fixed(scanner));
    }
    {
        SpecctraDsnStreamReader scanner("normal)", true);
        EXPECT_EQ(board::FixedState::NOT_FIXED, Wiring::calc_fixed(scanner));
    }
    {
        SpecctraDsnStreamReader scanner("protect extra)", true);
        EXPECT_EQ(board::FixedState::NOT_FIXED, Wiring::calc_fixed(scanner)); // ")" expected
    }
    {
        SpecctraDsnStreamReader scanner("GND 3)", true);
        const Net::Id id = Wiring::read_net_id(scanner);
        EXPECT_EQ("GND", id.name);
        EXPECT_EQ(3, id.subnet_number);
    }
    {
        SpecctraDsnStreamReader scanner("VCC)", true);
        EXPECT_EQ(0, Wiring::read_net_id(scanner).subnet_number);
    }
}

TEST(ParserWiringTest, Issue103WiringMatchesTheJavaReader)
{
    // Debug aid: FREEROUTING_WIRES_DSN=<dsn> loads another board (the comparison then fails, but
    // FREEROUTING_DUMP_WIRES writes its C++ dump for a diff with the Java WireDump output).
    const char* other_dsn = std::getenv("FREEROUTING_WIRES_DSN");
    std::ifstream dsn_file(other_dsn != nullptr ? std::string(other_dsn)
                                                : std::string(FREEROUTING_TEST_DATA_DIR) + "/Issue103-Board-Routed.dsn");
    ASSERT_TRUE(dsn_file.good());
    std::stringstream dsn_text;
    dsn_text << dsn_file.rdbuf();
    SpecctraDsnStreamReader scanner(dsn_text.str(), true);
    ReadScopeParameter par(scanner);
    ASSERT_TRUE(Keyword::PCB_SCOPE->read_scope(par));
    ASSERT_NE(nullptr, par.get_board());

    std::ifstream expected_file(
        std::string(FREEROUTING_TEST_DATA_DIR) + "/expected/Issue103-Board-Routed.dsn.wires.txt");
    ASSERT_TRUE(expected_file.good());
    std::vector<std::string> expected;
    for (std::string line; std::getline(expected_file, line);)
    {
        if (!line.empty())
        {
            expected.push_back(line);
        }
    }
    std::sort(expected.begin(), expected.end());
    ASSERT_EQ(2769u, expected.size()); // 2326 traces + 443 vias (T007 counts)

    const std::vector<std::string> actual = wire_lines(*par.get_board());
    if (const char* dump_path = std::getenv("FREEROUTING_DUMP_WIRES"))
    {
        std::ofstream dump(dump_path);
        for (const auto& line : actual)
        {
            dump << line << "\n";
        }
    }
    EXPECT_EQ(expected.size(), actual.size());
    std::vector<std::string> missing;
    std::set_difference(expected.begin(), expected.end(), actual.begin(), actual.end(), std::back_inserter(missing));
    std::vector<std::string> extra;
    std::set_difference(actual.begin(), actual.end(), expected.begin(), expected.end(), std::back_inserter(extra));
    if (!missing.empty())
    {
        ADD_FAILURE() << missing.size() << " Java items missing, first: " << missing.front();
    }
    if (!extra.empty())
    {
        ADD_FAILURE() << extra.size() << " extra C++ items, first: " << extra.front();
    }
}

} // namespace
} // namespace freerouting::designforms::specctra::parser
