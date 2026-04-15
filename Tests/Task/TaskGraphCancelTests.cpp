/**
 * @File TaskGraphCancelTests.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/11
 * @Brief This file is part of Bee.
 */

#include <gtest/gtest.h>

#include <atomic>
#include <latch>
#include <stop_token>

#include "Task/Task.hpp"
#include "Concurrency/Thread/ThreadPool.hpp"

namespace
{

using bee::NodeHandle;
using bee::TaskGraph;
using bee::TaskState;
using bee::ThreadPool;

TEST(TaskGraphCancelTests, CancelBeforeExecution)
{
    ThreadPool pool(2);
    TaskGraph  graph;
    auto       a = graph.add_node([] { return 42; });

    std::stop_source source;
    source.request_stop();

    auto task = graph.execute(pool, source.get_token());
    task.wait();
    EXPECT_EQ(task.state(), TaskState::Cancelled);
}

TEST(TaskGraphCancelTests, CancelDuringExecution)
{
    ThreadPool       pool(2);
    TaskGraph        graph;
    std::stop_source source;

    std::latch node_started(1);
    std::latch can_proceed(1);

    auto a = graph.add_node([&] {
        node_started.count_down();
        can_proceed.wait();
        return 1;
    });
    auto b = graph.add_node([](int x) { return x + 1; }, a);

    auto task = graph.execute(pool, source.get_token());
    node_started.wait();
    source.request_stop();
    can_proceed.count_down();

    task.wait();
    EXPECT_TRUE(task.is_ready());
    // a completes (was already running), b should be cancelled
    EXPECT_EQ(graph.result(a), 1);
}

TEST(TaskGraphCancelTests, CancelPropagation)
{
    ThreadPool       pool(1);
    TaskGraph        graph;
    std::stop_source source;

    std::latch       node_started(1);
    std::latch       can_proceed(1);
    std::atomic<int> run_count{0};

    auto a = graph.add_node([&] {
        node_started.count_down();
        can_proceed.wait();
    });
    auto b = graph.add_node_after([&] { run_count.fetch_add(1, std::memory_order_relaxed); }, a);
    auto c = graph.add_node_after([&] { run_count.fetch_add(1, std::memory_order_relaxed); }, b);

    auto task = graph.execute(pool, source.get_token());
    node_started.wait();
    source.request_stop();
    can_proceed.count_down();

    task.wait();
    EXPECT_TRUE(task.is_ready());
    // b and c should have been cancelled, not run
    EXPECT_EQ(run_count.load(), 0);
}

TEST(TaskGraphCancelTests, CompletedNodesUnaffected)
{
    ThreadPool       pool(2);
    TaskGraph        graph;
    std::stop_source source;

    std::latch gate_started(1);
    std::latch gate_proceed(1);

    auto a = graph.add_node([] { return 42; });
    auto b = graph.add_node(
        [&](int x) {
            gate_started.count_down();
            gate_proceed.wait();
            return x + 1;
        },
        a
    );
    auto c = graph.add_node([](int x) { return x * 2; }, b);

    auto task = graph.execute(pool, source.get_token());
    gate_started.wait();
    source.request_stop();
    gate_proceed.count_down();

    task.wait();
    // a should have completed before cancel was requested
    EXPECT_EQ(graph.result(a), 42);
}

TEST(TaskGraphCancelTests, ReExecuteAfterCancel)
{
    ThreadPool pool(2);
    TaskGraph  graph;
    auto       a = graph.add_node([] { return 42; });

    {
        std::stop_source source;
        source.request_stop();
        auto task = graph.execute(pool, source.get_token());
        task.wait();
        EXPECT_EQ(task.state(), TaskState::Cancelled);
    }

    auto task2 = graph.execute(pool);
    task2.wait();
    EXPECT_EQ(task2.state(), TaskState::Completed);
    EXPECT_EQ(graph.result(a), 42);
}

} // namespace
