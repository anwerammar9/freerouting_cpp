#pragma once

#include "designforms/specctra/Scanner.h"
#include "designforms/specctra/Keyword.h"

#include <string>
#include <vector>

namespace freerouting::designforms::specctra {

class Placement
{
public:
    static bool read_scope(Scanner& p_scanner, Result& p_result);

private:
    static bool read_component_scope(Scanner& p_scanner, Result& p_result);
    static bool read_place_scope(Scanner& p_scanner, Result& p_result, const std::string& p_footprint);
    static bool skip_scope(Scanner& p_scanner);
};

} // namespace freerouting::designforms::specctra