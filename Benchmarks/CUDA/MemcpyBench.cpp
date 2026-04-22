/**
 * @File MemcpyBench.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Brief 内存迁移基准：H2D / D2H / D2D（通过 Tensor::to 与 clone）。
 *
 * 用于量化 PCIe 带宽、显存带宽与 Bee 设备间搬运的开销，供后续决定
 * 是否启用 pinned memory、memory pool 调优等。
 */

#include <benchmark/benchmark.h>

#include <cstdio>
#include <cstdlib>

#include "Tensor/Tensor.hpp"

namespace
{

using namespace bee;

static constexpr int64_t kSmall  = 64 * 1024;           // 256 KiB F32
static constexpr int64_t kMedium = 4 * 1024 * 1024;     // 16 MiB F32
static constexpr int64_t kLarge  = 64 * 1024 * 1024;    // 256 MiB F32

template <class T>
static T must(Result<T>&& r)
{
    if (!r) {
        const auto sv = r.error().message.view();
        std::fprintf(stderr, "memcpy bench must failed: %.*s\n",
                     static_cast<int>(sv.size()), sv.data());
        std::abort();
    }
    return std::move(r.value());
}

static void BM_H2D_F32(benchmark::State& state)
{
    const int64_t n = state.range(0);
    auto host = must(Tensor::full(Shape{n}, DType::F32, 1.0, Device::CPU));
    for (auto _ : state) {
        auto dev = must(host.to(Device::CUDA));
        benchmark::DoNotOptimize(dev);
        benchmark::ClobberMemory();
    }
    state.SetBytesProcessed(state.iterations() * n * sizeof(float));
}
BENCHMARK(BM_H2D_F32)->Arg(kSmall)->Arg(kMedium)->Arg(kLarge)
    ->Unit(benchmark::kMicrosecond);

static void BM_D2H_F32(benchmark::State& state)
{
    const int64_t n = state.range(0);
    auto dev = must(Tensor::full(Shape{n}, DType::F32, 1.0, Device::CUDA));
    for (auto _ : state) {
        auto host = must(dev.to(Device::CPU));
        benchmark::DoNotOptimize(host);
        benchmark::ClobberMemory();
    }
    state.SetBytesProcessed(state.iterations() * n * sizeof(float));
}
BENCHMARK(BM_D2H_F32)->Arg(kSmall)->Arg(kMedium)->Arg(kLarge)
    ->Unit(benchmark::kMicrosecond);

static void BM_D2D_F32(benchmark::State& state)
{
    const int64_t n = state.range(0);
    auto dev = must(Tensor::full(Shape{n}, DType::F32, 1.0, Device::CUDA));
    for (auto _ : state) {
        auto copy = must(dev.clone());
        benchmark::DoNotOptimize(copy);
        benchmark::ClobberMemory();
    }
    state.SetBytesProcessed(state.iterations() * n * sizeof(float));
}
BENCHMARK(BM_D2D_F32)->Arg(kSmall)->Arg(kMedium)->Arg(kLarge)
    ->Unit(benchmark::kMicrosecond);

} // namespace
