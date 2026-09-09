#pragma once

#include "designforms/specctra/Scanner.h"
#include "designforms/specctra/Keyword.h"
#include "board/Layer.h"
#include "geometry/planar/IntBox.h"

#include <string>
#include <vector>
#include <memory>

namespace freerouting::board {
class BasicBoard;
class BoardRules;
class CoordinateTransform;
} // namespace freerouting::board

namespace freerouting::designforms::specctra {

class Structure
{
public:
    struct LayerInfo
    {
        std::string name;
        int index;
        bool is_signal = true;
        std::vector<std::string> net_names;
    };

    struct ViaPadstackInfo
    {
        std::string name;
        int64_t drill;
        int64_t start_x, start_y;
        int64_t end_x, end_y;
        std::vector<int> layer_indices;
        std::vector<LayerInfo> shapes;
    };

    struct BoundaryInfo
    {
        std::string type;  // "pcb" or "signal"
        int64_t x1, y1, x2, y2;
    };

    struct ReadScopeParameter
    {
        Scanner& scanner;
        Result& result;
        std::string string_quote = "\"";
    };

    static bool read_scope(Scanner& p_scanner, Result& p_result);

private:
    static bool read_layer_scope(Scanner& p_scanner, Result& p_result);
    static bool read_boundary_scope(Scanner& p_scanner, Result& p_result);
    static bool read_via_padstack_scope(Scanner& p_scanner, Result& p_result);
    static bool skip_scope(Scanner& p_scanner);
};

} // namespace freerouting::designforms::specctra