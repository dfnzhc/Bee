/**
 * @File TaskSubmitBench.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Brief Task 单体 submit + get 延迟基准。
 *
 * 衡量"单个 Task 从 spawn_task 到 get 返回"的端到端延迟。任何未来的
 * Task 瘦身 / 调度器优化都应该先看这里的数字有没有回归。
 */

#include <benchmark/benchmark.h>

#include "Task/Task.hpp"
#include "Concurrency/Thread/WorkPool.hpp"

namespace
{

using bee::WorkPool;
using bee::spawn_task;

// 单体最小工作量：提交一个返回常量的协程并立即 get。
static void BM_TaskSubmitGetMinimal(benchmark::State& state)
{
    WorkPool pool(static_cast<std::size_t>(state.range(0)));
    for (auto _ : state) {
        auto t = spawn_task(pool, [] { return 42; });
        benchmark::DoNotOptimize(t.get());
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_TaskSubmitGetMinimal)->Arg(1)->Arg(2)->Arg(4)->Arg(8);

// 带工作量的 Task：模拟非平凡 workload 下 submit/get 开销占比。
static void BM_TaskSubmitGetWithWork(benchmark::State& state)
{
    WorkPool pool(static_cast<std::size_t>(state.range(0)));
    const int iters = 1024;
    for (auto _ : state) {
        auto t = spawn_task(pool, [iters] {
            volatile int acc = 0;
            for (int i = 0; i < iters; ++i) acc += i;
            return acc;
        });
        benchmark::DoNotOptimize(t.get());
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_TaskSubmitGetWithWork)->Arg(1)->Arg(2)->Arg(4)->Arg(8);

} // namespace
