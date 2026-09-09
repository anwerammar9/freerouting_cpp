#include "datastructures/IdentifierType.h"

#include <gtest/gtest.h>
#include <sstream>

namespace freerouting::datastructures {

TEST(IdentifierTypeTest, LeavesLegalIdentifierUnquoted)
{
    IdentifierType identifiers({" ", "("}, "\"");
    std::ostringstream output;

    identifiers.write("signal_A", output);

    EXPECT_EQ(output.str(), "signal_A");
}

TEST(IdentifierTypeTest, QuotesReservedCharactersAndNumericPrefixes)
{
    IdentifierType identifiers({" ", "("}, "\"");
    std::ostringstream output;

    identifiers.write("signal A", output);
    EXPECT_EQ(output.str(), "\"signal A\"");

    output.str("");
    output.clear();
    identifiers.write("-12V", output);
    EXPECT_EQ(output.str(), "\"-12V\"");
}

TEST(IdentifierTypeTest, RemovesExistingQuotesBeforeWriting)
{
    IdentifierType identifiers({" "}, "\"");
    std::ostringstream output;

    identifiers.write("\"signal A\"", output);

    EXPECT_EQ(output.str(), "\"signal A\"");
}

TEST(IdentifierTypeTest, RemovesEmbeddedQuoteCharacters)
{
    IdentifierType identifiers({" "}, "\"");
    std::ostringstream output;

    identifiers.write("sig\"nal", output);

    EXPECT_EQ(output.str(), "signal");
}

TEST(IdentifierTypeTest, QuotesNonAsciiIdentifiers)
{
    IdentifierType identifiers({}, "\"");
    std::ostringstream output;

    identifiers.write("cafe\xC3\xA9", output);

    EXPECT_EQ(output.str(), "\"cafe\xC3\xA9\"");
}

} // namespace freerouting::datastructures
