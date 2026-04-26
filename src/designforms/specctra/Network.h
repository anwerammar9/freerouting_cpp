#pragma once

#include "designforms/specctra/Scanner.h"
#include "designforms/specctra/Keyword.h"

#include <string>
#include <vector>

namespace freerouting::designforms::specctra {

class Network
{
public:
    static bool read_scope(Scanner& p_scanner, Result& p_result);

private:
    static bool read_net_scope(Scanner& p_scanner, Result& p_result);
    static bool read_class_scope(Scanner& p_scanner, Result& p_result);
    static bool read_via_scope(Scanner& p_scanner, Result& p_result);
    static bool skip_scope(Scanner& p_scanner);
};

} // namespace freerouting::designforms::specctra