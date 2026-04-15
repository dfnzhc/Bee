/**
 * @File CacheTests.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/3/26
 * @Brief This file is part of Bee.
 */

#include <gtest/gtest.h>

#include <cstddef>
#include <string>
#include <vector>

#include "DataStructure/Cache.hpp"

using namespace bee;

// 使用类型别名避免 MSVC 预处理器误将模板逗号当作宏参数分隔符
using IntLRU = LRUCache<int, int>;
using IntMRU = MRUCache<int, int>;
using IntLFU = LFUCache<int, int>;
using IntMFU = MFUCache<int, int>;
using IntARC = ARCache<int, int>;

// =============================================================================
// LRUCache 测试
// =============================================================================

TEST(LRUCacheTests, ConstructWithZeroCapacityThrows)
{
    EXPECT_THROW(IntLRU(0), std::invalid_argument);
}

TEST(LRUCacheTests, BasicPutAndGet)
{
    LRUCache<int, int> cache(3);
    cache.put(1, 10);
    cache.put(2, 20);

    auto v1 = cache.get(1);
    ASSERT_TRUE(v1.has_value());
    EXPECT_EQ(v1->get(), 10);

    auto v2 = cache.get(2);
    ASSERT_TRUE(v2.has_value());
    EXPECT_EQ(v2->get(), 20);
}

TEST(LRUCacheTests, GetMissReturnsNullopt)
{
    LRUCache<int, int> cache(3);
    EXPECT_FALSE(cache.get(999).has_value());
}

TEST(LRUCacheTests, EvictsLeastRecentlyUsed)
{
    LRUCache<int, int> cache(3);
    cache.put(1, 10);
    cache.put(2, 20);
    cache.put(3, 30);

    // 缓存满，插入 4 应该淘汰 1（最久未使用）
    cache.put(4, 40);
    EXPECT_FALSE(cache.get(1).has_value());
    EXPECT_TRUE(cache.get(2).has_value());
    EXPECT_TRUE(cache.get(3).has_value());
    EXPECT_TRUE(cache.get(4).has_value());
}

TEST(LRUCacheTests, GetUpdatesRecency)
{
    LRUCache<int, int> cache(3);
    cache.put(1, 10);
    cache.put(2, 20);
    cache.put(3, 30);

    // 访问 1，使其变为最近使用
    cache.get(1);

    // 插入 4 应淘汰 2（而非 1）
    cache.put(4, 40);
    EXPECT_TRUE(cache.get(1).has_value());
    EXPECT_FALSE(cache.get(2).has_value());
}

TEST(LRUCacheTests, PutUpdatesExistingValue)
{
    LRUCache<int, int> cache(3);
    cache.put(1, 10);
    cache.put(1, 100);
    EXPECT_EQ(cache.size(), 1u);

    auto v = cache.get(1);
    ASSERT_TRUE(v.has_value());
    EXPECT_EQ(v->get(), 100);
}

TEST(LRUCacheTests, EraseKey)
{
    LRUCache<int, int> cache(3);
    cache.put(1, 10);
    cache.put(2, 20);

    EXPECT_TRUE(cache.erase(1));
    EXPECT_FALSE(cache.contains(1));
    EXPECT_EQ(cache.size(), 1u);
}

TEST(LRUCacheTests, EraseNonExisting)
{
    LRUCache<int, int> cache(3);
    EXPECT_FALSE(cache.erase(999));
}

TEST(LRUCacheTests, ContainsDoesNotAffectOrder)
{
    LRUCache<int, int> cache(2);
    cache.put(1, 10);
    cache.put(2, 20);

    // contains 不改变顺序
    EXPECT_TRUE(cache.contains(1));

    cache.put(3, 30);
    // 1 应被淘汰（contains 不算访问）
    EXPECT_FALSE(cache.get(1).has_value());
}

TEST(LRUCacheTests, SizeAndCapacity)
{
    LRUCache<int, int> cache(5);
    EXPECT_EQ(cache.capacity(), 5u);
    EXPECT_EQ(cache.size(), 0u);
    EXPECT_TRUE(cache.empty());

    cache.put(1, 10);
    EXPECT_EQ(cache.size(), 1u);
    EXPECT_FALSE(cache.empty());
}

TEST(LRUCacheTests, ClearRemovesAll)
{
    LRUCache<int, int> cache(3);
    cache.put(1, 10);
    cache.put(2, 20);
    cache.clear();
    EXPECT_TRUE(cache.empty());
    EXPECT_EQ(cache.size(), 0u);
    EXPECT_FALSE(cache.contains(1));
}

