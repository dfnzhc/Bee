/**
 * @File ParallelReduceTests.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/12
 * @Brief This file is part of Bee.
 */

#include <gtest/gtest.h>

#include <functional>
#include <numeric>
#include <stdexcept>
#include <stop_token>
#include <vector>

#include "Task/Task.hpp"
#include "Concurrency/Thread/ThreadPool.hpp"

namespace
{

using bee::ThreadPool;

// =========================================================================
// parallel_reduce 测试
// =========================================================================

TEST(ParallelReduceTests, SumOfIntegers)
{
    ThreadPool             pool(4);
    constexpr size_t       N = 100'000;
    std::vector<long long> data(N);
    std::iota(data.begin(), data.end(), 1LL);

    auto result = bee::parallel_reduce(pool, data.begin(), data.end(), 0LL, std::plus<>{});

    long long expected = static_cast<long long>(N) * (N + 1) / 2;
    EXPECT_EQ(result, expected);
}

TEST(ParallelReduceTests, ProductOfDoubles)
{
    ThreadPool pool(4);
    // 使用较小的值以避免溢出。
    std::vector<double> data = {1.0, 2.0, 3.0, 4.0, 5.0};

    auto result = bee::parallel_reduce(pool, data.begin(), data.end(), 1.0, std::multiplies<>{});

    EXPECT_DOUBLE_EQ(result, 120.0);
}

TEST(ParallelReduceTests, EmptyRange)
{
    ThreadPool       pool(4);
    std::vector<int> data;

    auto result = bee::parallel_reduce(pool, data.begin(), data.end(), 42, std::plus<>{});

    EXPECT_EQ(result, 42);
}

TEST(ParallelReduceTests, SingleElement)
{
    ThreadPool       pool(4);
    std::vector<int> data = {7};

    auto result = bee::parallel_reduce(pool, data.begin(), data.end(), 0, std::plus<>{});

    EXPECT_EQ(result, 7);
}

TEST(ParallelReduceTests, ExceptionPropagation)
{
    ThreadPool       pool(4);
    constexpr size_t N = 100'000;
    std::vector<int> data(N, 1);

    EXPECT_THROW(
        bee::parallel_reduce(pool, data.begin(), data.end(), 0, [](int, int) -> int { throw std::runtime_error("reduce error"); }), std::runtime_error
    );
}

TEST(ParallelReduceTests, Cancellation)
{
    ThreadPool       pool(4);
    constexpr size_t N = 200'000;
    std::vector<int> data(N, 1);

    std::stop_source source;
    source.request_stop();

    EXPECT_THROW(bee::parallel_reduce(pool, data.begin(), data.end(), 0, std::plus<>{}, source.get_token()), std::runtime_error);
}

// =========================================================================
// parallel_transform_reduce 测试
// =========================================================================

TEST(ParallelReduceTests, TransformReduceSumOfSquares)
{
    ThreadPool       pool(4);
    constexpr size_t N = 100'000;
    std::vector<int> data(N);
    std::iota(data.begin(), data.end(), 1);

    auto result = bee::parallel_transform_reduce(pool, data.begin(), data.end(), 0LL, std::plus<>{}, [](int x) -> long long {
        return static_cast<long long>(x) * x;
    });

    // 平方和公式：N(N+1)(2N+1)/6
    long long expected = static_cast<long long>(N) * (N + 1) * (2 * N + 1) / 6;
    EXPECT_EQ(result, expected);
}

TEST(ParallelReduceTests, TransformReduceExceptionPropagation)
{
    ThreadPool       pool(4);
    constexpr size_t N = 100'000;
    std::vector<int> data(N, 1);

    EXPECT_THROW(
        bee::parallel_transform_reduce(
            pool, data.begin(), data.end(), 0LL, std::plus<>{}, [](int) -> long long { throw std::runtime_error("transform error"); }
        ),
        std::runtime_error
    );
}

TEST(ParallelReduceTests, TransformReduceCancellation)
{
    ThreadPool       pool(4);
    constexpr size_t N = 200'000;
    std::vector<int> data(N, 1);

    std::stop_source source;
    source.request_stop();

    EXPECT_THROW(
        bee::parallel_transform_reduce(
            pool, data.begin(), data.end(), 0LL, std::plus<>{}, [](int x) -> long long { return x; }, source.get_token()
        ),
        std::runtime_error
    );
}

} // namespace
