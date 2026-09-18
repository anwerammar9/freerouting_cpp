#include "datastructures/UndoableObjects.h"

#include <gtest/gtest.h>

#include <memory>
#include <vector>

namespace {

using freerouting::datastructures::UndoableObjects;
using Storable = UndoableObjects::Storable;

struct TestStorable : UndoableObjects::Storable {
    explicit TestStorable(int p_value) : value(p_value) {}
    std::unique_ptr<Storable> clone() const override
    {
        return std::make_unique<TestStorable>(value);
    }
    bool operator<(const Storable& other) const override
    {
        return value < static_cast<const TestStorable&>(other).value;
    }
    int value = 0;
};

/// Reads all current objects, skipping redo-only entries.
std::vector<int> read_all(UndoableObjects& p_objects)
{
    std::vector<int> result;
    UndoableObjects::Iterator it = p_objects.start_read_object();
    while (const UndoableObjects::Storable* curr = p_objects.read_object(it))
    {
        result.push_back(static_cast<const TestStorable*>(curr)->value);
    }
    return result;
}

std::unique_ptr<TestStorable> make_item(int p_value)
{
    return std::make_unique<TestStorable>(p_value);
}

int value_of(const std::vector<Storable*>& p_list, size_t p_index)
{
    return static_cast<const TestStorable*>(p_list[p_index])->value;
}

} // namespace

TEST(UndoableObjectsTest, InsertAndReadAll)
{
    UndoableObjects objects;
    objects.insert(make_item(1));
    objects.insert(make_item(2));
    objects.insert(make_item(3));
    EXPECT_EQ(read_all(objects), (std::vector<int>{1, 2, 3}));
    EXPECT_EQ(objects.object_count(), 3u);
}

TEST(UndoableObjectsTest, StackLevelStartsAtZero)
{
    UndoableObjects objects;
    EXPECT_EQ(objects.stack_level(), 0);
}

TEST(UndoableObjectsTest, DeleteMissingObjectReturnsFalse)
{
    UndoableObjects objects;
    auto item = make_item(1);
    EXPECT_FALSE(objects.delete_object(item.get()));
}

TEST(UndoableObjectsTest, DeleteThenUndoRestores)
{
    UndoableObjects objects;
    auto item = make_item(1);
    TestStorable* ptr = item.get();
    objects.insert(std::move(item)); // existed before the snapshot
    objects.generate_snapshot();

    EXPECT_TRUE(objects.delete_object(ptr));
    EXPECT_EQ(read_all(objects), (std::vector<int>{}));

    std::vector<Storable*> cancelled;
    std::vector<Storable*> restored;
    EXPECT_TRUE(objects.undo(&cancelled, &restored));
    EXPECT_EQ(read_all(objects), (std::vector<int>{1}));
    ASSERT_EQ(restored.size(), 1u);
    EXPECT_EQ(value_of(restored, 0), 1);
}

TEST(UndoableObjectsTest, SaveForUndoThenUndoRestoresOldState)
{
    UndoableObjects objects;
    auto item = make_item(1);
    TestStorable* ptr = item.get();
    objects.insert(std::move(item)); // level 0
    objects.generate_snapshot();     // level 1
    objects.save_for_undo(ptr);      // stores the old state (value 1)

    ptr->value = 5; // Java callers mutate the object in place after save_for_undo
    EXPECT_EQ(read_all(objects), (std::vector<int>{5}));

    std::vector<Storable*> cancelled;
    std::vector<Storable*> restored;
    EXPECT_TRUE(objects.undo(&cancelled, &restored));
    EXPECT_EQ(read_all(objects), (std::vector<int>{1}));
    ASSERT_EQ(restored.size(), 1u);
    EXPECT_EQ(value_of(restored, 0), 1);
    ASSERT_EQ(cancelled.size(), 1u);
    EXPECT_EQ(value_of(cancelled, 0), 5);
}

TEST(UndoableObjectsTest, InsertedOnCurrentLevelIsRemovedByUndo)
{
    UndoableObjects objects;
    objects.insert(make_item(1));
    objects.generate_snapshot();
    objects.insert(make_item(2));
    EXPECT_EQ(read_all(objects), (std::vector<int>{1, 2}));

    std::vector<Storable*> cancelled;
    std::vector<Storable*> restored;
    EXPECT_TRUE(objects.undo(&cancelled, &restored));
    EXPECT_EQ(read_all(objects), (std::vector<int>{1}));
    ASSERT_EQ(cancelled.size(), 1u);
    EXPECT_EQ(value_of(cancelled, 0), 2);
    EXPECT_TRUE(restored.empty());
}

