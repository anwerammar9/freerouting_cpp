#pragma once

#include <ostream>
#include <string>
#include <utility>

namespace freerouting::datastructures {

/**
 * Handles indentation while writing parenthesized scopes to a text stream.
 */
class IndentFileWriter
{
public:
    explicit IndentFileWriter(std::ostream& p_stream)
        : stream_(p_stream)
    {
    }

    void start_scope(bool p_new_line = true);
    void end_scope();
    void new_line();

    template <typename T>
    IndentFileWriter& operator<<(T&& p_value)
    {
        stream_ << std::forward<T>(p_value);
        return *this;
    }

    std::ostream& stream() { return stream_; }
    const std::ostream& stream() const { return stream_; }
    int current_indent_level() const { return current_indent_level_; }

private:
    std::ostream& stream_;
    int current_indent_level_ = 0;
};

} // namespace freerouting::datastructures
