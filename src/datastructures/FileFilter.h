#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace freerouting::datastructures {

/**
 * Filters paths by filename extension, while accepting directories.
 */
class FileFilter
{
public:
    explicit FileFilter(std::vector<std::string> p_extensions);

    std::string get_description() const;
    bool accept(const std::filesystem::path& p_file) const;

private:
    std::vector<std::string> extensions_;
};

} // namespace freerouting::datastructures
