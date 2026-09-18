#pragma once

#include <string>
#include <array>

namespace freerouting {
namespace core {

/**
 * @brief File format types supported by freerouting
 * 
 * This enum defines the various file formats that can be processed
 * by the freerouting system, including design files, session files,
 * and output formats.
 */
enum class FileFormat {
    UNKNOWN,    ///< Unknown or unrecognized format
    DSN,        ///< Specctra DSN design file format
    FRB,        ///< Freerouting binary format
    SES,        ///< Session file format
    RULES,      ///< Design rules file format
    SCR,        ///< Script file format
    DRC_JSON,   ///< DRC results in JSON format
    KICAD_DESIGN_JSON,  ///< KiCad design file in JSON format
    KICAD_SESSION_JSON  ///< KiCad session file in JSON format
};

/**
 * @brief Convert FileFormat enum to string representation
 * 
 * @param format The file format enum value
 * @return String representation of the format
 */
std::string to_string(FileFormat format);

/**
 * @brief Get file extension for a given format
 * 
 * @param format The file format enum value
 * @return File extension (including dot) for the format
 */
std::string file_extension(FileFormat format);

/**
 * @brief Parse FileFormat from string
 * 
 * @param format_str String representation of format
 * @return FileFormat enum value, defaults to UNKNOWN if not recognized
 */
FileFormat parse_file_format(const std::string& format_str);

/**
 * @brief Get all supported file formats
 * 
 * @return Array of all supported file formats
 */
constexpr std::array<FileFormat, 9> get_all_formats() {
    return {
        FileFormat::UNKNOWN,
        FileFormat::DSN,
        FileFormat::FRB,
        FileFormat::SES,
        FileFormat::RULES,
        FileFormat::SCR,
        FileFormat::DRC_JSON,
        FileFormat::KICAD_DESIGN_JSON,
        FileFormat::KICAD_SESSION_JSON
    };
}

} // namespace core
} // namespace freerouting
