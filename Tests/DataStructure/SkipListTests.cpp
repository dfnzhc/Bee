/**
 * @File SkipListTests.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/3/26
 * @Brief This file is part of Bee.
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "DataStructure/SkipList.hpp"

using namespace bee;

struct AlwaysThrowOnCopy
{
    AlwaysThrowOnCopy() = default;

    AlwaysThrowOnCopy(const AlwaysThrowOnCopy&)
    {
        throw std::runtime_error("copy failure");
    }

    AlwaysThrowOnCopy(AlwaysThrowOnCopy&&) noexcept            = default;
    AlwaysThrowOnCopy& operator=(const AlwaysThrowOnCopy&)     = default;
    AlwaysThrowOnCopy& operator=(AlwaysThrowOnCopy&&) noexcept = default;
};

// =============================================================================
// 构造测试
// =============================================================================

TEST(SkipListTests, DefaultConstruct)
{
    SkipList<int, int> sl;
    EXPECT_TRUE(sl.empty());
    EXPECT_EQ(sl.size(), 0u);
}

TEST(SkipListTests, InitializerListConstruct)
{
    SkipList<int, std::string> sl{{1, "one"}, {2, "two"}, {3, "three"}};
    EXPECT_EQ(sl.size(), 3u);
    EXPECT_EQ(sl.at(1), "one");
    EXPECT_EQ(sl.at(2), "two");
    EXPECT_EQ(sl.at(3), "three");
}

TEST(SkipListTests, MoveConstruct)
{
    SkipList<int, int> sl;
    sl.insert(1, 10);
    sl.insert(2, 20);

    SkipList<int, int> moved(std::move(sl));
    EXPECT_EQ(moved.size(), 2u);
    EXPECT_EQ(moved.at(1), 10);
    EXPECT_EQ(moved.at(2), 20);
    EXPECT_TRUE(sl.empty()); // NOLINT: 测试移动后状态
}

TEST(SkipListTests, MoveAssign)
{
    SkipList<int, int> sl;
    sl.insert(1, 10);

    SkipList<int, int> other;
    other.insert(5, 50);

    other = std::move(sl);
    EXPECT_EQ(other.size(), 1u);
    EXPECT_EQ(other.at(1), 10);
}

// =============================================================================
// 插入测试
// =============================================================================

TEST(SkipListTests, InsertSingleElement)
{
    SkipList<int, int> sl;
    auto [it, inserted] = sl.insert(1, 100);
    EXPECT_TRUE(inserted);
    EXPECT_EQ(sl.size(), 1u);

    auto [k, v] = *it;
    EXPECT_EQ(k, 1);
    EXPECT_EQ(v, 100);
}

TEST(SkipListTests, InsertDuplicateKeyUpdatesValue)
{
    SkipList<int, int> sl;
    sl.insert(1, 100);
    auto [it, inserted] = sl.insert(1, 200);

    EXPECT_FALSE(inserted);
    EXPECT_EQ(sl.size(), 1u);
    EXPECT_EQ(sl.at(1), 200);
}

TEST(SkipListTests, InsertMultipleElements)
{
    SkipList<int, int> sl;
    for (int i = 0; i < 100; ++i) {
        sl.insert(i, i * 10);
    }
    EXPECT_EQ(sl.size(), 100u);

    for (int i = 0; i < 100; ++i) {
        EXPECT_EQ(sl.at(i), i * 10);
    }
}

TEST(SkipListTests, InsertReverseOrder)
{
    SkipList<int, int> sl;
    for (int i = 99; i >= 0; --i) {
        sl.insert(i, i);
    }
    EXPECT_EQ(sl.size(), 100u);

    // 验证迭代器顺序
    int expected = 0;
    for (auto [k, v] : sl) {
        EXPECT_EQ(k, expected);
        ++expected;
    }
}

TEST(SkipListTests, InsertWithMoveValue)
{
    SkipList<int, std::string> sl;
    std::string                val = "hello";
    sl.insert(1, std::move(val));
    EXPECT_EQ(sl.at(1), "hello");
}

TEST(SkipListTests, EmplaceConstruct)
{
    SkipList<int, std::string> sl;
    auto [it, ok] = sl.emplace(1, "hello");
    EXPECT_TRUE(ok);
    EXPECT_EQ(sl.at(1), "hello");
}

// =============================================================================
// 删除测试
// =============================================================================

TEST(SkipListTests, EraseExistingKey)
{
    SkipList<int, int> sl;
    sl.insert(1, 10);
    sl.insert(2, 20);
    sl.insert(3, 30);

    EXPECT_TRUE(sl.erase(2));
    EXPECT_EQ(sl.size(), 2u);
    EXPECT_FALSE(sl.contains(2));
    EXPECT_TRUE(sl.contains(1));
    EXPECT_TRUE(sl.contains(3));
}

TEST(SkipListTests, EraseNonExistingKey)
{
    SkipList<int, int> sl;
    sl.insert(1, 10);
    EXPECT_FALSE(sl.erase(999));
    EXPECT_EQ(sl.size(), 1u);
}

TEST(SkipListTests, EraseAllElements)
{
    SkipList<int, int> sl;
    for (int i = 0; i < 50; ++i) {
        sl.insert(i, i);
    }

    for (int i = 0; i < 50; ++i) {
        EXPECT_TRUE(sl.erase(i));
    }

    EXPECT_TRUE(sl.empty());
    EXPECT_EQ(sl.size(), 0u);
}

TEST(SkipListTests, EraseFromEmpty)
{
    SkipList<int, int> sl;
    EXPECT_FALSE(sl.erase(1));
}

TEST(SkipListTests, EraseByIterator)
{
    SkipList<int, int> sl;
    sl.insert(1, 10);
    sl.insert(2, 20);
    sl.insert(3, 30);

    auto it   = sl.find(2);
    auto next = sl.erase(it);
    EXPECT_EQ(sl.size(), 2u);
    EXPECT_FALSE(sl.contains(2));

    // next 应指向 3
    if (next != sl.end()) {
        auto [k, v] = *next;
        EXPECT_EQ(k, 3);
    }
}

TEST(SkipListTests, EraseFirstElement)
{
    SkipList<int, int> sl;
    sl.insert(1, 10);
    sl.insert(2, 20);
    sl.insert(3, 30);

    EXPECT_TRUE(sl.erase(1));
    EXPECT_EQ(sl.size(), 2u);
    auto [k, v] = *sl.begin();
    EXPECT_EQ(k, 2);
}

TEST(SkipListTests, EraseLastElement)
{
    SkipList<int, int> sl;
    sl.insert(1, 10);
    sl.insert(2, 20);
    sl.insert(3, 30);

    EXPECT_TRUE(sl.erase(3));
    EXPECT_EQ(sl.size(), 2u);
}

// =============================================================================
// 查找测试
// =============================================================================

TEST(SkipListTests, FindExistingElement)
{
    SkipList<int, int> sl;
    sl.insert(5, 50);
    auto it = sl.find(5);
    EXPECT_NE(it, sl.end());
    auto [k, v] = *it;
    EXPECT_EQ(k, 5);
    EXPECT_EQ(v, 50);
}

TEST(SkipListTests, FindNonExisting)
{
    SkipList<int, int> sl;
    sl.insert(5, 50);
    EXPECT_EQ(sl.find(10), sl.end());
}

TEST(SkipListTests, ContainsKey)
{
    SkipList<int, int> sl;
    sl.insert(1, 10);
    EXPECT_TRUE(sl.contains(1));
    EXPECT_FALSE(sl.contains(2));
}

TEST(SkipListTests, AtExistingKey)
{
    SkipList<int, std::string> sl;
    sl.insert(1, "hello");
    EXPECT_EQ(sl.at(1), "hello");
}

TEST(SkipListTests, AtNonExistingKeyThrows)
{
    SkipList<int, int> sl;
    EXPECT_THROW(sl.at(1), std::out_of_range);
}

TEST(SkipListTests, SubscriptOperatorCreatesDefault)
{
    SkipList<int, int> sl;
    sl[5] = 50;
    EXPECT_EQ(sl.size(), 1u);
    EXPECT_EQ(sl.at(5), 50);
}

TEST(SkipListTests, SubscriptOperatorExisting)
{
    SkipList<int, int> sl;
    sl.insert(1, 10);
    sl[1] = 20;
    EXPECT_EQ(sl.at(1), 20);
}

// =============================================================================
// 迭代器测试
// =============================================================================

TEST(SkipListTests, IteratorOrderIsSorted)
{
    SkipList<int, int> sl;
    // 以打乱的顺序插入
    std::vector<int> keys = {5, 3, 8, 1, 4, 7, 2, 6, 9, 0};
    for (int k : keys) {
        sl.insert(k, k * 10);
    }

    int prev = -1;
    for (auto [k, v] : sl) {
        EXPECT_GT(k, prev);
        EXPECT_EQ(v, k * 10);
        prev = k;
    }
}

TEST(SkipListTests, EmptyIterator)
{
    SkipList<int, int> sl;
    EXPECT_EQ(sl.begin(), sl.end());
}

TEST(SkipListTests, ConstIterator)
{
    SkipList<int, int> sl;
    sl.insert(1, 10);
    sl.insert(2, 20);

    const auto& csl   = sl;
    int         count = 0;
    for (auto it = csl.begin(); it != csl.end(); ++it) {
        ++count;
    }
    EXPECT_EQ(count, 2);
}

TEST(SkipListTests, PostfixIncrement)
{
    SkipList<int, int> sl;
    sl.insert(1, 10);
    sl.insert(2, 20);

    auto it       = sl.begin();
    auto old      = it++;
    auto [k1, v1] = *old;
    auto [k2, v2] = *it;
    EXPECT_EQ(k1, 1);
    EXPECT_EQ(k2, 2);
}

// =============================================================================
// 范围查询测试
// =============================================================================

TEST(SkipListTests, LowerBound)
{
    SkipList<int, int> sl;
    for (int i = 0; i < 10; i += 2) {
        sl.insert(i, i);
    }

    // lower_bound(3) 应返回指向 4 的迭代器
    auto it = sl.lower_bound(3);
    EXPECT_NE(it, sl.end());
    auto [k, v] = *it;
    EXPECT_EQ(k, 4);
}

TEST(SkipListTests, LowerBoundExactMatch)
{
    SkipList<int, int> sl;
    for (int i = 0; i < 10; ++i) {
        sl.insert(i, i);
    }

    auto it = sl.lower_bound(5);
    EXPECT_NE(it, sl.end());
    auto [k, v] = *it;
    EXPECT_EQ(k, 5);
}

TEST(SkipListTests, LowerBoundBeyondAll)
{
    SkipList<int, int> sl;
    sl.insert(1, 1);
    sl.insert(2, 2);

    EXPECT_EQ(sl.lower_bound(100), sl.end());
}

TEST(SkipListTests, UpperBound)
{
    SkipList<int, int> sl;
    for (int i = 0; i < 10; i += 2) {
        sl.insert(i, i);
    }

    // upper_bound(4) 应返回指向 6 的迭代器
    auto it = sl.upper_bound(4);
    EXPECT_NE(it, sl.end());
    auto [k, v] = *it;
    EXPECT_EQ(k, 6);
}

TEST(SkipListTests, UpperBoundAll)
{
    SkipList<int, int> sl;
    sl.insert(1, 1);
    EXPECT_EQ(sl.upper_bound(1), sl.end());
}

TEST(SkipListTests, RangeQuery)
{
    SkipList<int, int> sl;
    for (int i = 0; i < 100; ++i) {
        sl.insert(i, i);
    }

    // 查询 [25, 75) 范围内的所有元素
    std::vector<int> result;
    for (auto it = sl.lower_bound(25); it != sl.end(); ++it) {
        auto [k, v] = *it;
        if (k >= 75)
            break;
        result.push_back(k);
    }

    EXPECT_EQ(result.size(), 50u);
    for (int i = 0; i < 50; ++i) {
        EXPECT_EQ(result[i], 25 + i);
    }
}

// =============================================================================
// 容量与清空测试
// =============================================================================

TEST(SkipListTests, ClearRemovesAll)
{
    SkipList<int, int> sl;
    for (int i = 0; i < 100; ++i) {
        sl.insert(i, i);
    }

    sl.clear();
    EXPECT_TRUE(sl.empty());
    EXPECT_EQ(sl.size(), 0u);
    EXPECT_EQ(sl.begin(), sl.end());
}

TEST(SkipListTests, ClearThenReinsert)
{
    SkipList<int, int> sl;
    sl.insert(1, 10);
    sl.clear();
    sl.insert(2, 20);
    EXPECT_EQ(sl.size(), 1u);
    EXPECT_EQ(sl.at(2), 20);
    EXPECT_FALSE(sl.contains(1));
}

// =============================================================================
// 字符串键值测试
// =============================================================================

TEST(SkipListTests, StringKeys)
{
    SkipList<std::string, int> sl;
    sl.insert("banana", 2);
    sl.insert("apple", 1);
    sl.insert("cherry", 3);

    // 应按字典序排列
    std::vector<std::string> order;
    for (auto [k, v] : sl) {
        order.push_back(std::string(k));
    }
    EXPECT_EQ(order, (std::vector<std::string>{"apple", "banana", "cherry"}));
}

// =============================================================================
// 自定义比较器测试
// =============================================================================

TEST(SkipListTests, CustomComparatorDescending)
{
    SkipList<int, int, std::greater<int>> sl;
    sl.insert(1, 10);
    sl.insert(3, 30);
    sl.insert(2, 20);

    std::vector<int> order;
    for (auto [k, v] : sl) {
        order.push_back(k);
    }
    EXPECT_EQ(order, (std::vector<int>{3, 2, 1}));
}

// =============================================================================
// 大规模压力测试
// =============================================================================

TEST(SkipListTests, LargeInsertAndLookup)
{
    SkipList<int, int> sl;
    int                n = 10000;

    for (int i = 0; i < n; ++i) {
        sl.insert(i, i * 2);
    }
    EXPECT_EQ(sl.size(), static_cast<std::size_t>(n));

    for (int i = 0; i < n; ++i) {
        EXPECT_TRUE(sl.contains(i));
        EXPECT_EQ(sl.at(i), i * 2);
    }
}

TEST(SkipListTests, LargeInsertDeleteAndVerify)
{
    SkipList<int, int> sl;
    int                n = 5000;

    for (int i = 0; i < n; ++i) {
        sl.insert(i, i);
    }

    // 删除偶数
    for (int i = 0; i < n; i += 2) {
        EXPECT_TRUE(sl.erase(i));
    }

    EXPECT_EQ(sl.size(), static_cast<std::size_t>(n / 2));

    // 验证奇数仍在
    for (int i = 1; i < n; i += 2) {
        EXPECT_TRUE(sl.contains(i));
    }

    // 验证偶数已删除
    for (int i = 0; i < n; i += 2) {
        EXPECT_FALSE(sl.contains(i));
    }
}

TEST(SkipListTests, LevelProperty)
{
    SkipList<int, int> sl;
    EXPECT_EQ(sl.level(), 1);

    // 插入足够多的元素后，层数应该增长
    for (int i = 0; i < 10000; ++i) {
        sl.insert(i, i);
    }
    EXPECT_GT(sl.level(), 1);
}

TEST(SkipListTests, ExceptionDuringInsertDoesNotMutateContainerState)
{
    SkipList<int, AlwaysThrowOnCopy> sl;
    const AlwaysThrowOnCopy          value;

    for (int i = 0; i < 40; ++i) {
        EXPECT_THROW(sl.insert(i, value), std::runtime_error);
    }

    EXPECT_TRUE(sl.empty());
    EXPECT_EQ(sl.size(), 0u);
    EXPECT_EQ(sl.level(), 1);
}

TEST(SkipListTests, ConstLookupApisWork)
{
    SkipList<int, int> sl;
    sl.insert(10, 100);
    sl.insert(20, 200);
    const auto& csl = sl;

    auto fit = csl.find(20);
    ASSERT_NE(fit, csl.end());
    auto [fk, fv] = *fit;
    EXPECT_EQ(fk, 20);
    EXPECT_EQ(fv, 200);

    auto lb = csl.lower_bound(11);
    ASSERT_NE(lb, csl.end());
    auto [lk, lv] = *lb;
    EXPECT_EQ(lk, 20);
    EXPECT_EQ(lv, 200);

    auto ub = csl.upper_bound(10);
    ASSERT_NE(ub, csl.end());
    auto [uk, uv] = *ub;
    EXPECT_EQ(uk, 20);
    EXPECT_EQ(uv, 200);
}

// =============================================================================
// SkipList Edge Cases
// =============================================================================

TEST(SkipListTests, ExtremeIntValues)
{
    SkipList<int, int> sl;
    sl.insert(std::numeric_limits<int>::min(), -1);
    sl.insert(0, 0);
    sl.insert(std::numeric_limits<int>::max(), 1);

    EXPECT_EQ(sl.size(), 3u);

    // Verify sorted order via iteration
    auto it = sl.begin();
    ASSERT_NE(it, sl.end());
    EXPECT_EQ((*it).first, std::numeric_limits<int>::min());
    ++it;
    ASSERT_NE(it, sl.end());
    EXPECT_EQ((*it).first, 0);
    ++it;
    ASSERT_NE(it, sl.end());
    EXPECT_EQ((*it).first, std::numeric_limits<int>::max());

    // Verify find works for all extreme values
    EXPECT_NE(sl.find(std::numeric_limits<int>::min()), sl.end());
    EXPECT_NE(sl.find(0), sl.end());
    EXPECT_NE(sl.find(std::numeric_limits<int>::max()), sl.end());
}

TEST(SkipListTests, EraseByIteratorSequential)
{
    SkipList<int, int> sl;
    for (int i = 0; i < 10; ++i)
        sl.insert(i, i * 100);

    EXPECT_EQ(sl.size(), 10u);

    for (int i = 10; i > 0; --i) {
        EXPECT_EQ(sl.size(), static_cast<std::size_t>(i));
        auto it = sl.begin();
        ASSERT_NE(it, sl.end());
        sl.erase(it);
    }

    EXPECT_TRUE(sl.empty());
    EXPECT_EQ(sl.size(), 0u);
}

TEST(SkipListTests, OperatorBracketOnEmpty)
{
    SkipList<int, int> sl;
    EXPECT_TRUE(sl.empty());

    (void)sl[42]; // creates entry with default value
    EXPECT_EQ(sl.size(), 1u);
    EXPECT_EQ(sl[42], 0); // default int is 0
}

TEST(SkipListTests, LargeScaleInsertEraseReinsert)
{
    SkipList<int, int> sl;
    constexpr int      kCount = 5000;

    // Phase 1: insert
    for (int i = 0; i < kCount; ++i)
        sl.insert(i, i * 2);
    EXPECT_EQ(sl.size(), static_cast<std::size_t>(kCount));

    // Phase 2: erase all
    for (int i = 0; i < kCount; ++i)
        EXPECT_TRUE(sl.erase(i));
    EXPECT_TRUE(sl.empty());

    // Phase 3: reinsert with different values
    for (int i = 0; i < kCount; ++i)
        sl.insert(i, i * 3);
    EXPECT_EQ(sl.size(), static_cast<std::size_t>(kCount));

    // Verify all present with correct values
    for (int i = 0; i < kCount; ++i) {
        auto it = sl.find(i);
        ASSERT_NE(it, sl.end());
        EXPECT_EQ((*it).second, i * 3);
    }
}
