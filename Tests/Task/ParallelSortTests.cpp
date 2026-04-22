/**
 * @File ParallelSortTests.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/19
 * @Brief This file is part of Bee.
 */

#include <gtest/gtest.h>
#include <algorithm>
#include <functional>
#include <numeric>
#include <random>
#include <ranges>
#include <vector>

#include "Task/Parallel/Sort.hpp"
#include "Concurrency/Thread/WorkPool.hpp"

using namespace bee;

// =========================================================================
// parallel_sort — 迭代器接口
// =========================================================================

TEST(ParallelSortTests, BasicDefaultComp)
{
    WorkPool         pool(4);
    std::vector<int> data(10000);
    std::iota(data.begin(), data.end(), 0);

    // 打乱
    std::mt19937 rng(42);
    std::shuffle(data.begin(), data.end(), rng);

    parallel_sort(pool, data.begin(), data.end());

    EXPECT_TRUE(std::is_sorted(data.begin(), data.end()));
}

TEST(ParallelSortTests, CustomComparator)
{
    WorkPool         pool(4);
    std::vector<int> data(10000);
    std::iota(data.begin(), data.end(), 0);

    std::mt19937 rng(123);
    std::shuffle(data.begin(), data.end(), rng);

    parallel_sort(pool, data.begin(), data.end(), std::greater<>{});

    EXPECT_TRUE(std::is_sorted(data.begin(), data.end(), std::greater<>{}));
}

TEST(ParallelSortTests, AlreadySorted)
{
    WorkPool         pool(4);
    std::vector<int> data(10000);
    std::iota(data.begin(), data.end(), 0);

    parallel_sort(pool, data.begin(), data.end());

    EXPECT_TRUE(std::is_sorted(data.begin(), data.end()));
}

TEST(ParallelSortTests, ReverseSorted)
{
    WorkPool         pool(4);
    std::vector<int> data(10000);
    std::iota(data.begin(), data.end(), 0);
    std::reverse(data.begin(), data.end());

    parallel_sort(pool, data.begin(), data.end());

    EXPECT_TRUE(std::is_sorted(data.begin(), data.end()));
}

TEST(ParallelSortTests, SerialFallback)
{
    WorkPool         pool(4);
    std::vector<int> data = {5, 3, 1, 4, 2};

    parallel_sort(pool, data.begin(), data.end());

    EXPECT_EQ(data, (std::vector<int>{1, 2, 3, 4, 5}));
}

TEST(ParallelSortTests, EmptyRange)
{
    WorkPool         pool(4);
    std::vector<int> data;

    parallel_sort(pool, data.begin(), data.end());

    EXPECT_TRUE(data.empty());
}

TEST(ParallelSortTests, SingleElement)
{
    WorkPool         pool(4);
    std::vector<int> data = {42};

    parallel_sort(pool, data.begin(), data.end());

    EXPECT_EQ(data[0], 42);
}

TEST(ParallelSortTests, AllEqual)
{
    WorkPool         pool(4);
    std::vector<int> data(10000, 7);

    parallel_sort(pool, data.begin(), data.end());

    EXPECT_TRUE(std::all_of(data.begin(), data.end(), [](int v) { return v == 7; }));
}

// =========================================================================
// 取消支持
// =========================================================================

TEST(ParallelSortTests, Cancellation)
{
    WorkPool         pool(4);
    std::vector<int> data(10000);
    std::iota(data.begin(), data.end(), 0);
    std::mt19937 rng(42);
    std::shuffle(data.begin(), data.end(), rng);

    std::stop_source ss;
    ss.request_stop();

    EXPECT_THROW(parallel_sort(pool, data.begin(), data.end(), std::less<>{}, ss.get_token()), std::runtime_error);
}

// =========================================================================
// Ranges 接口
// =========================================================================

TEST(ParallelSortTests, RangesBasic)
{
    WorkPool         pool(4);
    std::vector<int> data(10000);
    std::iota(data.begin(), data.end(), 0);
    std::mt19937 rng(42);
    std::shuffle(data.begin(), data.end(), rng);

    parallel_sort(pool, data);

    EXPECT_TRUE(std::is_sorted(data.begin(), data.end()));
}

TEST(ParallelSortTests, RangesCustomComp)
{
    WorkPool         pool(4);
    std::vector<int> data(10000);
    std::iota(data.begin(), data.end(), 0);
    std::mt19937 rng(42);
    std::shuffle(data.begin(), data.end(), rng);

    parallel_sort(pool, data, std::greater<>{});

    EXPECT_TRUE(std::is_sorted(data.begin(), data.end(), std::greater<>{}));
}

TEST(ParallelSortTests, LargeDataCorrectnessCheck)
{
    // 排序后数据与 std::sort 结果完全一致
    WorkPool         pool(4);
    std::vector<int> data(50000);
    std::mt19937     rng(999);
    std::generate(data.begin(), data.end(), [&rng]() { return rng() % 10000; });

    auto expected = data;
    std::sort(expected.begin(), expected.end());

    parallel_sort(pool, data.begin(), data.end());

    EXPECT_EQ(data, expected);
}
