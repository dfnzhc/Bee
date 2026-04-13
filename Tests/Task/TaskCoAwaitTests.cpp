/**
 * @File TaskCoAwaitTests.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/13
 * @Brief This file is part of Bee.
 */

#include <gtest/gtest.h>

#include <atomic>
#include <stdexcept>
#include <string>
#include <thread>
#include <variant>
#include <vector>

#include "Task/Task.hpp"

namespace
{

using bee::AsyncTask;
using bee::Task;
using bee::TaskState;

// =========================================================================
// co_await Task<T> 测试
// =========================================================================

TEST(TaskCoAwaitTests, CoAwaitSubmitInt)
{
    bee::ThreadPool pool(2);

    auto coro = [&pool]() -> AsyncTask<int> {
        int x = co_await bee::submit(pool, [] {
            return 42;
        });
        co_return x;
    };

    EXPECT_EQ(coro().get(), 42);
}

TEST(TaskCoAwaitTests, CoAwaitSubmitVoid)
{
    bee::ThreadPool pool(2);
    std::atomic<bool> executed{false};

    auto coro = [&pool, &executed]() -> AsyncTask<void> {
        co_await bee::submit(pool, [&executed] {
            executed.store(true);
        });
        co_return;
    };

    coro().get();
    EXPECT_TRUE(executed.load());
}

TEST(TaskCoAwaitTests, CoAwaitFailedTask)
{
    bee::ThreadPool pool(2);

    auto coro = [&pool]() -> AsyncTask<int> {
        int x = co_await bee::submit(pool, []() -> int {
            throw std::runtime_error("fail");
        });
        co_return x;
    };

    EXPECT_THROW(coro().get(), std::runtime_error);
}

TEST(TaskCoAwaitTests, CoAwaitCancelledTask)
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

    EXPECT_THROW(task.get(), std::runtime_error);
}

TEST(TaskCoAwaitTests, CoAwaitAlreadyCompleted)
{
    bee::ThreadPool pool(2);

    auto coro = [&pool]() -> AsyncTask<int> {
        auto task = bee::submit(pool, [] {
            return 7;
        });
        task.wait(); // 确保在 co_await 前已完成
        int x = co_await std::move(task);
        co_return x;
    };

    EXPECT_EQ(coro().get(), 7);
}

TEST(TaskCoAwaitTests, CoAwaitWhenAll)
{
    bee::ThreadPool pool(4);

    auto coro = [&pool]() -> AsyncTask<int> {
        auto t1 = bee::submit(pool, [] {
            return 10;
        });
        auto t2 = bee::submit(pool, [] {
            return 20;
        });
        auto [a, b] = co_await bee::when_all(std::move(t1), std::move(t2));
        co_return a + b;
    };

    EXPECT_EQ(coro().get(), 30);
}

TEST(TaskCoAwaitTests, CoAwaitWhenAny)
{
    bee::ThreadPool pool(4);
    std::stop_source source;

    auto coro = [&pool, &source]() -> AsyncTask<int> {
        std::vector<Task<int>> tasks;
        tasks.push_back(bee::submit(pool, [] {
            return 42;
        }, source.get_token()));
        tasks.push_back(bee::submit_cancellable(
                pool,
                [](std::stop_token token) -> int {
                    while (!token.stop_requested())
                        std::this_thread::yield();
                    return -1;
                },
                source));

        auto result = co_await bee::when_any(source, std::move(tasks));
        co_return result.value;
    };

    EXPECT_EQ(coro().get(), 42);
}

TEST(TaskCoAwaitTests, MultipleCoAwaitsInChain)
{
    bee::ThreadPool pool(2);

    auto coro = [&pool]() -> AsyncTask<std::string> {
        int a = co_await bee::submit(pool, [] {
            return 10;
        });
        int b = co_await bee::submit(pool, [a] {
            return a * 2;
        });
        auto text = co_await bee::submit(pool, [b] {
            return std::to_string(b);
        });
        co_return text;
    };

    EXPECT_EQ(coro().get(), "20");
}

} // namespace
