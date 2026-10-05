#pragma once

#include <memory>
#include <string>

#include "board/Unit.h"

namespace freerouting::board {

class CoordinateTransform;

/**
 * Communication information to host systems or host design formats.
 * Mirrors Java Communication.java (headless subset).
 */
class Communication
{
public:
    /**
     * Information from the parser scope in a Specctra-dsn-file.
     * Mirrors Java Communication.SpecctraParserInfo.
     */
    struct SpecctraParserInfo
    {
        std::string string_quote;
        std::string host_cad;
        std::string host_version;
        bool dsn_file_generated_by_host = false;

        bool has_host_cad() const { return !host_cad.empty(); }
    };

    Communication();
    Communication(const Communication&) = default;

    bool host_cad_is_eagle() const { return parser_info_.has_host_cad() && parser_info_.host_cad == "CadSoft"; }
    bool host_is_old_kicad() const { return false; }
    bool host_cad_exists() const { return parser_info_.has_host_cad(); }

    int get_resolution() const { return resolution_; }
    void set_resolution(int p_resolution) { resolution_ = p_resolution; }

    /**
     * Returns the board resolution scaled from the unit of this communication
     * information to p_unit. Mirrors Java
     * Communication.get_resolution(Unit).
     */
    double get_resolution(Unit p_unit) const
    {
        return UnitHelper::scale(resolution_, p_unit, unit_);
    }

    Unit get_unit() const { return unit_; }
    void set_unit(Unit p_unit) { unit_ = p_unit; }

    const SpecctraParserInfo& get_parser_info() const { return parser_info_; }
    void set_parser_info(const SpecctraParserInfo& p_info) { parser_info_ = p_info; }

    std::string to_string() const { return "Communication"; }

private:
    int resolution_ = 1;
    Unit unit_ = Unit::MIL;
    SpecctraParserInfo parser_info_;
};

} // namespace freerouting::board