TEST(LRUCacheTests, StringKeysAndValues)
{
    LRUCache<std::string, std::string> cache(2);
    cache.put("hello", "world");
    cache.put("foo", "bar");

    auto v = cache.get("hello");
    ASSERT_TRUE(v.has_value());
    EXPECT_EQ(v->get(), "world");
}

TEST(LRUCacheTests, CapacityOne)
{
    LRUCache<int, int> cache(1);
    cache.put(1, 10);
    cache.put(2, 20);
    EXPECT_FALSE(cache.contains(1));
    EXPECT_TRUE(cache.contains(2));
}

TEST(LRUCacheTests, RepeatedEviction)
{
    LRUCache<int, int> cache(2);
    for (int i = 0; i < 100; ++i) {
        cache.put(i, i * 10);
    }
    // 最后两个应该在缓存中
    EXPECT_TRUE(cache.contains(98));
    EXPECT_TRUE(cache.contains(99));
    EXPECT_EQ(cache.size(), 2u);
}

// =============================================================================
// MRUCache 测试
// =============================================================================

TEST(MRUCacheTests, ConstructWithZeroCapacityThrows)
{
    EXPECT_THROW(IntMRU(0), std::invalid_argument);
}

TEST(MRUCacheTests, BasicPutAndGet)
{
    MRUCache<int, int> cache(3);
    cache.put(1, 10);
    cache.put(2, 20);

    auto v = cache.get(1);
    ASSERT_TRUE(v.has_value());
    EXPECT_EQ(v->get(), 10);
}

TEST(MRUCacheTests, EvictsMostRecentlyUsed)
{
    MRUCache<int, int> cache(3);
    cache.put(1, 10);
    cache.put(2, 20);
    cache.put(3, 30);

    // 缓存满，插入 4 应淘汰最近使用的（3，刚刚插入）
    cache.put(4, 40);
    EXPECT_TRUE(cache.contains(1));
    EXPECT_TRUE(cache.contains(2));
    EXPECT_FALSE(cache.contains(3));
    EXPECT_TRUE(cache.contains(4));
}

TEST(MRUCacheTests, GetUpdatesRecencyThenEvicts)
{
    MRUCache<int, int> cache(3);
    cache.put(1, 10);
    cache.put(2, 20);
    cache.put(3, 30);

    // 访问 1，使其变为最近使用
    cache.get(1);

    // 插入 4 应淘汰 1（最近使用过的）
    cache.put(4, 40);
    EXPECT_FALSE(cache.contains(1));
    EXPECT_TRUE(cache.contains(2));
    EXPECT_TRUE(cache.contains(3));
    EXPECT_TRUE(cache.contains(4));
}

TEST(MRUCacheTests, PutUpdatesExistingValue)
{
    MRUCache<int, int> cache(3);
    cache.put(1, 10);
    cache.put(1, 100);

    auto v = cache.get(1);
    ASSERT_TRUE(v.has_value());
    EXPECT_EQ(v->get(), 100);
    EXPECT_EQ(cache.size(), 1u);
}

TEST(MRUCacheTests, EraseKey)
{
    MRUCache<int, int> cache(3);
    cache.put(1, 10);
    EXPECT_TRUE(cache.erase(1));
    EXPECT_FALSE(cache.contains(1));
}

TEST(MRUCacheTests, EraseNonExisting)
{
    MRUCache<int, int> cache(3);
    EXPECT_FALSE(cache.erase(999));
}

TEST(MRUCacheTests, SizeCapacityClear)
{
    MRUCache<int, int> cache(5);
    cache.put(1, 10);
    cache.put(2, 20);
    EXPECT_EQ(cache.size(), 2u);
    EXPECT_EQ(cache.capacity(), 5u);

    cache.clear();
    EXPECT_TRUE(cache.empty());
}

TEST(MRUCacheTests, CapacityOne)
{
    MRUCache<int, int> cache(1);
    cache.put(1, 10);
    cache.put(2, 20);
    // MRU 淘汰最近使用的（即 1），保留新插入的 2
    EXPECT_FALSE(cache.contains(1));
    EXPECT_TRUE(cache.contains(2));
}

// =============================================================================
// LFUCache 测试
// =============================================================================

TEST(LFUCacheTests, ConstructWithZeroCapacityThrows)
{
    EXPECT_THROW(IntLFU(0), std::invalid_argument);
}

TEST(LFUCacheTests, BasicPutAndGet)
{
    LFUCache<int, int> cache(3);
    cache.put(1, 10);
    cache.put(2, 20);

    auto v = cache.get(1);
    ASSERT_TRUE(v.has_value());
    EXPECT_EQ(v->get(), 10);
}

