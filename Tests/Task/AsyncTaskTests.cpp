/**
 * @File AsyncTaskTests.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/13
 * @Brief This file is part of Bee.
 */

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>

#include "Task/Task.hpp"

namespace
{

using bee::AsyncTask;
using bee::Task;

// =========================================================================
// 基础测试
// =========================================================================

TEST(AsyncTaskTests, LazyNotStarted)
{
    std::atomic<bool> started{false};
    auto coro = [&started]() -> AsyncTask<int> {
        started.store(true);
        co_return 42;
    };

    auto task = coro();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    EXPECT_FALSE(started.load());

    auto result = task.get();
    EXPECT_TRUE(started.load());
    EXPECT_EQ(result, 42);
}

TEST(AsyncTaskTests, GetStartsAndReturns)
{
    bee::ThreadPool pool(2);

    auto coro = [&pool]() -> AsyncTask<int> {
        int x = co_await bee::submit(pool, [] {
            return 42;
        });
        co_return x;
    };

    auto task = coro();
    EXPECT_EQ(task.get(), 42);
}

TEST(AsyncTaskTests, GetVoidTask)
{
    std::atomic<bool> executed{false};

    auto coro = [&executed]() -> AsyncTask<void> {
        executed.store(true);
        co_return;
    };

    auto task = coro();
    task.get();
    EXPECT_TRUE(executed.load());
}

TEST(AsyncTaskTests, SequentialCoAwaits)
{
    bee::ThreadPool pool(2);

    auto coro = [&pool]() -> AsyncTask<int> {
        int a = co_await bee::submit(pool, [] {
            return 10;
        });
        int b = co_await bee::submit(pool, [] {
            return 20;
        });
        int c = co_await bee::submit(pool, [] {
            return 30;
        });
        co_return a + b + c;
    };

    EXPECT_EQ(coro().get(), 60);
}

// =========================================================================
// 错误测试
// =========================================================================

TEST(AsyncTaskTests, ExceptionInBody)
{
    auto coro = []() -> AsyncTask<int> {
        throw std::runtime_error("oops");
        co_return 0; // 不可达但协程需要
    };

    EXPECT_THROW(coro().get(), std::runtime_error);
}

TEST(AsyncTaskTests, CoAwaitFailedTask)
{
    bee::ThreadPool pool(2);

    auto coro = [&pool]() -> AsyncTask<int> {
        int x = co_await bee::submit(pool, []() -> int {
            throw std::logic_error("bad");
        });
        co_return x;
    };

    EXPECT_THROW(coro().get(), std::logic_error);
}

TEST(AsyncTaskTests, CoAwaitCancelledTask)
{
    bee::ThreadPool pool(1);

    std::atomic<bool> unblock{false};
    auto blocker = bee::submit(pool, [&unblock] {
        while (!unblock.load(std::memory_order_acquire))
            std::this_thread::yield();
    });

    std::stop_source source;
    source.request_stop();

    auto coro = [&pool, &source]() -> AsyncTask<int> {
        int x = co_await bee::submit(pool, [] {
            return 1;
        }, source.get_token());
        co_return x;
    };

    auto task = coro();

    unblock.store(true, std::memory_order_release);
    blocker.wait();

    EXPECT_THROW(task.get(), std::runtime_error); // "Task was cancelled"
}

// =========================================================================
// 嵌套 / 组合测试
// =========================================================================

TEST(AsyncTaskTests, NestedAsyncTasks)
{
    bee::ThreadPool pool(2);

    auto inner = [&pool]() -> AsyncTask<int> {
        int x = co_await bee::submit(pool, [] {
            return 7;
        });
        co_return x * 2;
    };

    auto outer = [&pool, &inner]() -> AsyncTask<int> {
        int a = co_await inner();
        int b = co_await bee::submit(pool, [] {
            return 3;
        });
        co_return a + b;
    };

    EXPECT_EQ(outer().get(), 17); // 7*2 + 3
}

TEST(AsyncTaskTests, MoveOnlyResult)
{
    auto coro = []() -> AsyncTask<std::unique_ptr<int>> {
        co_return std::make_unique<int>(42);
    };

    auto result = coro().get();
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(*result, 42);
}

// =========================================================================
// 生命周期测试
// =========================================================================

TEST(AsyncTaskTests, MoveConstruct)
{
    auto coro = []() -> AsyncTask<int> {
        co_return 99;
    };

    auto task1 = coro();
    EXPECT_TRUE(static_cast<bool>(task1));

    auto task2 = std::move(task1);
    EXPECT_FALSE(static_cast<bool>(task1));
    EXPECT_TRUE(static_cast<bool>(task2));
    EXPECT_EQ(task2.get(), 99);
}

TEST(AsyncTaskTests, DestroyWithoutConsuming)
{
    std::atomic<bool> started{false};

    auto coro = [&started]() -> AsyncTask<int> {
        started.store(true);
        co_return 42;
    };

    {
        auto task = coro();
        // 让 task 在未调用 get() 或 co_await 的情况下离开作用域。
    }
    // 不应崩溃。协程是惰性的，所以从未启动。
    EXPECT_FALSE(started.load());
}

TEST(AsyncTaskTests, IsReadyAfterCompletion)
{
    auto coro = []() -> AsyncTask<int> {
        co_return 1;
    };

    auto task = coro();
    EXPECT_FALSE(task.is_ready());

    task.get();
    EXPECT_TRUE(task.is_ready());
}

} // namespace
