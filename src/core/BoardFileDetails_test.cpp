#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <memory>
#include <string>

#include "board/Layer.h"
#include "board/RoutingBoard.h"
#include "core/BoardFileDetails.h"
#include "core/RoutingJob.h"
#include "core/events/BoardFileDetailsUpdatedEvent.h"
#include "core/events/BoardFileDetailsUpdatedEventListener.h"
#include "rules/BoardRules.h"
#include "geometry/planar/IntBox.h"

using namespace freerouting::core;
using namespace freerouting::core::events;
using namespace freerouting::board;
using namespace freerouting::rules;
using namespace freerouting::geometry::planar;

namespace {

class CountingListener : public BoardFileDetailsUpdatedEventListener
{
public:
    CountingListener() : count(0) {}
    int count;
    void on_board_file_details_updated(const BoardFileDetailsUpdatedEvent&) override { ++count; }
};

std::string write_temp_file(const std::string& p_content)
{
    auto path = std::filesystem::temp_directory_path() / "freerouting_boardfiledetails_test.txt";
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out << p_content;
    out.close();
    return path.string();
}

} // namespace

TEST(BoardFileDetailsTest, Crc32MatchesKnownVector)
{
    // Standard CRC32 check value: crc32("123456789") == 0xCBF43926.
    EXPECT_EQ(BoardFileDetails::calculate_crc32("123456789"), 0xCBF43926u);
    EXPECT_EQ(BoardFileDetails::calculate_crc32(""), 0u);
}

TEST(BoardFileDetailsTest, DefaultConstructorIsEmpty)
{
    BoardFileDetails details;
    EXPECT_EQ(details.size(), 0);
    EXPECT_EQ(details.crc32(), 0u);
    EXPECT_EQ(details.format(), FileFormat::UNKNOWN);
    EXPECT_TRUE(details.filename().empty());
    EXPECT_TRUE(details.directory_path().empty());
    EXPECT_TRUE(details.absolute_path().empty());
}

TEST(BoardFileDetailsTest, BoardConstructorSetsStatistics)
{
    std::vector<std::shared_ptr<Layer>> layers;
    layers.push_back(std::make_shared<Layer>("Top", true));
    layers.push_back(std::make_shared<Layer>("Bottom", true));
    LayerStructure layer_structure(layers);
    auto rules = std::make_unique<BoardRules>(layer_structure, 1);
    RoutingBoard board(IntBox(0, 0, 1000, 1000), layer_structure, std::move(rules));

    BoardFileDetails details(&board);
    EXPECT_EQ(details.format(), FileFormat::UNKNOWN);
    EXPECT_EQ(details.size(), 0);
    EXPECT_EQ(details.statistics().host(), "Freerouting,2.1.0");
}

TEST(BoardFileDetailsTest, SetDataDetectsDsnFormatAndCounts)
{
    BoardFileDetails details;
    std::string content = "(pcb test 1.0\n(layer F.Cu)\n(layer B.Cu)\n(net \"n1\")\n";
    details.set_data(content);

    EXPECT_EQ(details.format(), FileFormat::DSN);
    EXPECT_EQ(details.size(), static_cast<int64_t>(content.size()));
    EXPECT_EQ(details.crc32(), BoardFileDetails::calculate_crc32(content));
    EXPECT_EQ(details.statistics().total_layers(), 2);
    EXPECT_EQ(details.statistics().net_count(), 1);
}

TEST(BoardFileDetailsTest, SetDataDetectsSesFormat)
{
    BoardFileDetails details;
    std::string content = "(ses version 1.0\n(path F.Cu)\n(path B.Cu)\n(component R1 0 0 0 SMD)\n";
    details.set_data(content);

    EXPECT_EQ(details.format(), FileFormat::SES);
    EXPECT_EQ(details.statistics().total_layers(), 2);
    EXPECT_EQ(details.statistics().component_count(), 1);
}

