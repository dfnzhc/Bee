/**
 * @File SchedulerTests.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/18
 * @Brief Scheduler concept + spawn_task + co_await 测试。
 */

#include <gtest/gtest.h>
#include <atomic>
#include <thread>

#include "Task/Core/Scheduler.hpp"
#include "Task/Core/Task.hpp"
#include "Concurrency/Thread/WorkPool.hpp"

using namespace bee;

// ── Scheduler concept 静态验证 ──

static_assert(Scheduler<WorkPool>, "WorkPool must satisfy Scheduler concept");

// ── schedule() 基础 ──

TEST(SchedulerTests, ScheduleTransfersToWorkerThread)
{
    WorkPool pool(2);

    auto main_tid = std::this_thread::get_id();

    auto t = [](WorkPool& p, std::thread::id caller_tid) -> Task<bool> {
        co_await p.schedule();
        co_return std::this_thread::get_id() != caller_tid;
    }(pool, main_tid);

    EXPECT_TRUE(t.get());
    pool.shutdown();
}

// ── spawn_task ──

TEST(SchedulerTests, SpawnTaskReturnsValue)
{
    WorkPool pool(2);

    auto t = spawn_task(pool, [] { return 42; });
    EXPECT_EQ(t.get(), 42);

    pool.shutdown();
}

TEST(SchedulerTests, SpawnTaskVoid)
{
    WorkPool          pool(2);
    std::atomic<bool> executed{false};

    auto t = spawn_task(pool, [&] { executed.store(true, std::memory_order_release); });
    t.get();

    EXPECT_TRUE(executed.load(std::memory_order_acquire));
    pool.shutdown();
}

TEST(SchedulerTests, SpawnTaskPropagatesException)
{
    WorkPool pool(2);

    auto t = spawn_task(pool, []() -> int { throw std::runtime_error("spawn boom"); });
    EXPECT_THROW(t.get(), std::runtime_error);

    pool.shutdown();
}

TEST(SchedulerTests, SpawnTaskRunsOnWorkerThread)
{
    WorkPool pool(2);
    auto     main_tid = std::this_thread::get_id();

    auto t = spawn_task(pool, [main_tid] { return std::this_thread::get_id() != main_tid; });
    EXPECT_TRUE(t.get());

    pool.shutdown();
}

// ── co_await 链式调用 ──

TEST(SchedulerTests, CoAwaitChainedTasks)
{
    WorkPool pool(2);

    auto inner = [](WorkPool& p) -> Task<int> {
        co_await p.schedule();
        co_return 10;
    };

    auto outer = [&](WorkPool& p) -> Task<int> {
        int a = co_await inner(p);
        int b = co_await inner(p);
        co_return a + b;
    };

    EXPECT_EQ(outer(pool).get(), 20);
    pool.shutdown();
}

TEST(SchedulerTests, NestedCoAwait)
{
    WorkPool pool(4);

    auto leaf = [](WorkPool& p, int v) -> Task<int> {
        co_await p.schedule();
        co_return v * 2;
    };

    auto mid = [&](WorkPool& p) -> Task<int> {
        int a = co_await leaf(p, 3);
        int b = co_await leaf(p, 7);
        co_return a + b;
    };

    auto root = [&](WorkPool& p) -> Task<int> {
        int m = co_await mid(p);
        co_return m + 100;
    };

    EXPECT_EQ(root(pool).get(), 120); // (3*2 + 7*2) + 100
    pool.shutdown();
}

// ── 多任务并发 ──

TEST(SchedulerTests, MultipleConcurrentSpawnTasks)
{
    WorkPool         pool(4);
    constexpr int    N = 100;
    std::atomic<int> sum{0};

    std::vector<Task<void>> tasks;
    tasks.reserve(N);
    for (int i = 0; i < N; ++i) {
        tasks.push_back(spawn_task(pool, [&sum, i] { sum.fetch_add(i, std::memory_order_relaxed); }));
    }

    for (auto& t : tasks) {
        t.get();
    }

    EXPECT_EQ(sum.load(), N * (N - 1) / 2);
    pool.shutdown();
}
