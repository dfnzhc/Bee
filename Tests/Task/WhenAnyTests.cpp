/**
 * @File WhenAnyTests.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/18
 * @Brief when_any() 组合子测试。
 */

#include <gtest/gtest.h>
#include <chrono>
#include <thread>
#include <vector>

#include "Task/Core/WhenAny.hpp"
#include "Task/Core/Scheduler.hpp"
#include "Concurrency/Thread/WorkPool.hpp"

using namespace bee;

// ── 基础：第一个完成的获胜 ──

TEST(WhenAnyTests, FirstToCompleteWins)
{
    WorkPool pool(4);

    std::vector<Task<int>> tasks;
    // 快任务
    tasks.push_back(spawn_task(pool, [] { return 42; }));
    // 慢任务
    tasks.push_back(spawn_task(pool, [] {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        return 99;
    }));

    auto result = when_any(std::move(tasks)).get();
    // 快任务应该先完成（但不保证，所以只检查结果有效）
    EXPECT_TRUE(result.index == 0 || result.index == 1);
    EXPECT_TRUE(result.value == 42 || result.value == 99);

    pool.shutdown();
}

// ── 单任务 ──

TEST(WhenAnyTests, SingleTask)
{
    WorkPool pool(2);

    std::vector<Task<int>> tasks;
    tasks.push_back(spawn_task(pool, [] { return 7; }));

    auto result = when_any(std::move(tasks)).get();
    EXPECT_EQ(result.index, 0u);
    EXPECT_EQ(result.value, 7);

    pool.shutdown();
}

// ── 异常传播（获胜者抛异常）──

TEST(WhenAnyTests, WinnerExceptionPropagated)
{
    WorkPool pool(2);

    std::vector<Task<int>> tasks;
    // 快任务抛异常
    tasks.push_back(spawn_task(pool, []() -> int { throw std::runtime_error("any fail"); }));
    // 慢任务正常
    tasks.push_back(spawn_task(pool, [] {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        return 1;
    }));

    // 获胜者可能是抛异常的那个，也可能是正常的
    auto combined = when_any(std::move(tasks));
    try {
        auto result = combined.get();
        // 如果正常任务先完成，结果应该有效
        EXPECT_EQ(result.value, 1);
    }
    catch (const std::runtime_error& e) {
        // 如果异常任务先完成
        EXPECT_STREQ(e.what(), "any fail");
    }

    pool.shutdown();
}

// ── 多任务全部异常 ──

TEST(WhenAnyTests, AllTasksFail)
{
    WorkPool pool(2);

    std::vector<Task<int>> tasks;
    tasks.push_back(spawn_task(pool, []() -> int { throw std::runtime_error("fail1"); }));
    tasks.push_back(spawn_task(pool, []() -> int { throw std::runtime_error("fail2"); }));

    EXPECT_THROW(when_any(std::move(tasks)).get(), std::runtime_error);

    pool.shutdown();
}

// ── 大规模竞争 ──

TEST(WhenAnyTests, LargeRace)
{
    WorkPool pool(8);
    constexpr int N = 100;

    std::vector<Task<int>> tasks;
    tasks.reserve(N);
    for (int i = 0; i < N; ++i) {
        tasks.push_back(spawn_task(pool, [i] { return i; }));
    }

    auto result = when_any(std::move(tasks)).get();
    EXPECT_GE(result.index, 0u);
    EXPECT_LT(result.index, static_cast<size_t>(N));
    EXPECT_EQ(result.value, static_cast<int>(result.index));

    pool.shutdown();
}

// ── 同步任务竞争 ──

TEST(WhenAnyTests, SynchronousTasksRace)
{
    // 纯同步协程——不需要 WorkPool
    std::vector<Task<int>> tasks;
    for (int i = 0; i < 5; ++i) {
        auto t = [](int v) -> Task<int> { co_return v; }(i * 10);
        tasks.push_back(std::move(t));
    }

    auto result = when_any(std::move(tasks)).get();
    // 某个任务会获胜
    EXPECT_EQ(result.value, static_cast<int>(result.index) * 10);
}
