#include "datastructures/JavaHashMapOrder.h"

#include <sstream>
#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace freerouting::datastructures {
namespace {

std::vector<int> parse(const std::string& p_text)
{
    std::vector<int> result;
    std::stringstream stream(p_text);
    for (std::string token; std::getline(stream, token, ',');)
    {
        if (!token.empty())
        {
            result.push_back(std::stoi(token));
        }
    }
    return result;
}

// "<keys in insertion order>|<HashMap.put order>|<HashMap.computeIfAbsent order>", printed by a
// throwaway JDK 25 program iterating HashMap<Integer, Integer>.keySet().
const char* const kJavaCases[] = {
        "1,17,2|1,17,2|17,1,2",
        "40,3,19,35,3,51,70000,65536,2|70000,65536,2,3,19,35,51,40|65536,70000,2,51,35,19,3,40",
        "5,21,37,53,69,85,101,117,133,1|1,5,37,69,101,133,21,53,85,117|1,133,101,69,37,5,117,85,53,21",
        "5,21,37,53,69,85,101,117,133,149,165,181,1|1,5,37,69,101,133,165,21,53,85,117,149,181|1,165,133,101,69,37,5,181,149,117,85,53,21",
        "40,39,38,37,36,35,34,33,32,31,30,29,28,27,26,25,24,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1|1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40|1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40",
        "236,164,485,44,380,254,468,149,350,34,200,212,208,411,495,162,461,238,307,279,342,424,383,51,218,4,239,400,76,311,123,73,179,391,35,492,318,6,146,234,325,380,173,47,333,395,12,289,276,152,12,245,194,170,344,452,208,65,226,157|4,6,391,395,12,400,146,276,149,279,152,411,157,289,34,162,35,164,424,170,44,173,47,307,51,179,311,318,65,194,452,325,200,73,76,461,333,208,468,212,342,344,218,350,226,485,234,236,492,238,495,239,245,123,380,254,383|4,6,391,395,12,400,146,276,149,279,152,411,157,289,162,34,35,164,424,170,44,173,47,179,51,307,311,318,65,194,452,325,200,73,76,333,461,208,212,468,342,344,218,350,226,485,234,492,236,238,239,495,245,123,380,254,383",
};

TEST(JavaHashMapOrderTest, MatchesJdkIterationOrder)
{
    for (const char* line : kJavaCases)
    {
        const std::string text = line;
        const std::size_t first = text.find('|');
        const std::size_t second = text.find('|', first + 1);
        const std::vector<int> keys = parse(text.substr(0, first));
        EXPECT_EQ(parse(text.substr(first + 1, second - first - 1)),
            java_hashmap_int_iteration_order(keys, JavaHashMapInsert::PUT))
            << "put: " << text;
        EXPECT_EQ(parse(text.substr(second + 1)),
            java_hashmap_int_iteration_order(keys, JavaHashMapInsert::COMPUTE_IF_ABSENT))
            << "computeIfAbsent: " << text;
    }
}

TEST(JavaHashMapOrderTest, ComputeIfAbsentPrependsWithinABucket)
{
    // 1 and 17 share bucket 1 of the initial 16-bucket table.
    EXPECT_EQ((std::vector<int>{17, 1, 2}),
        java_hashmap_int_iteration_order({1, 17, 2}, JavaHashMapInsert::COMPUTE_IF_ABSENT));
    EXPECT_EQ((std::vector<int>{1, 17, 2}), java_hashmap_int_iteration_order({1, 17, 2}, JavaHashMapInsert::PUT));
    EXPECT_TRUE(java_hashmap_int_iteration_order({}, JavaHashMapInsert::PUT).empty());
}

} // namespace
} // namespace freerouting::datastructures
