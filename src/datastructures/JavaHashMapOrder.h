#pragma once

#include <cstdint>
#include <unordered_set>
#include <vector>

namespace freerouting::datastructures {

/** How the keys were inserted into the java.util.HashMap; the two differ in bucket order. */
enum class JavaHashMapInsert
{
    /** HashMap.put: appends to the bucket, resizes after the insert when size > threshold. */
    PUT,
    /** HashMap.computeIfAbsent: prepends to the bucket, resizes before the insert when size > threshold. */
    COMPUTE_IF_ABSENT
};

/**
 * Returns the iteration order (keySet / entrySet) of a java.util.HashMap<Integer, ?> after the
 * distinct keys p_keys_in_insertion_order were inserted with p_insert (repeated keys are ignored,
 * as an existing mapping does not move). Java iterates the table buckets in ascending index
 * (hash & (capacity - 1), hash = key ^ (key >>> 16)); a resize splits every bucket keeping the
 * relative order; a bucket reaching TREEIFY_THRESHOLD (8) entries while the capacity is below 64
 * forces a resize. Treeified buckets (8+ colliding keys at capacity >= 64) are not modelled; with
 * net numbers or item ids as keys that never happens. Verified against JDK 25 (see the test).
 */
inline std::vector<int> java_hashmap_int_iteration_order(const std::vector<int>& p_keys_in_insertion_order,
    JavaHashMapInsert p_insert)
{
    constexpr int kDefaultCapacity = 16;
    constexpr int kTreeifyThreshold = 8;
    constexpr int kMinTreeifyCapacity = 64;

    auto spread = [](int p_key) {
        const uint32_t h = static_cast<uint32_t>(p_key);
        return h ^ (h >> 16);
    };

    std::vector<std::vector<int>> table;
    int threshold = 0;
    int size = 0;

    auto resize = [&]() {
        const std::size_t new_capacity = table.empty() ? kDefaultCapacity : table.size() * 2;
        std::vector<std::vector<int>> new_table(new_capacity);
        for (const auto& bucket : table)
        {
            for (int key : bucket)
            {
                new_table[spread(key) & (new_capacity - 1)].push_back(key);
            }
        }
        table = std::move(new_table);
        threshold = static_cast<int>(new_capacity * 3 / 4);
    };

    std::unordered_set<int> present;
    for (int key : p_keys_in_insertion_order)
    {
        if (present.count(key) != 0)
        {
            continue;
        }
        present.insert(key);
        if (p_insert == JavaHashMapInsert::COMPUTE_IF_ABSENT && (table.empty() || size > threshold))
        {
            resize();
        }
        if (table.empty())
        {
            resize();
        }
        auto& bucket = table[spread(key) & (table.size() - 1)];
        const int bin_count = static_cast<int>(bucket.size());
        bool treeify = false;
        if (p_insert == JavaHashMapInsert::COMPUTE_IF_ABSENT)
        {
            bucket.insert(bucket.begin(), key);
            treeify = bin_count >= kTreeifyThreshold - 1;
        }
        else
        {
            bucket.push_back(key);
            treeify = bin_count >= kTreeifyThreshold;
        }
        if (treeify && static_cast<int>(table.size()) < kMinTreeifyCapacity)
        {
            resize();
        }
        ++size;
        if (p_insert == JavaHashMapInsert::PUT && size > threshold)
        {
            resize();
        }
    }

    std::vector<int> result;
    result.reserve(present.size());
    for (const auto& bucket : table)
    {
        result.insert(result.end(), bucket.begin(), bucket.end());
    }
    return result;
}

} // namespace freerouting::datastructures
