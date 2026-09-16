#include "designforms/specctra/Scanner.h"

#include <gtest/gtest.h>

#include <sstream>

namespace freerouting::designforms::specctra {
namespace {

const char* kTestDsn = R"(
(pcb test_board
  (structure
    (layer "F.Cu45" (type signal))
    (via "V1" (hole 100) 0)
  )
  (network
    (net "n1" (pins))
  )
  (wiring
    (wire
      (polyline_path "F.Cu45" 10 200 200 200 500)
      (net "n1" 1)
      (clearance_class "default")
      (type shove_fixed)
    )
    (wire
      (polyline_path "F.Cu45" 20 200 500 200 800)
      (net "")
    )
    (via "V1" 200 800
      (net "n1" 1)
      (clearance_class "default")
    )
  )
)
)";

TEST(WiringTest, ParsesWiringScopeIntoResult)
{
    std::istringstream input(kTestDsn);
    const Result result = read_dsn(input);

    ASSERT_TRUE(result.success);
    ASSERT_EQ(result.wires.size(), 2u);

    // First wire: trace on layer 0, width 10, one segment (200,200)-(200,500).
    const WireDef& trace = result.wires[0];
    EXPECT_EQ(trace.layer_no, 0);
    EXPECT_DOUBLE_EQ(trace.width, 10.0);
    ASSERT_EQ(trace.coords.size(), 4u);
    EXPECT_DOUBLE_EQ(trace.coords[0], 200.0);
    EXPECT_DOUBLE_EQ(trace.coords[1], 200.0);
    EXPECT_DOUBLE_EQ(trace.coords[2], 200.0);
    EXPECT_DOUBLE_EQ(trace.coords[3], 500.0);
    ASSERT_EQ(trace.net_nos.size(), 1u);
    EXPECT_EQ(trace.net_nos[0], 1);
    EXPECT_TRUE(trace.fixed);
    EXPECT_FALSE(trace.is_conduction_area);

    // Second wire: no net.
    const WireDef& no_net_trace = result.wires[1];
    EXPECT_EQ(no_net_trace.layer_no, 0);
    EXPECT_DOUBLE_EQ(no_net_trace.width, 20.0);
    EXPECT_TRUE(no_net_trace.net_nos.empty());

    // Via placement resolves its padstack by name.
    ASSERT_EQ(result.via_placements.size(), 1u);
    const ViaPlacementDef& via = result.via_placements[0];
    EXPECT_EQ(via.padstack_name, "V1");
    ASSERT_EQ(via.location.size(), 2u);
    EXPECT_DOUBLE_EQ(via.location[0], 200.0);
    EXPECT_DOUBLE_EQ(via.location[1], 800.0);
    ASSERT_EQ(via.net_nos.size(), 1u);
    EXPECT_EQ(via.net_nos[0], 1);
    EXPECT_FALSE(via.fixed);
}

TEST(WiringTest, SkipsConductionAreaWiresButKeepsTrace)
{
    const char* dsn = R"(
(pcb with_plane
  (structure
    (layer "F.Cu" (type signal))
  )
  (wiring
    (wire
      (rect "F.Cu" 0 0 1000 1000)
      (net "n1")
    )
    (wire
      (polyline_path "F.Cu" 10 500 500 600 600)
      (net "")
    )
  )
)
)";

    std::istringstream input(dsn);
    const Result result = read_dsn(input);

    ASSERT_TRUE(result.success);
    ASSERT_EQ(result.wires.size(), 2u);
    EXPECT_TRUE(result.wires[0].is_conduction_area);
    EXPECT_FALSE(result.wires[1].is_conduction_area);
    EXPECT_GE(result.wires[1].coords.size(), 4u);
}

} // namespace
} // namespace freerouting::designforms::specctra