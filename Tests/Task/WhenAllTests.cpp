/**
 * @File WhenAllTests.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/11
 * @Brief This file is part of Bee.
 */

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

#include "Task/Task.hpp"

namespace
{

using bee::Task;
using bee::TaskState;

TEST(WhenAllTests, HeterogeneousTuple)
{
    bee::ThreadPool pool(4);

    auto t1 = bee::submit(pool, [] { return 42; });
    auto t2 = bee::submit(pool, [] { return std::string("hello"); });
    auto t3 = bee::submit(pool, [] { return 3.14; });

    auto combined  = bee::when_all(std::move(t1), std::move(t2), std::move(t3));
    auto [a, b, c] = combined.get();

    EXPECT_EQ(a, 42);
    EXPECT_EQ(b, "hello");
    EXPECT_DOUBLE_EQ(c, 3.14);
}

TEST(WhenAllTests, HomogeneousVector)
{
    bee::ThreadPool pool(4);

    std::vector<Task<int>> tasks;
    for (int i = 0; i < 5; ++i) {
        tasks.push_back(bee::submit(pool, [i] { return i * 10; }));
    }

    auto combined = bee::when_all(std::move(tasks));
    auto results  = combined.get();

    ASSERT_EQ(results.size(), 5u);
    for (int i = 0; i < 5; ++i) {
        EXPECT_EQ(results[i], i * 10);
    }
}

TEST(WhenAllTests, OneFailsCombinedFails)
{
    bee::ThreadPool pool(4);

    auto t1 = bee::submit(pool, [] { return 1; });
    auto t2 = bee::submit(pool, []() -> int { throw std::runtime_error("boom"); });
    auto t3 = bee::submit(pool, [] { return 3; });

    auto combined = bee::when_all(std::move(t1), std::move(t2), std::move(t3));
    EXPECT_THROW(combined.get(), std::runtime_error);
    EXPECT_EQ(combined.state(), TaskState::Failed);
}

TEST(WhenAllTests, EmptyVector)
{
    bee::ThreadPool pool(2);

    std::vector<Task<int>> empty;
    auto                   combined = bee::when_all(std::move(empty));
    auto                   results  = combined.get();

    EXPECT_TRUE(results.empty());
    EXPECT_EQ(combined.state(), TaskState::Completed);
}

TEST(WhenAllTests, SingleTask)
{
    bee::ThreadPool pool(2);

    std::vector<Task<int>> tasks;
    tasks.push_back(bee::submit(pool, [] { return 77; }));

    auto combined = bee::when_all(std::move(tasks));
    auto results  = combined.get();

    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0], 77);
}

TEST(WhenAllTests, WithVoidTasks)
{
    bee::ThreadPool  pool(4);
    std::atomic<int> counter{0};

    auto t1 = bee::submit(pool, [&] { counter.fetch_add(1); });
    auto t2 = bee::submit(pool, [&] { counter.fetch_add(1); });

    auto combined = bee::when_all(std::move(t1), std::move(t2));
    combined.get();

    EXPECT_EQ(counter.load(), 2);
}

} // namespace
