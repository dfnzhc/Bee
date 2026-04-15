/**
 * @File TaskGraphErrorTests.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/11
 * @Brief This file is part of Bee.
 */

#include <gtest/gtest.h>

#include <atomic>
#include <stdexcept>

#include "Task/Task.hpp"
#include "Concurrency/Thread/ThreadPool.hpp"

namespace
{

using bee::NodeHandle;
using bee::TaskGraph;
using bee::TaskState;
using bee::ThreadPool;

TEST(TaskGraphErrorTests, NodeThrows)
{
    ThreadPool pool(2);
    TaskGraph  graph;

    auto a    = graph.add_node([]() -> int { throw std::runtime_error("boom"); });
    auto task = graph.execute(pool);
    task.wait();
    EXPECT_EQ(task.state(), TaskState::Failed);
    EXPECT_THROW(task.get(), std::runtime_error);
}

TEST(TaskGraphErrorTests, UpstreamFailPropagation)
{
    ThreadPool pool(2);
    TaskGraph  graph;

    auto a    = graph.add_node([]() -> int { throw std::runtime_error("upstream"); });
    auto b    = graph.add_node([](int x) { return x + 1; }, a);
    auto c    = graph.add_node([](int x) { return x * 2; }, a);
    auto task = graph.execute(pool);
    task.wait();
    EXPECT_EQ(task.state(), TaskState::Failed);
}

TEST(TaskGraphErrorTests, IndependentBranchContinues)
{
    ThreadPool pool(4);
    TaskGraph  graph;

    auto fail_root  = graph.add_node([]() -> int { throw std::runtime_error("fail"); });
    auto fail_child = graph.add_node([](int x) { return x; }, fail_root);

    auto ok_root  = graph.add_node([] { return 100; });
    auto ok_child = graph.add_node([](int x) { return x + 1; }, ok_root);

    auto task = graph.execute(pool);
    task.wait();

    // Graph fails because at least one node failed
    EXPECT_EQ(task.state(), TaskState::Failed);

    // Independent branch should have completed successfully
    EXPECT_EQ(graph.result(ok_root), 100);
    EXPECT_EQ(graph.result(ok_child), 101);
}

TEST(TaskGraphErrorTests, MultipleFailures)
{
    ThreadPool pool(4);
    TaskGraph  graph;

    auto a    = graph.add_node([]() -> int { throw std::runtime_error("first"); });
    auto b    = graph.add_node([]() -> int { throw std::runtime_error("second"); });
    auto task = graph.execute(pool);
    task.wait();
    EXPECT_EQ(task.state(), TaskState::Failed);
    // Should get one of the exceptions (whichever was recorded first)
    EXPECT_THROW(task.get(), std::runtime_error);
}

TEST(TaskGraphErrorTests, ReExecuteAfterFailure)
{
    ThreadPool       pool(2);
    TaskGraph        graph;
    std::atomic<int> attempt{0};

    auto a = graph.add_node([&]() -> int {
        if (attempt.fetch_add(1, std::memory_order_relaxed) == 0) {
            throw std::runtime_error("first attempt");
        }
        return 42;
    });

    auto task1 = graph.execute(pool);
    task1.wait();
    EXPECT_EQ(task1.state(), TaskState::Failed);

    auto task2 = graph.execute(pool);
    task2.wait();
    EXPECT_EQ(task2.state(), TaskState::Completed);
    EXPECT_EQ(graph.result(a), 42);
}

} // namespace
