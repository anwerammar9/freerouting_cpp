#pragma once

#include <array>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "board/Unit.h"

namespace freerouting::board {

/**
 * Communication information to host systems or host design formats. Port of Java
 * board.state.Communication (headless subset).
 *
 * C++ deviations (layering): Java stores an io.CoordinateTransform here; the board library may
 * not depend on designforms, so the transform is kept as its three numbers (scale factor and base
 * point) and the DSN/SES writers rebuild a parser::CoordinateTransform from them. Java's
 * idGenerator/observers are not needed headless (item ids come from BasicBoard).
 */
class Communication
{
public:
    /** Java Communication.SpecctraParserInfo.WriteResolution. */
    struct WriteResolution
    {
        std::string char_name;
        int positive_int = 0;
    };

    /**
     * Information from the parser scope in a Specctra-dsn-file. Java SpecctraParserInfo; an empty
     * host_cad / host_version stands for Java null.
     */
    struct SpecctraParserInfo
    {
        std::string string_quote = "\"";
        std::string host_cad;
        std::string host_version;
        std::vector<std::array<std::string, 2>> constants;
        std::optional<WriteResolution> write_resolution;
        bool dsn_file_generated_by_host = false;

        bool has_host_cad() const { return !host_cad.empty(); }
    };

    /** Java Communication(): MIL, resolution 1, quote '"', transform (1, 0, 0). */
    Communication();
    Communication(Unit p_unit, int p_resolution, SpecctraParserInfo p_parser_info, double p_transform_scale_factor,
        double p_transform_base_x, double p_transform_base_y);
    Communication(const Communication&) = default;

    /** Java hostCadIsFusion: host cad contains "fusion", "cadsoft" or "eagle" (case-insensitive). */
    bool host_cad_is_fusion() const;
    /** Kept for existing callers: host cad is exactly "CadSoft". */
    bool host_cad_is_eagle() const { return parser_info_.has_host_cad() && parser_info_.host_cad == "CadSoft"; }
    /** Java hostIsOldKicad: host cad contains "kicad" and the first number in host_version is <= 5. */
    bool host_is_old_kicad() const;
    bool host_cad_exists() const { return parser_info_.has_host_cad(); }

    int get_resolution() const { return resolution_; }
    void set_resolution(int p_resolution) { resolution_ = p_resolution; }

    /** Java getResolution(Unit): the resolution scaled to p_unit (Java argument order kept). */
    double get_resolution(Unit p_unit) const { return UnitHelper::scale(resolution_, p_unit, unit_); }

    Unit get_unit() const { return unit_; }
    void set_unit(Unit p_unit) { unit_ = p_unit; }

    const SpecctraParserInfo& get_parser_info() const { return parser_info_; }
    void set_parser_info(const SpecctraParserInfo& p_info) { parser_info_ = p_info; }

    /** The dsn coordinate transform (Java coordinateTransform) as scale factor and base point. */
    double get_transform_scale_factor() const { return transform_scale_factor_; }
    double get_transform_base_x() const { return transform_base_x_; }
    double get_transform_base_y() const { return transform_base_y_; }

    std::string to_string() const { return "Communication"; }

private:
    int resolution_ = 1;
    Unit unit_ = Unit::MIL;
    SpecctraParserInfo parser_info_;
    double transform_scale_factor_ = 1;
    double transform_base_x_ = 0;
    double transform_base_y_ = 0;
};

} // namespace freerouting::board