TEST(BoardFileDetailsTest, FileConstructorReadsFile)
{
    std::string content = "(pcb test 1.0\n(layer F.Cu)\n(layer B.Cu)\n(wire F.Cu path)\n";
    std::string path = write_temp_file(content);

    BoardFileDetails details(path);
    EXPECT_EQ(details.format(), FileFormat::DSN);
    EXPECT_EQ(details.size(), static_cast<int64_t>(content.size()));
    EXPECT_EQ(details.crc32(), BoardFileDetails::calculate_crc32(content));
    EXPECT_EQ(details.statistics().total_layers(), 2);
    EXPECT_EQ(details.statistics().trace_count(), 1);
    EXPECT_FALSE(details.filename().empty());
    EXPECT_EQ(details.absolute_path(), path);

    std::filesystem::remove(path);
}

TEST(BoardFileDetailsTest, FilenameWithoutExtension)
{
    BoardFileDetails details;
    details.set_filename("/some/dir/board_v2.dsn");
    EXPECT_EQ(details.filename(), "board_v2.dsn");
    EXPECT_EQ(details.filename_without_extension(), "board_v2");
    EXPECT_FALSE(details.directory_path().empty());
}

TEST(BoardFileDetailsTest, SetFilenameAddsDefaultExtension)
{
    BoardFileDetails details;
    details.set_data("(pcb test 1.0\n");
    details.set_filename("customname");
    EXPECT_EQ(details.format(), FileFormat::DSN);
    EXPECT_EQ(details.filename(), "customname.dsn");
}

TEST(BoardFileDetailsTest, SaveAsWritesJson)
{
    BoardFileDetails details;
    details.set_data("(pcb test 1.0\n)");
    std::string path = write_temp_file("");
    std::filesystem::remove(path);
    path = std::filesystem::temp_directory_path() / "freerouting_boardfiledetails_out.json";
    details.save_as(path);

    std::ifstream in(path);
    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();
    EXPECT_NE(content.find("\"size\""), std::string::npos);
    EXPECT_NE(content.find("\"crc32\""), std::string::npos);
    EXPECT_NE(content.find("\"format\""), std::string::npos);
    EXPECT_NE(content.find("\"statistics\""), std::string::npos);
    std::filesystem::remove(path);
}

TEST(BoardFileDetailsTest, ListenerIsNotified)
{
    BoardFileDetails details;
    CountingListener listener;
    details.add_updated_event_listener(&listener);
    details.set_data("(pcb test 1.0\n");
    EXPECT_EQ(listener.count, 1);
}

TEST(BoardFileDetailsTest, GetFileFormatDetectsContent)
{
    EXPECT_EQ(RoutingJob::get_file_format("(pcb test 1.0\n"), FileFormat::DSN);
    EXPECT_EQ(RoutingJob::get_file_format("\n\n(PCB BOARD\n"), FileFormat::DSN);
    EXPECT_EQ(RoutingJob::get_file_format("(ses version 1.0\n"), FileFormat::SES);
    EXPECT_EQ(RoutingJob::get_file_format("(rules\n"), FileFormat::RULES);
    EXPECT_EQ(RoutingJob::get_file_format("{ \"board\" : { } }"), FileFormat::KICAD_DESIGN_JSON);
    EXPECT_EQ(RoutingJob::get_file_format("GRID mm;\n"), FileFormat::SCR);
    EXPECT_EQ(RoutingJob::get_file_format("LAYER F.Cu\n"), FileFormat::SCR);
    EXPECT_EQ(RoutingJob::get_file_format("hello world"), FileFormat::UNKNOWN);
}

TEST(BoardFileDetailsTest, GetFileFormatFromPath)
{
    EXPECT_EQ(RoutingJob::get_file_format_from_path("dir/board.dsn"), FileFormat::DSN);
    EXPECT_EQ(RoutingJob::get_file_format_from_path("session.SES"), FileFormat::SES);
    EXPECT_EQ(RoutingJob::get_file_format_from_path("out.rules"), FileFormat::RULES);
    EXPECT_EQ(RoutingJob::get_file_format_from_path("output.scr"), FileFormat::SCR);
    EXPECT_EQ(RoutingJob::get_file_format_from_path("saved.frb"), FileFormat::FRB);
    EXPECT_EQ(RoutingJob::get_file_format_from_path("data.json"), FileFormat::KICAD_DESIGN_JSON);
    EXPECT_EQ(RoutingJob::get_file_format_from_path("readme.txt"), FileFormat::UNKNOWN);
}