#pragma once

#include "designforms/specctra/Scanner.h"
#include "designforms/specctra/Keyword.h"

#include <string>
#include <vector>
#include <memory>

namespace freerouting::designforms::specctra {

class Library
{
public:
    static bool read_scope(Scanner& p_scanner, Result& p_result);

private:
    static bool read_padstack_scope(Scanner& p_scanner, Result& p_result);
    static bool read_image_scope(Scanner& p_scanner, Result& p_result);
    static bool skip_scope(Scanner& p_scanner);
};

} // namespace freerouting::designforms::specctra