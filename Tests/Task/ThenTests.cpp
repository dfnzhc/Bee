/**
 * @File ThenTests.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/18
 * @Brief Task::then() continuation 测试。
 */

#include <gtest/gtest.h>
#include <string>
#include <thread>

#include "Task/Core/Scheduler.hpp"
#include "Task/Core/Task.hpp"
#include "Concurrency/Thread/WorkPool.hpp"

using namespace bee;

// ── 内联 then：值 → 值 ──

TEST(ThenTests, InlineThenIntToString)
{
    auto t = []() -> Task<int> { co_return 42; }();

    auto t2 = t.then([](int v) { return std::to_string(v); });

    EXPECT_EQ(t2.get(), "42");
}

// ── 内联 then：值 → void ──

TEST(ThenTests, InlineThenIntToVoid)
{
    int captured = 0;
    auto t = []() -> Task<int> { co_return 7; }();

    auto t2 = t.then([&captured](int v) { captured = v; });
    t2.get();

    EXPECT_EQ(captured, 7);
}

// ── 内联 then：void → 值 ──

TEST(ThenTests, InlineThenVoidToInt)
{
    auto t = []() -> Task<void> { co_return; }();

    auto t2 = t.then([] { return 99; });

    EXPECT_EQ(t2.get(), 99);
}

// ── 内联 then：void → void ──

TEST(ThenTests, InlineThenVoidToVoid)
{
    bool called = false;
    auto t = []() -> Task<void> { co_return; }();

    auto t2 = t.then([&called] { called = true; });
    t2.get();

    EXPECT_TRUE(called);
}

// ── then 链式调用 ──

TEST(ThenTests, ChainedThen)
{
    auto t = []() -> Task<int> { co_return 1; }();

    auto t2 = t.then([](int v) { return v + 10; })
                .then([](int v) { return v * 2; })
                .then([](int v) { return std::to_string(v); });

    EXPECT_EQ(t2.get(), "22"); // (1 + 10) * 2 = 22
}

// ── 异常传播 ──

TEST(ThenTests, ThenPropagatesExceptionFromPredecessor)
{
    auto t = []() -> Task<int> {
        throw std::runtime_error("pred fail");
        co_return 0;
    }();

    auto t2 = t.then([](int v) { return v + 1; });

    EXPECT_THROW(t2.get(), std::runtime_error);
}

TEST(ThenTests, ThenPropagatesExceptionFromContinuation)
{
    auto t = []() -> Task<int> { co_return 5; }();

    auto t2 = t.then([](int) -> int { throw std::logic_error("cont fail"); });

    EXPECT_THROW(t2.get(), std::logic_error);
}

// ── Scheduler then ──

TEST(ThenTests, SchedulerThenRunsOnWorkerThread)
{
    WorkPool pool(2);
    auto main_tid = std::this_thread::get_id();

    auto t = []() -> Task<int> { co_return 10; }();

    auto t2 = t.then(pool, [main_tid](int v) -> std::pair<int, bool> {
        return {v * 3, std::this_thread::get_id() != main_tid};
    });

    auto [result, on_worker] = t2.get();
    EXPECT_EQ(result, 30);
    EXPECT_TRUE(on_worker);

    pool.shutdown();
}

// ── Scheduler then void ──

TEST(ThenTests, SchedulerThenVoidToVoid)
{
    WorkPool pool(2);
    std::atomic<bool> ran{false};

    auto t = []() -> Task<void> { co_return; }();

    auto t2 = t.then(pool, [&ran] { ran.store(true, std::memory_order_release); });
    t2.get();

    EXPECT_TRUE(ran.load());
    pool.shutdown();
}

// ── Scheduler then 链式 ──

TEST(ThenTests, SchedulerChainedThen)
{
    WorkPool pool(2);

    auto t = []() -> Task<int> { co_return 5; }();

    auto t2 = t.then(pool, [](int v) { return v * 10; })
                .then([](int v) { return v + 1; });

    EXPECT_EQ(t2.get(), 51); // 5 * 10 + 1
    pool.shutdown();
}
