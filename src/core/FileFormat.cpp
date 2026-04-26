#include "FileFormat.h"

namespace freerouting {
namespace core {

std::string to_string(FileFormat format) {
    switch (format) {
        case FileFormat::UNKNOWN:
            return "UNKNOWN";
        case FileFormat::DSN:
            return "DSN";
        case FileFormat::FRB:
            return "FRB";
        case FileFormat::SES:
            return "SES";
        case FileFormat::RULES:
            return "RULES";
        case FileFormat::SCR:
            return "SCR";
        case FileFormat::DRC_JSON:
            return "DRC_JSON";
        default:
            return "UNKNOWN";
    }
}

std::string file_extension(FileFormat format) {
    switch (format) {
        case FileFormat::DSN:
            return ".dsn";
        case FileFormat::FRB:
            return ".frb";
        case FileFormat::SES:
            return ".ses";
        case FileFormat::RULES:
            return ".rules";
        case FileFormat::SCR:
            return ".scr";
        case FileFormat::DRC_JSON:
            return ".json";
        case FileFormat::UNKNOWN:
        default:
            return "";
    }
}

FileFormat parse_file_format(const std::string& format_str) {
    if (format_str == "DSN" || format_str == "dsn") {
        return FileFormat::DSN;
    } else if (format_str == "FRB" || format_str == "frb") {
        return FileFormat::FRB;
    } else if (format_str == "SES" || format_str == "ses") {
        return FileFormat::SES;
    } else if (format_str == "RULES" || format_str == "rules") {
        return FileFormat::RULES;
    } else if (format_str == "SCR" || format_str == "scr") {
        return FileFormat::SCR;
    } else if (format_str == "DRC_JSON" || format_str == "drc_json") {
        return FileFormat::DRC_JSON;
    }
    return FileFormat::UNKNOWN;
}

} // namespace core
} // namespace freerouting
