#include "core/RoutingJob.h"
#include "logger/FRLogger.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <filesystem>

namespace freerouting {
namespace core {

namespace {

bool starts_with_ci(const std::string& p_value, const std::string& p_prefix)
{
    if (p_value.size() < p_prefix.size())
    {
        return false;
    }
    for (size_t i = 0; i < p_prefix.size(); ++i)
    {
        if (std::tolower(static_cast<unsigned char>(p_value[i])) !=
            std::tolower(static_cast<unsigned char>(p_prefix[i])))
        {
            return false;
        }
    }
    return true;
}

} // namespace

const std::string RoutingJob::DSN_FILE_EXTENSION = "dsn";
const std::string RoutingJob::BINARY_FILE_EXTENSION = "frb";

FileFormat RoutingJob::get_file_format(const std::string& p_content)
{
    // First, check if it is a text-based format (JSON or SCR).
    size_t first_non_ws = 0;
    while (first_non_ws < p_content.size())
    {
        char c = p_content[first_non_ws];
        if (c != ' ' && c != '\t' && c != '\r' && c != '\n')
        {
            break;
        }
        ++first_non_ws;
    }
    if (first_non_ws < p_content.size())
    {
        if (p_content[first_non_ws] == '{')
        {
            return FileFormat::KICAD_DESIGN_JSON;
        }
        std::string text_sample =
            p_content.substr(first_non_ws, std::min<size_t>(64, p_content.size() - first_non_ws));
        text_sample.resize(std::min<size_t>(text_sample.size(), 64));
        if (starts_with_ci(text_sample, "GRID ")
            || starts_with_ci(text_sample, "GRID;")
            || starts_with_ci(text_sample, "SET ")
            || starts_with_ci(text_sample, "LAYER ")
            || starts_with_ci(text_sample, "WIRE ")
            || starts_with_ci(text_sample, "CHANGE LAYER ")
            || starts_with_ci(text_sample, "SCRIPT")
            || starts_with_ci(text_sample, "RIPUP"))
        {
            return FileFormat::SCR;
        }
    }

    // Read the first six bytes to determine the binary file format.
    if (p_content.size() >= 6)
    {
        char buffer[6];
        for (int i = 0; i < 6; ++i)
        {
            buffer[i] = p_content[static_cast<size_t>(i)];
        }

        // Java-serialised freerouting binary files start with the magic bytes
        // 0xAC 0xED 0x00 0x05.
        if (static_cast<unsigned char>(buffer[0]) == 0xAC
            && static_cast<unsigned char>(buffer[1]) == 0xED
            && static_cast<unsigned char>(buffer[2]) == 0x00
            && static_cast<unsigned char>(buffer[3]) == 0x05)
        {
            return FileFormat::FRB;
        }

        // Skip leading carriage-return / line-feed bytes.
        while (buffer[0] == 0x0A || buffer[0] == 0x0D)
        {
            for (int i = 0; i < 5; ++i)
            {
                buffer[i] = buffer[i + 1];
            }
        }

        // DSN files start with "(pcb" or "(PCB".
        if ((static_cast<unsigned char>(buffer[0]) == 0x28
             && static_cast<unsigned char>(buffer[1]) == 0x70
             && static_cast<unsigned char>(buffer[2]) == 0x63
             && static_cast<unsigned char>(buffer[3]) == 0x62)
            || (static_cast<unsigned char>(buffer[0]) == 0x28
                && static_cast<unsigned char>(buffer[1]) == 0x50
                && static_cast<unsigned char>(buffer[2]) == 0x43
                && static_cast<unsigned char>(buffer[3]) == 0x42))
        {
            return FileFormat::DSN;
        }

        // SES files start with "(ses" or "(SES".
        if ((static_cast<unsigned char>(buffer[0]) == 0x28
             && static_cast<unsigned char>(buffer[1]) == 0x73
             && static_cast<unsigned char>(buffer[2]) == 0x65
             && static_cast<unsigned char>(buffer[3]) == 0x73)
            || (static_cast<unsigned char>(buffer[0]) == 0x28
                && static_cast<unsigned char>(buffer[1]) == 0x53
                && static_cast<unsigned char>(buffer[2]) == 0x45
                && static_cast<unsigned char>(buffer[3]) == 0x53))
        {
            return FileFormat::SES;
        }

        // RULES files start with "(rules" or "(RULES".
        if (static_cast<unsigned char>(buffer[0]) == 0x28
            && (static_cast<unsigned char>(buffer[1]) == 0x72
                || static_cast<unsigned char>(buffer[1]) == 0x52)
            && (static_cast<unsigned char>(buffer[2]) == 0x75
                || static_cast<unsigned char>(buffer[2]) == 0x55)
            && (static_cast<unsigned char>(buffer[3]) == 0x6C
                || static_cast<unsigned char>(buffer[3]) == 0x4C))
        {
            return FileFormat::RULES;
        }
    }

    return FileFormat::UNKNOWN;
}

FileFormat RoutingJob::get_file_format_from_path(const std::string& p_path)
{
    std::string filename = p_path;
    std::transform(filename.begin(), filename.end(), filename.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    size_t last_dot = filename.find_last_of('.');
    if (last_dot != std::string::npos)
    {
        std::string extension = filename.substr(last_dot + 1);
        if (extension == RoutingJob::DSN_FILE_EXTENSION)
        {
            return FileFormat::DSN;
        }
        if (extension == RoutingJob::BINARY_FILE_EXTENSION)
        {
            return FileFormat::FRB;
        }
        if (extension == "ses")
        {
            return FileFormat::SES;
        }
        if (extension == "rules")
        {
            return FileFormat::RULES;
        }
        if (extension == "scr")
        {
            return FileFormat::SCR;
        }
        if (extension == "json")
        {
            return FileFormat::KICAD_DESIGN_JSON;
        }
    }
    return FileFormat::UNKNOWN;
}

std::string to_string(RoutingJobState p_state)
{
    switch (p_state) {
        case RoutingJobState::INVALID:
            return "INVALID";
        case RoutingJobState::QUEUED:
            return "QUEUED";
        case RoutingJobState::READY_TO_START:
            return "READY_TO_START";
        case RoutingJobState::RUNNING:
            return "RUNNING";
        case RoutingJobState::PAUSED:
            return "PAUSED";
        case RoutingJobState::COMPLETED:
            return "COMPLETED";
        case RoutingJobState::TIMED_OUT:
            return "TIMED_OUT";
        case RoutingJobState::CANCELLED:
            return "CANCELLED";
        case RoutingJobState::TERMINATED:
            return "TERMINATED";
        default:
            return "UNKNOWN";
    }
}

std::string to_string(RoutingJobPriority p_priority)
{
    switch (p_priority) {
        case RoutingJobPriority::LOW:
            return "LOW";
        case RoutingJobPriority::NORMAL:
            return "NORMAL";
        case RoutingJobPriority::HIGH:
            return "HIGH";
        default:
            return "UNKNOWN";
    }
}

RoutingJob::RoutingJob()
    : created_at_(std::chrono::system_clock::now())
{
    logger::FRLogger::instance().info("Creating default RoutingJob");
}

RoutingJob::RoutingJob(const std::string& p_name)
    : name_(p_name), created_at_(std::chrono::system_clock::now())
{
    logger::FRLogger::instance().info("Creating RoutingJob with name: " + p_name);
}

RoutingJob::RoutingJob(const std::string& p_name, const std::string& p_input_file,
                       const std::string& p_output_file)
    : name_(p_name), input_file_(p_input_file), output_file_(p_output_file),
      created_at_(std::chrono::system_clock::now())
{
    logger::FRLogger::instance().info("Creating RoutingJob: " + p_name + 
                                     " (input: " + p_input_file + 
                                     ", output: " + p_output_file + ")");
}

std::string RoutingJob::to_string() const
{
    std::ostringstream oss;
    oss << "RoutingJob{"
        << "name='" << name_ << "'"
        << ", input_file='" << input_file_ << "'"
        << ", output_file='" << output_file_ << "'"
        << ", state=" << freerouting::core::to_string(state_)
        << ", priority=" << freerouting::core::to_string(priority_);
    
    // Format timestamp
    auto time_t = std::chrono::system_clock::to_time_t(created_at_);
    oss << ", created_at=" << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    
    oss << "}";
    return oss.str();
}

void RoutingJob::set_input(const std::string& p_file_path)
{
    input_file_ = p_file_path;
    logger::FRLogger::instance().info("RoutingJob input set to: " + p_file_path);
}

bool RoutingJob::try_to_set_output_file(const std::string& p_file_path)
{
    namespace fs = std::filesystem;
    std::string ext = fs::path(p_file_path).extension().string();

    // Convert to lowercase for comparison
    std::string lower_ext;
    for (char c : ext) lower_ext += static_cast<char>(std::tolower(c));

    if (lower_ext == ".dsn" || lower_ext == ".ses" || lower_ext == ".frb" || lower_ext == ".scr") {
        output_file_ = p_file_path;
        logger::FRLogger::instance().info("RoutingJob output set to: " + p_file_path);
        return true;
    }

    logger::FRLogger::instance().warn("Unsupported output file format: " + ext);
    return false;
}

} // namespace core
} // namespace freerouting
