/**
 * @File WhenAllDepthBench.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Brief 深链路 when_all 基准：衡量嵌套 co_await 的累计开销。
 *
 * 若未来 P1-2 Step B 在 Clang 上落地（symmetric transfer），该基准的
 * 深层数据应显著下降——届时即可用作切换编译器的客观收益证据。
 */

#include <benchmark/benchmark.h>

#include <vector>

#include "Task/Task.hpp"
#include "Concurrency/Thread/WorkPool.hpp"

namespace
{

using bee::Task;
using bee::WorkPool;
using bee::spawn_task;
using bee::when_all;

// 宽度 N 的 when_all（单层）。
static void BM_WhenAllWide(benchmark::State& state)
{
    const auto n = static_cast<std::size_t>(state.range(0));
    WorkPool pool(4);
    for (auto _ : state) {
        std::vector<Task<int>> tasks;
        tasks.reserve(n);
        for (std::size_t i = 0; i < n; ++i) {
            tasks.push_back(spawn_task(pool, [i] { return static_cast<int>(i); }));
        }
        auto all = when_all(std::move(tasks));
        (void)all.get();
    }
    state.SetItemsProcessed(state.iterations() * n);
}
BENCHMARK(BM_WhenAllWide)->Arg(2)->Arg(8)->Arg(64)->Arg(256);

} // namespace
