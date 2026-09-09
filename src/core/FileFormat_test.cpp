/**
 * @file FileFormat_test.cpp
 * @brief Unit tests for FileFormat enum class
 */

#include "FileFormat.h"
#include <gtest/gtest.h>
#include <string>

namespace freerouting {
namespace core {

class FileFormatTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Test setup if needed
    }
};

// Test basic enum functionality
TEST_F(FileFormatTest, BasicEnumValues) {
    EXPECT_EQ(static_cast<int>(FileFormat::UNKNOWN), 0);
    EXPECT_EQ(static_cast<int>(FileFormat::DSN), 1);
    EXPECT_EQ(static_cast<int>(FileFormat::FRB), 2);
    EXPECT_EQ(static_cast<int>(FileFormat::SES), 3);
    EXPECT_EQ(static_cast<int>(FileFormat::RULES), 4);
    EXPECT_EQ(static_cast<int>(FileFormat::SCR), 5);
    EXPECT_EQ(static_cast<int>(FileFormat::DRC_JSON), 6);
}

// Test string conversion
TEST_F(FileFormatTest, ToStringConversion) {
    EXPECT_EQ(to_string(FileFormat::UNKNOWN), "UNKNOWN");
    EXPECT_EQ(to_string(FileFormat::DSN), "DSN");
    EXPECT_EQ(to_string(FileFormat::FRB), "FRB");
    EXPECT_EQ(to_string(FileFormat::SES), "SES");
    EXPECT_EQ(to_string(FileFormat::RULES), "RULES");
    EXPECT_EQ(to_string(FileFormat::SCR), "SCR");
    EXPECT_EQ(to_string(FileFormat::DRC_JSON), "DRC_JSON");
}

// Test file extension mapping
TEST_F(FileFormatTest, FileExtensions) {
    EXPECT_EQ(file_extension(FileFormat::UNKNOWN), "");
    EXPECT_EQ(file_extension(FileFormat::DSN), ".dsn");
    EXPECT_EQ(file_extension(FileFormat::FRB), ".frb");
    EXPECT_EQ(file_extension(FileFormat::SES), ".ses");
    EXPECT_EQ(file_extension(FileFormat::RULES), ".rules");
    EXPECT_EQ(file_extension(FileFormat::SCR), ".scr");
    EXPECT_EQ(file_extension(FileFormat::DRC_JSON), ".json");
}

// Test string parsing
TEST_F(FileFormatTest, ParseFromString) {
    // Test uppercase parsing
    EXPECT_EQ(parse_file_format("DSN"), FileFormat::DSN);
    EXPECT_EQ(parse_file_format("FRB"), FileFormat::FRB);
    EXPECT_EQ(parse_file_format("SES"), FileFormat::SES);
    EXPECT_EQ(parse_file_format("RULES"), FileFormat::RULES);
    EXPECT_EQ(parse_file_format("SCR"), FileFormat::SCR);
    EXPECT_EQ(parse_file_format("DRC_JSON"), FileFormat::DRC_JSON);
    
    // Test lowercase parsing
    EXPECT_EQ(parse_file_format("dsn"), FileFormat::DSN);
    EXPECT_EQ(parse_file_format("frb"), FileFormat::FRB);
    EXPECT_EQ(parse_file_format("ses"), FileFormat::SES);
    EXPECT_EQ(parse_file_format("rules"), FileFormat::RULES);
    EXPECT_EQ(parse_file_format("scr"), FileFormat::SCR);
    EXPECT_EQ(parse_file_format("drc_json"), FileFormat::DRC_JSON);
    
    // Test invalid strings
    EXPECT_EQ(parse_file_format("invalid"), FileFormat::UNKNOWN);
    EXPECT_EQ(parse_file_format(""), FileFormat::UNKNOWN);
    EXPECT_EQ(parse_file_format("TXT"), FileFormat::UNKNOWN);
}

// Test all formats array
TEST_F(FileFormatTest, GetAllFormats) {
    auto formats = get_all_formats();
    EXPECT_EQ(formats.size(), 7);
    
    // Verify all formats are included
    bool has_unknown = false, has_dsn = false, has_frb = false;
    bool has_ses = false, has_rules = false, has_scr = false, has_drc_json = false;
    
    for (const auto& format : formats) {
        switch (format) {
            case FileFormat::UNKNOWN: has_unknown = true; break;
            case FileFormat::DSN: has_dsn = true; break;
            case FileFormat::FRB: has_frb = true; break;
            case FileFormat::SES: has_ses = true; break;
            case FileFormat::RULES: has_rules = true; break;
            case FileFormat::SCR: has_scr = true; break;
            case FileFormat::DRC_JSON: has_drc_json = true; break;
        }
    }
    
    EXPECT_TRUE(has_unknown);
    EXPECT_TRUE(has_dsn);
    EXPECT_TRUE(has_frb);
    EXPECT_TRUE(has_ses);
    EXPECT_TRUE(has_rules);
    EXPECT_TRUE(has_scr);
    EXPECT_TRUE(has_drc_json);
}

// Test round-trip conversion
TEST_F(FileFormatTest, RoundTripConversion) {
    // Test that to_string -> parse_file_format gives original value
    auto formats = get_all_formats();
    for (const auto& format : formats) {
        std::string str = to_string(format);
        FileFormat parsed = parse_file_format(str);
        EXPECT_EQ(format, parsed) << "Failed round-trip for " << str;
    }
}

// Test edge cases
TEST_F(FileFormatTest, EdgeCases) {
    // Test with whitespace
    EXPECT_EQ(parse_file_format(" DSN "), FileFormat::UNKNOWN);
    EXPECT_EQ(parse_file_format("\tDSN\t"), FileFormat::UNKNOWN);
    
    // Test with mixed case
    EXPECT_EQ(parse_file_format("Dsn"), FileFormat::UNKNOWN);
    EXPECT_EQ(parse_file_format("dSn"), FileFormat::UNKNOWN);
}

} // namespace core
} // namespace freerouting
