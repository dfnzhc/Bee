/**
 * @File AsyncScopeTests.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/18
 * @Brief AsyncScope 动态 spawn 管理测试。
 */

#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

#include "Task/Core/AsyncScope.hpp"
#include "Task/Core/Scheduler.hpp"
#include "Concurrency/Thread/WorkPool.hpp"

using namespace bee;

// ── 基础 spawn + join ──

TEST(AsyncScopeTests, SpawnAndJoin)
{
    WorkPool pool(2);
    AsyncScope scope;

    std::atomic<int> counter{0};

    scope.spawn(pool, spawn_task(pool, [&] { counter.fetch_add(1); }));
    scope.spawn(pool, spawn_task(pool, [&] { counter.fetch_add(1); }));
    scope.spawn(pool, spawn_task(pool, [&] { counter.fetch_add(1); }));

    scope.join();
    EXPECT_EQ(counter.load(), 3);

    pool.shutdown();
}

// ── join() 重复调用安全 ──

TEST(AsyncScopeTests, JoinIdempotent)
{
    WorkPool pool(2);
    AsyncScope scope;

    scope.spawn(pool, spawn_task(pool, [] {}));
    scope.join();
    scope.join(); // 第二次调用不应阻塞或崩溃

    pool.shutdown();
}

// ── 析构时自动 join ──

TEST(AsyncScopeTests, DestructorAutoJoins)
{
    WorkPool pool(2);
    std::atomic<int> counter{0};

    {
        AsyncScope scope;
        for (int i = 0; i < 10; ++i) {
            scope.spawn(pool, spawn_task(pool, [&] { counter.fetch_add(1); }));
        }
        // scope 析构时自动 join
    }

    EXPECT_EQ(counter.load(), 10);
    pool.shutdown();
}

// ── pending_count / is_idle ──

TEST(AsyncScopeTests, PendingCountTracking)
{
    AsyncScope scope;
    EXPECT_EQ(scope.pending_count(), 0u);
    EXPECT_TRUE(scope.is_idle());
}

// ── 异常收集 ──

TEST(AsyncScopeTests, JoinRethrowsFirstException)
{
    WorkPool pool(2);
    AsyncScope scope;

    scope.spawn(pool, spawn_task(pool, [] {}));
    scope.spawn(pool, spawn_task(pool, []() -> void { throw std::runtime_error("scope fail"); }));

    EXPECT_THROW(scope.join(), std::runtime_error);
    pool.shutdown();
}

// ── request_stop ──

TEST(AsyncScopeTests, RequestStopSetsToken)
{
    WorkPool pool(2);
    AsyncScope scope;

    scope.spawn(pool, [](WorkPool& p) -> Task<void> {
        co_await p.schedule();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }(pool));

    scope.request_stop();
    // scope 析构自动 join
    pool.shutdown();
}

// ── 大规模 spawn ──

TEST(AsyncScopeTests, LargeScaleSpawn)
{
    WorkPool pool(8);
    AsyncScope scope;
    constexpr int N = 1000;

    std::atomic<int> sum{0};
    for (int i = 0; i < N; ++i) {
        scope.spawn(pool, spawn_task(pool, [&sum, i] { sum.fetch_add(i); }));
    }

    scope.join();
    EXPECT_EQ(sum.load(), N * (N - 1) / 2);

    pool.shutdown();
}

// ── join 后可再次 spawn ──

TEST(AsyncScopeTests, SpawnAfterJoin)
{
    WorkPool pool(2);
    AsyncScope scope;

    std::atomic<int> counter{0};

    scope.spawn(pool, spawn_task(pool, [&] { counter.fetch_add(1); }));
    scope.join();
    EXPECT_EQ(counter.load(), 1);

    scope.spawn(pool, spawn_task(pool, [&] { counter.fetch_add(1); }));
    scope.join();
    EXPECT_EQ(counter.load(), 2);

    pool.shutdown();
}
