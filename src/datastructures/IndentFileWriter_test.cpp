#include "datastructures/IndentFileWriter.h"

#include <gtest/gtest.h>
#include <sstream>

namespace freerouting::datastructures {

TEST(IndentFileWriterTest, WritesNestedScopesWithIndentation)
{
    std::ostringstream output;
    IndentFileWriter writer(output);

    writer.start_scope(false);
    writer << "pcb";
    writer.start_scope();
    writer << "structure";
    writer.end_scope();
    writer.end_scope();

    EXPECT_EQ(output.str(), "(pcb\n  (structure\n  )\n)");
}

TEST(IndentFileWriterTest, CanStartScopeWithoutLeadingNewline)
{
    std::ostringstream output;
    IndentFileWriter writer(output);

    writer << "prefix";
    writer.start_scope(false);
    writer << "value";
    writer.end_scope();

    EXPECT_EQ(output.str(), "prefix(value\n)");
}

TEST(IndentFileWriterTest, DoesNotCreateNegativeIndentation)
{
    std::ostringstream output;
    IndentFileWriter writer(output);

    writer.end_scope();

    EXPECT_EQ(writer.current_indent_level(), 0);
    EXPECT_EQ(output.str(), "\n)");
}

} // namespace freerouting::datastructures
