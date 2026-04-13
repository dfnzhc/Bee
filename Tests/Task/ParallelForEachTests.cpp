/**
 * @File ParallelForEachTests.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/12
 * @Brief This file is part of Bee.
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
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
// parallel_for_each 测试
// =========================================================================

TEST(ParallelForEachTests, BasicMutation)
{
    ThreadPool pool(4);
    std::vector<int> data(10000);
    std::iota(data.begin(), data.end(), 0);

    bee::parallel_for_each(pool, data.begin(), data.end(), [](int& x) {
        x *= 2;
    });

    for (int i = 0; i < 10000; ++i) {
        EXPECT_EQ(data[i], i * 2);
    }
}

TEST(ParallelForEachTests, EmptyRange)
{
    ThreadPool pool(4);
    std::vector<int> data;
    // 不应崩溃或抛出异常。
    bee::parallel_for_each(pool, data.begin(), data.end(), [](int&) {
        FAIL();
    });
}

TEST(ParallelForEachTests, SingleElement)
{
    ThreadPool pool(4);
    std::vector<int> data = {42};
    bee::parallel_for_each(pool, data.begin(), data.end(), [](int& x) {
        x += 1;
    });
    EXPECT_EQ(data[0], 43);
}

TEST(ParallelForEachTests, LargeRange)
{
    ThreadPool pool(4);
    constexpr size_t N = 200'000;
    std::vector<int> data(N, 1);

    std::atomic<int> sum{0};
    bee::parallel_for_each(pool, data.begin(), data.end(), [&sum](const int& x) {
        sum.fetch_add(x, std::memory_order_relaxed);
    });

    EXPECT_EQ(sum.load(), static_cast<int>(N));
}

TEST(ParallelForEachTests, ExceptionPropagation)
{
    ThreadPool pool(4);
    constexpr size_t N = 100'000;
    std::vector<int> data(N, 0);

    EXPECT_THROW(
            bee::parallel_for_each(pool, data.begin(), data.end(), [](int& x) {
                if (x == 0)
                throw std::runtime_error("test error");
                }),
            std::runtime_error);
}

TEST(ParallelForEachTests, CancellationStopsProcessing)
{
    ThreadPool pool(4);
    constexpr size_t N = 200'000;
    std::vector<int> data(N, 0);
    std::atomic<size_t> processed{0};

    std::stop_source source;
    // 立即请求取消，使大部分块被跳过。
    source.request_stop();

    EXPECT_THROW(
            bee::parallel_for_each(
                pool,
                data.begin(),
                data.end(),
                [&processed](int&) { processed.fetch_add(1, std::memory_order_relaxed); },
                source.get_token()),
            std::runtime_error);

    // 因为立即取消了，不应处理所有元素。
    EXPECT_LT(processed.load(), N);
}

TEST(ParallelForEachTests, SequentialFallbackCorrectness)
{
    ThreadPool pool(4);
    // 低于 kParallelThreshold（4096）——串行执行。
    std::vector<int> data(100);
    std::iota(data.begin(), data.end(), 0);

    bee::parallel_for_each(pool, data.begin(), data.end(), [](int& x) {
        x += 10;
    });

    for (int i = 0; i < 100; ++i) {
        EXPECT_EQ(data[i], i + 10);
    }
}

} // namespace
