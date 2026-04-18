/**
 * @File TaskGraphTests.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/18
 * @Brief This file is part of Bee.
 */

#include <gtest/gtest.h>
#include <string>

#include "Task/Graph/TaskGraph.hpp"

namespace
{

using bee::NodeHandle;
using bee::TaskGraph;

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

} // namespace
