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

} // namespace
