/**
 * @file BoardHistoryEntry_test.cpp
 * @brief Unit tests for BoardHistoryEntry class
 */

#include "autoroute/BoardHistoryEntry.h"
#include <gtest/gtest.h>
#include <memory>
#include <chrono>

namespace freerouting {
namespace autoroute {

// Mock implementations for testing
class MockRoutingBoard {
public:
    std::shared_ptr<BoardStatistics> get_statistics() const {
        return std::make_shared<BoardStatistics>();
    }
    
    std::string get_hash() const {
        return "mock_hash_12345";
    }
};

class MockBoardStatistics : public BoardStatistics {
public:
    float get_normalized_score(const RouterScoringSettings& settings) const {
        return 85.5f; // Mock score
    }
};

class MockRouterScoringSettings {
public:
    // Mock scoring settings
};

class BoardHistoryEntryTest : public ::testing::Test {
protected:
    void SetUp() override {
        board = std::make_shared<MockRoutingBoard>();
        settings = std::make_unique<MockRouterScoringSettings>();
    }
    
    std::shared_ptr<MockRoutingBoard> board;
    std::unique_ptr<MockRouterScoringSettings> settings;
};

// Test constructor and basic functionality
TEST_F(BoardHistoryEntryTest, ConstructorAndBasicFunctionality) {
    // Test that we can create a BoardHistoryEntry
    EXPECT_NO_THROW({
        BoardHistoryEntry entry(board, *settings);
    });
    
    BoardHistoryEntry entry(board, *settings);
    
    // Test basic accessors
    EXPECT_EQ(entry.get_board(), board);
    EXPECT_EQ(entry.get_score(), 85.5f);
    EXPECT_EQ(entry.get_hash(), "mock_hash_12345");
    EXPECT_EQ(entry.get_restore_count(), 0);
    EXPECT_NE(entry.get_statistics(), nullptr);
}

// Test timestamp functionality
TEST_F(BoardHistoryEntryTest, TimestampFunctionality) {
    auto before_creation = std::chrono::system_clock::now();
    
    BoardHistoryEntry entry(board, *settings);
    auto timestamp = entry.get_timestamp();
    
    auto after_creation = std::chrono::system_clock::now();
    
    // Timestamp should be between before and after creation
    EXPECT_GE(timestamp, before_creation);
    EXPECT_LE(timestamp, after_creation);
}

// Test restore count functionality
TEST_F(BoardHistoryEntryTest, RestoreCountFunctionality) {
    BoardHistoryEntry entry(board, *settings);
    
    // Initial restore count should be 0
    EXPECT_EQ(entry.get_restore_count(), 0);
    
    // Increment restore count
    entry.increment_restore_count();
    EXPECT_EQ(entry.get_restore_count(), 1);
    
    // Increment multiple times
    entry.increment_restore_count();
    entry.increment_restore_count();
    entry.increment_restore_count();
    EXPECT_EQ(entry.get_restore_count(), 4);
}

// Test comparison operators
TEST_F(BoardHistoryEntryTest, ComparisonOperators) {
    // Create entries with different scores
    auto board1 = std::make_shared<MockRoutingBoard>();
    auto board2 = std::make_shared<MockRoutingBoard>();
    
    BoardHistoryEntry entry1(board1, *settings);
    BoardHistoryEntry entry2(board2, *settings);
    
    // Both should have the same score initially
    EXPECT_EQ(entry1.get_score(), entry2.get_score());
    
    // Test less than operator
    EXPECT_FALSE(entry1 < entry2);
    EXPECT_FALSE(entry2 < entry1);
    
    // Test three-way comparison
    EXPECT_EQ(entry1 <=> entry2, std::strong_ordering::equal);
    EXPECT_EQ(entry2 <=> entry1, std::strong_ordering::equal);
    
    // Test with different scores (we'll need to mock different scores)
    // For now, just test the comparison operators exist and work
}

// Test copy and move semantics
TEST_F(BoardHistoryEntryTest, CopyAndMoveSemantics) {
    BoardHistoryEntry original(board, *settings);
    
    // Test copy constructor
    BoardHistoryEntry copied(original);
    EXPECT_EQ(copied.get_board(), original.get_board());
    EXPECT_EQ(copied.get_score(), original.get_score());
    EXPECT_EQ(copied.get_hash(), original.get_hash());
    EXPECT_EQ(copied.get_restore_count(), original.get_restore_count());
    
    // Test copy assignment
    BoardHistoryEntry assigned(board, *settings);
    assigned = original;
    EXPECT_EQ(assigned.get_board(), original.get_board());
    EXPECT_EQ(assigned.get_score(), original.get_score());
    EXPECT_EQ(assigned.get_hash(), original.get_hash());
    EXPECT_EQ(assigned.get_restore_count(), original.get_restore_count());
    
    // Test move constructor
    BoardHistoryEntry moved(std::move(original));
    EXPECT_EQ(moved.get_board(), board);
    EXPECT_EQ(moved.get_score(), 85.5f);
    EXPECT_EQ(moved.get_hash(), "mock_hash_12345");
    EXPECT_EQ(moved.get_restore_count(), 0);
}

// Test const correctness
TEST_F(BoardHistoryEntryTest, ConstCorrectness) {
    const BoardHistoryEntry entry(board, *settings);
    
    // Should be able to call const methods
    EXPECT_EQ(entry.get_board(), board);
    EXPECT_EQ(entry.get_score(), 85.5f);
    EXPECT_EQ(entry.get_hash(), "mock_hash_12345");
    EXPECT_EQ(entry.get_restore_count(), 0);
    EXPECT_NE(entry.get_statistics(), nullptr);
    
    // Should be able to get timestamp
    auto timestamp = entry.get_timestamp();
    EXPECT_NE(timestamp, std::chrono::time_point<std::chrono::system_clock>{});
}

// Test edge cases
TEST_F(BoardHistoryEntryTest, EdgeCases) {
    // Test with null board (should throw)
    std::shared_ptr<MockRoutingBoard> null_board = nullptr;
    EXPECT_THROW({
        BoardHistoryEntry entry(null_board, *settings);
    }, std::invalid_argument);
}

// Test sorting functionality
TEST_F(BoardHistoryEntryTest, SortingFunctionality) {
    // Create multiple entries
    std::vector<BoardHistoryEntry> entries;
    
    // We'll need to mock different scores for proper sorting test
    // For now, just test that entries can be sorted (they have equal scores)
    for (int i = 0; i < 3; ++i) {
        auto test_board = std::make_shared<MockRoutingBoard>();
        entries.emplace_back(test_board, *settings);
        entries.back().increment_restore_count(); // Different restore counts
    }
    
    // Sort the entries
    std::sort(entries.begin(), entries.end());
    
    // All entries should have equal scores, so order should be stable
    EXPECT_EQ(entries[0].get_score(), entries[1].get_score());
    EXPECT_EQ(entries[1].get_score(), entries[2].get_score());
}

} // namespace autoroute
} // namespace freerouting
