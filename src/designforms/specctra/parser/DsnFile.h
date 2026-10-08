#pragma once

#include <optional>
#include <string>
#include <vector>

#include "designforms/specctra/parser/IJFlexScanner.h"

namespace freerouting::designforms::specctra::parser {

/** Scope reading helpers of Java parser.DsnFile. */
class DsnFile
{
public:
    /** Java DsnFile.CLASS_CLEARANCE_SEPARATOR. */
    static constexpr char kClassClearanceSeparator = '-';

    /** Reads "on)" / "off)" (then skips the rest of the scope). Java readOnOffScope. */
    static bool read_on_off_scope(IJFlexScanner& p_scanner);
    /** Reads "<int>)"; 0 on error. Java readIntegerScope. */
    static int read_integer_scope(IJFlexScanner& p_scanner);
    /** Reads "<number>)"; 0 on error. Java readFloatScope. */
    static double read_float_scope(IJFlexScanner& p_scanner);
    /** Reads "<string>)" with nextString(true). Java readStringScope (null -> nullopt). */
    static std::optional<std::string> read_string_scope(IJFlexScanner& p_scanner);
    /** Reads a string list and the closing bracket; nullopt if the bracket is missing. Java readStringListScope. */
    static std::optional<std::vector<std::string>> read_string_list_scope(IJFlexScanner& p_scanner);
};

} // namespace freerouting::designforms::specctra::parser
