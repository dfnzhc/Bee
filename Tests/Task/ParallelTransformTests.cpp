/**
 * @File ParallelTransformTests.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/19
 * @Brief This file is part of Bee.
 */

#include <gtest/gtest.h>
#include <algorithm>
#include <numeric>
#include <ranges>
#include <vector>

#include "Task/Parallel/Transform.hpp"
#include "Concurrency/Thread/WorkPool.hpp"

using namespace bee;

// =========================================================================
// 迭代器接口
// =========================================================================

TEST(ParallelTransformTests, BasicIterator)
{
    WorkPool pool(4);
    std::vector<int> input(10000);
    std::iota(input.begin(), input.end(), 0);
    std::vector<int> output(10000, -1);

    auto result_it = parallel_transform(pool, input.begin(), input.end(), output.begin(),
        [](int v) { return v * 2; });

    // 返回尾后迭代器
    EXPECT_EQ(result_it, output.end());

    // 验证结果
    for (int i = 0; i < 10000; ++i) {
        EXPECT_EQ(output[i], i * 2);
    }
}

TEST(ParallelTransformTests, SerialFallback)
{
    WorkPool pool(4);
    std::vector<int> input = {1, 2, 3};
    std::vector<int> output(3);

    auto result_it = parallel_transform(pool, input.begin(), input.end(), output.begin(),
        [](int v) { return v + 10; });

    EXPECT_EQ(result_it, output.end());
    EXPECT_EQ(output[0], 11);
    EXPECT_EQ(output[1], 12);
    EXPECT_EQ(output[2], 13);
}

TEST(ParallelTransformTests, EmptyRange)
{
    WorkPool pool(4);
    std::vector<int> input;
    std::vector<int> output;

    auto result_it = parallel_transform(pool, input.begin(), input.end(), output.begin(),
        [](int v) { return v; });

    EXPECT_EQ(result_it, output.end());
}

TEST(ParallelTransformTests, ExceptionPropagation)
{
    WorkPool pool(4);
    std::vector<int> input(10000, 0);
    std::vector<int> output(10000);

    EXPECT_THROW(
        (void)parallel_transform(pool, input.begin(), input.end(), output.begin(),
            [](int) -> int { throw std::runtime_error("test"); }),
        std::runtime_error
    );
}

// =========================================================================
// 取消支持
// =========================================================================

TEST(ParallelTransformTests, CancellationStopsWork)
{
    WorkPool pool(4);
    std::vector<int> input(10000, 0);
    std::vector<int> output(10000);
    std::stop_source ss;
    ss.request_stop();

    EXPECT_THROW(
        (void)parallel_transform(pool, input.begin(), input.end(), output.begin(),
            [](int v) { return v; }, ss.get_token()),
        std::runtime_error
    );
}

// =========================================================================
// Ranges 接口
// =========================================================================

TEST(ParallelTransformTests, RangesBasic)
{
    WorkPool pool(4);
    std::vector<int> input(10000);
    std::iota(input.begin(), input.end(), 0);
    std::vector<int> output(10000);

    auto result_it = parallel_transform(pool, input, output.begin(),
        [](int v) { return v * 3; });

    EXPECT_EQ(result_it, output.end());
    for (int i = 0; i < 10000; ++i) {
        EXPECT_EQ(output[i], i * 3);
    }
}

TEST(ParallelTransformTests, RangesCancellation)
{
    WorkPool pool(4);
    std::vector<int> input(10000, 0);
    std::vector<int> output(10000);
    std::stop_source ss;
    ss.request_stop();

    EXPECT_THROW(
        (void)parallel_transform(pool, input, output.begin(),
            [](int v) { return v; }, ss.get_token()),
        std::runtime_error
    );
}
