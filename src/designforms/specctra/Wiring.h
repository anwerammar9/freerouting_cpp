#pragma once

#include <vector>

namespace freerouting {
namespace designforms {
namespace specctra {

class Scanner;
struct Result;

/**
 * Parses the DSN "wiring" scope (traces, vias and conduction areas) into the
 * parse Result. The parse keeps DSN data in the Result; board items are
 * materialized later by BoardLoader.
 */
class Wiring
{
public:
    Wiring() = default;

    bool read_scope(Scanner& p_scanner, Result& p_result);

private:
    bool read_wire_scope(Scanner& p_scanner, Result& p_result);
    bool read_via_scope(Scanner& p_scanner, Result& p_result);
};

} // namespace specctra
} // namespace designforms
} // namespace freerouting