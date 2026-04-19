/**
 * @File TaskGraphScaleBench.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Brief TaskGraph 在不同规模下的 execute wall-clock 基准。
 *
 * 衡量 Graph 的调度开销（per-node 调度 + counter 原子操作）。基线
 * 数据用于后续任何 Graph 内部优化（对象池 / DCAS 等）的决策。
 */

#include <benchmark/benchmark.h>

#include <vector>

#include "Task/Task.hpp"
#include "Concurrency/Thread/WorkPool.hpp"

namespace
{

using bee::TaskGraph;
using bee::NodeHandle;
using bee::WorkPool;

// Chain 拓扑：N 个节点首尾相连，最大限度暴露 counter 同步开销。
static void BM_TaskGraphChainExecute(benchmark::State& state)
{
    const auto n = static_cast<std::size_t>(state.range(0));
    TaskGraph graph;
    NodeHandle<int> prev = graph.node([] { return 0; });
    for (std::size_t i = 1; i < n; ++i) {
        prev = graph.node([](int x) { return x + 1; }, prev);
    }
    WorkPool pool(4);
    for (auto _ : state) {
        auto t = graph.execute(pool);
        t.wait();
    }
    state.SetItemsProcessed(state.iterations() * n);
}
BENCHMARK(BM_TaskGraphChainExecute)->Arg(10)->Arg(100)->Arg(1000);

// Fan-out/fan-in：1 个根 → N 个并行 → 1 个汇聚，测最佳并行度。
static void BM_TaskGraphFanExecute(benchmark::State& state)
{
    const auto n = static_cast<std::size_t>(state.range(0));
    TaskGraph graph;
    auto root = graph.node([] { return 1; });
    std::vector<NodeHandle<int>> mids;
    mids.reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
        mids.push_back(graph.node([](int x) { return x * 2; }, root));
    }
    // 汇聚：简单 void 节点等所有 mid 完成
    // TaskGraph 当前没有 variadic node_after 接收 vector，简化为连 chain
    NodeHandle<int> sink = mids.front();
    for (std::size_t i = 1; i < mids.size(); ++i) {
        sink = graph.node([](int a, int b) { return a + b; }, sink, mids[i]);
    }

    WorkPool pool(4);
    for (auto _ : state) {
        auto t = graph.execute(pool);
        t.wait();
    }
    state.SetItemsProcessed(state.iterations() * (n + 2));
}
BENCHMARK(BM_TaskGraphFanExecute)->Arg(10)->Arg(100)->Arg(500);

} // namespace
