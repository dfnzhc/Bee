/**
 * @File TaskTests.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/11
 * @Brief This file is part of Bee.
 */

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <stdexcept>
#include <string>
#include <thread>

#include "Task/Task.hpp"

namespace
{

using bee::TaskState;
using bee::Task;

// =========================================================================
// Basic Tests
// =========================================================================

TEST(TaskBasicTests, SubmitAndGetInt)
{
    bee::ThreadPool pool(2);
    auto task = bee::submit(pool, [] {
        return 42;
    });
    EXPECT_EQ(task.get(), 42);
}

TEST(TaskBasicTests, SubmitAndGetVoid)
{
    bee::ThreadPool pool(2);
    std::atomic<bool> executed{false};
    auto task = bee::submit(pool, [&] {
        executed.store(true);
    });
    task.get();
    EXPECT_TRUE(executed.load());
}

TEST(TaskBasicTests, SubmitAndGetString)
{
    bee::ThreadPool pool(2);
    auto task = bee::submit(pool, [] {
        return std::string("hello");
    });
    EXPECT_EQ(task.get(), "hello");
}

TEST(TaskBasicTests, DefaultConstructedIsInvalid)
{
    Task<int> task;
    EXPECT_FALSE(static_cast<bool>(task));
    EXPECT_EQ(task.state(), TaskState::Pending);
    EXPECT_FALSE(task.is_ready());
}

TEST(TaskBasicTests, MoveConstruct)
{
    bee::ThreadPool pool(2);
    auto task1 = bee::submit(pool, [] {
        return 7;
    });
    auto task2 = std::move(task1);
    EXPECT_FALSE(static_cast<bool>(task1));
    EXPECT_TRUE(static_cast<bool>(task2));
    EXPECT_EQ(task2.get(), 7);
}

TEST(TaskBasicTests, MoveAssign)
{
    bee::ThreadPool pool(2);
    auto task1 = bee::submit(pool, [] {
        return 1;
    });
    Task<int> task2;
    task2 = std::move(task1);
    EXPECT_FALSE(static_cast<bool>(task1));
    EXPECT_TRUE(static_cast<bool>(task2));
    EXPECT_EQ(task2.get(), 1);
}

TEST(TaskBasicTests, IsReadyAfterCompletion)
{
    bee::ThreadPool pool(2);
    auto task = bee::submit(pool, [] {
        return 0;
    });
    task.wait();
    EXPECT_TRUE(task.is_ready());
}

TEST(TaskBasicTests, StateTransitions)
{
    bee::ThreadPool pool(1);
    std::atomic<bool> started{false};
    std::atomic<bool> proceed{false};

    auto task = bee::submit(pool, [&] {
        started.store(true, std::memory_order_release);
        while (!proceed.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
        return 42;
    });

    while (!started.load(std::memory_order_acquire)) {
        std::this_thread::yield();
    }

    EXPECT_EQ(task.state(), TaskState::Running);

    proceed.store(true, std::memory_order_release);
    auto result = task.get();
    EXPECT_EQ(result, 42);
    EXPECT_EQ(task.state(), TaskState::Completed);
}

TEST(TaskBasicTests, WaitBlocks)
{
    bee::ThreadPool pool(1);
    std::atomic<bool> proceed{false};

    auto task = bee::submit(pool, [&] {
        while (!proceed.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
        return 99;
    });

    proceed.store(true, std::memory_order_release);
    task.wait();
    EXPECT_EQ(task.state(), TaskState::Completed);
}

TEST(TaskBasicTests, WaitForTimesOut)
{
    bee::ThreadPool pool(1);
    std::atomic<bool> proceed{false};

    auto task = bee::submit(pool, [&] {
        while (!proceed.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
        return 1;
    });

    auto s = task.wait_for(std::chrono::milliseconds(1));
    EXPECT_TRUE(s == TaskState::Pending || s == TaskState::Running);

    proceed.store(true, std::memory_order_release);
    task.wait();
    EXPECT_EQ(task.state(), TaskState::Completed);
}

TEST(TaskBasicTests, WaitForSucceeds)
{
    bee::ThreadPool pool(2);
    auto task = bee::submit(pool, [] {
        return 5;
    });
    auto s = task.wait_for(std::chrono::seconds(5));
    EXPECT_EQ(s, TaskState::Completed);
}

TEST(TaskBasicTests, GetVoidMultipleTimes)
{
    bee::ThreadPool pool(2);
    auto task = bee::submit(pool, [] {
    });
    task.get();
    task.get(); // should not crash
}

TEST(TaskBasicTests, ExceptionPropagation)
{
    bee::ThreadPool pool(2);
    auto task = bee::submit(pool, []() -> int {
        throw std::runtime_error("oops");
    });
    EXPECT_THROW(task.get(), std::runtime_error);
    EXPECT_EQ(task.state(), TaskState::Failed);
}

// =========================================================================
// Continuation Tests
// =========================================================================

TEST(TaskContinuationTests, ThenInline)
{
    bee::ThreadPool pool(2);
    auto task = bee::submit(pool, [] {
                return 10;
            })
           .then([](int v) {
                return v * 2;
            });
    EXPECT_EQ(task.get(), 20);
}

TEST(TaskContinuationTests, ThenPoolDispatched)
{
    bee::ThreadPool pool(2);
    std::atomic<std::thread::id> cont_thread{};

    auto task = bee::submit(pool, [] {
                return 5;
            })
           .then(pool, [&](int v) {
                cont_thread.store(std::this_thread::get_id());
                return v + 1;
            });

    EXPECT_EQ(task.get(), 6);
    EXPECT_NE(cont_thread.load(), std::this_thread::get_id());
}

TEST(TaskContinuationTests, ThenChained)
{
    bee::ThreadPool pool(2);
    auto task = bee::submit(pool, [] {
                    return 1;
                })
               .then([](int v) {
                    return v + 1;
                })
               .then([](int v) {
                    return v * 10;
                })
               .then([](int v) {
                    return v + 3;
                });
    EXPECT_EQ(task.get(), 23);
}

TEST(TaskContinuationTests, ThenOnCompletedTask)
{
    bee::ThreadPool pool(2);
    auto task = bee::submit(pool, [] {
        return 42;
    });
    task.wait();
    EXPECT_TRUE(task.is_ready());

    auto next = task.then([](int v) {
        return v + 8;
    });
    EXPECT_EQ(next.get(), 50);
}

TEST(TaskContinuationTests, ThenOnFailedTask)
{
    bee::ThreadPool pool(2);
    auto task = bee::submit(pool, []() -> int {
        throw std::runtime_error("fail");
    });
    task.wait();

    auto next = task.then([](int v) {
        return v + 1;
    });
    EXPECT_THROW(next.get(), std::runtime_error);
    EXPECT_EQ(next.state(), TaskState::Failed);
}

TEST(TaskContinuationTests, ThenVoidToInt)
{
    bee::ThreadPool pool(2);
    auto task = bee::submit(pool, [] {
            })
           .then([] {
                return 99;
            });
    EXPECT_EQ(task.get(), 99);
}

TEST(TaskContinuationTests, ThenIntToVoid)
{
    bee::ThreadPool pool(2);
    std::atomic<int> captured{0};
    auto task = bee::submit(pool, [] {
                return 7;
            })
           .then([&](int v) {
                captured.store(v);
            });
    task.get();
    EXPECT_EQ(captured.load(), 7);
}

TEST(TaskContinuationTests, ThenTypeChanging)
{
    bee::ThreadPool pool(2);
    auto task = bee::submit(pool, [] {
                })
               .then([] {
                    return 42;
                })
               .then([](int v) {
                    return std::to_string(v);
                });
    EXPECT_EQ(task.get(), "42");
}

TEST(TaskContinuationTests, ThenMoveOnlyCapture)
{
    bee::ThreadPool pool(2);
    auto ptr  = std::make_unique<int>(100);
    auto task = bee::submit(pool, [] {
                return 1;
            })
           .then([p = std::move(ptr)](int v) {
                return v + *p;
            });
    EXPECT_EQ(task.get(), 101);
}

TEST(TaskContinuationTests, ErrorPropagationThroughChain)
{
    bee::ThreadPool pool(2);
    std::atomic<bool> f1_ran{false};
    std::atomic<bool> f2_ran{false};

    auto task = bee::submit(pool, []() -> int {
                    throw std::runtime_error("err");
                })
               .then([&](int v) {
                    f1_ran.store(true);
                    return v;
                })
               .then([&](int v) {
                    f2_ran.store(true);
                    return v;
                });

    EXPECT_THROW(task.get(), std::runtime_error);
    EXPECT_FALSE(f1_ran.load());
    EXPECT_FALSE(f2_ran.load());
}

// =========================================================================
// 错误测试
// =========================================================================

TEST(TaskErrorTests, FailedStateAfterException)
{
    bee::ThreadPool pool(2);
    auto task = bee::submit(pool, []() -> int {
        throw std::logic_error("logic");
    });
    EXPECT_THROW(task.get(), std::logic_error);
    EXPECT_EQ(task.state(), TaskState::Failed);
}

TEST(TaskErrorTests, ContinuationExceptionPropagates)
{
    bee::ThreadPool pool(2);
    auto task = bee::submit(pool, [] {
                return 1;
            })
           .then([](int) -> int {
                throw std::runtime_error("in-cont");
            });
    EXPECT_THROW(task.get(), std::runtime_error);
    EXPECT_EQ(task.state(), TaskState::Failed);
}

// =========================================================================
// 取消测试
// =========================================================================

TEST(TaskCancellationTests, CancelBeforeExecution)
{
    bee::ThreadPool pool(1);

    // Block the pool's single worker so our task sits in the queue.
    std::atomic<bool> unblock{false};
    auto blocker = bee::submit(pool, [&] {
        while (!unblock.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
    });

    std::stop_source source;
    auto task = bee::submit(pool, [] {
        return 42;
    }, source.get_token());

    // Cancel before the blocked worker picks up our task.
    source.request_stop();
    unblock.store(true, std::memory_order_release);
    blocker.wait();

    EXPECT_EQ(task.state(), TaskState::Cancelled);
    EXPECT_THROW(task.get(), std::runtime_error);
}

TEST(TaskCancellationTests, CancelDuringExecution)
{
    bee::ThreadPool pool(2);
    std::stop_source source;

    auto task = bee::submit_cancellable(pool, [](std::stop_token token) -> int {
        for (int i = 0; i < 1000000; ++i) {
            if (token.stop_requested()) {
                return -1;
            }
            std::this_thread::yield();
        }
        return 42;
    }, source);

    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    source.request_stop();

    auto result = task.get();
    EXPECT_EQ(result, -1);
}

TEST(TaskCancellationTests, SubmitCancellablePassesToken)
{
    bee::ThreadPool pool(2);
    std::stop_source source;

    auto task = bee::submit_cancellable(pool, [](std::stop_token token) {
        EXPECT_FALSE(token.stop_requested());
    }, source);

    task.get(); // should not throw
}

TEST(TaskCancellationTests, CancelledTaskState)
{
    bee::ThreadPool pool(1);

    std::atomic<bool> unblock{false};
    auto blocker = bee::submit(pool, [&] {
        while (!unblock.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
    });

    std::stop_source source;
    source.request_stop(); // cancel immediately
    auto task = bee::submit(pool, [] {
        return 1;
    }, source.get_token());

    unblock.store(true, std::memory_order_release);
    blocker.wait();
    task.wait();

    EXPECT_EQ(task.state(), TaskState::Cancelled);
    EXPECT_FALSE(task.is_ready() && task.state() == TaskState::Completed);
}

TEST(TaskCancellationTests, CancelPropagatesThroughThen)
{
    bee::ThreadPool pool(1);

    std::atomic<bool> unblock{false};
    auto blocker = bee::submit(pool, [&] {
        while (!unblock.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
    });

    std::stop_source source;
    source.request_stop();

    std::atomic<bool> then_ran{false};
    auto task = bee::submit(pool, [] {
                return 1;
            }, source.get_token())
           .then([&](int v) {
                then_ran.store(true);
                return v;
            });

    unblock.store(true, std::memory_order_release);
    blocker.wait();
    task.wait();

    EXPECT_FALSE(then_ran.load());
    EXPECT_EQ(task.state(), TaskState::Cancelled);
}

} // namespace
