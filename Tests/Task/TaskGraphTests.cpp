/**
 * @File TaskGraphTests.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/11
 * @Brief This file is part of Bee.
 */

#include <gtest/gtest.h>

#include <atomic>
#include <string>

#include "Task/Task.hpp"
#include "Base/Reflection/Nameof.hpp"
#include "Concurrency/Thread/ThreadPool.hpp"

namespace
{

using bee::NodeHandle;
using bee::NodeState;
using bee::TaskGraph;
using bee::TaskState;
using bee::ThreadPool;

// =========================================================================
// NodeState 枚举测试
// =========================================================================

TEST(TaskGraphTests, NodeStateAllValues)
{
    EXPECT_EQ(static_cast<int>(NodeState::Pending), 0);
    EXPECT_EQ(static_cast<int>(NodeState::Ready), 1);
    EXPECT_EQ(static_cast<int>(NodeState::Running), 2);
    EXPECT_EQ(static_cast<int>(NodeState::Completed), 3);
    EXPECT_EQ(static_cast<int>(NodeState::Failed), 4);
    EXPECT_EQ(static_cast<int>(NodeState::Cancelled), 5);
}

TEST(TaskGraphTests, NodeStateEnumToName)
{
    EXPECT_EQ(bee::enum_to_name(NodeState::Pending), "Pending");
    EXPECT_EQ(bee::enum_to_name(NodeState::Ready), "Ready");
    EXPECT_EQ(bee::enum_to_name(NodeState::Running), "Running");
    EXPECT_EQ(bee::enum_to_name(NodeState::Completed), "Completed");
    EXPECT_EQ(bee::enum_to_name(NodeState::Failed), "Failed");
    EXPECT_EQ(bee::enum_to_name(NodeState::Cancelled), "Cancelled");
}

TEST(TaskGraphTests, NodeStateEnumFromName)
{
    EXPECT_EQ(bee::enum_from_name<NodeState>("Pending"), NodeState::Pending);
    EXPECT_EQ(bee::enum_from_name<NodeState>("Ready"), NodeState::Ready);
    EXPECT_EQ(bee::enum_from_name<NodeState>("Running"), NodeState::Running);
    EXPECT_EQ(bee::enum_from_name<NodeState>("Completed"), NodeState::Completed);
    EXPECT_EQ(bee::enum_from_name<NodeState>("Failed"), NodeState::Failed);
    EXPECT_EQ(bee::enum_from_name<NodeState>("Cancelled"), NodeState::Cancelled);
}

// =========================================================================
// 构建器 + 查询测试
// =========================================================================

TEST(TaskGraphTests, EmptyGraph)
{
    TaskGraph graph;
    EXPECT_TRUE(graph.empty());
    EXPECT_EQ(graph.node_count(), 0u);
    EXPECT_FALSE(graph.has_cycle());
}

TEST(TaskGraphTests, AddRootNode)
{
    TaskGraph graph;
    auto      a = graph.add_node([] {
        return 42;
    });
    EXPECT_FALSE(graph.empty());
    EXPECT_EQ(graph.node_count(), 1u);
    EXPECT_FALSE(graph.has_cycle());
}

TEST(TaskGraphTests, BuildDiamondNoCycle)
{
    TaskGraph graph;
    auto      a = graph.add_node([] {
        return 1;
    });
    auto      b = graph.add_node(
            [](int x) {
                return x + 1;
            },
            a
    );
    auto c = graph.add_node(
            [](int x) {
                return x * 2;
            },
            a
    );
    auto d = graph.add_node(
            [](int x, int y) {
                return x + y;
            },
            b,
            c
    );
    EXPECT_EQ(graph.node_count(), 4u);
    EXPECT_FALSE(graph.has_cycle());
}

// =========================================================================
// Execution + data flow tests
// =========================================================================

TEST(TaskGraphTests, EmptyGraphExecute)
{
    ThreadPool pool(2);
    TaskGraph  graph;
    auto       task = graph.execute(pool);
    task.wait();
    EXPECT_EQ(task.state(), TaskState::Completed);
}

TEST(TaskGraphTests, SingleNodeExecute)
{
    ThreadPool pool(2);
    TaskGraph  graph;
    auto       a    = graph.add_node([] {
        return 42;
    });
    auto       task = graph.execute(pool);
    task.wait();
    EXPECT_EQ(task.state(), TaskState::Completed);
    EXPECT_EQ(graph.result(a), 42);
}

TEST(TaskGraphTests, SimpleChain)
{
    ThreadPool pool(2);
    TaskGraph  graph;
    auto       a = graph.add_node([] {
        return 10;
    });
    auto       b = graph.add_node(
            [](int x) {
                return x * 2;
            },
            a
    );
    auto c = graph.add_node(
            [](int x) {
                return x + 5;
            },
            b
    );
    auto task = graph.execute(pool);
    task.wait();
    EXPECT_EQ(graph.result(a), 10);
    EXPECT_EQ(graph.result(b), 20);
    EXPECT_EQ(graph.result(c), 25);
}

TEST(TaskGraphTests, DiamondDependency)
{
    ThreadPool pool(4);
    TaskGraph  graph;
    auto       a = graph.add_node([] {
        return 1;
    });
    auto       b = graph.add_node(
            [](int x) {
                return x + 10;
            },
            a
    );
    auto c = graph.add_node(
            [](int x) {
                return x * 100;
            },
            a
    );
    auto d = graph.add_node(
            [](int x, int y) {
                return x + y;
            },
            b,
            c
    );
    auto task = graph.execute(pool);
    task.wait();
    EXPECT_EQ(graph.result(a), 1);
    EXPECT_EQ(graph.result(b), 11);
    EXPECT_EQ(graph.result(c), 100);
    EXPECT_EQ(graph.result(d), 111);
}

TEST(TaskGraphTests, WideFanOut)
{
    ThreadPool                   pool(4);
    TaskGraph                    graph;
    auto                         root = graph.add_node([] {
        return 5;
    });
    std::vector<NodeHandle<int>> children;
    for (int i = 0; i < 10; ++i) {
        children.push_back(graph.add_node(
                [i](int x) {
                    return x + i;
                },
                root
        ));
    }
    auto task = graph.execute(pool);
    task.wait();
    EXPECT_EQ(task.state(), TaskState::Completed);
    EXPECT_EQ(graph.result(root), 5);
    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(graph.result(children[static_cast<size_t>(i)]), 5 + i);
    }
}

