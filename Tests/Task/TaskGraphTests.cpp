/**
 * @File TaskGraphTests.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/18
 * @Brief This file is part of Bee.
 */

#include <gtest/gtest.h>
#include <string>
#include <atomic>
#include <thread>
#include <chrono>
#include <stdexcept>
#include <vector>

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
    WorkPool  pool(4);
    TaskGraph graph;

    // 10 个根节点，每个产生一个 int
    std::vector<NodeHandle<int>> roots;
    for (int i = 0; i < 10; ++i) {
        roots.push_back(graph.node([i] { return i; }));
    }

    // 汇聚节点：通过类型化依赖链式累加，避免共享原子量
    // 构造一棵归约树：两两求和
    std::vector<NodeHandle<int>> level = roots;
    while (level.size() > 1) {
        std::vector<NodeHandle<int>> next;
        for (std::size_t i = 0; i + 1 < level.size(); i += 2) {
            next.push_back(graph.node([](int a, int b) { return a + b; }, level[i], level[i + 1]));
        }
        // 奇数个节点时，最后一个直接晋级
        if (level.size() % 2 == 1) {
            next.push_back(level.back());
        }
        level = std::move(next);
    }

    auto task = graph.execute(pool);
    task.wait();
    EXPECT_EQ(graph.result(level[0]), 45); // 0+1+...+9
}

// =========================================================================
// void 节点测试
// =========================================================================

TEST(TaskGraphTests, VoidNodesWithNodeAfter)
{
    WorkPool         pool(2);
    TaskGraph        graph;
    std::atomic<int> counter{0};
    auto             a = graph.node([&] { counter.fetch_add(1, std::memory_order_relaxed); });
    auto             b = graph.node_after([&] { counter.fetch_add(10, std::memory_order_relaxed); }, a);
    auto             task = graph.execute(pool);
    task.wait();
    EXPECT_EQ(counter.load(), 11);
    (void)b;
}

TEST(TaskGraphTests, MixedVoidAndTyped)
{
    WorkPool         pool(2);
    TaskGraph        graph;
    std::atomic<int> side_effect{0};

    auto producer   = graph.node([] { return 10; });
    auto void_node  = graph.node([&] { side_effect.store(99, std::memory_order_relaxed); });
    auto consumer   = graph.node([](int x) { return x * 3; }, producer);
    auto final_node = graph.node_after([&] { return side_effect.load(std::memory_order_relaxed); }, void_node);

    auto task = graph.execute(pool);
    task.wait();
    EXPECT_EQ(graph.result(consumer), 30);
    EXPECT_EQ(graph.result(final_node), 99);
}

TEST(TaskGraphTests, VoidNodeChain)
{
    WorkPool         pool(2);
    TaskGraph        graph;
    std::atomic<int> order_tracker{0};
    int              step1_val = 0, step2_val = 0, step3_val = 0;

    auto a = graph.node([&] {
        step1_val = order_tracker.fetch_add(1, std::memory_order_relaxed);
    });
    auto b = graph.node_after([&] {
        step2_val = order_tracker.fetch_add(1, std::memory_order_relaxed);
    }, a);
    auto c = graph.node_after([&] {
        step3_val = order_tracker.fetch_add(1, std::memory_order_relaxed);
    }, b);

    auto task = graph.execute(pool);
    task.wait();
    // 链式依赖保证执行顺序
    EXPECT_LT(step1_val, step2_val);
    EXPECT_LT(step2_val, step3_val);
    (void)c;
}

// =========================================================================
// 错误处理测试
// =========================================================================

TEST(TaskGraphTests, NodeExceptionPropagates)
{
    WorkPool  pool(2);
    TaskGraph graph;
    auto      a = graph.node([]() -> int { throw std::runtime_error("boom"); });
    auto      b = graph.node([](int x) { return x + 1; }, a);
    (void)b;

    auto task = graph.execute(pool);
    EXPECT_THROW(task.wait(), std::runtime_error);
}