TEST(LFUCacheTests, EvictsLeastFrequentlyUsed)
{
    LFUCache<int, int> cache(3);
    cache.put(1, 10);
    cache.put(2, 20);
    cache.put(3, 30);

    cache.get(1);
    cache.get(1);
    cache.get(2);

    cache.put(4, 40);
    EXPECT_FALSE(cache.contains(3));
    EXPECT_TRUE(cache.contains(1));
    EXPECT_TRUE(cache.contains(2));
    EXPECT_TRUE(cache.contains(4));
}

TEST(LFUCacheTests, SameFrequencyEvictsLeastRecentlyUsed)
{
    LFUCache<int, int> cache(2);
    cache.put(1, 10);
    cache.put(2, 20);

    cache.put(3, 30);
    EXPECT_FALSE(cache.contains(1));
    EXPECT_TRUE(cache.contains(2));
    EXPECT_TRUE(cache.contains(3));
}

TEST(LFUCacheTests, PutUpdatesExistingValue)
{
    LFUCache<int, int> cache(2);
    cache.put(1, 10);
    cache.put(1, 100);

    auto v = cache.get(1);
    ASSERT_TRUE(v.has_value());
    EXPECT_EQ(v->get(), 100);
    EXPECT_EQ(cache.size(), 1u);
}

TEST(LFUCacheTests, EraseAndClear)
{
    LFUCache<int, int> cache(3);
    cache.put(1, 10);
    cache.put(2, 20);

    EXPECT_TRUE(cache.erase(1));
    EXPECT_FALSE(cache.contains(1));
    EXPECT_FALSE(cache.erase(42));

    cache.clear();
    EXPECT_TRUE(cache.empty());
}

TEST(LFUCacheTests, CapacityOne)
{
    LFUCache<int, int> cache(1);
    cache.put(1, 10);
    cache.get(1);
    cache.put(2, 20);

    EXPECT_FALSE(cache.contains(1));
    EXPECT_TRUE(cache.contains(2));
}

TEST(LFUCacheTests, EvictionIsCorrectAfterFrequencyBuildUp)
{
    // Verify that after heavy frequency build-up, eviction still picks
    // the truly least-frequent item, and _minFreq is correctly maintained.
    IntLFU cache(3);

    cache.put(1, 10); // freq 1
    cache.put(2, 20); // freq 1
    cache.put(3, 30); // freq 1

    // Build up frequencies: key 1 gets freq 4, key 2 gets freq 3, key 3 stays freq 1
    cache.get(1);
    cache.get(1);
    cache.get(1); // key 1: freq 4
    cache.get(2);
    cache.get(2); // key 2: freq 3

    // Insert key 4 — should evict key 3 (freq 1, the minimum)
    cache.put(4, 40);
    EXPECT_FALSE(cache.get(3).has_value()) << "key 3 (lowest freq) should have been evicted";
    EXPECT_TRUE(cache.get(1).has_value());
    EXPECT_TRUE(cache.get(2).has_value());
    EXPECT_TRUE(cache.get(4).has_value());

    // Insert key 5 — should evict key 4 (freq 1, just inserted)
    cache.put(5, 50);
    EXPECT_FALSE(cache.get(4).has_value()) << "key 4 (freq 1) should have been evicted";

    // Keys 1, 2, 5 should remain
    EXPECT_TRUE(cache.get(1).has_value());
    EXPECT_TRUE(cache.get(2).has_value());
    EXPECT_TRUE(cache.get(5).has_value());
}

TEST(LFUCacheTests, EraseMinFreqItemRecomputesCorrectly)
{
    IntLFU cache(4);
    cache.put(1, 10); // freq 1
    cache.put(2, 20); // freq 1
    cache.put(3, 30); // freq 1
    cache.put(4, 40); // freq 1

    // Boost keys 2,3,4 so key 1 remains sole freq=1
    cache.get(2); // freq 2
    cache.get(3); // freq 2
    cache.get(4); // freq 2

    // Erase key 1 (the only freq=1 entry). _minFreq should recompute to 2.
    EXPECT_TRUE(cache.erase(1));
    EXPECT_EQ(cache.size(), 3u);

    // Now insert a new key — it gets freq=1, becomes the new min
    cache.put(5, 50);
    // Cache now has 2(f2), 3(f2), 4(f2), 5(f1) — at capacity
    // Inserting 6 should evict 5
    cache.put(6, 60);
    EXPECT_FALSE(cache.get(5).has_value()) << "key 5 (freq 1) should be evicted";
    EXPECT_TRUE(cache.get(6).has_value());
}

// =============================================================================
// MFUCache 测试
// =============================================================================

TEST(MFUCacheTests, ConstructWithZeroCapacityThrows)
{
    EXPECT_THROW(IntMFU(0), std::invalid_argument);
}

