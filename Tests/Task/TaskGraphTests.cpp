/**
 * @File TaskGraphTests.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/18
 * @Brief This file is part of Bee.
 */

#include <gtest/gtest.h>
#include <string>

#include "Task/Graph/TaskGraph.hpp"
#include "Concurrency/Thread/WorkPool.hpp"

namespace
{

using bee::NodeHandle;
using bee::TaskGraph;
using bee::WorkPool;
using bee::TaskState;

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

TEST(TaskGraphTests, SingleRootNode)
{
    TaskGraph graph;
    auto a = graph.node([] { return 42; });
    EXPECT_FALSE(graph.empty());
    EXPECT_EQ(graph.node_count(), 1u);
    EXPECT_FALSE(graph.has_cycle());
    (void)a;
}

TEST(TaskGraphTests, DiamondNoCycle)
{
    TaskGraph graph;
    auto a = graph.node([] { return 1; });
    auto b = graph.node([](int x) { return x + 1; }, a);
    auto c = graph.node([](int x) { return x * 2; }, a);
    auto d = graph.node([](int x, int y) { return x + y; }, b, c);
    EXPECT_EQ(graph.node_count(), 4u);
    EXPECT_FALSE(graph.has_cycle());
    (void)d;
}

TEST(TaskGraphTests, NodeCountIncremental)
{
    TaskGraph graph;
    EXPECT_EQ(graph.node_count(), 0u);
    auto a = graph.node([] { return 1; });
    EXPECT_EQ(graph.node_count(), 1u);
    auto b = graph.node([](int x) { return x; }, a);
    EXPECT_EQ(graph.node_count(), 2u);
    auto c = graph.node_after([] {}, b);
    EXPECT_EQ(graph.node_count(), 3u);
    (void)c;
}

TEST(TaskGraphTests, HasCycleReturnsFalseForDAG)
{
    TaskGraph graph;
    auto a = graph.node([] { return 1; });
    auto b = graph.node([](int x) { return x; }, a);
    auto c = graph.node([](int x) { return x; }, a);
    auto d = graph.node([](int x, int y) { return x + y; }, b, c);
    auto e = graph.node([](int x) { return x; }, d);
    EXPECT_FALSE(graph.has_cycle());
    (void)e;
}

// =========================================================================
// DOT 输出测试
// =========================================================================

TEST(TaskGraphTests, ToDotEmptyGraph)
{
    TaskGraph graph;
    auto      dot = graph.to_dot();
    EXPECT_NE(dot.find("digraph TaskGraph"), std::string::npos);
    // 空图不应包含任何节点声明或边
    EXPECT_EQ(dot.find("->"), std::string::npos);
}

TEST(TaskGraphTests, ToDotWithLabels)
{
    TaskGraph graph;
    auto a = graph.node("load", [] { return 42; });
    auto b = graph.node([](int x) { return x * 2; }, a);  // 无标签
    auto c = graph.node("save", [](int x) { return std::to_string(x); }, b);
    (void)c;

    auto dot = graph.to_dot();
    // 用户标签节点
    EXPECT_NE(dot.find("\"load\""), std::string::npos);
    EXPECT_NE(dot.find("\"save\""), std::string::npos);
    // 边
    EXPECT_NE(dot.find("0 -> 1"), std::string::npos);
    EXPECT_NE(dot.find("1 -> 2"), std::string::npos);
}

TEST(TaskGraphTests, ToDotAutoLabel)
{
    TaskGraph graph;
    auto a = graph.node([] { return 42; });  // 无标签，应自动生成含 "int" 的标签
    (void)a;

    auto dot = graph.to_dot();
    // 自动标签应包含 "node_0" 和类型名
    EXPECT_NE(dot.find("node_0"), std::string::npos);
}

// =========================================================================
// 执行 + 数据流测试
// =========================================================================

TEST(TaskGraphTests, EmptyGraphExecute)
{
    WorkPool  pool(2);
    TaskGraph graph;
    auto      task = graph.execute(pool);
    task.wait();
}

TEST(TaskGraphTests, SingleNodeExecute)
{
    WorkPool  pool(2);
    TaskGraph graph;
    auto      a    = graph.node([] { return 42; });
    auto      task = graph.execute(pool);
    task.wait();
    EXPECT_EQ(graph.result(a), 42);
}

TEST(TaskGraphTests, SimpleChain)
{
    WorkPool  pool(2);
    TaskGraph graph;
    auto      a = graph.node([] { return 10; });
    auto      b = graph.node([](int x) { return x * 2; }, a);
    auto      c = graph.node([](int x) { return x + 5; }, b);
    auto      task = graph.execute(pool);
    task.wait();
    EXPECT_EQ(graph.result(a), 10);
    EXPECT_EQ(graph.result(b), 20);
    EXPECT_EQ(graph.result(c), 25);
}

TEST(TaskGraphTests, DiamondDependency)
{
    WorkPool  pool(4);
    TaskGraph graph;
    auto      a = graph.node([] { return 1; });
    auto      b = graph.node([](int x) { return x + 10; }, a);
    auto      c = graph.node([](int x) { return x * 100; }, a);
    auto      d = graph.node([](int x, int y) { return x + y; }, b, c);
    auto      task = graph.execute(pool);
    task.wait();
    EXPECT_EQ(graph.result(a), 1);
    EXPECT_EQ(graph.result(b), 11);
    EXPECT_EQ(graph.result(c), 100);
    EXPECT_EQ(graph.result(d), 111);
}

TEST(TaskGraphTests, WideFanOut)
{
    WorkPool                     pool(4);
    TaskGraph                    graph;
    auto                         root = graph.node([] { return 5; });
    std::vector<NodeHandle<int>> children;
    for (int i = 0; i < 10; ++i) {
        children.push_back(graph.node([i](int x) { return x + i; }, root));
    }
    auto task = graph.execute(pool);
    task.wait();
    EXPECT_EQ(graph.result(root), 5);
    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(graph.result(children[static_cast<std::size_t>(i)]), 5 + i);
    }
}

