/**
 * @File ParallelTransformTests.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/12
 * @Brief This file is part of Bee.
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
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
// parallel_transform 测试
// =========================================================================

TEST(ParallelTransformTests, BasicSquare)
{
    ThreadPool       pool(4);
    constexpr size_t N = 10000;
    std::vector<int> input(N);
    std::iota(input.begin(), input.end(), 0);
    std::vector<int> output(N);

    auto it = bee::parallel_transform(pool, input.begin(), input.end(), output.begin(), [](int x) {
        return x * x;
    });

    EXPECT_EQ(it, output.end());
    for (size_t i = 0; i < N; ++i) {
        EXPECT_EQ(output[i], static_cast<int>(i * i));
    }
}

TEST(ParallelTransformTests, EmptyRange)
{
    ThreadPool       pool(4);
    std::vector<int> input;
    std::vector<int> output;

    auto it = bee::parallel_transform(pool, input.begin(), input.end(), output.begin(), [](int x) {
        return x;
    });
    EXPECT_EQ(it, output.end());
}

TEST(ParallelTransformTests, TypeConversion)
{
    ThreadPool       pool(4);
    constexpr size_t N = 10000;
    std::vector<int> input(N);
    std::iota(input.begin(), input.end(), 1);
    std::vector<double> output(N);

    bee::parallel_transform(pool, input.begin(), input.end(), output.begin(), [](int x) -> double {
        return std::sqrt(static_cast<double>(x));
    });

    for (size_t i = 0; i < N; ++i) {
        EXPECT_DOUBLE_EQ(output[i], std::sqrt(static_cast<double>(i + 1)));
    }
}

TEST(ParallelTransformTests, LargeRange)
{
    ThreadPool       pool(4);
    constexpr size_t N = 200'000;
    std::vector<int> input(N);
    std::iota(input.begin(), input.end(), 0);
    std::vector<int> output(N);

    bee::parallel_transform(pool, input.begin(), input.end(), output.begin(), [](int x) {
        return x + 1;
    });

    for (size_t i = 0; i < N; ++i) {
        EXPECT_EQ(output[i], static_cast<int>(i + 1));
    }
}

TEST(ParallelTransformTests, ExceptionPropagation)
{
    ThreadPool       pool(4);
    constexpr size_t N = 100'000;
    std::vector<int> input(N, 0);
    std::vector<int> output(N);

    EXPECT_THROW(
            bee::parallel_transform(
                    pool,
                    input.begin(),
                    input.end(),
                    output.begin(),
                    [](int) -> int {
                        throw std::runtime_error("transform error");
                    }
            ),
            std::runtime_error
    );
}

TEST(ParallelTransformTests, Cancellation)
{
    ThreadPool       pool(4);
    constexpr size_t N = 200'000;
    std::vector<int> input(N, 1);
    std::vector<int> output(N, 0);

    std::stop_source source;
    source.request_stop();

    EXPECT_THROW(
            bee::parallel_transform(
                    pool,
                    input.begin(),
                    input.end(),
                    output.begin(),
                    [](int x) {
                        return x * 2;
                    },
                    source.get_token()
            ),
            std::runtime_error
    );
}

} // namespace
