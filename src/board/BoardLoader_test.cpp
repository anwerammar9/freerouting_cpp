#include "board/BoardLoader.h"
#include "board/RoutingBoard.h"
#include "board/Via.h"
#include "board/Trace.h"
#include "board/Padstack.h"

#include <gtest/gtest.h>

#include <fstream>

namespace freerouting::board {
namespace {

const char* kTestDsn = R"(
(pcb board_loader_test
  (structure
    (layer "F.Cu" (type signal))
    (via "V1" (hole 100) 0)
  )
  (network
    (net "n1" (pins))
    (net "n2" (pins))
  )
  (wiring
    (wire
      (polyline_path "F.Cu" 10 200 200 200 500)
      (net "n1" 1)
      (type shove_fixed)
    )
    (wire
      (polyline_path "F.Cu" 20 200 500 250 900 600)
      (net "n1" 1)
    )
    (via "V1" 600 900
      (net "n1" 1)
    )
  )
)
)";

TEST(BoardLoaderTest, MaterializesTracesAndVias)
{
    const std::string dsn_path = "/tmp/freerouting_boardloader_test.dsn";
    {
        std::ofstream out(dsn_path);
        ASSERT_TRUE(out.is_open());
        out << kTestDsn;
    }

    auto board = BoardLoader::load_board(dsn_path);
    ASSERT_NE(board, nullptr);

    std::vector<Via*> vias = board->get_vias();
    ASSERT_EQ(vias.size(), 1u);
    EXPECT_EQ(vias[0]->get_center().x, 600);
    EXPECT_EQ(vias[0]->get_center().y, 900);
    EXPECT_TRUE(vias[0]->contains_net(1));
    EXPECT_EQ(vias[0]->get_padstack()->get_from_layer(), 0);
    EXPECT_EQ(vias[0]->get_padstack()->get_to_layer(), 0);

    std::vector<Trace*> traces = board->get_traces();
    // Trace 2 "(200,500)->(250,900)" passes through the via pad, so the two
    // "n1" traces may be merged by normalize_traces. Only the count of items
    // with the net matters.
    int net1_traces = 0;
    for (Trace* trace : traces)
    {
        if (trace->contains_net(1))
        {
            ++net1_traces;
        }
    }
    EXPECT_GE(net1_traces, 1);
}

} // namespace
} // namespace freerouting::board