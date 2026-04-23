/**
 * @File RandomCudaBench.cpp
 * @Brief B7 设备侧 Philox 随机数基准：rand/randn F32 × {4K, 1M, 16M}。
 */

#include <benchmark/benchmark.h>

#include <cstdio>
#include <cstdlib>

#include "Tensor/Tensor.hpp"
#include "Tensor/Ops/Random.hpp"

namespace
{

using namespace bee;

static constexpr int64_t kSmall  = 4 * 1024;
static constexpr int64_t kMedium = 1 * 1024 * 1024;
static constexpr int64_t kLarge  = 16 * 1024 * 1024;

template <class T>
static T must(Result<T>&& r)
{
    if (!r) {
        const auto sv = r.error().message.view();
        std::fprintf(stderr, "CUDA random bench must failed: %.*s\n",
                     static_cast<int>(sv.size()), sv.data());
        std::abort();
    }
    return std::move(r.value());
}

static void BM_RandF32_CUDA(benchmark::State& state)
{
    const int64_t  n    = state.range(0);
    std::uint64_t  seed = 0xC0FFEEull;
    for (auto _ : state) {
        auto t = must(rand(Shape{n}, DType::F32, seed, Device::CUDA));
        benchmark::DoNotOptimize(t);
        benchmark::ClobberMemory();
        ++seed;
    }
    state.SetItemsProcessed(state.iterations() * n);
    state.SetBytesProcessed(state.iterations() * n * sizeof(float));
}
BENCHMARK(BM_RandF32_CUDA)->Arg(kSmall)->Arg(kMedium)->Arg(kLarge)->Unit(benchmark::kMicrosecond);

static void BM_RandnF32_CUDA(benchmark::State& state)
{
    const int64_t  n    = state.range(0);
    std::uint64_t  seed = 0xBEEFull;
    for (auto _ : state) {
        auto t = must(randn(Shape{n}, DType::F32, seed, Device::CUDA));
        benchmark::DoNotOptimize(t);
        benchmark::ClobberMemory();
        ++seed;
    }
    state.SetItemsProcessed(state.iterations() * n);
    state.SetBytesProcessed(state.iterations() * n * sizeof(float));
}
BENCHMARK(BM_RandnF32_CUDA)->Arg(kSmall)->Arg(kMedium)->Arg(kLarge)->Unit(benchmark::kMicrosecond);

static void BM_RandintI32_CUDA(benchmark::State& state)
{
    const int64_t  n    = state.range(0);
    std::uint64_t  seed = 0xD00Dull;
    for (auto _ : state) {
        auto t = must(randint(-1000, 1000, Shape{n}, DType::I32, seed, Device::CUDA));
        benchmark::DoNotOptimize(t);
        benchmark::ClobberMemory();
        ++seed;
    }
    state.SetItemsProcessed(state.iterations() * n);
    state.SetBytesProcessed(state.iterations() * n * sizeof(std::int32_t));
}
BENCHMARK(BM_RandintI32_CUDA)->Arg(kSmall)->Arg(kMedium)->Arg(kLarge)->Unit(benchmark::kMicrosecond);

} // namespace