TEST(TaskGraphTests, TypedDataFlow)
{
    ThreadPool pool(2);
    TaskGraph  graph;
    auto       a = graph.add_node([] {
        return 42;
    });
    auto       b = graph.add_node(
            [](int x) {
                return std::to_string(x);
            },
            a
    );
    auto c = graph.add_node(
            [](const std::string& s) {
                return static_cast<double>(s.size());
            },
            b
    );
    auto task = graph.execute(pool);
    task.wait();
    EXPECT_EQ(graph.result(a), 42);
    EXPECT_EQ(graph.result(b), "42");
    EXPECT_DOUBLE_EQ(graph.result(c), 2.0);
}

TEST(TaskGraphTests, VoidNodesWithAddNodeAfter)
{
    ThreadPool       pool(2);
    TaskGraph        graph;
    std::atomic<int> counter{0};
    auto             a = graph.add_node([&] {
        counter.fetch_add(1, std::memory_order_relaxed);
    });
    auto             b = graph.add_node_after(
            [&] {
                counter.fetch_add(10, std::memory_order_relaxed);
            },
            a
    );
    auto task = graph.execute(pool);
    task.wait();
    EXPECT_EQ(task.state(), TaskState::Completed);
    EXPECT_EQ(counter.load(), 11);
}

TEST(TaskGraphTests, WideFanIn)
{
    ThreadPool                   pool(4);
    TaskGraph                    graph;
    std::vector<NodeHandle<int>> roots;
    for (int i = 0; i < 10; ++i) {
        roots.push_back(graph.add_node([i] {
            return i;
        }));
    }

    std::atomic<int>              sum{0};
    std::vector<NodeHandle<void>> void_roots;
    for (auto& r : roots) {
        void_roots.push_back(graph.add_node(
                [&sum](int x) {
                    sum.fetch_add(x, std::memory_order_relaxed);
                },
                r
        ));
    }

    // Sink node depends on all void nodes
    auto sink = graph.add_node_after(
            [&sum] {
                return sum.load(std::memory_order_relaxed);
            },
            void_roots[0],
            void_roots[1],
            void_roots[2],
            void_roots[3],
            void_roots[4],
            void_roots[5],
            void_roots[6],
            void_roots[7],
            void_roots[8],
            void_roots[9]
    );

    auto task = graph.execute(pool);
    task.wait();
    EXPECT_EQ(task.state(), TaskState::Completed);
    // 0+1+2+...+9 = 45
    EXPECT_EQ(graph.result(sink), 45);
}

TEST(TaskGraphTests, MixedVoidAndTyped)
{
    ThreadPool       pool(2);
    TaskGraph        graph;
    std::atomic<int> side_effect{0};

    auto producer  = graph.add_node([] {
        return 10;
    });
    auto void_node = graph.add_node([&] {
        side_effect.store(99, std::memory_order_relaxed);
    });
    auto consumer  = graph.add_node(
            [](int x) {
                return x * 3;
            },
            producer
    );
    auto final_node = graph.add_node_after(
            [&] {
                return side_effect.load(std::memory_order_relaxed);
            },
            void_node
    );

    auto task = graph.execute(pool);
    task.wait();
    EXPECT_EQ(task.state(), TaskState::Completed);
    EXPECT_EQ(graph.result(consumer), 30);
    EXPECT_EQ(graph.result(final_node), 99);
}

TEST(TaskGraphTests, ReExecution)
{
    ThreadPool       pool(2);
    TaskGraph        graph;
    std::atomic<int> call_count{0};
    auto             a = graph.add_node([&] {
        return call_count.fetch_add(1, std::memory_order_relaxed);
    });

    auto task1 = graph.execute(pool);
    task1.wait();
    EXPECT_EQ(task1.state(), TaskState::Completed);
    EXPECT_EQ(graph.result(a), 0);

    auto task2 = graph.execute(pool);
    task2.wait();
    EXPECT_EQ(task2.state(), TaskState::Completed);
    EXPECT_EQ(graph.result(a), 1);
    EXPECT_EQ(call_count.load(), 2);
}

} // namespace
