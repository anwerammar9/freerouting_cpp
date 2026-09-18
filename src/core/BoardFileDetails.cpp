#include "core/BoardFileDetails.h"

#include <array>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <system_error>

#include "board/RoutingBoard.h"
#include "core/RoutingJob.h"
#include "core/events/BoardFileDetailsUpdatedEvent.h"
#include "core/events/BoardFileDetailsUpdatedEventListener.h"

namespace freerouting {
namespace core {

namespace {

/// Standard IEEE 802.3 (zlib) CRC32 lookup table.
const std::array<uint32_t, 256>& crc32_table()
{
    static const std::array<uint32_t, 256> table = [] {
        std::array<uint32_t, 256> result{};
        for (uint32_t i = 0; i < 256; ++i)
        {
            uint32_t c = i;
            for (int k = 0; k < 8; ++k)
            {
                c = (c & 1u) != 0u ? 0xEDB88320u ^ (c >> 1) : c >> 1;
            }
            result[i] = c;
        }
        return result;
    }();
    return table;
}

std::string json_escape(const std::string& p_value)
{
    std::ostringstream oss;
    for (char c : p_value)
    {
        switch (c)
        {
        case '"':
            oss << "\\\"";
            break;
        case '\\':
            oss << "\\\\";
            break;
        case '\n':
            oss << "\\n";
            break;
        case '\r':
            oss << "\\r";
            break;
        case '\t':
            oss << "\\t";
            break;
        default:
            oss << c;
            break;
        }
    }
    return oss.str();
}

std::string json_string(const std::string& p_value)
{
    return "\"" + json_escape(p_value) + "\"";
}

std::string json_number(float p_value)
{
    std::ostringstream oss;
    oss << p_value;
    return oss.str();
}

} // namespace

BoardFileDetails::BoardFileDetails() = default;

BoardFileDetails::BoardFileDetails(const std::string& p_file_path)
{
    this->set_filename(p_file_path);
    if (!p_file_path.empty())
    {
        std::ifstream in(p_file_path, std::ios::binary);
        if (in)
        {
            std::ostringstream buffer;
            buffer << in.rdbuf();
            this->set_data(buffer.str());
        }
    }
}

BoardFileDetails::BoardFileDetails(board::RoutingBoard* p_board)
{
    this->statistics_ = scoring::BoardStatistics(p_board);
}

uint32_t BoardFileDetails::calculate_crc32(const std::string& p_data)
{
    const auto& table = crc32_table();
    uint32_t crc = 0xFFFFFFFFu;
    for (unsigned char c : p_data)
    {
        crc = table[(crc ^ c) & 0xFFu] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFFu;
}

std::string BoardFileDetails::absolute_path() const
{
    if (filename_.empty())
    {
        return "";
    }
    std::filesystem::path path;
    if (!directory_path_.empty())
    {
        path = std::filesystem::path(directory_path_) / filename_;
    }
    else
    {
        path = std::filesystem::path(filename_);
    }
    return path.string();
}

std::string BoardFileDetails::filename_without_extension() const
{
    size_t last_dot = filename_.find_last_of('.');
    if (last_dot != std::string::npos)
    {
        return filename_.substr(0, last_dot);
    }
    return filename_;
}

void BoardFileDetails::set_data(const std::string& p_data)
{
    this->data_ = p_data;
    this->crc32_ = calculate_crc32(p_data);

    // Read the file contents to determine the file format.
    FileFormat detected_format = RoutingJob::get_file_format(p_data);
    if (detected_format != FileFormat::UNKNOWN)
    {
        this->format_ = detected_format;
    }
    else if (this->format_ == FileFormat::UNKNOWN && !this->filename_.empty())
    {
        FileFormat from_path = RoutingJob::get_file_format_from_path(this->filename_);
        if (from_path != FileFormat::UNKNOWN)
        {
            this->format_ = from_path;
        }
    }

    // Set the statistical data based on the file content.
    this->statistics_ = scoring::BoardStatistics(p_data, this->format_);

    this->fire_updated_event();
}

void BoardFileDetails::set_filename(const std::string& p_filename)
{
    if (p_filename.empty())
    {
        this->directory_path_ = "";
        this->filename_ = "";
        return;
    }

    std::error_code ec;
    std::filesystem::path path = std::filesystem::absolute(p_filename, ec);
    if (ec)
    {
        path = std::filesystem::path(p_filename);
    }

    // Separate the filename into its absolute path and its filename only.
    std::filesystem::path parent = path.parent_path();
    this->directory_path_ = parent.string();
    this->filename_ = path.filename().string();

    if (this->format_ == FileFormat::UNKNOWN)
    {
        // Try to read the contents to determine the file format from the path.
        FileFormat from_path = RoutingJob::get_file_format_from_path(this->filename_);
        if (from_path != FileFormat::UNKNOWN)
        {
            this->format_ = from_path;
        }
    }

    // Add the default file extension if it is missing.
    if (this->format_ != FileFormat::UNKNOWN && this->filename_.find('.') == std::string::npos)
    {
        std::string extension = file_extension(this->format_);
        if (!extension.empty())
        {
            this->filename_ += extension;
        }
    }

    this->fire_updated_event();
}

void BoardFileDetails::save_as(const std::string& p_filename) const
{
    std::ofstream out(p_filename, std::ios::binary | std::ios::trunc);
    if (out)
    {
        out << this->toString();
    }
}

std::string BoardFileDetails::toString() const
{
    std::ostringstream oss;
    oss << "{";
    oss << "\"size\": " << this->size() << ",";
    oss << "\"crc32\": " << this->crc32() << ",";
    oss << "\"format\": " << json_string(to_string(this->format_)) << ",";
    oss << "\"filename\": " << json_string(this->filename_) << ",";
    oss << "\"path\": " << json_string(this->directory_path_) << ",";
    oss << "\"statistics\": {";
    const scoring::BoardStatistics& s = this->statistics_;
    oss << "\"host\": " << json_string(s.host()) << ",";
    oss << "\"unit\": " << json_string(s.get_unit()) << ",";
    oss << "\"board\": {"
        << "\"size\": {\"x\": " << json_number(s.get_size().x)
        << ", \"y\": " << json_number(s.get_size().y) << "}"
        << ", \"areaCm2\": " << json_number(s.board_area_cm2()) << "},";
    oss << "\"layers\": {"
        << "\"totalCount\": " << s.total_layers()
        << ", \"signalCount\": " << s.signal_layer_count() << "},";
    oss << "\"items\": {"
        << "\"totalCount\": " << s.totalCount()
        << ", \"traceCount\": " << s.traceCount()
        << ", \"viaCount\": " << s.viaCount()
        << ", \"pinCount\": " << s.difficulty_pin_count() << "},";
    oss << "\"components\": {\"totalCount\": " << s.component_count() << "},";
    oss << "\"pads\": {\"totalCount\": " << s.pad_count() << "},";
    oss << "\"nets\": {"
        << "\"totalCount\": " << s.net_count()
        << ", \"classCount\": " << s.net_class_count() << "},";
    oss << "\"connections\": {"
        << "\"maximumCount\": " << s.maximum_connections()
        << ", \"incompleteCount\": " << s.incomplete_connections() << "},";
    oss << "\"traces\": {"
        << "\"totalCount\": " << s.trace_count()
        << ", \"totalLength\": " << json_number(s.total_trace_length())
        << ", \"totalLengthMm\": " << json_number(s.total_length_mm())
        << ", \"averageLength\": " << json_number(s.average_trace_length())
        << ", \"totalSegmentCount\": " << s.total_segment_count()
        << ", \"totalHorizontalLength\": " << json_number(s.total_horizontal_length())
        << ", \"totalVerticalLength\": " << json_number(s.total_vertical_length())
        << ", \"totalAngledLength\": " << json_number(s.total_angled_length()) << "},";
    oss << "\"bends\": {"
        << "\"totalCount\": " << s.bend_count()
        << ", \"ninetyDegreeCount\": " << s.ninety_degree_bend_count()
        << ", \"fortyFiveDegreeCount\": " << s.forty_five_degree_bend_count() << "},";
    oss << "\"vias\": {"
        << "\"totalCount\": " << s.via_total_count()
        << ", \"throughHoleCount\": " << s.via_through_hole_count() << "},";
    oss << "\"clearanceViolations\": {\"totalCount\": " << s.clearance_violation_count() << "},";
    oss << "\"difficulty\": {"
        << "\"pinCount\": " << s.difficulty_pin_count()
        << ", \"signalLayerCount\": " << s.difficulty_signal_layer_count()
        << ", \"complexityC\": " << s.difficulty_complexity_c()
        << ", \"boardAreaCm2\": " << json_number(s.difficulty_d()) << "},";
    oss << "\"fanout\": {"
        << "\"totalSmdPins\": " << s.fanout_total_smd_pins()
        << ", \"pinsToEscape\": " << s.fanout_pins_to_escape()
        << ", \"escapedCount\": " << s.fanout_escaped_count() << "}";
    oss << "}";
    oss << "}";
    return oss.str();
}

void BoardFileDetails::add_updated_event_listener(
    events::BoardFileDetailsUpdatedEventListener* p_listener)
{
    if (p_listener != nullptr)
    {
        updated_event_listeners_.push_back(p_listener);
    }
}

void BoardFileDetails::fire_updated_event()
{
    events::BoardFileDetailsUpdatedEvent event(this, this);
    for (events::BoardFileDetailsUpdatedEventListener* listener : updated_event_listeners_)
    {
        listener->on_board_file_details_updated(event);
    }
}

} // namespace core
} // namespace freerouting