TEST(MFUCacheTests, BasicPutAndGet)
{
    MFUCache<int, int> cache(3);
    cache.put(1, 10);
    cache.put(2, 20);

    auto v = cache.get(1);
    ASSERT_TRUE(v.has_value());
    EXPECT_EQ(v->get(), 10);
}

TEST(MFUCacheTests, EvictsMostFrequentlyUsed)
{
    MFUCache<int, int> cache(3);
    cache.put(1, 10);
    cache.put(2, 20);
    cache.put(3, 30);

    // 多次访问 1，使其频率最高
    cache.get(1);
    cache.get(1);
    cache.get(1);

    // 访问 2 一次
    cache.get(2);

    // 插入 4 应淘汰频率最高的 1
    cache.put(4, 40);
    EXPECT_FALSE(cache.contains(1));
    EXPECT_TRUE(cache.contains(2));
    EXPECT_TRUE(cache.contains(3));
    EXPECT_TRUE(cache.contains(4));
}

TEST(MFUCacheTests, SameFrequencyEvicts)
{
    MFUCache<int, int> cache(2);
    cache.put(1, 10);
    cache.put(2, 20);

    // 两者频率都是 1（仅 put），插入 3 应淘汰其中一个
    cache.put(3, 30);
    EXPECT_EQ(cache.size(), 2u);
    EXPECT_TRUE(cache.contains(3));
}

TEST(MFUCacheTests, PutUpdatesExistingValue)
{
    MFUCache<int, int> cache(3);
    cache.put(1, 10);
    cache.put(1, 100);

    auto v = cache.get(1);
    ASSERT_TRUE(v.has_value());
    EXPECT_EQ(v->get(), 100);
}

TEST(MFUCacheTests, GetIncreasesFrequency)
{
    MFUCache<int, int> cache(3);
    cache.put(1, 10);
    cache.put(2, 20);
    cache.put(3, 30);

    // 频繁访问 2 和 3
    for (int i = 0; i < 5; ++i) {
        cache.get(2);
        cache.get(3);
    }

    // 访问 1 一次
    cache.get(1);

    // 插入 4 应淘汰频率最高的 2 或 3
    cache.put(4, 40);
    EXPECT_TRUE(cache.contains(1));
    EXPECT_TRUE(cache.contains(4));
    // 2 和 3 中有一个被淘汰
    int existing = (cache.contains(2) ? 1 : 0) + (cache.contains(3) ? 1 : 0);
    EXPECT_EQ(existing, 1);
}

TEST(MFUCacheTests, EraseKey)
{
    MFUCache<int, int> cache(3);
    cache.put(1, 10);
    EXPECT_TRUE(cache.erase(1));
    EXPECT_FALSE(cache.contains(1));
    EXPECT_TRUE(cache.empty());
}

TEST(MFUCacheTests, EraseNonExisting)
{
    MFUCache<int, int> cache(3);
    EXPECT_FALSE(cache.erase(999));
}

TEST(MFUCacheTests, ClearAll)
{
    MFUCache<int, int> cache(3);
    cache.put(1, 10);
    cache.put(2, 20);
    cache.clear();
    EXPECT_TRUE(cache.empty());
}

TEST(MFUCacheTests, CapacityOne)
{
    MFUCache<int, int> cache(1);
    cache.put(1, 10);
    cache.get(1); // freq=2
    cache.put(2, 20);
    // 1 的频率更高，应被淘汰
    EXPECT_FALSE(cache.contains(1));
    EXPECT_TRUE(cache.contains(2));
}

TEST(MFUCacheTests, EvictsHighestFrequencyItem)
{
    IntMFU cache(3);
    cache.put(1, 10);
    cache.put(2, 20);
    cache.put(3, 30);

    // key 1: freq 4, key 2: freq 2, key 3: freq 1
    cache.get(1);
    cache.get(1);
    cache.get(1);
    cache.get(2);

    // Insert 4 — should evict key 1 (highest freq = 4)
    cache.put(4, 40);
    EXPECT_FALSE(cache.get(1).has_value()) << "key 1 (highest freq) should be evicted";
    EXPECT_TRUE(cache.get(2).has_value());
    EXPECT_TRUE(cache.get(3).has_value());
    EXPECT_TRUE(cache.get(4).has_value());
}

TEST(MFUCacheTests, EraseMaxFreqRecomputesCorrectly)
{
    IntMFU cache(4);
    cache.put(1, 10);
    cache.put(2, 20);
    cache.put(3, 30);
    cache.put(4, 40);

    // key 1: freq 5 (max), key 2: freq 3, key 3: freq 2, key 4: freq 1
    cache.get(1);
    cache.get(1);
    cache.get(1);
    cache.get(1);
    cache.get(2);
    cache.get(2);
    cache.get(3);

    // Erase key 1 (sole freq=5 entry). _maxFreq should recompute to 3.
    EXPECT_TRUE(cache.erase(1));

    // Insert 5 — cache has 2(f3), 3(f2), 4(f1), 5(f1)
    // Should evict key 2 (highest freq = 3)
    cache.put(5, 50);
    cache.put(6, 60);
    EXPECT_FALSE(cache.get(2).has_value()) << "key 2 (now highest freq=3) should be evicted";
}

