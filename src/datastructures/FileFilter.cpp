#include "datastructures/FileFilter.h"

#include <algorithm>
#include <cctype>

namespace freerouting::datastructures {

namespace {

bool equals_ignore_case(const std::string& p_left, const std::string& p_right)
{
    if (p_left.size() != p_right.size())
    {
        return false;
    }
    return std::equal(p_left.begin(), p_left.end(), p_right.begin(),
                      [](unsigned char left, unsigned char right) {
                          return std::tolower(left) == std::tolower(right);
                      });
}

} // namespace

FileFilter::FileFilter(std::vector<std::string> p_extensions)
    : extensions_(std::move(p_extensions))
{
}

std::string FileFilter::get_description() const
{
    std::string description = "Files with the extensions";
    for (std::size_t index = 0; index < extensions_.size(); ++index)
    {
        description += " ." + extensions_[index];
        if (index + 1 == extensions_.size() - 1)
        {
            description += " or";
        }
        else if (index + 1 < extensions_.size() - 1)
        {
            description += ",";
        }
    }
    return description;
}

bool FileFilter::accept(const std::filesystem::path& p_file) const
{
    if (std::filesystem::is_directory(p_file))
    {
        return true;
    }

    const std::string filename = p_file.filename().string();
    const std::size_t separator = filename.rfind('.');
    if (separator == std::string::npos || separator + 1 == filename.size())
    {
        return false;
    }

    const std::string extension = filename.substr(separator + 1);
    return std::any_of(extensions_.begin(), extensions_.end(),
                       [&extension](const std::string& accepted) {
                           return equals_ignore_case(extension, accepted);
                       });
}

} // namespace freerouting::datastructures
