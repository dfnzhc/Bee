/**
 * @File ParallelForTests.cpp
 * @Brief Base::Parallel::parallel_for 的基础正确性与并发行为测试。
 */

#include <gtest/gtest.h>

#include <atomic>
#include <numeric>
#include <stdexcept>
#include <thread>
#include <unordered_set>
#include <vector>

#include "Base/Parallel.hpp"

using bee::parallel::parallel_for;
using bee::parallel::parallel_for_each;

TEST(ParallelFor, EmptyRangeIsNoop)
{
    int called = 0;
    parallel_for(10, 10, 1, [&](std::size_t, std::size_t) { ++called; });
    EXPECT_EQ(called, 0);

    parallel_for(10, 5, 1, [&](std::size_t, std::size_t) { ++called; });
    EXPECT_EQ(called, 0);
}

TEST(ParallelFor, SmallRangeRunsSerialOnce)
{
    std::atomic<int> invocations{0};
    parallel_for(0, 4, 64, [&](std::size_t lo, std::size_t hi) {
        ++invocations;
        EXPECT_EQ(lo, 0u);
        EXPECT_EQ(hi, 4u);
    });
    EXPECT_EQ(invocations.load(), 1);
}

TEST(ParallelFor, CoversExactlyOnceAndDisjoint)
{
    constexpr std::size_t         N = 100'003;
    std::vector<std::atomic<int>> hit(N);
    for (auto& x : hit)
        x.store(0);

    parallel_for(0, N, 1024, [&](std::size_t lo, std::size_t hi) {
        for (std::size_t i = lo; i < hi; ++i)
            hit[i].fetch_add(1, std::memory_order_relaxed);
    });

    for (std::size_t i = 0; i < N; ++i)
        ASSERT_EQ(hit[i].load(), 1) << "index " << i;
}

TEST(ParallelFor, ParallelSumMatchesSerial)
{
    constexpr std::size_t N = 1'000'000;
    std::vector<int>      data(N);
    std::iota(data.begin(), data.end(), 1);

    std::atomic<long long> sum{0};
    parallel_for(0, N, 2048, [&](std::size_t lo, std::size_t hi) {
        long long local = 0;
        for (std::size_t i = lo; i < hi; ++i)
            local += data[i];
        sum.fetch_add(local, std::memory_order_relaxed);
    });

    const long long expected = static_cast<long long>(N) * (N + 1) / 2;
    EXPECT_EQ(sum.load(), expected);
}

TEST(ParallelForEach, PerIndexCoversAll)
{
    constexpr std::size_t         N = 50'000;
    std::vector<std::atomic<int>> hit(N);
    for (auto& x : hit)
        x.store(0);

    parallel_for_each(0, N, 512, [&](std::size_t i) { hit[i].fetch_add(1, std::memory_order_relaxed); });

    for (std::size_t i = 0; i < N; ++i)
        ASSERT_EQ(hit[i].load(), 1);
}

TEST(ParallelFor, UsesMultipleThreadsWhenLargeEnough)
{
    const std::size_t workers = bee::parallel::available_parallelism();
    if (workers <= 1) {
        GTEST_SKIP() << "单核环境跳过并发线程观测";
    }

    constexpr std::size_t               N = 200'000;
    std::mutex                          mu;
    std::unordered_set<std::thread::id> tids;

    parallel_for(0, N, 1024, [&](std::size_t, std::size_t) {
        std::lock_guard lk(mu);
        tids.insert(std::this_thread::get_id());
    });

    EXPECT_GT(tids.size(), 1u) << "期望在大区间上看到至少两个不同线程";
}

TEST(ParallelFor, ExceptionPropagates)
{
    constexpr std::size_t N = 10'000;
    EXPECT_THROW(parallel_for(0, N, 256, [](std::size_t, std::size_t) { throw std::runtime_error("boom"); }), std::runtime_error);
}
