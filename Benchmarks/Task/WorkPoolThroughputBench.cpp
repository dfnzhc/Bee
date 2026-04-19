/**
 * @File WorkPoolThroughputBench.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Brief WorkPool 吞吐基准：N 生产者 × M 任务 → wall-clock。
 *
 * 主要观察：空闲唤醒（EventCount）在高并发投递下是否存在尾延迟；
 * thread_count 与生产者数比例对吞吐的影响。
 */

#include <benchmark/benchmark.h>

#include <atomic>
#include <thread>
#include <vector>

#include "Concurrency/Thread/WorkPool.hpp"

namespace
{

using bee::WorkPool;

// 单生产者向 pool 投递 N 个空任务，等待全部完成。
static void BM_WorkPoolPostEmpty(benchmark::State& state)
{
    const auto threads = static_cast<std::size_t>(state.range(0));
    const auto count   = static_cast<std::size_t>(state.range(1));
    for (auto _ : state) {
        WorkPool pool(threads);
        std::atomic<std::size_t> done{0};
        for (std::size_t i = 0; i < count; ++i) {
            pool.post([&done] { done.fetch_add(1, std::memory_order_relaxed); });
        }
        while (done.load(std::memory_order_acquire) < count) {
            std::this_thread::yield();
        }
    }
    state.SetItemsProcessed(state.iterations() * count);
}
BENCHMARK(BM_WorkPoolPostEmpty)
    ->Args({2, 1000})
    ->Args({4, 1000})
    ->Args({8, 1000})
    ->Args({4, 10000})
    ->Args({8, 10000});

// 多生产者并发投递。
static void BM_WorkPoolPostMultiProducer(benchmark::State& state)
{
    const auto threads   = static_cast<std::size_t>(state.range(0));
    const auto producers = static_cast<std::size_t>(state.range(1));
    const auto per_prod  = static_cast<std::size_t>(state.range(2));
    const auto total     = producers * per_prod;
    for (auto _ : state) {
        WorkPool pool(threads);
        std::atomic<std::size_t> done{0};
        std::vector<std::thread> prods;
        prods.reserve(producers);
        for (std::size_t p = 0; p < producers; ++p) {
            prods.emplace_back([&pool, &done, per_prod] {
                for (std::size_t i = 0; i < per_prod; ++i) {
                    pool.post([&done] { done.fetch_add(1, std::memory_order_relaxed); });
                }
            });
        }
        for (auto& t : prods) t.join();
        while (done.load(std::memory_order_acquire) < total) {
            std::this_thread::yield();
        }
    }
    state.SetItemsProcessed(state.iterations() * total);
}
BENCHMARK(BM_WorkPoolPostMultiProducer)
    ->Args({4, 2, 1000})
    ->Args({4, 4, 1000})
    ->Args({8, 4, 1000})
    ->Args({8, 8, 1000});

} // namespace
