#include "datastructures/IndentFileWriter.h"

namespace freerouting::datastructures {

void IndentFileWriter::start_scope(bool p_new_line)
{
    if (p_new_line)
    {
        new_line();
    }
    stream_ << '(';
    ++current_indent_level_;
}

void IndentFileWriter::end_scope()
{
    if (current_indent_level_ > 0)
    {
        --current_indent_level_;
    }
    new_line();
    stream_ << ')';
}

void IndentFileWriter::new_line()
{
    stream_ << '\n';
    for (int index = 0; index < current_indent_level_; ++index)
    {
        stream_ << "  ";
    }
}

} // namespace freerouting::datastructures
