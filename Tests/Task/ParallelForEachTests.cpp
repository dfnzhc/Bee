/**
 * @File ParallelForEachTests.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/19
 * @Brief This file is part of Bee.
 */

#include <gtest/gtest.h>
#include <algorithm>
#include <atomic>
#include <numeric>
#include <ranges>
#include <vector>

#include "Task/Parallel/ForEach.hpp"
#include "Concurrency/Thread/WorkPool.hpp"

using namespace bee;

// =========================================================================
// 迭代器接口
// =========================================================================

TEST(ParallelForEachTests, BasicIterator)
{
    WorkPool pool(4);
    std::vector<int> data(10000);
    std::iota(data.begin(), data.end(), 0);

    std::atomic<int> sum{0};
    parallel_for_each(pool, data.begin(), data.end(), [&sum](int v) {
        sum.fetch_add(v, std::memory_order_relaxed);
    });

    // sum(0..9999) = 9999 * 10000 / 2
    EXPECT_EQ(sum.load(), 9999 * 10000 / 2);
}

TEST(ParallelForEachTests, SerialFallback)
{
    // 低于阈值时退化为串行
    WorkPool pool(4);
    std::vector<int> data(100, 1);

    int sum = 0;
    parallel_for_each(pool, data.begin(), data.end(), [&sum](int v) {
        sum += v; // 串行安全 — 低于阈值不会并行
    });

    EXPECT_EQ(sum, 100);
}

TEST(ParallelForEachTests, EmptyRange)
{
    WorkPool pool(4);
    std::vector<int> data;
    bool called = false;
    parallel_for_each(pool, data.begin(), data.end(), [&called](int) {
        called = true;
    });
    EXPECT_FALSE(called);
}

TEST(ParallelForEachTests, ExceptionPropagation)
{
    WorkPool pool(4);
    std::vector<int> data(10000, 0);

    EXPECT_THROW(
        parallel_for_each(pool, data.begin(), data.end(), [](int& v) {
            if (&v - &*(&v) == 0) { // 总是执行
                throw std::runtime_error("test");
            }
        }),
        std::runtime_error
    );
}

// =========================================================================
// 取消支持
// =========================================================================

TEST(ParallelForEachTests, CancellationStopsWork)
{
    WorkPool pool(4);
    std::vector<int> data(10000, 0);
    std::stop_source ss;
    ss.request_stop();

    EXPECT_THROW(
        parallel_for_each(pool, data.begin(), data.end(), [](int&) {}, ss.get_token()),
        std::runtime_error
    );
}

// =========================================================================
// Ranges 接口
// =========================================================================

TEST(ParallelForEachTests, RangesBasic)
{
    WorkPool pool(4);
    std::vector<int> data(10000);
    std::iota(data.begin(), data.end(), 0);

    std::atomic<int> sum{0};
    parallel_for_each(pool, data, [&sum](int v) {
        sum.fetch_add(v, std::memory_order_relaxed);
    });

    EXPECT_EQ(sum.load(), 9999 * 10000 / 2);
}

TEST(ParallelForEachTests, RangesEmpty)
{
    WorkPool pool(4);
    std::vector<int> data;
    bool called = false;
    parallel_for_each(pool, data, [&called](int) {
        called = true;
    });
    EXPECT_FALSE(called);
}

TEST(ParallelForEachTests, RangesCancellation)
{
    WorkPool pool(4);
    std::vector<int> data(10000, 0);
    std::stop_source ss;
    ss.request_stop();

    EXPECT_THROW(
        parallel_for_each(pool, data, [](int&) {}, ss.get_token()),
        std::runtime_error
    );
}