TEST(TaskGraphTests, CascadeFailure)
{
    WorkPool          pool(2);
    TaskGraph         graph;
    std::atomic<bool> downstream_ran{false};

    auto a = graph.node([]() -> int { throw std::runtime_error("fail"); });
    auto b = graph.node([&](int x) -> int {
        downstream_ran.store(true, std::memory_order_relaxed);
        return x;
    }, a);
    (void)b;

    auto task = graph.execute(pool);
    EXPECT_THROW(task.wait(), std::runtime_error);
    EXPECT_FALSE(downstream_ran.load());
}

TEST(TaskGraphTests, CycleDetectedAtExecute)
{
    // 注意：正常 API 无法构造环（因为节点只能引用已创建的节点）。
    // 这里测试的是 has_cycle() 在合法 DAG 上返回 false（已在 builder 测试覆盖）。
    // execute() 内部也调用 has_cycle() 作为防御检查。
    WorkPool  pool(2);
    TaskGraph graph;
    auto      a = graph.node([] { return 1; });
    auto      b = graph.node([](int x) { return x; }, a);
    (void)b;
    // 合法 DAG，execute 不应抛异常
    auto task = graph.execute(pool);
    task.wait();
    EXPECT_EQ(graph.result(a), 1);
}

TEST(TaskGraphTests, ResultOfFailedNodeThrows)
{
    WorkPool  pool(2);
    TaskGraph graph;
    auto      a = graph.node([]() -> int { throw std::runtime_error("fail"); });

    auto task = graph.execute(pool);
    EXPECT_THROW(task.wait(), std::runtime_error);
    EXPECT_THROW({ (void)graph.result(a); }, std::logic_error);
}

// =========================================================================
// 并发测试
// =========================================================================

TEST(TaskGraphTests, WideFanOutParallelism)
{
    WorkPool  pool(4);
    TaskGraph graph;

    auto root = graph.node([] { return 0; });
    constexpr int    N = 100;
    std::atomic<int> parallel_count{0};
    std::atomic<int> max_parallel{0};

    std::vector<NodeHandle<void>> leaves;
    for (int i = 0; i < N; ++i) {
        leaves.push_back(graph.node([&](int) {
            auto cur  = parallel_count.fetch_add(1, std::memory_order_relaxed) + 1;
            // 更新最大并行度
            auto prev = max_parallel.load(std::memory_order_relaxed);
            while (cur > prev && !max_parallel.compare_exchange_weak(prev, cur, std::memory_order_relaxed)) {}

            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            parallel_count.fetch_sub(1, std::memory_order_relaxed);
        }, root));
    }

    auto task = graph.execute(pool);
    task.wait();
    // 至少有 2 个节点同时运行（4 线程 + 100 个独立节点）
    EXPECT_GE(max_parallel.load(), 2);
}

TEST(TaskGraphTests, LargeGraphStress)
{
    WorkPool  pool(4);
    TaskGraph graph;

    // 构建 1000 节点的链
    auto prev = graph.node([] { return 0; });
    for (int i = 1; i < 1000; ++i) {
        prev = graph.node([](int x) { return x + 1; }, prev);
    }

    auto task = graph.execute(pool);
    task.wait();
    EXPECT_EQ(graph.result(prev), 999);
}

TEST(TaskGraphTests, DoubleExecuteThrows)
{
    WorkPool  pool(2);
    TaskGraph graph;
    auto      a = graph.node([] { return 1; });
    (void)a;

    auto task = graph.execute(pool);
    task.wait();
    // 第二次 execute() 创建的协程在启动时抛出异常
    auto task2 = graph.execute(pool);
    EXPECT_THROW(task2.wait(), std::logic_error);
}

TEST(TaskGraphTests, DotLabelEscaping)
{
    TaskGraph graph;
    auto a = graph.node("node with \"quotes\"", [] { return 1; });
    auto b = graph.node("back\\slash", [](int x) { return x; }, a);
    (void)b;

    auto dot = graph.to_dot();
    // 引号和反斜杠应被转义
    EXPECT_NE(dot.find("node with \\\"quotes\\\""), std::string::npos);
    EXPECT_NE(dot.find("back\\\\slash"), std::string::npos);
}

} // namespace