// =============================================================================
// ARCache 测试
// =============================================================================

TEST(ARCacheTests, ConstructWithZeroCapacityThrows)
{
    EXPECT_THROW(IntARC(0), std::invalid_argument);
}

TEST(ARCacheTests, BasicPutAndGet)
{
    ARCache<int, int> cache(5);
    cache.put(1, 10);
    cache.put(2, 20);

    auto v1 = cache.get(1);
    ASSERT_TRUE(v1.has_value());
    EXPECT_EQ(v1->get(), 10);

    auto v2 = cache.get(2);
    ASSERT_TRUE(v2.has_value());
    EXPECT_EQ(v2->get(), 20);
}

TEST(ARCacheTests, GetMissReturnsNullopt)
{
    ARCache<int, int> cache(5);
    EXPECT_FALSE(cache.get(999).has_value());
}

TEST(ARCacheTests, PutUpdatesExisting)
{
    ARCache<int, int> cache(5);
    cache.put(1, 10);
    cache.put(1, 100);

    auto v = cache.get(1);
    ASSERT_TRUE(v.has_value());
    EXPECT_EQ(v->get(), 100);
}

TEST(ARCacheTests, EvictionHappensAtCapacity)
{
    ARCache<int, int> cache(3);
    cache.put(1, 10);
    cache.put(2, 20);
    cache.put(3, 30);

    // 缓存满
    EXPECT_EQ(cache.size(), 3u);

    // 插入 4 应触发淘汰
    cache.put(4, 40);
    EXPECT_LE(cache.size(), 3u);
    EXPECT_TRUE(cache.contains(4));
}

TEST(ARCacheTests, T1ToT2Promotion)
{
    ARCache<int, int> cache(5);
    cache.put(1, 10); // 进入 T1

    // 访问 1，从 T1 移到 T2
    auto v = cache.get(1);
    ASSERT_TRUE(v.has_value());
    EXPECT_EQ(v->get(), 10);

    // T2 应有 1 个条目
    EXPECT_EQ(cache.t2Size(), 1u);
    EXPECT_EQ(cache.t1Size(), 0u);
}

TEST(ARCacheTests, GhostHitAdaptsP)
{
    ARCache<int, int> cache(2);
    cache.put(1, 10); // T1: [1]
    cache.put(2, 20); // T1: [2, 1]

    // 填满缓存后插入 3，淘汰 1 到 B1
    cache.put(3, 30);

    // 再次插入 1，B1 中命中，应增大 p
    size_t pBefore = cache.targetT1Size();
    cache.put(1, 100);
    // p 应该增大了（给 T1 更多空间）
    EXPECT_GE(cache.targetT1Size(), pBefore);
    EXPECT_TRUE(cache.contains(1));
}

TEST(ARCacheTests, SizeAndCapacity)
{
    ARCache<int, int> cache(5);
    EXPECT_EQ(cache.capacity(), 5u);
    EXPECT_EQ(cache.size(), 0u);
    EXPECT_TRUE(cache.empty());

    cache.put(1, 10);
    EXPECT_EQ(cache.size(), 1u);
    EXPECT_FALSE(cache.empty());
}

TEST(ARCacheTests, ContainsOnlyCounted)
{
    ARCache<int, int> cache(2);
    cache.put(1, 10);
    cache.put(2, 20);
    cache.put(3, 30); // 淘汰某个条目

    // contains 只检查 T1+T2，不检查幽灵列表
    int cachedCount = 0;
    for (int i = 1; i <= 3; ++i) {
        if (cache.contains(i))
            ++cachedCount;
    }
    EXPECT_LE(cachedCount, 2);
}

TEST(ARCacheTests, EraseFromT1)
{
    ARCache<int, int> cache(5);
    cache.put(1, 10);
    EXPECT_TRUE(cache.erase(1));
    EXPECT_FALSE(cache.contains(1));
    EXPECT_EQ(cache.size(), 0u);
}

TEST(ARCacheTests, EraseFromT2)
{
    ARCache<int, int> cache(5);
    cache.put(1, 10);
    cache.get(1); // 移到 T2
    EXPECT_TRUE(cache.erase(1));
    EXPECT_FALSE(cache.contains(1));
}