TEST(TaskGraphTests, TypedDataFlow)
{
    WorkPool  pool(2);
    TaskGraph graph;
    auto      a = graph.node([] { return 42; });
    auto      b = graph.node([](int x) { return std::to_string(x); }, a);
    auto      c = graph.node([](const std::string& s) { return static_cast<double>(s.size()); }, b);
    auto      task = graph.execute(pool);
    task.wait();
    EXPECT_EQ(graph.result(a), 42);
    EXPECT_EQ(graph.result(b), "42");
    EXPECT_DOUBLE_EQ(graph.result(c), 2.0);
}

TEST(TaskGraphTests, WideFanIn)
{
    WorkPool                      pool(4);
    TaskGraph                     graph;
    std::vector<NodeHandle<int>>  roots;
    for (int i = 0; i < 10; ++i) {
        roots.push_back(graph.node([i] { return i; }));
    }

    std::atomic<int>               sum{0};
    std::vector<NodeHandle<void>>  void_nodes;
    for (auto& r : roots) {
        void_nodes.push_back(graph.node([&sum](int x) { sum.fetch_add(x, std::memory_order_relaxed); }, r));
    }

    // sink 依赖所有 void 节点
    auto sink = graph.node_after(
        [&sum] { return sum.load(std::memory_order_relaxed); },
        void_nodes[0], void_nodes[1], void_nodes[2], void_nodes[3], void_nodes[4],
        void_nodes[5], void_nodes[6], void_nodes[7], void_nodes[8], void_nodes[9]);

    auto task = graph.execute(pool);
    task.wait();
    EXPECT_EQ(graph.result(sink), 45); // 0+1+...+9
}

} // namespace
