/**
 * @File TaskBasicTests.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/18
 * @Brief Task V2 基础生命周期测试。
 */

#include <gtest/gtest.h>
#include <chrono>
#include <thread>
#include "Task/Core/Scheduler.hpp"
#include "Task/Core/Task.hpp"
#include "Concurrency/Thread/WorkPool.hpp"

using namespace bee;

// ── 辅助协程 ──

static auto coro_return_int(int v) -> Task<int>
{
    co_return v;
}

static auto coro_return_void() -> Task<void>
{
    co_return;
}

static auto coro_return_string(std::string s) -> Task<std::string>
{
    co_return std::move(s);
}

// ── 默认构造 ──

TEST(TaskBasicTests, DefaultConstructIsEmpty)
{
    Task<int> t;
    EXPECT_FALSE(t.is_ready());
    EXPECT_FALSE(static_cast<bool>(t));
}

// ── 惰性：创建后处于 Pending 状态 ──

TEST(TaskBasicTests, LazyNotStartedOnCreation)
{
    auto t = coro_return_int(42);
    EXPECT_EQ(t.state(), TaskState::Pending);
    EXPECT_FALSE(t.is_ready());
    // 未调用 get/wait，析构应安全
}

// ── get() 启动并返回结果 ──

TEST(TaskBasicTests, GetReturnsValue)
{
    auto t = coro_return_int(42);
    EXPECT_EQ(t.get(), 42);
}

TEST(TaskBasicTests, GetVoidCompletes)
{
    auto t = coro_return_void();
    t.get(); // 不应抛异常
}

TEST(TaskBasicTests, GetString)
{
    auto t = coro_return_string("hello");
    EXPECT_EQ(t.get(), "hello");
}

// ── 异常传播 ──

TEST(TaskBasicTests, GetPropagatesException)
{
    auto t = []() -> Task<int> {
        throw std::runtime_error("boom");
        co_return 0;
    }();

    EXPECT_THROW(t.get(), std::runtime_error);
}

// ── wait() ──

TEST(TaskBasicTests, WaitBlocksUntilComplete)
{
    auto t = coro_return_int(99);
    t.wait(); // 消费 Task
    // wait() 后 Task 已空
    EXPECT_FALSE(static_cast<bool>(t));
}

TEST(TaskBasicTests, WaitPropagatesException)
{
    auto t = []() -> Task<int> {
        throw std::logic_error("logic fail");
        co_return 0;
    }();

    EXPECT_THROW(t.wait(), std::logic_error);
}

// ── wait_for() ──

TEST(TaskBasicTests, WaitForReturnsCompletedImmediately)
{
    auto t = coro_return_int(1);
    // 同步协程：wait_for 会先启动再等
    auto st = t.wait_for(std::chrono::seconds(5));
    EXPECT_EQ(st, TaskState::Completed);
    // wait_for 不消费 Task，后续仍可 get()
    EXPECT_EQ(t.get(), 1);
}

// ── get/wait 消费语义 ──

TEST(TaskBasicTests, GetConsumesTask)
{
    auto t = coro_return_int(42);
    EXPECT_EQ(t.get(), 42);
    // get() 后 Task 已空
    EXPECT_FALSE(static_cast<bool>(t));
}

TEST(TaskBasicTests, WaitConsumesTask)
{
    auto t = coro_return_void();
    t.wait();
    EXPECT_FALSE(static_cast<bool>(t));
}

// ── 移动语义 ──

TEST(TaskBasicTests, MoveConstructTransfersOwnership)
{
    auto t1 = coro_return_int(7);
    auto t2 = std::move(t1);

    // t1 已空
    EXPECT_FALSE(static_cast<bool>(t1));
    EXPECT_EQ(t2.get(), 7);
}

TEST(TaskBasicTests, MoveAssignTransfersOwnership)
{
    auto t1 = coro_return_int(10);
    Task<int> t2;
    t2 = std::move(t1);

    EXPECT_FALSE(static_cast<bool>(t1));
    EXPECT_EQ(t2.get(), 10);
}

// ── 未启动 Task 安全析构 ──

TEST(TaskBasicTests, UnstartedTaskDestroySafely)
{
    { auto t = coro_return_int(1); }
    { auto t = coro_return_void(); }
    // 无崩溃即通过
}

// ── wait_for 超时场景 ──

TEST(TaskBasicTests, WaitForTimesOutOnSlowTask)
{
    WorkPool pool(1);

    auto t = spawn_task(pool, [] {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        return 42;
    });

    // 极短超时 — 任务不可能在 1ms 内完成
    auto st = t.wait_for(std::chrono::milliseconds(1));
    EXPECT_EQ(st, TaskState::Running);

    // 后续 get() 仍能正确获取结果
    EXPECT_EQ(t.get(), 42);
    pool.shutdown();
}

TEST(TaskBasicTests, WaitForSucceedsWithLongTimeout)
{
    WorkPool pool(1);

    auto t = spawn_task(pool, [] { return 7; });

    auto st = t.wait_for(std::chrono::seconds(5));
    EXPECT_EQ(st, TaskState::Completed);

    // wait_for 不消费 Task
    EXPECT_EQ(t.get(), 7);
    pool.shutdown();
}