TEST(UndoableObjectsTest, RedoReappliesInsertion)
{
    UndoableObjects objects;
    objects.insert(make_item(1));
    objects.generate_snapshot();
    objects.insert(make_item(2));

    std::vector<Storable*> cancelled;
    std::vector<Storable*> restored;
    EXPECT_TRUE(objects.undo(&cancelled, &restored));
    EXPECT_EQ(read_all(objects), (std::vector<int>{1}));

    cancelled.clear();
    restored.clear();
    EXPECT_TRUE(objects.redo(&cancelled, &restored));
    EXPECT_EQ(read_all(objects), (std::vector<int>{1, 2}));
    ASSERT_EQ(restored.size(), 1u);
    EXPECT_EQ(value_of(restored, 0), 2);
}

TEST(UndoableObjectsTest, RedoReappliesInPlaceModification)
{
    UndoableObjects objects;
    auto item = make_item(1);
    TestStorable* ptr = item.get();
    objects.insert(std::move(item));
    objects.generate_snapshot();
    objects.save_for_undo(ptr);
    ptr->value = 5;

    std::vector<Storable*> cancelled;
    std::vector<Storable*> restored;
    EXPECT_TRUE(objects.undo(&cancelled, &restored));
    EXPECT_EQ(read_all(objects), (std::vector<int>{1}));

    cancelled.clear();
    restored.clear();
    EXPECT_TRUE(objects.redo(&cancelled, &restored));
    EXPECT_EQ(read_all(objects), (std::vector<int>{5}));
    ASSERT_EQ(restored.size(), 1u);
    EXPECT_EQ(value_of(restored, 0), 5);
    ASSERT_EQ(cancelled.size(), 1u);
    EXPECT_EQ(value_of(cancelled, 0), 1);
}

TEST(UndoableObjectsTest, RedoNotPossibleAfterNewSnapshot)
{
    UndoableObjects objects;
    objects.insert(make_item(1));
    objects.generate_snapshot();
    objects.insert(make_item(2));

    std::vector<Storable*> cancelled;
    std::vector<Storable*> restored;
    EXPECT_TRUE(objects.undo(&cancelled, &restored));

    // A new snapshot disables redo.
    objects.generate_snapshot();
    EXPECT_FALSE(objects.redo(&cancelled, &restored));
}

TEST(UndoableObjectsTest, UndoNotPossibleAtLevelZero)
{
    UndoableObjects objects;
    std::vector<Storable*> cancelled;
    std::vector<Storable*> restored;
    EXPECT_FALSE(objects.undo(&cancelled, &restored));
}

TEST(UndoableObjectsTest, DeletedOnCurrentLevelStaysDeletedAfterUndo)
{
    UndoableObjects objects;
    objects.insert(make_item(1));
    objects.generate_snapshot();
    // Item created and deleted on the current level.
    auto item = make_item(2);
    TestStorable* ptr = item.get();
    objects.insert(std::move(item));
    EXPECT_TRUE(objects.delete_object(ptr));
    EXPECT_EQ(read_all(objects), (std::vector<int>{1}));

    std::vector<Storable*> cancelled;
    std::vector<Storable*> restored;
    EXPECT_TRUE(objects.undo(&cancelled, &restored));
    // Not restored because it only existed inside the snapshot it was deleted in.
    EXPECT_EQ(read_all(objects), (std::vector<int>{1}));
}

TEST(UndoableObjectsTest, PopSnapshotDropsTopLevel)
{
    UndoableObjects objects;
    objects.insert(make_item(1));
    objects.generate_snapshot();
    objects.generate_snapshot();
    objects.insert(make_item(2));
    EXPECT_EQ(objects.stack_level(), 2);
    EXPECT_TRUE(objects.pop_snapshot());
    EXPECT_EQ(objects.stack_level(), 1);
    // The object created inside the popped level collapses into level 1
    // (mirrors Java), so it still shows up during read.
    EXPECT_EQ(read_all(objects), (std::vector<int>{1, 2}));
}

TEST(UndoableObjectsTest, PopSnapshotNotPossibleAtLevelZero)
{
    UndoableObjects objects;
    EXPECT_FALSE(objects.pop_snapshot());
}

TEST(UndoableObjectsTest, SaveForUndoOnlySnapshotsStoredObjects)
{
    UndoableObjects objects;
    auto item = make_item(1);
    objects.save_for_undo(item.get());
    // save_for_undo on a non-stored object must not insert anything.
    EXPECT_EQ(objects.object_count(), 0u);
}

TEST(UndoableObjectsTest, DeleteObjectCountsShrink)
{
    UndoableObjects objects;
    auto item = make_item(1);
    TestStorable* ptr = item.get();
    objects.insert(std::move(item));
    objects.generate_snapshot();
    EXPECT_EQ(objects.object_count(), 1u);
    EXPECT_TRUE(objects.delete_object(ptr));
    EXPECT_EQ(objects.object_count(), 0u);
}