TEST(ARCacheTests, EraseNonExisting)
{
    ARCache<int, int> cache(5);
    EXPECT_FALSE(cache.erase(999));
}

TEST(ARCacheTests, ClearAll)
{
    ARCache<int, int> cache(5);
    cache.put(1, 10);
    cache.put(2, 20);
    cache.get(1);
    cache.clear();
    EXPECT_TRUE(cache.empty());
    EXPECT_EQ(cache.targetT1Size(), 0u);
}

TEST(ARCacheTests, AdaptiveBalancingWorkload)
{
    // 模拟一个工作负载来测试 ARC 的自适应性
    ARCache<int, int> cache(10);

    // 第一阶段：大量顺序访问（偏好 T1）
    for (int i = 0; i < 20; ++i) {
        cache.put(i, i);
    }

    // 第二阶段：重复访问一小部分数据（偏好 T2）
    for (int round = 0; round < 10; ++round) {
        for (int i = 10; i < 15; ++i) {
            cache.get(i);
        }
    }

    // 验证缓存仍然正常工作
    EXPECT_LE(cache.size(), 10u);
}

TEST(ARCacheTests, CapacityOne)
{
    ARCache<int, int> cache(1);
    cache.put(1, 10);
    EXPECT_TRUE(cache.contains(1));

    cache.put(2, 20);
    EXPECT_EQ(cache.size(), 1u);
    EXPECT_TRUE(cache.contains(2));
}

TEST(ARCacheTests, ManyInsertions)
{
    ARCache<int, int> cache(100);

    for (int i = 0; i < 10000; ++i) {
        cache.put(i, i * 2);
    }

    EXPECT_LE(cache.size(), 100u);

    // 最近插入的一些应该在缓存中
    int hits = 0;
    for (int i = 9900; i < 10000; ++i) {
        if (cache.contains(i))
            ++hits;
    }
    EXPECT_GT(hits, 0);
}

TEST(ARCacheTests, StringKeysAndValues)
{
    ARCache<std::string, std::string> cache(3);
    cache.put("a", "alpha");
    cache.put("b", "beta");
    cache.put("c", "charlie");

    auto v = cache.get("b");
    ASSERT_TRUE(v.has_value());
    EXPECT_EQ(v->get(), "beta");
}

TEST(ARCacheTests, T1T2SizesConsistent)
{
    ARCache<int, int> cache(5);
    for (int i = 0; i < 5; ++i) {
        cache.put(i, i);
    }
    // T1 + T2 不超过容量
    EXPECT_LE(cache.t1Size() + cache.t2Size(), 5u);
    EXPECT_EQ(cache.size(), cache.t1Size() + cache.t2Size());
}

TEST(ARCacheTests, PutOnT2Hit)
{
    ARCache<int, int> cache(5);
    cache.put(1, 10);
    cache.get(1);      // 从 T1 移到 T2
    cache.put(1, 100); // 已在 T2 中，更新值

    auto v = cache.get(1);
    ASSERT_TRUE(v.has_value());
    EXPECT_EQ(v->get(), 100);
    EXPECT_EQ(cache.size(), 1u);
}

// =============================================================================
// LFU Eviction Correctness
// =============================================================================

TEST(LFUCacheTests, EvictsCorrectItemWithMixedFrequencies)
{
    IntLFU cache(5);
    for (int i = 1; i <= 5; ++i)
        cache.put(i, i * 10);

    // Build distinct frequencies: 1→f5, 2→f3, 3→f1 (no extra gets), 4→f2, 5→f4
    for (int j = 0; j < 4; ++j)
        cache.get(1); // freq 1→5
    for (int j = 0; j < 2; ++j)
        cache.get(2); // freq 2→3
    // key 3 stays at freq 1
    for (int j = 0; j < 1; ++j)
        cache.get(4); // freq 4→2
    for (int j = 0; j < 3; ++j)
        cache.get(5); // freq 5→4

    cache.put(6, 60); // evicts key 3 (lowest freq=1)
    EXPECT_FALSE(cache.contains(3));
    EXPECT_TRUE(cache.contains(1));
    EXPECT_TRUE(cache.contains(2));
    EXPECT_TRUE(cache.contains(4));
    EXPECT_TRUE(cache.contains(5));
    EXPECT_TRUE(cache.contains(6));
    EXPECT_EQ(cache.size(), 5u);
}

