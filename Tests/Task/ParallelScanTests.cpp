/**
 * @File ParallelScanTests.cpp
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
// parallel_inclusive_scan 测试
// =========================================================================

TEST(ParallelScanTests, InclusiveSumSmall)
{
    ThreadPool       pool(4);
    std::vector<int> input = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    std::vector<int> output(input.size());
    std::vector<int> expected(input.size());
    std::inclusive_scan(input.begin(), input.end(), expected.begin(), std::plus<>{});

    bee::parallel_inclusive_scan(pool, input.begin(), input.end(), output.begin(), std::plus<>{});

    EXPECT_EQ(output, expected);
}

TEST(ParallelScanTests, InclusiveSumLarge)
{
    ThreadPool       pool(4);
    constexpr size_t N = 200'000;
    std::vector<int> input(N, 1);
    std::vector<int> output(N);
    std::vector<int> expected(N);
    std::inclusive_scan(input.begin(), input.end(), expected.begin(), std::plus<>{});

    bee::parallel_inclusive_scan(pool, input.begin(), input.end(), output.begin(), std::plus<>{});

    EXPECT_EQ(output, expected);
}

TEST(ParallelScanTests, InclusiveMultiply)
{
    ThreadPool pool(4);
    // 使用超过阈值但安全的值范围。
    constexpr size_t    N = 5000;
    std::vector<double> input(N, 1.0);
    input[0] = 2.0;
    input[1] = 3.0;
    std::vector<double> output(N);
    std::vector<double> expected(N);
    std::inclusive_scan(input.begin(), input.end(), expected.begin(), std::multiplies<>{});

    bee::parallel_inclusive_scan(pool, input.begin(), input.end(), output.begin(), std::multiplies<>{});

    EXPECT_EQ(output, expected);
}

// =========================================================================
// parallel_exclusive_scan 测试
// =========================================================================

TEST(ParallelScanTests, ExclusiveSumSmall)
{
    ThreadPool       pool(4);
    std::vector<int> input = {1, 2, 3, 4, 5};
    std::vector<int> output(input.size());
    std::vector<int> expected(input.size());
    std::exclusive_scan(input.begin(), input.end(), expected.begin(), 0, std::plus<>{});

    bee::parallel_exclusive_scan(pool, input.begin(), input.end(), output.begin(), 0, std::plus<>{});

    EXPECT_EQ(output, expected);
}

TEST(ParallelScanTests, ExclusiveSumLarge)
{
    ThreadPool       pool(4);
    constexpr size_t N = 200'000;
    std::vector<int> input(N);
    std::iota(input.begin(), input.end(), 1);
    std::vector<int> output(N);
    std::vector<int> expected(N);
    std::exclusive_scan(input.begin(), input.end(), expected.begin(), 0, std::plus<>{});

    bee::parallel_exclusive_scan(pool, input.begin(), input.end(), output.begin(), 0, std::plus<>{});

    EXPECT_EQ(output, expected);
}

TEST(ParallelScanTests, EmptyRange)
{
    ThreadPool       pool(4);
    std::vector<int> input;
    std::vector<int> output;

    auto it = bee::parallel_inclusive_scan(pool, input.begin(), input.end(), output.begin(), std::plus<>{});
    EXPECT_EQ(it, output.end());

    auto it2 = bee::parallel_exclusive_scan(pool, input.begin(), input.end(), output.begin(), 0, std::plus<>{});
    EXPECT_EQ(it2, output.end());
}

TEST(ParallelScanTests, CancellationInclusive)
{
    ThreadPool       pool(4);
    constexpr size_t N = 200'000;
    std::vector<int> input(N, 1);
    std::vector<int> output(N, 0);

    std::stop_source source;
    source.request_stop();

    EXPECT_THROW(
            bee::parallel_inclusive_scan(pool, input.begin(), input.end(), output.begin(), std::plus<>{}, source.get_token()), std::runtime_error
    );
}

TEST(ParallelScanTests, CancellationExclusive)
{
    ThreadPool       pool(4);
    constexpr size_t N = 200'000;
    std::vector<int> input(N, 1);
    std::vector<int> output(N, 0);

    std::stop_source source;
    source.request_stop();

    EXPECT_THROW(
            bee::parallel_exclusive_scan(pool, input.begin(), input.end(), output.begin(), 0, std::plus<>{}, source.get_token()), std::runtime_error
    );
}

} // namespace
