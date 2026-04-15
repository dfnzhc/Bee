/**
 * @File ParallelSortTests.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/12
 * @Brief This file is part of Bee.
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <functional>
#include <numeric>
#include <random>
#include <stdexcept>
#include <stop_token>
#include <vector>

#include "Task/Task.hpp"
#include "Concurrency/Thread/ThreadPool.hpp"

namespace
{

using bee::ThreadPool;

// =========================================================================
// parallel_sort 测试
// =========================================================================

TEST(ParallelSortTests, AlreadySorted)
{
    ThreadPool       pool(4);
    constexpr size_t N = 10000;
    std::vector<int> data(N);
    std::iota(data.begin(), data.end(), 0);

    bee::parallel_sort(pool, data.begin(), data.end());

    EXPECT_TRUE(std::is_sorted(data.begin(), data.end()));
}

TEST(ParallelSortTests, ReverseSorted)
{
    ThreadPool       pool(4);
    constexpr size_t N = 10000;
    std::vector<int> data(N);
    std::iota(data.rbegin(), data.rend(), 0);

    bee::parallel_sort(pool, data.begin(), data.end());

    EXPECT_TRUE(std::is_sorted(data.begin(), data.end()));
}

TEST(ParallelSortTests, RandomData)
{
    ThreadPool       pool(4);
    constexpr size_t N = 200'000;
    std::vector<int> data(N);
    std::mt19937     rng(42);
    std::generate(data.begin(), data.end(), [&rng] {
        return static_cast<int>(rng() % 1'000'000);
    });

    auto expected = data;
    std::sort(expected.begin(), expected.end());

    bee::parallel_sort(pool, data.begin(), data.end());

    EXPECT_EQ(data, expected);
}

TEST(ParallelSortTests, Duplicates)
{
    ThreadPool       pool(4);
    constexpr size_t N = 50'000;
    std::vector<int> data(N);
    // 仅 10 个不同的值。
    for (size_t i = 0; i < N; ++i)
        data[i] = static_cast<int>(i % 10);

    bee::parallel_sort(pool, data.begin(), data.end());

    EXPECT_TRUE(std::is_sorted(data.begin(), data.end()));
}

TEST(ParallelSortTests, CustomComparator)
{
    ThreadPool       pool(4);
    constexpr size_t N = 10000;
    std::vector<int> data(N);
    std::iota(data.begin(), data.end(), 0);

    bee::parallel_sort(pool, data.begin(), data.end(), std::greater<>{});

    EXPECT_TRUE(std::is_sorted(data.begin(), data.end(), std::greater<>{}));
}

TEST(ParallelSortTests, Cancellation)
{
    ThreadPool       pool(4);
    constexpr size_t N = 200'000;
    std::vector<int> data(N);
    std::mt19937     rng(123);
    std::generate(data.begin(), data.end(), [&rng] {
        return static_cast<int>(rng());
    });

    std::stop_source source;
    source.request_stop();

    EXPECT_THROW(bee::parallel_sort(pool, data.begin(), data.end(), std::less<>{}, source.get_token()), std::runtime_error);
}

} // namespace
