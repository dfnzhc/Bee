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
    auto a = graph.add_node([] { return 42; });
    EXPECT_FALSE(graph.empty());
    EXPECT_EQ(graph.node_count(), 1u);
    EXPECT_FALSE(graph.has_cycle());
}

TEST(TaskGraphTests, BuildDiamondNoCycle)
{
    TaskGraph graph;
    auto a = graph.add_node([] { return 1; });
    auto b = graph.add_node([](int x) { return x + 1; }, a);
    auto c = graph.add_node([](int x) { return x * 2; }, a);
    auto d = graph.add_node([](int x, int y) { return x + y; }, b, c);
    EXPECT_EQ(graph.node_count(), 4u);
    EXPECT_FALSE(graph.has_cycle());
}

} // namespace
