#ifndef AUTOROUTE_INSERT_FOUND_CONNECTION_ALGO_H
#define AUTOROUTE_INSERT_FOUND_CONNECTION_ALGO_H

#include <memory>
#include <vector>

#include "board/Item.h"
#include "board/FixedState.h"
#include "autoroute/AutorouteControl.h"
#include "autoroute/LocateFoundConnectionAlgo.h"

namespace freerouting::board {
class RoutingBoard;
}

namespace freerouting::autoroute {

class InsertFoundConnectionAlgo
{
public:
    static bool insert(std::unique_ptr<LocateResult> p_connection,
                      board::RoutingBoard* p_board,
                      const AutorouteControl& p_ctrl);

private:
    InsertFoundConnectionAlgo(board::RoutingBoard* p_board, const AutorouteControl& p_ctrl);

    bool insert_via(int p_from_layer, int p_to_layer);
    bool insert_trace(LocateResultItem& p_trace);

    board::RoutingBoard* board_ = nullptr;
    const AutorouteControl* ctrl_ = nullptr;
    int last_x_ = 0;
    int last_y_ = 0;
    int first_x_ = 0;
    int first_y_ = 0;
};

} // namespace freerouting::autoroute

#endif // AUTOROUTE_INSERT_FOUND_CONNECTION_ALGO_H