TEST(LFUCacheTests, FrequencyGapRecomputesMinCorrectly)
{
    IntLFU cache(3);
    cache.put(1, 10);
    cache.put(2, 20);
    cache.put(3, 30);

    // Build frequencies: 1→f10, 2→f5, 3→f1
    for (int j = 0; j < 9; ++j)
        cache.get(1);
    for (int j = 0; j < 4; ++j)
        cache.get(2);

    // Erase key 3 (the only item at freq 1); minFreq should jump to 5
    cache.erase(3);
    EXPECT_EQ(cache.size(), 2u);

    // Insert D: freq=1, minFreq reset to 1
    cache.put(4, 40);
    EXPECT_EQ(cache.size(), 3u);

    // Insert E: should evict key 4 (freq=1, the lowest)
    cache.put(5, 50);
    EXPECT_FALSE(cache.contains(4));
    EXPECT_TRUE(cache.contains(1));
    EXPECT_TRUE(cache.contains(2));
    EXPECT_TRUE(cache.contains(5));
}

TEST(LFUCacheTests, LargeScaleEvictionOrder)
{
    IntLFU cache(100);

    // Insert first 100 keys
    for (int i = 0; i < 100; ++i)
        cache.put(i, i);

    // Give them high frequency (freq → 11 each)
    for (int i = 0; i < 100; ++i)
        for (int j = 0; j < 10; ++j)
            cache.get(i);

    // Insert 100 more keys; low-freq newcomers keep evicting each other
    for (int i = 100; i < 200; ++i)
        cache.put(i, i);

    EXPECT_EQ(cache.size(), 100u);

    int highFreqSurvivors = 0;
    for (int i = 0; i < 100; ++i)
        if (cache.contains(i))
            ++highFreqSurvivors;

    // All but the first evicted high-freq key should survive
    EXPECT_GE(highFreqSurvivors, 90);
}

// =============================================================================
// MFU Eviction Correctness
// =============================================================================

TEST(MFUCacheTests, EvictsCorrectItemWithDistinctFrequencies)
{
    IntMFU cache(5);
    for (int i = 1; i <= 5; ++i)
        cache.put(i, i * 10);

    // Build frequencies: 1→f1, 2→f3, 3→f5, 4→f2, 5→f4
    // (no extra gets for key 1)
    for (int j = 0; j < 2; ++j)
        cache.get(2); // freq 2→3
    for (int j = 0; j < 4; ++j)
        cache.get(3); // freq 3→5
    for (int j = 0; j < 1; ++j)
        cache.get(4); // freq 4→2
    for (int j = 0; j < 3; ++j)
        cache.get(5); // freq 5→4

    cache.put(6, 60); // MFU evicts key 3 (highest freq=5)
    EXPECT_FALSE(cache.contains(3));
    EXPECT_TRUE(cache.contains(1));
    EXPECT_TRUE(cache.contains(2));
    EXPECT_TRUE(cache.contains(4));
    EXPECT_TRUE(cache.contains(5));
    EXPECT_TRUE(cache.contains(6));
    EXPECT_EQ(cache.size(), 5u);
}

TEST(MFUCacheTests, FrequencyGapRecomputesMaxCorrectly)
{
    IntMFU cache(3);
    cache.put(1, 10);
    cache.put(2, 20);
    cache.put(3, 30);

    // Build frequencies: 1→f1, 2→f5, 3→f10
    for (int j = 0; j < 4; ++j)
        cache.get(2);
    for (int j = 0; j < 9; ++j)
        cache.get(3);

    // Erase key 3 (freq 10); maxFreq should drop to 5
    cache.erase(3);
    EXPECT_EQ(cache.size(), 2u);

    // Insert D (freq=1)
    cache.put(4, 40);
    EXPECT_EQ(cache.size(), 3u);

    // Insert E: should evict key 2 (maxFreq=5)
    cache.put(5, 50);
    EXPECT_FALSE(cache.contains(2));
    EXPECT_TRUE(cache.contains(1));
    EXPECT_TRUE(cache.contains(4));
    EXPECT_TRUE(cache.contains(5));
}

// =============================================================================
// ARC Adaptation Tests
// =============================================================================

TEST(ARCacheTests, B1GhostHitIncreasesP)
{
    IntARC cache(5);
    // Fill T1
    for (int i = 1; i <= 5; ++i)
        cache.put(i, i);

    // Promote keys 1,2 to T2 via get → T1=[5,4,3], T2=[2,1]
    cache.get(1);
    cache.get(2);

    // Insert 6,7 to push T1 items (3, then 4) into B1 via _replace
    cache.put(6, 6);
    cache.put(7, 7);

    // Key 4 should be in B1; ghost hit it to increase p
    auto pBefore = cache.targetT1Size();
    cache.put(4, 40);
    EXPECT_GT(cache.targetT1Size(), pBefore);
}

