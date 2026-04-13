/**
 * @File WhenAnyTests.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/13
 * @Brief This file is part of Bee.
 */

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <stdexcept>
#include <string>
#include <thread>
#include <variant>
#include <vector>

#include "Task/Task.hpp"

namespace
{

using bee::Task;
using bee::TaskState;
using bee::WhenAnyResult;

// =========================================================================
// Vector（同构）测试
// =========================================================================

TEST(WhenAnyTests, BasicFirstWins)
{
    bee::ThreadPool pool(4);
    std::stop_source source;

    std::vector<Task<int>> tasks;
    tasks.push_back(bee::submit(pool, [] {
        return 42;
    }, source.get_token()));
    for (int i = 0; i < 2; ++i) {
        tasks.push_back(bee::submit_cancellable(
                pool,
                [](std::stop_token token) -> int {
                    while (!token.stop_requested())
                        std::this_thread::yield();
                    return -1;
                },
                source));
    }

    auto combined = bee::when_any(source, std::move(tasks));
    auto result   = combined.get();

    EXPECT_EQ(result.index, 0u);
    EXPECT_EQ(result.value, 42);
}

TEST(WhenAnyTests, SingleTask)
{
    bee::ThreadPool pool(2);
    std::stop_source source;

    std::vector<Task<int>> tasks;
    tasks.push_back(bee::submit(pool, [] {
        return 77;
    }, source.get_token()));

    auto combined = bee::when_any(source, std::move(tasks));
    auto result   = combined.get();

    EXPECT_EQ(result.index, 0u);
    EXPECT_EQ(result.value, 77);
}

TEST(WhenAnyTests, FirstFailurePropagates)
{
    bee::ThreadPool pool(4);
    std::stop_source source;

    std::vector<Task<int>> tasks;
    tasks.push_back(
            bee::submit(pool, []() -> int {
                throw std::runtime_error("boom");
            }, source.get_token()));
    for (int i = 0; i < 2; ++i) {
        tasks.push_back(bee::submit_cancellable(
                pool,
                [](std::stop_token token) -> int {
                    while (!token.stop_requested())
                        std::this_thread::yield();
                    return -1;
                },
                source));
    }

    auto combined = bee::when_any(source, std::move(tasks));
    EXPECT_THROW(combined.get(), std::runtime_error);
    EXPECT_EQ(combined.state(), TaskState::Failed);
}

TEST(WhenAnyTests, RemainingTasksCancelled)
{
    bee::ThreadPool pool(4);
    std::stop_source source;

    std::atomic<bool> slow_saw_stop{false};
    std::atomic<bool> slow_started{false};

    std::vector<Task<int>> tasks;
    // 慢任务先入队——保证它在快任务运行前已进入自旋等待。
    tasks.push_back(bee::submit_cancellable(
            pool,
            [&slow_saw_stop, &slow_started](std::stop_token token) -> int {
                slow_started.store(true, std::memory_order_release);
                while (!token.stop_requested())
                    std::this_thread::yield();
                slow_saw_stop.store(true, std::memory_order_release);
                return -1;
            },
            source));
    // 快任务等待慢任务启动后再返回。
    tasks.push_back(bee::submit(
            pool,
            [&slow_started] {
                while (!slow_started.load(std::memory_order_acquire))
                    std::this_thread::yield();
                return 1;
            },
            source.get_token()));

    auto combined = bee::when_any(source, std::move(tasks));
    combined.get();

    EXPECT_TRUE(source.stop_requested());
    EXPECT_TRUE(slow_saw_stop.load(std::memory_order_acquire));
}

TEST(WhenAnyTests, AllExternallyCancelled)
{
    bee::ThreadPool pool(1);
    std::stop_source source;

    std::atomic<bool> unblock{false};
    auto blocker = bee::submit(pool, [&unblock] {
        while (!unblock.load(std::memory_order_acquire))
            std::this_thread::yield();
    });

    std::vector<Task<int>> tasks;
    for (int i = 0; i < 3; ++i) {
        tasks.push_back(bee::submit(pool, [i] {
            return i;
        }, source.get_token()));
    }

    source.request_stop();

    auto combined = bee::when_any(source, std::move(tasks));

    unblock.store(true, std::memory_order_release);
    blocker.wait();

    combined.wait();
    EXPECT_EQ(combined.state(), TaskState::Cancelled);
}

TEST(WhenAnyTests, CompletionAndFailureRace)
{
    bee::ThreadPool pool(4);
    std::stop_source source;

    std::vector<Task<int>> tasks;
    tasks.push_back(bee::submit(pool, [] {
        return 1;
    }, source.get_token()));
    tasks.push_back(
            bee::submit(pool, []() -> int {
                throw std::runtime_error("err");
            }, source.get_token()));

    auto combined = bee::when_any(source, std::move(tasks));

    try {
        auto result = combined.get();
        EXPECT_EQ(result.value, 1);
    } catch (const std::runtime_error&) {
        EXPECT_EQ(combined.state(), TaskState::Failed);
    }
}

TEST(WhenAnyTests, VectorLargeCount)
{
    bee::ThreadPool pool(4);
    std::stop_source source;

    std::vector<Task<int>> tasks;
    tasks.push_back(bee::submit(pool, [] {
        return 999;
    }, source.get_token()));
    for (int i = 0; i < 99; ++i) {
        tasks.push_back(bee::submit_cancellable(
                pool,
                [](std::stop_token token) -> int {
                    while (!token.stop_requested())
                        std::this_thread::yield();
                    return -1;
                },
                source));
    }

    auto combined = bee::when_any(source, std::move(tasks));
    auto result   = combined.get();

    EXPECT_EQ(result.index, 0u);
    EXPECT_EQ(result.value, 999);
}

// =========================================================================
// 变参（异构）测试
// =========================================================================

TEST(WhenAnyTests, VariadicHeterogeneousTypes)
{
    bee::ThreadPool pool(4);
    std::stop_source source;

    auto t1 = bee::submit(pool, [] {
        return 42;
    }, source.get_token());
    auto t2 = bee::submit_cancellable(
            pool,
            [](std::stop_token token) -> std::string {
                while (!token.stop_requested())
                    std::this_thread::yield();
                return "cancelled";
            },
            source);

    auto combined = bee::when_any(source, std::move(t1), std::move(t2));
    auto result   = combined.get();

    EXPECT_EQ(result.index(), 0u);
    EXPECT_EQ(std::get<0>(result), 42);
}

TEST(WhenAnyTests, VariadicSingleTask)
{
    bee::ThreadPool pool(2);
    std::stop_source source;

    auto t1 = bee::submit(pool, [] {
        return 55;
    }, source.get_token());
    auto combined = bee::when_any(source, std::move(t1));
    auto result   = combined.get();

    EXPECT_EQ(result.index(), 0u);
    EXPECT_EQ(std::get<0>(result), 55);
}

TEST(WhenAnyTests, VariadicVoidTask)
{
    bee::ThreadPool pool(4);
    std::stop_source source;

    std::atomic<bool> executed{false};

    auto t1 = bee::submit(pool, [&executed] {
        executed.store(true);
    }, source.get_token());
    auto t2 = bee::submit_cancellable(
            pool,
            [](std::stop_token token) -> int {
                while (!token.stop_requested())
                    std::this_thread::yield();
                return -1;
            },
            source);

    auto combined = bee::when_any(source, std::move(t1), std::move(t2));
    auto result   = combined.get();

    EXPECT_EQ(result.index(), 0u);
    EXPECT_TRUE(executed.load());
}

} // namespace
