// Tests for WriteScopeParameter (Java parser.WriteScopeParameter) and the
// Resolution write half. Golden strings come from the Java baseline (JDK 25).

#include "designforms/specctra/parser/WriteScopeParameter.h"

#include <gtest/gtest.h>

#include <sstream>
#include <string>

#include "board/Communication.h"
#include "designforms/specctra/parser/Resolution.h"

namespace freerouting::designforms::specctra::parser {
namespace {

TEST(WriteScopeParameterTest, StoresFieldsAndBuildsIdentifierWithQuote)
{
    std::ostringstream stream;
    datastructures::IndentFileWriter file(stream);
    const CoordinateTransform transform(2.0, 1.0, -1.0);

    WriteScopeParameter parameter(nullptr, nullptr, file, "\"", transform, true);

    EXPECT_EQ(nullptr, parameter.board);
    EXPECT_EQ(nullptr, parameter.autoroute_settings);
    EXPECT_TRUE(parameter.compat_mode);
    EXPECT_DOUBLE_EQ(2.0, parameter.coordinate_transform.scale_factor());

    std::ostringstream identifier_out;
    parameter.identifier_type.write("F.Cu", identifier_out);
    parameter.identifier_type.write("A B", identifier_out);
    parameter.identifier_type.write("plainname", identifier_out);
    EXPECT_EQ("F.Cu\"A B\"plainname", identifier_out.str());
}

TEST(ResolutionTest, WriteScopeMatchesJava)
{
    const board::Communication communication(
        board::Unit::MM, 1000, board::Communication::SpecctraParserInfo{}, 1.0, 0.0, 0.0);

    std::ostringstream stream;
    datastructures::IndentFileWriter file(stream);
    Resolution::write_scope(file, communication);

    EXPECT_EQ("\n(resolution mm 1000)", stream.str());
}

} // namespace
} // namespace freerouting::designforms::specctra::parser
