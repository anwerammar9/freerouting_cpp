#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "core/FileFormat.h"
#include "core/scoring/BoardStatistics.h"

namespace freerouting {
namespace board {
class RoutingBoard;
}

namespace core {
namespace events {
class BoardFileDetailsUpdatedEvent;
class BoardFileDetailsUpdatedEventListener;
}

/**
 * Stores file data, metadata and derived board statistics for an input or
 * output board file. Mirrors Java BoardFileDetails.java.
 */
class BoardFileDetails
{
public:
    BoardFileDetails();
    explicit BoardFileDetails(const std::string& p_file_path);
    explicit BoardFileDetails(board::RoutingBoard* p_board);

    /// Size of the file data in bytes.
    int64_t size() const { return static_cast<int64_t>(data_.size()); }

    /// CRC32 checksum of the file data.
    uint32_t crc32() const { return crc32_; }

    FileFormat format() const { return format_; }

    const scoring::BoardStatistics& statistics() const { return statistics_; }

    const std::string& filename() const { return filename_; }

    const std::string& directory_path() const { return directory_path_; }

    const std::string& data() const { return data_; }

    /// Absolute path combining directory and filename, "" when no filename set.
    std::string absolute_path() const;

    /// Filename without its final extension.
    std::string filename_without_extension() const;

    /// Replaces the file data and updates size, checksum, format and statistics.
    void set_data(const std::string& p_data);

    /// Sets both the filename and the directory path.
    void set_filename(const std::string& p_filename);

    /// Saves this object as a UTF-8 JSON file.
    void save_as(const std::string& p_filename) const;

    /// Returns a JSON representation of this object.
    std::string toString() const;

    /// Calculates the standard (IEEE 802.3 / zlib) CRC32 checksum of the data.
    static uint32_t calculate_crc32(const std::string& p_data);

    /// Registers a listener for changes to these details.
    void add_updated_event_listener(events::BoardFileDetailsUpdatedEventListener* p_listener);

private:
    std::vector<events::BoardFileDetailsUpdatedEventListener*> updated_event_listeners_;
    uint32_t crc32_ = 0;
    FileFormat format_ = FileFormat::UNKNOWN;
    scoring::BoardStatistics statistics_;
    std::string filename_;
    std::string directory_path_;
    std::string data_;

    void fire_updated_event();
};

} // namespace core
} // namespace freerouting