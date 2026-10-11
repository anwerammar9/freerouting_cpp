#pragma once

#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "board/AngleRestriction.h"
#include "board/RoutingBoard.h"
#include "board/Unit.h"

namespace freerouting::settings {
class RouterSettings;
} // namespace freerouting::settings

namespace freerouting::designforms::specctra {

/**
 * Lightweight metadata of a DSN file, read without building the full board. Port of the Java
 * record io.BoardMetadata.
 */
struct BoardMetadata
{
    /** Absent = Java null. */
    std::optional<std::string> host_cad;
    std::optional<std::string> host_version;
    int layer_count = 0;
    board::Unit unit = board::Unit::MIL;
    int resolution = 0;
    board::AngleRestriction snap_angle = board::AngleRestriction::FORTYFIVE_DEGREE;
    /** The (autoroute_settings ...) of the structure scope; nullptr if absent (Java null). */
    std::shared_ptr<settings::RouterSettings> router_settings;
};

/** Typed result of reading a DSN file. Port of the Java sealed interface io.BoardReadResult. */
namespace board_read_result {

/** The board was read completely. */
struct Success
{
    std::unique_ptr<board::RoutingBoard> board;
    /** Only set by DsnReader::read_metadata (Java passes null from readBoard). */
    std::optional<BoardMetadata> metadata;
    std::vector<std::string> warnings;
};

/** The DSN has no usable board outline; the (partial) board is still returned. */
struct OutlineMissing
{
    std::unique_ptr<board::RoutingBoard> board;
    std::optional<BoardMetadata> metadata;
    std::vector<std::string> warnings;
};

/** The DSN could not be parsed. */
struct ParseError
{
    std::string location;
    std::string detail;
};

/** The stream could not be read. */
struct IoError
{
    std::string cause;
};

} // namespace board_read_result

using BoardReadResult = std::variant<board_read_result::Success, board_read_result::OutlineMissing,
    board_read_result::ParseError, board_read_result::IoError>;

} // namespace freerouting::designforms::specctra
