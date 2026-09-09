#include "datastructures/IdentifierType.h"

#include <algorithm>
#include <cctype>

namespace freerouting::datastructures {

IdentifierType::IdentifierType(std::vector<std::string> p_reserved_chars,
                               std::string p_string_quote)
    : reserved_chars_(std::move(p_reserved_chars))
    , string_quote_(std::move(p_string_quote))
{
}

void IdentifierType::write(std::string p_name, std::ostream& p_file) const
{
    if (string_quote_.empty())
    {
        p_file << p_name;
        return;
    }

    while (p_name.size() > 2 && p_name.front() == string_quote_.front()
           && p_name.back() == string_quote_.back())
    {
        p_name = p_name.substr(1, p_name.size() - 2);
    }

    if (p_name.find(string_quote_) != std::string::npos)
    {
        std::string without_quotes;
        without_quotes.reserve(p_name.size());
        for (std::size_t index = 0; index < p_name.size(); ++index)
        {
            if (p_name.compare(index, string_quote_.size(), string_quote_) == 0)
            {
                index += string_quote_.size() - 1;
                continue;
            }
            without_quotes += p_name[index];
        }
        p_name = std::move(without_quotes);
    }

    bool need_quotes = std::any_of(
        reserved_chars_.begin(), reserved_chars_.end(),
        [&p_name](const std::string& reserved) { return p_name.find(reserved) != std::string::npos; });

    if (!need_quotes)
    {
        need_quotes = std::any_of(
            p_name.begin(), p_name.end(),
            [](unsigned char character) { return character >= 0x80; });
    }

    if (!need_quotes && !p_name.empty()
        && (std::isdigit(static_cast<unsigned char>(p_name.front()))
            || (p_name.front() == '-' && p_name.size() > 1
                && std::isdigit(static_cast<unsigned char>(p_name[1])))))
    {
        need_quotes = true;
    }

    if (need_quotes)
    {
        p_file << string_quote_ << p_name << string_quote_;
    }
    else
    {
        p_file << p_name;
    }
}

} // namespace freerouting::datastructures
