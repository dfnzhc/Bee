/**
 * @File ParallelReduceTests.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/19
 * @Brief This file is part of Bee.
 */

#include <gtest/gtest.h>
#include <algorithm>
#include <cmath>
#include <functional>
#include <numeric>
#include <ranges>
#include <vector>

#include "Task/Parallel/Reduce.hpp"
#include "Concurrency/Thread/WorkPool.hpp"

using namespace bee;

// =========================================================================
// parallel_reduce — 迭代器接口
// =========================================================================

TEST(ParallelReduceTests, SumIterator)
{
    WorkPool         pool(4);
    std::vector<int> data(10000);
    std::iota(data.begin(), data.end(), 1);

    auto result = parallel_reduce(pool, data.begin(), data.end(), 0, std::plus<>{});

    // sum(1..10000) = 10000 * 10001 / 2
    EXPECT_EQ(result, 10000 * 10001 / 2);
}

TEST(ParallelReduceTests, ProductSmall)
{
    WorkPool         pool(4);
    std::vector<int> data = {1, 2, 3, 4, 5};

    auto result = parallel_reduce(pool, data.begin(), data.end(), 1, std::multiplies<>{});

    EXPECT_EQ(result, 120); // 5!
}

TEST(ParallelReduceTests, EmptyRange)
{
    WorkPool         pool(4);
    std::vector<int> data;

    auto result = parallel_reduce(pool, data.begin(), data.end(), 42, std::plus<>{});

    EXPECT_EQ(result, 42); // 返回 init
}

TEST(ParallelReduceTests, SerialFallback)
{
    WorkPool         pool(4);
    std::vector<int> data(100);
    std::iota(data.begin(), data.end(), 1);

    auto result = parallel_reduce(pool, data.begin(), data.end(), 0, std::plus<>{});

    EXPECT_EQ(result, 100 * 101 / 2);
}

TEST(ParallelReduceTests, ExceptionPropagation)
{
    WorkPool         pool(4);
    std::vector<int> data(10000, 0);

    EXPECT_THROW(
        (void)parallel_reduce(pool, data.begin(), data.end(), 0, [](int, int) -> int { throw std::runtime_error("test"); }), std::runtime_error
    );
}

TEST(ParallelReduceTests, Cancellation)
{
    WorkPool         pool(4);
    std::vector<int> data(10000, 1);
    std::stop_source ss;
    ss.request_stop();

    EXPECT_THROW((void)parallel_reduce(pool, data.begin(), data.end(), 0, std::plus<>{}, ss.get_token()), std::runtime_error);
}

// =========================================================================
// parallel_reduce — Ranges 接口
// =========================================================================

TEST(ParallelReduceTests, SumRanges)
{
    WorkPool         pool(4);
    std::vector<int> data(10000);
    std::iota(data.begin(), data.end(), 1);

    auto result = parallel_reduce(pool, data, 0, std::plus<>{});

    EXPECT_EQ(result, 10000 * 10001 / 2);
}

// =========================================================================
// parallel_transform_reduce
// =========================================================================

TEST(ParallelReduceTests, TransformReduceBasic)
{
    WorkPool         pool(4);
    std::vector<int> data(10000);
    std::iota(data.begin(), data.end(), 1);

    // sum of squares
    auto result = parallel_transform_reduce(pool, data.begin(), data.end(), static_cast<long long>(0), std::plus<>{}, [](int v) {
        return static_cast<long long>(v) * v;
    });

    // sum(i^2, i=1..n) = n*(n+1)*(2n+1)/6
    long long expected = 10000LL * 10001LL * 20001LL / 6;
    EXPECT_EQ(result, expected);
}

TEST(ParallelReduceTests, TransformReduceEmpty)
{
    WorkPool         pool(4);
    std::vector<int> data;

    auto result = parallel_transform_reduce(pool, data.begin(), data.end(), 100, std::plus<>{}, [](int v) { return v * v; });

    EXPECT_EQ(result, 100);
}

TEST(ParallelReduceTests, TransformReduceRanges)
{
    WorkPool         pool(4);
    std::vector<int> data(10000);
    std::iota(data.begin(), data.end(), 1);

    auto result =
        parallel_transform_reduce(pool, data, static_cast<long long>(0), std::plus<>{}, [](int v) { return static_cast<long long>(v) * v; });

    long long expected = 10000LL * 10001LL * 20001LL / 6;
    EXPECT_EQ(result, expected);
}

TEST(ParallelReduceTests, TransformReduceCancellation)
{
    WorkPool         pool(4);
    std::vector<int> data(10000, 1);
    std::stop_source ss;
    ss.request_stop();

    EXPECT_THROW(
        (void)parallel_transform_reduce(pool, data.begin(), data.end(), 0, std::plus<>{}, [](int v) { return v; }, ss.get_token()), std::runtime_error
    );
}
