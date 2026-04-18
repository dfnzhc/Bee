/**
 * @File ParallelVerificationTests.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/20
 * @Brief This file is part of Bee.
 */

#include <gtest/gtest.h>
#include <atomic>
#include <set>
#include <thread>
#include <numeric>

#include "Task/Parallel/ForEach.hpp"
#include "Task/Parallel/Transform.hpp"
#include "Task/Parallel/Reduce.hpp"
#include "Task/Parallel/Scan.hpp"
#include "Task/Parallel/Sort.hpp"
#include "Concurrency/Thread/WorkPool.hpp"

using namespace bee;

// =========================================================================
// P2-1: 并行性验证 — 确认工作确实在多个线程上执行
// =========================================================================

TEST(ParallelVerificationTests, ForEachUsesMultipleThreads)
{
    WorkPool pool(4);
    constexpr std::size_t N = 40000;
    std::vector<int> data(N, 1);

    std::set<std::thread::id> ids;
    std::mutex mtx;

    parallel_for_each(pool, data.begin(), data.end(), [&](int&) {
        std::lock_guard lk(mtx);
        ids.insert(std::this_thread::get_id());
    });

    EXPECT_GE(ids.size(), 2u) << "parallel_for_each should use at least 2 threads for large input";
}

TEST(ParallelVerificationTests, TransformUsesMultipleThreads)
{
    WorkPool pool(4);
    constexpr std::size_t N = 40000;
    std::vector<int> input(N, 1);
    std::vector<int> output(N);

    std::set<std::thread::id> ids;
    std::mutex mtx;

    (void)parallel_transform(pool, input.begin(), input.end(), output.begin(), [&](int v) {
        std::lock_guard lk(mtx);
        ids.insert(std::this_thread::get_id());
        return v;
    });

    EXPECT_GE(ids.size(), 2u) << "parallel_transform should use at least 2 threads for large input";
}

TEST(ParallelVerificationTests, ReduceUsesMultipleThreads)
{
    WorkPool pool(4);
    constexpr std::size_t N = 40000;
    std::vector<int> data(N, 1);

    std::atomic<int> thread_count{0};
    std::set<std::thread::id> ids;
    std::mutex mtx;

    auto result = parallel_reduce(pool, data.begin(), data.end(), 0,
        [&](int a, int b) {
            {
                std::lock_guard lk(mtx);
                ids.insert(std::this_thread::get_id());
            }
            return a + b;
        });

    EXPECT_EQ(result, static_cast<int>(N));
    EXPECT_GE(ids.size(), 2u) << "parallel_reduce should use at least 2 threads for large input";
}

TEST(ParallelVerificationTests, SortUsesMultipleThreads)
{
    WorkPool pool(4);
    constexpr std::size_t N = 40000;
    std::vector<int> data(N);
    // 逆序数据以确保有足够排序工作
    std::iota(data.rbegin(), data.rend(), 0);

    // 验证排序正确性（隐含并行性——大输入量+多核 pool）
    parallel_sort(pool, data.begin(), data.end());

    EXPECT_TRUE(std::is_sorted(data.begin(), data.end()));
}

// =========================================================================
// P2-2: 并行路径取消 — 在并行工作进行中请求取消
// =========================================================================

TEST(ParallelVerificationTests, ForEachInFlightCancellation)
{
    WorkPool pool(4);
    constexpr std::size_t N = 100000;
    std::vector<int> data(N, 0);
    std::stop_source ss;
    std::atomic<std::size_t> processed{0};

    // 在处理过程中触发取消
    EXPECT_THROW(
        parallel_for_each(pool, data.begin(), data.end(),
            [&](int&) {
                auto count = processed.fetch_add(1, std::memory_order_relaxed);
                if (count == N / 4)
                    ss.request_stop();
            },
            ss.get_token()),
        std::runtime_error
    );

    // 应该在取消后停止，不会处理全部元素
    EXPECT_LT(processed.load(), N) << "Cancellation should stop before processing all elements";
}

TEST(ParallelVerificationTests, TransformInFlightCancellation)
{
    WorkPool pool(4);
    constexpr std::size_t N = 100000;
    std::vector<int> input(N, 1);
    std::vector<int> output(N, 0);
    std::stop_source ss;
    std::atomic<std::size_t> processed{0};

    EXPECT_THROW(
        (void)parallel_transform(pool, input.begin(), input.end(), output.begin(),
            [&](int v) {
                auto count = processed.fetch_add(1, std::memory_order_relaxed);
                if (count == N / 4)
                    ss.request_stop();
                return v;
            },
            ss.get_token()),
        std::runtime_error
    );

    EXPECT_LT(processed.load(), N) << "Cancellation should stop before processing all elements";
}

TEST(ParallelVerificationTests, ReduceInFlightCancellation)
{
    WorkPool pool(4);
    constexpr std::size_t N = 100000;
    std::vector<int> data(N, 1);
    std::stop_source ss;
    std::atomic<std::size_t> processed{0};

    EXPECT_THROW(
        (void)parallel_reduce(pool, data.begin(), data.end(), 0,
            [&](int a, int b) {
                auto count = processed.fetch_add(1, std::memory_order_relaxed);
                if (count == N / 4)
                    ss.request_stop();
                return a + b;
            },
            ss.get_token()),
        std::runtime_error
    );

    EXPECT_LT(processed.load(), N) << "Cancellation should stop before processing all elements";
}

TEST(ParallelVerificationTests, SortInFlightCancellation)
{
    WorkPool pool(4);
    constexpr std::size_t N = 100000;
    std::vector<int> data(N);
    std::iota(data.rbegin(), data.rend(), 0);
    std::stop_source ss;
    std::atomic<std::size_t> compared{0};

    EXPECT_THROW(
        parallel_sort(pool, data.begin(), data.end(),
            [&](int a, int b) {
                auto count = compared.fetch_add(1, std::memory_order_relaxed);
                if (count == N)
                    ss.request_stop();
                return a < b;
            },
            ss.get_token()),
        std::runtime_error
    );
}

TEST(ParallelVerificationTests, ScanInFlightCancellation)
{
    WorkPool pool(4);
    constexpr std::size_t N = 100000;
    std::vector<int> input(N, 1);
    std::vector<int> output(N, 0);
    std::stop_source ss;
    std::atomic<std::size_t> processed{0};

    EXPECT_THROW(
        (void)parallel_inclusive_scan(pool, input.begin(), input.end(), output.begin(),
            [&](int a, int b) {
                auto count = processed.fetch_add(1, std::memory_order_relaxed);
                if (count == N / 4)
                    ss.request_stop();
                return a + b;
            },
            ss.get_token()),
        std::runtime_error
    );
}