TEST(ARCacheTests, B2GhostHitDecreasesP)
{
    IntARC cache(4);
    // Fill T1 and promote 1,2 to T2
    cache.put(1, 1);
    cache.put(2, 2);
    cache.put(3, 3);
    cache.put(4, 4);
    cache.get(1);
    cache.get(2); // T1=[4,3], T2=[2,1]

    // Insert new items to push T1 entries to B1
    cache.put(5, 5); // evicts T1 back(3)→B1
    cache.put(6, 6); // evicts T1 back(4)→B1

    // B1 ghost hit to raise p above 0
    cache.put(4, 40);
    ASSERT_GT(cache.targetT1Size(), 0u);

    // Insert to push a T2 entry to B2
    cache.put(7, 7);

    // B2 ghost hit decreases p
    auto pBefore = cache.targetT1Size();
    cache.put(1, 10);
    EXPECT_LT(cache.targetT1Size(), pBefore);
}

TEST(ARCacheTests, PNeverExceedsCapacity)
{
    constexpr std::size_t kCap = 5;
    IntARC                cache(kCap);

    for (int round = 0; round < 50; ++round) {
        int base = round * 20;
        for (int i = 0; i < 5; ++i)
            cache.put(base + i, i);

        // Promote two items to T2
        cache.get(base);
        cache.get(base + 1);

        // Insert new keys to force evictions into B1
        for (int i = 5; i < 10; ++i)
            cache.put(base + i, i);

        // Trigger ghost hits on evicted keys
        for (int i = 0; i < 5; ++i) {
            cache.put(base + i, i * 10);
            EXPECT_LE(cache.targetT1Size(), kCap);
        }
    }
}

TEST(ARCacheTests, PNeverGoesBelowZero)
{
    constexpr std::size_t kCap = 5;
    IntARC                cache(kCap);

    for (int round = 0; round < 50; ++round) {
        int base = round * 20;

        // Fill with items and promote all to T2
        for (int i = 0; i < 5; ++i)
            cache.put(base + i, i);
        for (int i = 0; i < 5; ++i)
            cache.get(base + i);

        // Insert new keys; with T1 small, _replace evicts from T2 → B2
        for (int i = 5; i < 10; ++i) {
            cache.put(base + i, i);
            // p is size_type (unsigned); if underflow occurred it would be huge
            EXPECT_LE(cache.targetT1Size(), kCap);
        }

        // Re-insert evicted T2 keys as B2 ghost hits to push p down
        for (int i = 0; i < 5; ++i) {
            cache.put(base + i, i * 10);
            EXPECT_LE(cache.targetT1Size(), kCap);
        }
    }
}

TEST(ARCacheTests, GhostListsBounded)
{
    constexpr std::size_t kCap = 10;
    IntARC                cache(kCap);

    for (int i = 0; i < 1000; ++i) {
        cache.put(i, i);
        // Some gets to exercise T2 promotion and B2 ghost generation
        if (i > 0 && (i % 3 == 0))
            cache.get(i - 1);

        EXPECT_LE(cache.t1Size() + cache.t2Size(), kCap);
        EXPECT_LE(cache.b1Size() + cache.t1Size(), kCap);
        EXPECT_LE(cache.b1Size() + cache.b2Size() + cache.t1Size() + cache.t2Size(), 2 * kCap);
    }
}

// =============================================================================
// Cache Stress Tests
// =============================================================================

TEST(LRUCacheTests, StressEvictionOrderWithLargeSequence)
{
    IntLRU cache(50);

    for (int i = 0; i < 1000; ++i) {
        cache.put(i, i);
        EXPECT_LE(cache.size(), 50u);
    }

    // Only last 50 keys (950–999) should remain
    EXPECT_EQ(cache.size(), 50u);
    for (int i = 950; i < 1000; ++i)
        EXPECT_TRUE(cache.contains(i));
    for (int i = 0; i < 950; ++i)
        EXPECT_FALSE(cache.contains(i));
}

TEST(MRUCacheTests, StressMRUEvictionPattern)
{
    IntMRU cache(50);

    // Insert 100 keys (0–99); MRU evicts most recently used
    for (int i = 0; i < 100; ++i)
        cache.put(i, i);

    // Repeatedly: access key 0 (making it MRU), then insert a new key.
    // The new key becomes the most recently inserted but key 0 was
    // just accessed so it is the MRU item among existing entries.
    // MRU should evict the most-recently-used item.
    int evictions = 0;
    for (int i = 100; i < 200; ++i) {
        cache.get(0);    // make key 0 most recently used
        cache.put(i, i); // insert triggers eviction of MRU → key 0
        if (!cache.contains(0))
            ++evictions;
        // re-insert key 0 if evicted, to repeat the pattern
        if (!cache.contains(0))
            cache.put(0, 0);
    }

    EXPECT_GT(evictions, 0);
    EXPECT_LE(cache.size(), 50u);
}
